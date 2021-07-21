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
 * $Id: qmnWindowSort.cpp 29304 2008-11-14 08:17:42Z jakim $
 *
 * Description :
 *    WNST(WiNdow SorT) Node
 *
 * 용어 설명 :
 *    같은 의미를 가지는 서로 다른 단어를 정리하면 아래와 같다.
 *    - Analytic Funtion = Window Function
 *    - Analytic Clause = Window Clause = Over Clause
 *
 * 약어 :
 *    WNST(Window Sort)
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmoUtil.h>
#include <qmnWindowSort.h>
#include <qcg.h>
#include <qmxResultCache.h>

extern mtfModule mtfRowNumber;
extern mtfModule mtfRowNumberLimit;
extern mtfModule mtfLagIgnoreNulls;
extern mtfModule mtfLeadIgnoreNulls;
extern mtfModule mtfRatioToReport;

IDE_RC
qmnWNST::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    WNST 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmncWNST * sCodePlan = (qmncWNST *) aPlan;
    qmndWNST       * sDataPlan =
        (qmndWNST *) (aTemplate->tmplate.data + aPlan->offset);
    idBool           sIsSkip = ID_FALSE;
    SLong            sNumber;
    UInt             sCodePlanFlag   = 0;
   
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnWNST::doItDefault;

    sCodePlanFlag = sCodePlan->flag;

    //----------------------------------------
    // 최초 초기화 수행
    //----------------------------------------

    if ( (*sDataPlan->flag & QMND_WNST_INIT_DONE_MASK)
         == QMND_WNST_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit( aTemplate,
                             sCodePlan,
                             sDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------------
    // Dependency를 검사하여 재 수행 여부 결정
    //----------------------------------------
    if( sDataPlan->depValue != sDataPlan->depTuple->modify )
    {
        // Sort Manager 설정
        if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
             == QMN_PLAN_STORAGE_DISK )
        {
            // 디스크 소트 템프인 경우
            // 처음 수행시 sortMgr가 sortMgrForDisk를 가리키고 있음
            // 하지만 반복 수행시 (firstInit)이 수행되지 않으므로
            // 이를 대비해여 값을 초기화
            sDataPlan->sortMgr = sDataPlan->sortMgrForDisk;
        }
        else
        {
            /* PROJ-2462 Result Cache */
            if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
                 == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
            {
                sIsSkip = ID_FALSE;
            }
            else
            {
                /* PROJ-2462 Result Cache */
                sDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[sCodePlan->planID];
                if ( ( *sDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
                     == QMX_RESULT_CACHE_STORED_TRUE )
                {
                    sIsSkip = ID_TRUE;
                }
                else
                {
                    sIsSkip = ID_FALSE;
                }
            }
        }

        if ( sIsSkip == ID_FALSE )
        {
            //----------------------------------------
            // Temp Table 구축 전 초기화
            //----------------------------------------
            IDE_TEST( qmcSortTemp::clear( sDataPlan->sortMgr )
                      != IDE_SUCCESS );

            //----------------------------------------
            // 1. Child를 반복 수행하여 Temp Table에 Insert
            //----------------------------------------
            IDE_TEST( sCodePlan->plan.left->init( aTemplate,
                                                  sCodePlan->plan.left )
                      != IDE_SUCCESS);

            /* BUG-40354 pushed rank */
            if ( ( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
                   == QMNC_WNST_STORE_LIMIT_SORTING ) ||
                 ( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
                   == QMNC_WNST_STORE_LIMIT_PRESERVED_ORDER ) )
            {
                // BUG-48905 (BUG-40409 적용) 1024보다 큰 경우 성능이 저하된다.
                IDE_TEST( getMinLimitValue( aTemplate,
                                            sDataPlan->wndNode[0],
                                            & sNumber )
                          != IDE_SUCCESS );

                if ( sNumber <= QMN_WNST_PUSH_RANK_MAX )
                {
                    IDE_TEST( insertLimitedRowsFromChild( aTemplate,
                                                          sCodePlan,
                                                          sDataPlan,
                                                          sNumber )
                              != IDE_SUCCESS );
                }
                else
                {
                    // 1024초과하면 기존과 동일하게 수행
                    IDE_TEST( insertRowsFromChild( aTemplate,
                                                   sCodePlan,
                                                   sDataPlan )
                              != IDE_SUCCESS );

                    // performAnalyticFunctions에서 sorting이 될 수 있도록
                    if( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
                        == QMNC_WNST_STORE_LIMIT_SORTING )
                    {
                        sCodePlanFlag &= ~QMNC_WNST_STORE_MASK;
                        sCodePlanFlag |= QMNC_WNST_STORE_SORTING;
                    }
                    else
                    {
                        sCodePlanFlag &= ~QMNC_WNST_STORE_MASK;
                        sCodePlanFlag |= QMNC_WNST_STORE_PRESERVED_ORDER;
                    }
                }
            }
            else
            {
                IDE_TEST( insertRowsFromChild( aTemplate,
                                               sCodePlan,
                                               sDataPlan )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( performAnalyticFunctions( aTemplate,
                                            sCodePlan,
                                            sDataPlan,
                                            sCodePlanFlag )
                  != IDE_SUCCESS );

        // Temp Table 구축 후 초기화
        sDataPlan->depValue = sDataPlan->depTuple->modify;
    }
    else
    {
        // Nothing To Do
    }

    // doIt 함수 설정
    sDataPlan->doIt = qmnWNST::doItFirst;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    WNST 의 doIt 함수
 *
 * Implementation :
 *    Analytic Function 수행 결과를 순차적으로 tuple에 설정함
 *
 ***********************************************************************/
    qmndWNST * sDataPlan =
        (qmndWNST *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate,
                               aPlan,
                               aFlag )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    WNST 노드의 Tuple에 Null Row를 설정한다.
 *
 * Implementation :
 *    Child Plan의 Null Padding을 수행하고,
 *    자신의 Null Row를 Temp Table로부터 획득한다.
 *
 ***********************************************************************/
    qmncWNST * sCodePlan = (qmncWNST *) aPlan;
    qmndWNST * sDataPlan =
        (qmndWNST *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_WNST_INIT_DONE_MASK)
         == QMND_WNST_INIT_DONE_FALSE )
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
}

IDE_RC
qmnWNST::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    WNST 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmncWNST * sCodePlan = (const qmncWNST*) aPlan;
    qmndWNST       * sDataPlan = (qmndWNST*)
        (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndWNST       * sCacheDataPlan = NULL;
    idBool           sIsInit       = ID_FALSE;

    ULong        i;
    ULong        sDiskPageCnt;
    SLong        sRecordCnt;
    UInt         sSortCount = 0;

    //----------------------------
    // SORT COUNT 결정
    //----------------------------
    if( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
        == QMNC_WNST_STORE_SORTING )
    {
        // 일반적인 경우
        sSortCount = sCodePlan->sortKeyCnt;
    }
    else if( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
        == QMNC_WNST_STORE_LIMIT_SORTING )
    {
        /* pushed rank인 경우 */
        IDE_DASSERT( sCodePlan->sortKeyCnt == 1 );
        sSortCount = sCodePlan->sortKeyCnt;
    }
    else if( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
             == QMNC_WNST_STORE_PRESERVED_ORDER )
    {
        // PRESERVED ORDER를 가지는 경우
        sSortCount = sCodePlan->sortKeyCnt - 1;
    }
    else if( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
             == QMNC_WNST_STORE_LIMIT_PRESERVED_ORDER )
    {
        /* pushed rank인 경우 */
        IDE_DASSERT( sCodePlan->sortKeyCnt == 1 );
    }
    else if( ( sCodePlan->flag & QMNC_WNST_STORE_MASK )
             == QMNC_WNST_STORE_ONLY )
    {
        // 빈 OVER()만을 가지는 정렬키
        IDE_DASSERT( sCodePlan->sortKeyCnt == 1 );
    }
    else
    {
        // 고려하지 않은 플래그 정보
        IDE_DASSERT(0);
    }
    
    //----------------------------
    // Display 위치 결정 (들여쓰기)
    //----------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString, " " );
    }

    //----------------------------
    // 수행 정보 출력
    //----------------------------

    if ( aMode == QMN_DISPLAY_ALL )
    {
        //----------------------------
        // explain plan = on; 인 경우
        //----------------------------

        if ( (*sDataPlan->flag & QMND_WNST_INIT_DONE_MASK)
             == QMND_WNST_INIT_DONE_TRUE )
        {
            sIsInit = ID_TRUE;
            // 초기화 된 경우

            // BUBBUG
            // -> 어차피 Disk의 경우도 여러개의 Sort Mgr간에 데이터를 옮기고 나면,
            // clear()를 수행할 것인데, 이 경우도 공간이 반환되지 않는가?
            // 모든 Sort Temp에 대한 공간을 더해야 할지 결정해야 함
            // Sort Temp Table로 부터 record, page 정보 가져움
            IDE_TEST( qmcSortTemp::getDisplayInfo( sDataPlan->sortMgr,
                                                   & sDiskPageCnt,
                                                   & sRecordCnt )
                      != IDE_SUCCESS );
            
            // Memory/Disk를 구별하여 출력함
            if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                 == QMN_PLAN_STORAGE_MEMORY )
            {
                if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                {
                    iduVarStringAppendFormat(
                        aString,
                        "WINDOW SORT ( "
                        "ITEM_SIZE: %"ID_UINT32_FMT", "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
                        "ACCESS: %"ID_UINT32_FMT", "
                        "SORT_COUNT: %"ID_UINT32_FMT,
                        sDataPlan->mtrRowSize,
                        sRecordCnt,
                        sDataPlan->plan.myTuple->modify,
                        sSortCount );
                }
                else
                {
                    // BUG-29209
                    // ITEM_SIZE 정보 보여주지 않음
                    iduVarStringAppendFormat(
                        aString,
                        "WINDOW SORT ( "
                        "ITEM_SIZE: BLOCKED, "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
                        "ACCESS: %"ID_UINT32_FMT", "
                        "SORT_COUNT: %"ID_UINT32_FMT,
                        sRecordCnt,
                        sDataPlan->plan.myTuple->modify,
                        sSortCount );
                }
            }
            else
            {
                if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                {
                    iduVarStringAppendFormat(
                        aString,
                        "WINDOW SORT ( "
                        "ITEM_SIZE: %"ID_UINT32_FMT", "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
                        "DISK_PAGE_COUNT: %"ID_UINT64_FMT", "
                        "ACCESS: %"ID_UINT32_FMT", "
                        "SORT_COUNT: %"ID_UINT32_FMT,
                        sDataPlan->mtrRowSize,
                        sRecordCnt,
                        sDiskPageCnt,
                        sDataPlan->plan.myTuple->modify,
                        sSortCount );
                }
                else
                {
                    // BUG-29209
                    // ITEM_SIZE, DISK_PAGE_COUNT 정보 보여주지 않음
                    iduVarStringAppendFormat(
                        aString,
                        "WINDOW SORT ( "
                        "ITEM_SIZE: BLOCKED, "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
                        "DISK_PAGE_COUNT: BLOCKED, "
                        "ACCESS: %"ID_UINT32_FMT", "
                        "SORT_COUNT: %"ID_UINT32_FMT,
                        sRecordCnt,
                        sDataPlan->plan.myTuple->modify,
                        sSortCount );
                }
            }
            
        }
        else
        {
            // 초기화 되지 않은 경우
            iduVarStringAppendFormat( aString,
                                      "WINDOW SORT ( ITEM_SIZE: 0, "
                                      "ITEM_COUNT: 0, ACCESS: 0, "
                                      "SORT_COUNT: %"ID_UINT32_FMT,
                                      sSortCount );
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------
        iduVarStringAppendFormat( aString,
                                  "WINDOW SORT ( ITEM_SIZE: ??, "
                                  "ITEM_COUNT: ??, ACCESS: ??, "
                                  "SORT_COUNT: %"ID_UINT32_FMT,
                                  sSortCount );
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
    // Materialize Node Info
    //----------------------------
    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndWNST *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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

        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->aggrNode,
                           "aggrNode",
                           sCodePlan->myNode->dstNode->node.table,
                           sCodePlan->depTupleRowID,
                           ID_USHORT_MAX );
    }
    else
    {
        // TRCLOG_DETAIL_MTRNODE = 0 인 경우
        // 아무 것도 출력하지 않는다.
    }
    
    //----------------------------
    // Predicate Info
    //----------------------------
    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        // TRCLOG_DETAIL_PREDICATE = 1 인 경우
        // 정렬 키와 각 정렬 키별 analytic clause 정보를 출력한다
        for ( i = 0; i < aDepth+1; i++ )
        {
            iduVarStringAppend( aString, " " );
        }

        iduVarStringAppend( aString, "[ ANALYTIC FUNCTION INFO ]\n" );
        
        // 분석 함수 정보 출력
        IDE_TEST( printAnalyticFunctionInfo( aTemplate,
                                             sCodePlan,
                                             sDataPlan,
                                             aDepth+1,
                                             aString,
                                             aMode )
                  != IDE_SUCCESS );
    }
    else
    {
        // TRCLOG_DETAIL_PREDICATE = 0 인 경우
        // 아무것도 출력하지 않는다
    }

    //----------------------------
    // Operator별 결과 정보 출력
    //----------------------------
    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    aPlan->resultDesc )
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
}

IDE_RC
qmnWNST::doItDefault( qcTemplate * /* aTemplate */,
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
    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    최초 수행 함수
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndWNST * sDataPlan =
        (qmndWNST *) (aTemplate->tmplate.data + aPlan->offset);
    
    void       * sOrgRow;
    void       * sSearchRow;

    // 질문: 검사하는 노드도 있고, 아닌 노드도 있는데, 해야하나?
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );
    
    // 첫번째 순차 검색
    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstSequence( sDataPlan->sortMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;
    
    // Row 존재 유무 설정 및 Tuple Set 복원
    if ( sSearchRow != NULL )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        // Data가 존재할 경우 Tuple Set 복원
        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan->mtrNode,
                               sDataPlan->plan.myTuple->row ) != IDE_SUCCESS );
        
        sDataPlan->plan.myTuple->modify++;

        sDataPlan->doIt = qmnWNST::doItNext;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
        
        sDataPlan->doIt = qmnWNST::doItFirst;
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색을 수행
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndWNST * sDataPlan =
        (qmndWNST *) (aTemplate->tmplate.data + aPlan->offset);
    void     * sOrgRow;
    void     * sSearchRow;
    
    // 순차 검색
    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    
    IDE_TEST( qmcSortTemp::getNextSequence( sDataPlan->sortMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정 및 Tuple Set 복원
    if ( sSearchRow != NULL )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        // Data가 존재할 경우 Tuple Set 복원
        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan->mtrNode,
                               sDataPlan->plan.myTuple->row ) != IDE_SUCCESS );
        
        sDataPlan->plan.myTuple->modify++;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
        
        sDataPlan->doIt = qmnWNST::doItFirst;

    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::firstInit( qcTemplate     * aTemplate,
                    const qmncWNST * aCodePlan,
                    qmndWNST       * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Wnst Node의 Data Plan 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    UChar const * sDataArea = aTemplate->tmplate.data;
    qmndWNST    * sCacheDataPlan = NULL;

    //---------------------------------
    // 적합성 검사
    //---------------------------------    
    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );
    IDE_DASSERT( aCodePlan->distNodeOffset > 0 );
    IDE_DASSERT( aCodePlan->aggrNodeOffset > 0 );    
    IDE_DASSERT( aCodePlan->sortNodeOffset > 0 );
    IDE_DASSERT( aCodePlan->wndNodeOffset > 0 );
    IDE_DASSERT( aCodePlan->sortMgrOffset > 0 );

    //---------------------------------
    // Data Plan의 Data 영역 주소 할당
    //---------------------------------
    aDataPlan->mtrNode  = (qmdMtrNode*)  (sDataArea + aCodePlan->mtrNodeOffset);
    aDataPlan->distNode = (qmdDistNode*) (sDataArea + aCodePlan->distNodeOffset);
    aDataPlan->aggrNode = (qmdAggrNode*) (sDataArea + aCodePlan->aggrNodeOffset);    
    aDataPlan->sortNode = (qmdMtrNode**) (sDataArea + aCodePlan->sortNodeOffset);
    aDataPlan->wndNode  = (qmdWndNode**) (sDataArea + aCodePlan->wndNodeOffset);
    
    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndWNST *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
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

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_DISK  )
    {
        // DISK 인 경우, sortMtrForDisk 또한 설정
        aDataPlan->sortMgr        = (qmcdSortTemp*)
            (sDataArea + aCodePlan->sortMgrOffset);
        aDataPlan->sortMgrForDisk = (qmcdSortTemp*)
            (sDataArea + aCodePlan->sortMgrOffset);
    }
    else
    {
        // MEMORY인 경우
        aDataPlan->sortMgr        = (qmcdSortTemp*)
            (sDataArea + aCodePlan->sortMgrOffset);
        aDataPlan->sortMgrForDisk = NULL;
    }

    //---------------------------------
    // Data Plan 정보 초기화
    //---------------------------------
    
    // 저장 칼럼 정보를 (Materialize 노드) 초기화
    IDE_TEST( initMtrNode( aTemplate,
                           aCodePlan,
                           aDataPlan )
              != IDE_SUCCESS );

    // 초기화된 mtrNode 정보를 이용하여 관련된 다른 정보 초기화
    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );
    aDataPlan->plan.myTuple    = aDataPlan->mtrNode->dstTuple;
    aDataPlan->depTuple   = & aTemplate->tmplate.rows[aCodePlan->depTupleRowID];
    aDataPlan->depValue   = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;
    
    // Analytic Function 인자애 DISTINCT가 있는 경우 정보 설정
    if( aCodePlan->distNode != NULL )
    {
        IDE_TEST( initDistNode( aTemplate,
                                aCodePlan,
                                aDataPlan->distNode,
                                & aDataPlan->distNodeCnt )
                  != IDE_SUCCESS );    
    }
    else
    {
        aDataPlan->distNodeCnt = 0;
        aDataPlan->distNode = NULL;
    }

    // Reporting Aggregation을 처리하는 칼럼(중간값) 정보의 설정
    IDE_TEST( initAggrNode( aTemplate,
                            aCodePlan->aggrNode,
                            aDataPlan->distNode,
                            aDataPlan->distNodeCnt,
                            aDataPlan->aggrNode)
              != IDE_SUCCESS );
    
    // init sort node
    IDE_TEST( initSortNode( aCodePlan,
                            aDataPlan,
                            aDataPlan->sortNode )
              != IDE_SUCCESS );
    
    // init wnd node
    IDE_TEST( initWndNode( aCodePlan,
                           aDataPlan,
                           aDataPlan->wndNode )
              != IDE_SUCCESS );

    
    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------
    IDE_TEST( initTempTable( aTemplate,
                             aCodePlan,
                             aDataPlan,
                             aDataPlan->sortMgr )
               != IDE_SUCCESS );
    

    // 서로 다른 Partition을 구별하기 위한 DataPlan->mtrRow[2]에 메모리 공간 할당
    IDE_TEST( allocMtrRow( aTemplate,
                           aCodePlan,
                           aDataPlan,
                           aDataPlan->mtrRow )
              != IDE_SUCCESS );

    
    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_WNST_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_WNST_INIT_DONE_TRUE;

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
qmnWNST::initMtrNode( qcTemplate     * aTemplate,
                      const qmncWNST * aCodePlan,
                      qmndWNST       * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 칼럼 정보를 (Materialize 노드) 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt        sHeaderSize = 0;

    // Memory/Disk여부에 따라 적절한 temp table header size 설정
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sHeaderSize = QMC_MEMSORT_TEMPHEADER_SIZE;

        /* PROJ-2462 Result Cache */
        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
        {
            aDataPlan->mtrNode  = ( qmdMtrNode * )( aTemplate->resultCache.data +
                                                    aCodePlan->mtrNodeOffset );
            aDataPlan->sortNode = ( qmdMtrNode** )( aTemplate->resultCache.data +
                                                    aCodePlan->sortNodeOffset);
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
    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    // 2.  저장 Column의 초기화
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                aCodePlan->baseTableCount )
              != IDE_SUCCESS );

    // 3.  저장 Column의 offset을 재조정
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  sHeaderSize )
              != IDE_SUCCESS );

    // 4.  Row Size의 계산
    //     - Disk Temp Table의 경우 Row를 위한 Memory도 할당받음.
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::initDistNode( qcTemplate     * aTemplate,
                       const qmncWNST * aCodePlan,
                       qmdDistNode    * aDistNode,
                       UInt           * aDistNodeCnt )
{
/***********************************************************************
 *
 * Description :
 *    Analytic Function 인자애 DISTINCT가 있는 경우 정보 설정
 *
 * Implementation :
 *    다른 저장 Column과 달리 Distinct Argument Column정보는
 *    서로 간의 연결 정보를 유지하지 않는다.
 *    이는 각 Column정보는 별도의 Tuple을 사용하며, 서로 간의 연관
 *    관계를 갖지 않을 뿐더러 Hash Temp Table의 수정 없이 쉽게
 *    사용하기 위해서이다.
 *
 ***********************************************************************/
    const qmcMtrNode * sCodeNode;
    qmdDistNode      * sDistNode;
    UInt               sDistNodeCnt = 0;
    UInt               sFlag;
    UInt               sHeaderSize;
    UInt               i;
     
    //------------------------------------------------------
    // Distinct 저장 Column의 기본 정보 구성
    // Distinct Node는 개별적으로 저장 공간을 갖고 처리되며,
    // 따라서 Distinct Node간에 연결 정보를 생성하지 않는다.
    //------------------------------------------------------

    for( sCodeNode = aCodePlan->distNode,
             sDistNode = aDistNode;
         sCodeNode != NULL;
         sCodeNode = sCodeNode->next,
             sDistNode++,
             sDistNodeCnt++ )
    {
        sDistNode->myNode  = (qmcMtrNode*)sCodeNode;
        sDistNode->srcNode = NULL;
        sDistNode->next    = NULL;
    }

    *aDistNodeCnt = sDistNodeCnt;
    
    //------------------------------------------------------------
    // [Hash Temp Table을 위한 정보 정의]
    // Distinct Column은 저장 매체가/ Memory 또는 Disk일 수 있다. 이
    // 정보는 plan.flag을 이용하여 판별하며, 해당 distinct column을
    // 저장하기 위한 Tuple Set또한 동일한 저장 매체를 사용하고 있어야
    // 한다.  이에 대한 적합성 검사는 Hash Temp Table에서 검사하게
    // 된다.
    //------------------------------------------------------------

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sFlag =
            QMCD_HASH_TMP_STORAGE_MEMORY |
            QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

        sHeaderSize = QMC_MEMHASH_TEMPHEADER_SIZE;
    }
    else
    {
        sFlag =
            QMCD_HASH_TMP_STORAGE_DISK |
            QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

        sHeaderSize = QMC_DISKHASH_TEMPHEADER_SIZE;
    }

    // PROJ-2553
    // DISTINCT Hashing은 Bucket List Hashing 방법을 써야 한다.
    sFlag &= ~QMCD_HASH_TMP_HASHING_TYPE;
    sFlag |= QMCD_HASH_TMP_HASHING_BUCKET;

    //----------------------------------------------------------
    // 개별 Distinct 저장 Column의 초기화
    //----------------------------------------------------------

    for ( i = 0, sDistNode = aDistNode;
          i < sDistNodeCnt;
          i++, sDistNode++ )
    {
        //---------------------------------------------------
        // 1. Dist Column의 구성 정보 초기화
        // 2. Dist Column의 offset재조정
        // 3. Disk Temp Table을 사용하는 경우 memory 공간을 할당받으며,
        //    Dist Node는 이 정보를 계속 유지하여야 한다.
        //    Memory Temp Table을 사용하는 경우 별도의 공간을 할당 받지
        //    않는다.
        // 4. Dist Column을 위한 Hash Temp Table을 초기화한다.
        //---------------------------------------------------

        IDE_TEST( qmc::initMtrNode( aTemplate,
                                    (qmdMtrNode*) sDistNode,
                                    0 )
                  != IDE_SUCCESS );

        IDE_TEST( qmc::refineOffsets( (qmdMtrNode*) sDistNode,
                                      sHeaderSize )
                  != IDE_SUCCESS );

        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   & aTemplate->tmplate,
                                   sDistNode->dstNode->node.table )
                  != IDE_SUCCESS );

        // Disk Temp Table을 사용하는 경우라면
        // 이 공간을 잃지 않도록 해야 한다.
        sDistNode->mtrRow = sDistNode->dstTuple->row;
        sDistNode->isDistinct = ID_TRUE;

        IDE_TEST( qmcHashTemp::init( & sDistNode->hashMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     (qmdMtrNode*) sDistNode,  // 저장 대상
                                     (qmdMtrNode*) sDistNode,  // 비교 대상
                                     NULL,
                                     sDistNode->myNode->bucketCnt,
                                     sFlag )
                  != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::initAggrNode( qcTemplate        * aTemplate,
                       const qmcMtrNode  * aCodeNode,
                       const qmdDistNode * aDistNode,
                       const UInt          aDistNodeCnt,
                       qmdAggrNode       * aAggrNode )
{
/***********************************************************************
 *
 * Description :
 *    Reporting Aggregation을 처리하는 칼럼(중간값) 정보의 설정
 *
 * Implementation :
 *
 ***********************************************************************/
    iduMemory         * sMemory;
    const qmcMtrNode  * sCodeNode;
    const qmdDistNode * sDistNode;
    qmdAggrNode       * sAggrNode;
    UInt                sAggrNodeCnt = 0;
    UInt                i;
    UInt                sAggrMtrRowSize;
    UInt                sAggrTupleRowID;
    
    sMemory = aTemplate->stmt->qmxMem;
    
    //-----------------------------------------------
    // 적합성 검사
    //-----------------------------------------------

    //-----------------------------------------------
    // Aggregation Node의 연결 정보를 설정하고 초기화
    // 초기화하면서 aggrNode의 개수 계산
    //-----------------------------------------------
    for( sCodeNode = aCodeNode,
             sAggrNode = aAggrNode;
         sCodeNode != NULL;
         sCodeNode = sCodeNode->next,
             sAggrNode = sAggrNode->next )
    {
        sAggrNode->myNode = (qmcMtrNode*)sCodeNode;
        sAggrNode->srcNode = NULL;
        sAggrNode->next = sAggrNode + 1;

        // Aggregation Node의 개수 기록
        sAggrNodeCnt++;
        
        if( sCodeNode->next == NULL )
        {
            sAggrNode->next = NULL;            
        }
    }

    IDE_DASSERT( sAggrNodeCnt != 0 );
    
    // aggregation node should not use converted source node
    //   e.g) SUM( MIN(I1) )
    //        MIN(I1)'s converted node is not aggregation node
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                (qmdMtrNode*)aAggrNode,
                                (UShort)sAggrNodeCnt )
              != IDE_SUCCESS );
    
    // Aggregation Column 의 offset을 재조정
    IDE_TEST( qmc::refineOffsets( (qmdMtrNode*)aAggrNode,
                                  0 ) // 별도의 header가 필요 없음
              != IDE_SUCCESS );

    // aggrNode를 위한 tupelID
    sAggrTupleRowID = aAggrNode->dstNode->node.table;
        
    // set row size (필요한 메모리 및 튜플 할당)
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               (UShort)sAggrTupleRowID )
              != IDE_SUCCESS );

    // aggrNode의 aggregation 중간 결과 저장을 위한 공간 할당
    sAggrMtrRowSize = qmc::getMtrRowSize( (qmdMtrNode*)aAggrNode );

    IDE_TEST( sMemory->cralloc( sAggrMtrRowSize,
                              (void**)&(aTemplate->tmplate.rows[sAggrTupleRowID].row))
              != IDE_SUCCESS);
    IDE_TEST_RAISE( aTemplate->tmplate.rows[sAggrTupleRowID].row == NULL,
                    err_mem_alloc );
    
    //-----------------------------------------------
    // Distinct Aggregation의 경우 해당 Distinct Node를
    // 찾아 연결한다.
    //-----------------------------------------------
    for( sAggrNode = aAggrNode;
         sAggrNode != NULL;
         sAggrNode = sAggrNode->next)
    {
        if( sAggrNode->myNode->myDist != NULL )
        {
            // Distinct Aggregation인 경우
            for( i = 0, sDistNode = aDistNode;
                 i < aDistNodeCnt;
                 i++, sDistNode++ )
            {
                if( sDistNode->myNode == sAggrNode->myNode->myDist )
                {
                    sAggrNode->myDist = (qmdDistNode*)sDistNode;
                    break;
                }
                else
                {
                    // do nothing
                }
            }
            
        }
        else
        {
            // 일반 Aggregation인 경우
            sAggrNode->myDist = NULL;
        }            
        
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( err_mem_alloc )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MEMORY_ALLOCATION));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::initSortNode( const qmncWNST * aCodePlan,
                       const qmndWNST * aDataPlan,
                       qmdMtrNode    ** aSortNode )
{
/***********************************************************************
 *
 * Description :
 *    모든 정렬키의 정보를 설정
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmcMtrNode * sCodeNode;
    qmdMtrNode       * sSortNode;
    const UInt         sSortKeyCnt = aCodePlan->sortKeyCnt;
    UInt               i;

    // sSortNode 위치 초기화
    // 앞부분에 각 정렬키를 위한 qmdMtrNode*를 저장할 공간을 제외한 위치를
    // 정렬키를 위한 칼럼 정보가 저장될 시작 주소
    sSortNode = (qmdMtrNode*)(aSortNode + sSortKeyCnt);
    
    for( i=0; i < sSortKeyCnt; i++ )
    {
        // 현재 정렬키에 해당하는 Code Plan 정보 설정
        sCodeNode = aCodePlan->sortNode[i];
        
        // 현재 정렬키의 시작 칼럼을 저장
        if( sCodeNode != NULL )
        {
            aSortNode[i] = sSortNode;

            //---------------------------------
            // 정렬키를 위한 Column의 초기화
            //---------------------------------
            IDE_TEST( initCopiedMtrNode( aDataPlan,
                                         sCodeNode,
                                         sSortNode )
                      != IDE_SUCCESS );            

            // sSortNode 값 변경
            // 다음에 정렬키에서 사용할 위치로 설정
            while( sSortNode->next != NULL )
            {
                sSortNode++;
            }
            sSortNode++;
            
        }
        else
        {
            // OVER절이 빈경우 정렬키가 NULL을 가리킬 수 있음
            aSortNode[i] = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::initCopiedMtrNode( const qmndWNST   * aDataPlan,
                            const qmcMtrNode * aCodeNode,
                            qmdMtrNode       * aDataNode )
{
/***********************************************************************
 *
 * Description :
 *    복사하여 사용하는 저장 칼럼 정보를 (Materialize 노드) 초기화
 *    사용되는 곳: 정렬키의 칼럼, PARTITION BY
 *
 * Implementation :
 *
 *    이미 initMtrNode에서 초기화한 정보를 다른 next를 가지는
 *    노드로 표현하여 초기화하는 것임. 따라서 이전에 초기화된 노드의 정보.
 *    1. 저장 칼럼의 연결 정보 생성
 *    2. 저장 칼럼을 (next를 제외한 정보를) 검색하여 복사
 *
 ***********************************************************************/
    qmdMtrNode    * sColumnNode;
    qmdMtrNode    * sFindNode;
    qmdMtrNode    * sNextNode;

    idBool          sIsMatched;

    
    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------
    
    // 1.  저장 Column의 연결 정보 생성
    IDE_TEST( qmc::linkMtrNode( aCodeNode,
                                aDataNode ) != IDE_SUCCESS );


    // 2. 이미 존재하는 노드를 검색하여 복사
    for( sColumnNode = aDataNode;
         sColumnNode != NULL;
         sColumnNode = sColumnNode->next )
    {
        sIsMatched = ID_FALSE;
        
        // mtrNode에서 동일한 노드를 검색하는 부분
        for( sFindNode = aDataPlan->mtrNode;
             sFindNode != NULL;
             sFindNode = sFindNode->next )
        {
            // 검색하는 정보는 결국 복사되어 생성된 정보이므로
            // myNode의 next를 제외한 정보는 일치해야 함
            if( ( sColumnNode->myNode->srcNode == sFindNode->myNode->srcNode ) &&
                ( sColumnNode->myNode->dstNode == sFindNode->myNode->dstNode ) )
            {
                if ( (sColumnNode->myNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                     == QMC_MTR_SORT_ORDER_FIXED_FALSE )
                {
                    if ( (sFindNode->myNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                         == QMC_MTR_SORT_ORDER_FIXED_FALSE )
                    {
                        sIsMatched = ID_TRUE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    IDE_DASSERT( (sColumnNode->myNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                                 == QMC_MTR_SORT_ORDER_FIXED_TRUE );
                    
                    if ( (sFindNode->myNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                         == QMC_MTR_SORT_ORDER_FIXED_TRUE )
                    {
                        if ( (sColumnNode->myNode->flag & QMC_MTR_SORT_ORDER_MASK)
                             == (sFindNode->myNode->flag & QMC_MTR_SORT_ORDER_MASK) )
                        {
                            sIsMatched = ID_TRUE;
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
                }

                if ( sIsMatched == ID_TRUE )
                {
                    // base table이 오면 안됨
                    // 만약 base table의 myNode정보와 일치한다면 오류
                    IDE_DASSERT( ((sFindNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_MEMORY_TABLE) &&
                                 ((sFindNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_DISK_TABLE) );
                    
                    sIsMatched = ID_TRUE;
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

        // DataPlan->mtrNode에 항상 일치하는 칼럼이 있어야 함
        IDE_TEST_RAISE( sIsMatched == ID_FALSE, ERR_COLUMN_NOT_FOUND );

        // 이전 노드를 복사하기 전에 next 값을 저장
        sNextNode = sColumnNode->next;
        
        // qmdMtrNode 복사
        *sColumnNode = *sFindNode;

        // next를 제대로 설정
        sColumnNode->next = sNextNode;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::initCopiedMtrNode",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::initCopiedAggrNode( const qmndWNST   * aDataPlan,
                             const qmcMtrNode * aCodeNode,
                             qmdAggrNode      * aAggrNode )
{
/***********************************************************************
 *
 * Description :
 *    복사하여 사용하는 저장 칼럼 정보를 (Materialize 노드) 초기화
 *    사용되는 곳: 정렬키의 칼럼, PARTITION BY, AGGREGATION RESULT
 *
 * Implementation :
 *
 *    이미 initMtrNode에서 초기화한 정보를 다른 next를 가지는
 *    노드로 표현하여 초기화하는 것임. 따라서 이전에 초기화된 노드의 정보.
 *    1. 저장 칼럼의 연결 정보 생성
 *    2. 저장 칼럼을 (next를 제외한 정보를) 검색하여 복사
 *
 ***********************************************************************/
    const qmcMtrNode  * sCodeNode;
    qmdAggrNode       * sAggrNode;
    
    qmdAggrNode    * sFindNode;
    qmdAggrNode    * sNextNode;

    idBool          sIsMatched;

    
    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------
    
    // 1.  저장 Column의 연결 정보 생성
    for( sCodeNode = aCodeNode,
             sAggrNode = aAggrNode;
         sCodeNode != NULL;
         sCodeNode = sCodeNode->next,
             sAggrNode = sAggrNode->next)
    {
        sAggrNode->myNode = (qmcMtrNode*) sCodeNode;
        sAggrNode->srcNode = NULL;
        sAggrNode->next = sAggrNode + 1;

        if( sCodeNode->next == NULL )
        {
            sAggrNode->next = NULL;
        }
    }

    // 2. 이미 존재하는 노드를 검색하여 복사
    for( sAggrNode = aAggrNode;
         sAggrNode != NULL;
         sAggrNode = sAggrNode->next )
    {
        sIsMatched = ID_FALSE;
        
        // DataPlan->aggrNode에서 동일한 노드를 검색하는 부분
        for( sFindNode = aDataPlan->aggrNode;
             sFindNode != NULL;
             sFindNode = sFindNode->next )
        {
            // 검색하는 정보는 결국 복사되어 생성된 정보이므로
            // myNode의 next를 제외한 정보는 일치해야 함
            if( ( sAggrNode->myNode->srcNode == sFindNode->myNode->srcNode ) &&
                ( sAggrNode->myNode->dstNode == sFindNode->myNode->dstNode ) )
            {
                // aggrNode에는 base table이 오면 안됨
                IDE_DASSERT( ((sFindNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_MEMORY_TABLE) &&
                             ((sFindNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_DISK_TABLE) );
                
                sIsMatched = ID_TRUE;
                break;
            }
            else
            {
                // Nothing To Do                 
            }
        }

        // DataPlan->mtrNode에 항상 일치하는 칼럼이 있어야 함
        IDE_TEST_RAISE( sIsMatched == ID_FALSE, ERR_COLUMN_NOT_FOUND );

        // 이전 노드를 복사하기 전에 next 값을 저장
        sNextNode = sAggrNode->next;
        
        // qmdMtrNode 복사
        *sAggrNode = *sFindNode;

        // netxt를 제대로 설정
        sAggrNode->next = sNextNode;
    }
    
    return IDE_SUCCESS;
 
    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::initCopiedAggrNode",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnWNST::initAggrResultMtrNode(const qmndWNST   * aDataPlan,
                                      const qmcMtrNode * aCodeNode,
                                      qmdMtrNode       * aDataNode)
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Result 저장 칼럼 정보 초기화
 *    사용되는 곳: analytic result node
 *
 * Implementation :
 *
 *    이미 initMtrNode에서 초기화한 정보를 다른 next를 가지는
 *    노드로 표현하여 초기화하는 것임. 따라서 이전에 초기화된 노드의 정보.
 *    1. 저장 칼럼의 연결 정보 생성
 *    2. 저장 칼럼을 (next를 제외한 정보를) 검색하여 복사
 *
 ***********************************************************************/

    qmdMtrNode    * sDataNode;
    qmdMtrNode    * sFindNode;
    qmdMtrNode    * sNextNode;
    
    idBool          sIsMatched;

    
    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------
    
    // 1.  저장 Column의 연결 정보 생성
    IDE_TEST( qmc::linkMtrNode( aCodeNode,
                                aDataNode ) != IDE_SUCCESS );


    // 2. 이미 존재하는 노드를 검색하여 복사
    for( sDataNode = aDataNode;
         sDataNode != NULL;
         sDataNode = sDataNode->next )
    {
        sIsMatched = ID_FALSE;
        
        // mtrNode에서 동일한 노드를 검색하는 부분
        for( sFindNode = aDataPlan->mtrNode;
             sFindNode != NULL;
             sFindNode = sFindNode->next )
        {
            // 검색하는 정보는 결국 복사되어 생성된 정보이므로
            // myNode의 next를 제외한 정보는 일치해야 함
            if( ( sDataNode->myNode->srcNode == sFindNode->myNode->srcNode ) &&
                ( sDataNode->myNode->dstNode == sFindNode->myNode->dstNode ) )
            {
                // base table이 오면 안됨
                // 만약 base table의 myNode정보와 일치한다면 오류
                IDE_DASSERT( ((sFindNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_MEMORY_TABLE) &&
                             ((sFindNode->flag & QMC_MTR_TYPE_MASK) != QMC_MTR_TYPE_DISK_TABLE) );
                
                sIsMatched = ID_TRUE;
                break;
            }
            else
            {
                // Nothing To Do                 
            }
        }

        // DataPlan->mtrNode에 항상 일치하는 칼럼이 있어야 함
        IDE_TEST_RAISE( sIsMatched == ID_FALSE, ERR_COLUMN_NOT_FOUND );
        
        // 이전 노드를 복사하기 전에 next 값을 저장
        sNextNode = sDataNode->next;        

        *sDataNode = *sFindNode;

        // BUG-31210 
        sDataNode->flag &= ~QMC_MTR_ANAL_FUNC_RESULT_OF_WND_NODE_MASK;
        sDataNode->flag |= QMC_MTR_ANAL_FUNC_RESULT_OF_WND_NODE_TRUE;
        IDE_TEST( qmc::setFunctionPointer( sDataNode ) != IDE_SUCCESS );

        // next를 제대로 설정
        sDataNode->next = sNextNode;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::initAggrResultMtrNode",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::initWndNode( const qmncWNST    * aCodePlan,
                      const qmndWNST    * aDataPlan,
                      qmdWndNode       ** aWndNode )
{
/***********************************************************************
 *
 * Description :
 *    Window Clause (Analytic Clause) 정보를 담는 qmdWndNode를 설정
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmcWndNode  * sCodeWndNode;
    qmdWndNode        * sDataWndNode;
    qmdMtrNode        * sMtrNode;
    UInt                sSortKeyCnt = aCodePlan->sortKeyCnt;
    UInt                i;

    const qmdWndNode  * sNodeBase;   // 노드가 저장될 시작 위치
    const void        * sNextNode;   // 다음 노드가 저장될 위치
    const qmcMtrNode  * sNode;       // 노드 탐색을 위한 임시 변수
    
    
    // sWNdNodeBase 위치 초기화
    // 앞 부분에 각 Clause를 위한 qmdWndNode*를 저장할 공간을 제외한 위치를
    // wndNode를 위한 정보가 저장될 시작 주소
    // 예를 들어 아래와 같이 정보가 연결됨
    // [wndNode*][wndNode*][wndNode*]
    // [wndNode][overColumnNodes...][aggrNodes...][aggrResultNodes...]
    // [wndNode][overColumnNodes...][aggrNodes...][aggrResultNodes...]
    sNodeBase = (qmdWndNode*)(aWndNode + sSortKeyCnt);
    sNextNode = (void*)sNodeBase;

    // 같은 정렬키를 공유하는 Clause는 next로 연결되어 있음
    for( i = 0;
         i < sSortKeyCnt;
         i++ )
    {
        // 현재 sDataWndNode의 위치 설정
        sDataWndNode = (qmdWndNode*)sNextNode;

        // 현재 data wnd node 위치를 설정
        aWndNode[i]  = sDataWndNode;

        // 정렬키를 공유하는 Wnd Node 연결
        for( sCodeWndNode = aCodePlan->wndNode[i];
             sCodeWndNode != NULL;
             sCodeWndNode = sCodeWndNode->next,
                 sDataWndNode = sDataWndNode->next )
        {
            // 다음 노드 위치 설정
            sNextNode    = (UChar*)sNextNode + idlOS::align8( ID_SIZEOF(qmdWndNode) );
            
            //-----------------------------------------------    
            // initOverColumnNode
            //-----------------------------------------------

            if( sCodeWndNode->overColumnNode != NULL )
            {
                // PARTITION BY가 존재하는 경우

                // overColumnNode위치 할당
                sDataWndNode->overColumnNode = (qmdMtrNode*)sNextNode;
                sDataWndNode->orderByColumnNode = NULL;

                // overColumnNode에 연결된 칼럼 수를 고려하여 다음 노드가 사용할 위치 설정
                for( sNode = sCodeWndNode->overColumnNode;
                     sNode != NULL;
                     sNode = sNode->next )
                {
                    sNextNode = (UChar*)sNextNode + idlOS::align8( ID_SIZEOF(qmdMtrNode) );
                }
            
                // initOverColumnNode
                IDE_TEST( initCopiedMtrNode( aDataPlan,
                                             sCodeWndNode->overColumnNode,
                                             sDataWndNode->overColumnNode )
                          != IDE_SUCCESS );
                for ( sMtrNode = sDataWndNode->overColumnNode;
                      sMtrNode != NULL;
                      sMtrNode = sMtrNode->next )
                {
                    if ( (sMtrNode->myNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
                         == QMC_MTR_SORT_ORDER_FIXED_TRUE )
                    {
                        sDataWndNode->orderByColumnNode = sMtrNode;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            else
            {
                // 빈 OVER()만을 위한 wndNode인 경우
                sDataWndNode->overColumnNode  = NULL;
            }            
            
            //-----------------------------------------------    
            // initAggrNode
            //-----------------------------------------------

            // aggrNode위치 할당
            sDataWndNode->aggrNode = (qmdAggrNode*)sNextNode;

            // aggrNode에 연결된 칼럼의 수를 고려하여 다음 노드가 사용할 위치 설정
            for( sNode = sCodeWndNode->aggrNode;
                 sNode != NULL;
                 sNode = sNode->next )
            {
                sNextNode = (UChar*)sNextNode + idlOS::align8( ID_SIZEOF(qmdAggrNode) );
            }
            
            // initAggrNode
            IDE_TEST( initCopiedAggrNode( aDataPlan,
                                          sCodeWndNode->aggrNode,
                                          sDataWndNode->aggrNode )
                      != IDE_SUCCESS );
            
            //-----------------------------------------------    
            // initAggrResultMtrNode
            //-----------------------------------------------
            
            // aggrResultNode위치 할당
            sDataWndNode->aggrResultNode = (qmdMtrNode*)sNextNode;

            // aggrResultNode에 연결된 칼럼의 수를 고려하여 다음 노드가 사용할 위치 설정
            for( sNode = sCodeWndNode->aggrResultNode;
                 sNode != NULL;
                 sNode = sNode->next )
            {
                sNextNode = (UChar*)sNextNode + idlOS::align8( ID_SIZEOF(qmdMtrNode) );
            }
            
            // initAggrResultNode
            IDE_TEST( initAggrResultMtrNode( aDataPlan,
                                             sCodeWndNode->aggrResultNode,
                                             sDataWndNode->aggrResultNode )
                      != IDE_SUCCESS );
            
            //-----------------------------------------------    
            // initExecMethod
            //-----------------------------------------------

            sDataWndNode->execMethod = sCodeWndNode->execMethod;
            sDataWndNode->window     = (qmcWndWindow *)&sCodeWndNode->window;

            if( sCodeWndNode->next == NULL )
            {
                // 마지막 노드는 next가 없음
                sDataWndNode->next = NULL;
            }
            else
            {
                // WndNode->next 위치 설정
                sDataWndNode->next = (qmdWndNode*)sNextNode;
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::initTempTable( qcTemplate      * aTemplate,
                        const qmncWNST  * aCodePlan,
                        qmndWNST        * aDataPlan,
                        qmcdSortTemp    * aSortMgr )
{
/***********************************************************************
 *
 * Description :
 *    Sort Temp Table을 초기화
 *
 * Implementation :
 *    Disk인 경우 모든 정렬키를 위한 각각의 Sort Manager를 미리 초기화
 *
 ***********************************************************************/
    UInt        sFlag;
    qmndWNST  * sCacheDataPlan = NULL;
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
        
        //-----------------------------
        // Temp Table 초기화
        //-----------------------------

        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
        {
            // Memory Sort Temp Table의 경우, 정렬 키를 변경하며
            // 반복 정렬이 가능하므로 첫번째 정렬키에 대해서만 초기화를 수행함
            IDE_TEST( qmcSortTemp::init( aSortMgr,
                                         aTemplate,
                                         ID_UINT_MAX,
                                         aDataPlan->mtrNode,
                                         aDataPlan->sortNode[0],
                                         0,
                                         sFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            /* PROJ-2462 Result Cache */
            sCacheDataPlan = (qmndWNST *) (aTemplate->resultCache.data +
                                          aCodePlan->plan.offset);
            if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
                 == QMX_RESULT_CACHE_INIT_DONE_FALSE )
            {
                aDataPlan->sortMgr = (qmcdSortTemp*)(aTemplate->resultCache.data +
                                                     aCodePlan->sortMgrOffset);
                IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                             aTemplate,
                                             sCacheDataPlan->resultData.memoryIdx,
                                             aDataPlan->mtrNode,
                                             aDataPlan->sortNode[0],
                                             0,
                                             sFlag )
                          != IDE_SUCCESS );
                sCacheDataPlan->sortMgr = aDataPlan->sortMgr;
            }
            else
            {
                aDataPlan->sortMgr = sCacheDataPlan->sortMgr;
                IDE_TEST( qmcSortTemp::setSortNode( aDataPlan->sortMgr,
                                                    aDataPlan->sortNode[0] )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        sFlag &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag |= QMCD_SORT_TMP_STORAGE_DISK;

        /* PROJ-2201 
         * 재정렬, BackwardScan등을 하려면 RangeFlag를 줘야함 */
        sFlag &= ~QMCD_SORT_TMP_SEARCH_MASK;
        sFlag |= QMCD_SORT_TMP_SEARCH_RANGE;

        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_DISK );
        
        //-----------------------------
        // Temp Table 초기화
        //-----------------------------

        // Disk Sort Temp Table의 경우,
        // 모든 정렬 키에 대해 미리 초기화를 수행함
        IDE_TEST( qmcSortTemp::init( aSortMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->sortNode[0],
                                     aCodePlan->storeRowCount,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::setMtrRow( qcTemplate     * aTemplate,
                    qmndWNST       * aDataPlan )
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
}

IDE_RC
qmnWNST::setTupleSet( qcTemplate   * aTemplate,
                      qmdMtrNode   * aMtrNode,
                      void         * aRow )
{
/***********************************************************************
 *
 * Description :
 *    검색된 저장 Row를 기준으로 Tuple Set을 복원한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmdMtrNode * sNode;

    for ( sNode = aMtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate,
                                        sNode,
                                        aRow )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::performAnalyticFunctions( qcTemplate     * aTemplate,
                                   const qmncWNST * aCodePlan,
                                   qmndWNST       * aDataPlan,
                                   UInt             aFlag )
{
/***********************************************************************
 *
 * Description :
 *    모든 Analytic Function을 수행하여 Temp Table에 저장한다
 *
 * Implementation :
 *    1. Child를 반복 수행하여 Temp Table에 Insert
 *    2. 첫 번째 정렬 키에 대해 sort() 수행
 *    3. Reporting Aggregation을 수행하고 결과를 Temp Table에 Update
 *    4. 만약 정렬키가 둘 이상이라면 아래를 반복
 *    4.1. 정렬키를 변경
 *    4.2. 변경된 정렬키에 대해 다시 정렬을 수행
 *    4.3. Reporting Aggregation을 수행하고 결과를 Temp Table에 Update
 *
 ***********************************************************************/
    UInt         i;

    // Sort Manager 설정
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_DISK )
    {
        // 디스크 소트 템프인 경우
        // 처음 수행시 sortMgr가 sortMgrForDisk를 가리키고 있음
        // 하지만 반복 수행시 (firstInit)이 수행되지 않으므로
        // 이를 대비해여 값을 초기화
        aDataPlan->sortMgr = aDataPlan->sortMgrForDisk;
    }
    else
    {
        // 메모리인 경우 할일이 없음
    }

    
    //----------------------------------------
    // 2. 첫 번째 정렬 키에 대해 sort() 수행
    //----------------------------------------
    if( ( aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK )
        == QMN_PLAN_STORAGE_DISK )
    {
        // disk일 경우 SORTING or PRESEVED_ORDER 일 경우 정렬을 함
        if( ( ( aFlag & QMNC_WNST_STORE_MASK )
              == QMNC_WNST_STORE_SORTING ) ||
            ( ( aFlag & QMNC_WNST_STORE_MASK )
              == QMNC_WNST_STORE_PRESERVED_ORDER ) )
        {
            IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        // memory일 경우 SORTING 일때만 정렬을 함
        if( ( aFlag & QMNC_WNST_STORE_MASK )
               == QMNC_WNST_STORE_SORTING )
        {
            IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2462 Result Cache
            if ( ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
                   == QMN_PLAN_RESULT_CACHE_EXIST_TRUE ) &&
                 ( ( aFlag & QMNC_WNST_STORE_MASK )
                     == QMNC_WNST_STORE_PRESERVED_ORDER ) )
            {
                if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_USE_PRESERVED_ORDER_MASK )
                     == QMX_RESULT_CACHE_USE_PRESERVED_ORDER_TRUE )
                {
                    IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                              != IDE_SUCCESS );
                }
                else
                {
                    if ( aCodePlan->sortKeyCnt > 1 )
                    {
                        *aDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_USE_PRESERVED_ORDER_MASK;
                        *aDataPlan->resultData.flag |= QMX_RESULT_CACHE_USE_PRESERVED_ORDER_TRUE;
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
        }
    }

    //----------------------------------------
    // 3. Reporting Aggregation을 수행하고 결과를 Temp Table에 Update
    //----------------------------------------
    IDE_TEST( aggregateAndUpdate( aTemplate,
                                  aDataPlan,
                                  aDataPlan->wndNode[0] )
              != IDE_SUCCESS);

    
    //----------------------------------------
    // 4. 만약 정렬키가 둘 이상이라면 아래를 반복
    //----------------------------------------
    for( i = 1;
         i < aCodePlan->sortKeyCnt;
         i++ )
    {
        // 4.1. 정렬키를 변경
        IDE_TEST( qmcSortTemp::setSortNode( aDataPlan->sortMgr,
                                            aDataPlan->sortNode[i] )
                  != IDE_SUCCESS );

        // 4.2. 변경된 정렬키에 대해 다시 정렬을 수행
        // 두 번째 이후 정렬키는 PRESERVED ORDER가 적용되지 않으므로 무조건 정렬
        IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        // 4.3. Reporting Aggregation을 수행하고 결과를 Temp Table에 Update
        IDE_TEST( aggregateAndUpdate( aTemplate,
                                      aDataPlan,
                                      aDataPlan->wndNode[i] )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::insertRowsFromChild( qcTemplate     * aTemplate,
                              const qmncWNST * aCodePlan,
                              qmndWNST       * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Child를 반복 수행하여 Temp Table을 구축
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;
    
    //------------------------------
    // Child Record의 저장
    //------------------------------

    // aggrNode에 초기 값을 설정
    // aggregation에서 execution 없이 init후 바로 finalize하면
    // grouping이 NULL인 경우의 값이 설정됨
    
    

    // Child 수행
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag )
              != IDE_SUCCESS );
    
    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // 공간의 할당
        IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                      & aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        // 저장 Row의 구성
        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );
        
        // Row의 삽입
        IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                       aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        // Child 수행
        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag )
                  != IDE_SUCCESS );
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
qmnWNST::insertLimitedRowsFromChild( qcTemplate     * aTemplate,
                                     const qmncWNST * aCodePlan,
                                     qmndWNST       * aDataPlan,
                                     SLong            aLimitNum )
{
/***********************************************************************
 *
 * Description :
 *    Child를 반복 수행하여 상위 n개의 rocord만 갖는 memory temp를 구축
 *
 * Implementation :
 *
 ***********************************************************************/
    
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;
    SLong        sCount = 0;

    IDE_TEST_RAISE( aCodePlan->sortKeyCnt != 1,
                    ERR_INVALID_KEY_COUNT );
    
    //------------------------------
    // Child Record의 저장
    //------------------------------

    // Child 수행
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );
    
    while ( ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST ) &&
            ( sCount < aLimitNum ) )
    {
        sCount++;
        
        // 공간의 할당
        IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                      & aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        // 저장 Row의 구성
        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );
        
        // Row의 삽입
        IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                       aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        // Child 수행
        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag )
                  != IDE_SUCCESS );
    }
    
    //------------------------------
    // 정렬 수행
    //------------------------------

    if ( ( aCodePlan->flag & QMNC_WNST_STORE_MASK )
         == QMNC_WNST_STORE_LIMIT_SORTING )
    {
        IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        //------------------------------
        // Limit Sorting 수행
        //------------------------------

        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            // 공간의 할당
            IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                          & aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                // 저장 Row의 구성
                IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                          != IDE_SUCCESS );

                IDE_TEST( qmcSortTemp::shiftAndAppend( aDataPlan->sortMgr,
                                                       aDataPlan->plan.myTuple->row,
                                                       & aDataPlan->plan.myTuple->row )
                          != IDE_SUCCESS );

                IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                                      aCodePlan->plan.left,
                                                      & sFlag )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* QMNC_WNST_STORE_LIMIT_PRESERVED_ORDER */
        
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_KEY_COUNT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::insertLimitedRowsFromChild",
                                  "Invalid key count" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::aggregateAndUpdate( qcTemplate       * aTemplate,
                             qmndWNST         * aDataPlan,
                             const qmdWndNode * aWndNode )
{
/***********************************************************************
 *
 * Description :
 *    Reporting Aggregation을 수행하고 결과를 Temp Table에 Update
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmdWndNode    * sWndNode;

    for( sWndNode = aWndNode;
         sWndNode != NULL;
         sWndNode = sWndNode->next )
    {
        switch ( sWndNode->execMethod )
        {
            case QMC_WND_EXEC_PARTITION_ORDER_UPDATE:
            {
                // partition by와 order by가 함께 있는 경우
                IDE_TEST( partitionOrderByAggregation( aTemplate,
                                                       aDataPlan,
                                                       sWndNode->overColumnNode,
                                                       sWndNode->aggrNode,
                                                       sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;   
            }

            case QMC_WND_EXEC_PARTITION_UPDATE:
            {
                // partition by만 있는 경우
                IDE_TEST( partitionAggregation( aTemplate,
                                                aDataPlan,
                                                sWndNode->overColumnNode,
                                                sWndNode->aggrNode,
                                                sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }

            case QMC_WND_EXEC_ORDER_UPDATE:
            {
                // order by만 있는 경우
                IDE_TEST( orderByAggregation( aTemplate,
                                            aDataPlan,
                                            sWndNode->overColumnNode,
                                            sWndNode->aggrNode,
                                            sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }

            case QMC_WND_EXEC_AGGR_UPDATE:
            {
                // 빈 over절인 경우
                IDE_TEST( aggregationOnly( aTemplate,
                                           aDataPlan,
                                           sWndNode->aggrNode,
                                           sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }

            case QMC_WND_EXEC_PARTITION_ORDER_WINDOW_UPDATE:
            {
                IDE_TEST( windowAggregation( aTemplate,
                                             aDataPlan,
                                             sWndNode->window,
                                             sWndNode->overColumnNode,
                                             sWndNode->orderByColumnNode,
                                             sWndNode->aggrNode,
                                             sWndNode->aggrResultNode,
                                             ID_TRUE )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_ORDER_WINDOW_UPDATE:
            {
                IDE_TEST( windowAggregation( aTemplate,
                                             aDataPlan,
                                             sWndNode->window,
                                             sWndNode->overColumnNode,
                                             sWndNode->orderByColumnNode,
                                             sWndNode->aggrNode,
                                             sWndNode->aggrResultNode,
                                             ID_FALSE )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_PARTITION_ORDER_UPDATE_LAG:
            {
                IDE_TEST( partitionOrderByLagAggr( aTemplate,
                                                   aDataPlan,
                                                   sWndNode->overColumnNode,
                                                   sWndNode->aggrNode,
                                                   sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_PARTITION_ORDER_UPDATE_LEAD:
            {
                IDE_TEST( partitionOrderByLeadAggr( aTemplate,
                                                    aDataPlan,
                                                    sWndNode->overColumnNode,
                                                    sWndNode->aggrNode,
                                                    sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_ORDER_UPDATE_LAG:
            {
                IDE_TEST( orderByLagAggr( aTemplate,
                                          aDataPlan,
                                          sWndNode->aggrNode,
                                          sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_ORDER_UPDATE_LEAD:
            {
                IDE_TEST( orderByLeadAggr( aTemplate,
                                           aDataPlan,
                                           sWndNode->aggrNode,
                                           sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_PARTITION_ORDER_UPDATE_NTILE:
            {
                IDE_TEST( partitionOrderByNtileAggr( aTemplate,
                                                     aDataPlan,
                                                     sWndNode->overColumnNode,
                                                     sWndNode->aggrNode,
                                                     sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }
            case QMC_WND_EXEC_ORDER_UPDATE_NTILE:
            {
                IDE_TEST( orderByNtileAggr( aTemplate,
                                            aDataPlan,
                                            sWndNode->aggrNode,
                                            sWndNode->aggrResultNode )
                          != IDE_SUCCESS );
                break;
            }
            default:
            {
                IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                break;
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_EXEC_METHOD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::aggregateAndUpdate",
                                  "Invalid exec method" ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmnWNST::aggregationOnly( qcTemplate        * aTemplate,
                          qmndWNST          * aDataPlan,
                          const qmdAggrNode * aAggrNode,
                          const qmdMtrNode  * aAggrResultNode )
{
/***********************************************************************
 *
 * Description :
 *    파티션이 지정되지 않은 경우 전체에 대해 aggregation을 수행하고,
 *    그 (단일) 결과를 Sort Temp에 반영함
 *
 * Implementation :
 *    1. 같은 파티션에 대해 aggregation 수행
 *    2. Aggregation 결과를 Sort Temp에 반영 (update)
 *
 ***********************************************************************/
    qmcRowFlag         sFlag = QMC_ROW_INITIALIZE;
    qmdMtrNode       * sNode;
    mtcRankValueType   sRankValue;
    
    //---------------------------------
    // set update columns
    //---------------------------------
    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );


    //----------------------------------------
    // 1. 같은 파티션에 대해 aggregation 수행
    //----------------------------------------    

    //---------------------------------
    // 첫 번째 레코드를 가져옴
    //---------------------------------

    // 현재 row 설정
    aDataPlan->mtrRowIdx = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate,
                              aDataPlan,
                              & sFlag )
              != IDE_SUCCESS );

    if( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //---------------------------------
        // clearDistNode
        //---------------------------------
        IDE_TEST( clearDistNode( aDataPlan->distNode,
                                 aDataPlan->distNodeCnt )
                  != IDE_SUCCESS );
        
        //---------------------------------
        // initAggregation
        //---------------------------------
        IDE_TEST( initAggregation( aTemplate,
                                   aAggrNode )
                  != IDE_SUCCESS );

        sRankValue = MTC_RANK_VALUE_FIRST;
        
        do
        {
            //---------------------------------
            // execAggregation
            //---------------------------------
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       (void*)&sRankValue,
                                       aDataPlan->distNode,
                                       aDataPlan->distNodeCnt )
                      != IDE_SUCCESS );

            //---------------------------------
            // 다음 레코드를 가져옴
            //---------------------------------

            // 현재 row 설정
            aDataPlan->mtrRowIdx = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            
            IDE_TEST( getNextRecord( aTemplate,
                                     aDataPlan,
                                     & sFlag )
                      != IDE_SUCCESS );

            //---------------------------------
            // 레코드가 존재하면 반복
            //---------------------------------
        }
        while( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST );

        //---------------------------------
        // finiAggregation
        //---------------------------------
        IDE_TEST( finiAggregation( aTemplate,
                                   aAggrNode )
                  != IDE_SUCCESS );

        //----------------------------------------
        // 2. Aggregation 결과를 Sort Temp에 반영
        //----------------------------------------
        
        //---------------------------------
        // 다시 첫 번째 레코드를 가져옴
        //---------------------------------

        // 현재 row 설정
        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        
        IDE_TEST( getFirstRecord( aTemplate,
                                  aDataPlan,
                                  & sFlag )
                  != IDE_SUCCESS );
        
        do
        {
            //---------------------------------
            // UPDATE
            //---------------------------------

            // 저장 Row의 구성
            for ( sNode = (qmdMtrNode*)aAggrResultNode;
                  sNode != NULL;
                  sNode = sNode->next )
            {
                /* BUG-43087 support ratio_to_report
                 * RATIO_TO_REPORT 함수는 finalize에서 비율을 결정하기 때문에
                 * Aggretation의 Result를 복사하기 전 현제 row를 구할 때 finalize
                 * 를 수행한다.
                 */
                if ( sNode->srcNode->node.module == &mtfRatioToReport )
                {
                    IDE_TEST( qtc::finalize( sNode->srcNode, aTemplate )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( sNode->func.setMtr( aTemplate,
                                              sNode,
                                              aDataPlan->plan.myTuple->row )
                          != IDE_SUCCESS );
            }

            // update()
            IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                      != IDE_SUCCESS );
            

            //---------------------------------
            // 다음 레코드를 가져옴
            //---------------------------------

            // 현재 row 설정
            aDataPlan->mtrRowIdx = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            
            IDE_TEST( getNextRecord( aTemplate,
                                     aDataPlan,
                                     & sFlag )
                      != IDE_SUCCESS );


            //---------------------------------
            // 레코드가 존재하면 반복
            //---------------------------------
        }
        while( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST );
        
    }
    else
    {
        // 레코드가 하나도 없는 경우 할일이 없음
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::partitionAggregation( qcTemplate        * aTemplate,
                                     qmndWNST    * aDataPlan,
                               const qmdMtrNode  * aOverColumnNode,
                               const qmdAggrNode * aAggrNode,
                               const qmdMtrNode  * aAggrResultNode )
{
/***********************************************************************
 *
 * Description :
 *    파티션 별로 aggregation을 수행하고, 그 결과를 Sort Temp에 반영함
 *
 * Implementation :
 *    1. 같은 파티션에 대해 aggregation 수행
 *    2. Aggregation 결과를 Sort Temp에 반영 (update)
 *
 ***********************************************************************/
    qmcRowFlag        sFlag = QMC_ROW_INITIALIZE;
    qmdMtrNode      * sNode;
    SLong             sExecAggrCnt = 0;  // execAggregation()을 수행한 카운트
    mtcRankValueType  sRankValue;

    //---------------------------------
    // set update columns
    //---------------------------------
    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    
    //----------------------------------------
    // 1. 같은 파티션에 대해 aggregation 수행
    //----------------------------------------    

    //---------------------------------
    // 첫 번째 레코드를 가져옴
    //---------------------------------

    // 현재 row 설정
    aDataPlan->mtrRowIdx = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate,
                              aDataPlan,
                              & sFlag )
              != IDE_SUCCESS );

    if( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // 레코드가 존재하면 아래를 반복
        do
        {   
            //---------------------------------
            // store cursor
            //---------------------------------
            // 현재 위치의 커서를 저장함
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                & aDataPlan->cursorInfo )
                      != IDE_SUCCESS );
        
            //---------------------------------
            // clearDistNode
            //---------------------------------
            IDE_TEST( clearDistNode( aDataPlan->distNode,
                                     aDataPlan->distNodeCnt )
                      != IDE_SUCCESS );
        
            //---------------------------------
            // initAggregation
            //---------------------------------
            IDE_TEST( initAggregation( aTemplate,
                                       aAggrNode )
                      != IDE_SUCCESS );

            sExecAggrCnt = 0;
            sRankValue = MTC_RANK_VALUE_FIRST;
            
            do
            {
                //---------------------------------
                // execAggregation
                //---------------------------------
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           (void*)&sRankValue,
                                           aDataPlan->distNode,
                                           aDataPlan->distNodeCnt )
                          != IDE_SUCCESS );

                sExecAggrCnt++;

                //---------------------------------
                // 다음 레코드를 가져옴
                //---------------------------------

                // 현재 row 설정
                aDataPlan->mtrRowIdx = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate,
                                         aDataPlan,
                                         & sFlag )
                          != IDE_SUCCESS );

                //---------------------------------
                // 같은 파티션인지 검사
                //---------------------------------

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           & sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Data가 없으면 종료
                    break;
                }

                //---------------------------------
                // 같은 값인지 검사
                //---------------------------------
                
                if ( (sFlag & QMC_ROW_COMPARE_MASK) == QMC_ROW_COMPARE_SAME )
                {
                    sRankValue = MTC_RANK_VALUE_SAME;
                }
                else
                {
                    sRankValue = MTC_RANK_VALUE_DIFF;
                }

                // 같은 파티션이면 반복
            }
            while( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME );
            
            
            //---------------------------------
            // finiAggregation
            //---------------------------------
            IDE_TEST( finiAggregation( aTemplate,
                                       aAggrNode )
                      != IDE_SUCCESS );

            
            //----------------------------------------
            // 2. Aggregation 결과를 Sort Temp에 반영
            //----------------------------------------

            //---------------------------------
            // restore cursor
            //---------------------------------

            // 현재 위치의 커서를 지정된 위치로 복원시킴
            // 현재 row 설정
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            
            // 검색된 Row를 이용한 Tuple Set 복원
            // getFirst & NextRecord 함수는 setTupleSet 기능을 포함하고 있어 필요가 없고,
            // restoreCursor을 호출한 경우에는 setTupleSet을 함께 호출해야 함
            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

            
            do
            {
                //---------------------------------
                // UPDATE
                //---------------------------------
                
                // 저장 Row의 구성
                for ( sNode = (qmdMtrNode*)aAggrResultNode;
                      sNode != NULL;
                      sNode = sNode->next )
                {
                    /* BUG-43087 support ratio_to_report
                     * RATIO_TO_REPORT 함수는 finalize에서 비율을 결정하기 때문에
                     * Aggretation의 Result를 복사하기 전 현제 row를 구할 때 finalize
                     * 를 수행한다.
                     */
                    if ( sNode->srcNode->node.module == &mtfRatioToReport )
                    {
                        IDE_TEST( qtc::finalize( sNode->srcNode, aTemplate )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    IDE_TEST( sNode->func.setMtr( aTemplate,
                                                  sNode,
                                                  aDataPlan->plan.myTuple->row )
                              != IDE_SUCCESS );
                }

                // update()
                IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                          != IDE_SUCCESS );

                sExecAggrCnt--;

                //---------------------------------
                // 같은 파티션인지 검사
                //---------------------------------

                IDE_DASSERT( sExecAggrCnt >= 0 );
                
                if( sExecAggrCnt > 0 )
                {
                    // 현재 row 설정
                    aDataPlan->mtrRowIdx = 1;
                    aDataPlan->plan.myTuple->row =
                        aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    // 현재 row 설정
                    aDataPlan->mtrRowIdx = 0;
                    aDataPlan->plan.myTuple->row =
                        aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                }

                //---------------------------------
                // 같은 파티션인 경우,
                // 다음 레코드를 가져옴
                //---------------------------------
                IDE_TEST( getNextRecord( aTemplate,
                                         aDataPlan,
                                         & sFlag )
                          != IDE_SUCCESS );

                if( sExecAggrCnt > 0 )
                {
                    sFlag &= ~QMC_ROW_GROUP_MASK;
                    sFlag |= QMC_ROW_GROUP_SAME;
                }
                else
                {
                    sFlag &= ~QMC_ROW_GROUP_MASK;
                    sFlag |= QMC_ROW_GROUP_NULL;
                }
                // 레코드가 존재하고, 같은 파티션이면 반복
            }
            while( ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST ) &&
                   ( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME ) );

            
            // 레코드가 존재하고, 다른 파티션이면 새로운 aggregation을 수행
        }
        while( ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST ) &&
               ( (sFlag & QMC_ROW_GROUP_MASK) != QMC_ROW_GROUP_SAME ) );
    }
    else
    {
        // 레코드가 하나도 없는 경우 할일이 없음
    }

    aDataPlan->mtrRowIdx = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Partition By Order By Aggregation
 *
 *   partition By order by RANGE betwwen UNBOUNDED PRECEDING and CURRENT ROW
 *   와 같음 하지만 이렇게 윈도우 구문에서는 Ranking관련 함수는 쓸수 없음.
 */
IDE_RC qmnWNST::partitionOrderByAggregation( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    mtcRankValueType   sRankValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );
        sExecAggrCnt = 0;
        sRankValue   = MTC_RANK_VALUE_FIRST;

        do
        {
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       (void*)&sRankValue,
                                       aDataPlan->distNode,
                                       aDataPlan->distNodeCnt )
                      != IDE_SUCCESS );

            sExecAggrCnt++;
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }

            if ( (sFlag & QMC_ROW_COMPARE_MASK) == QMC_ROW_COMPARE_SAME )
            {
                sRankValue = MTC_RANK_VALUE_SAME;
            }
            else
            {
                sRankValue = MTC_RANK_VALUE_DIFF;
                if ( sExecAggrCnt > 0 )
                {
                    IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                              != IDE_SUCCESS );

                    IDE_TEST( updateAggrRows( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              NULL,
                                              sExecAggrCnt )
                              != IDE_SUCCESS );
                    sExecAggrCnt = 0;
                }
                else
                {
                    /* Nothing to do */
                }
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

        if ( sExecAggrCnt > 0 )
        {
             IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                       != IDE_SUCCESS );

             IDE_TEST( updateAggrRows( aTemplate,
                                       aDataPlan,
                                       aAggrResultNode,
                                       &sFlag,
                                       sExecAggrCnt )
                       != IDE_SUCCESS );
             sExecAggrCnt = 0;
         }
         else
         {
             IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                       != IDE_SUCCESS );
         }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * update Aggregate Rows
 *
 *  Sort Temp에 aExecAggrCount 만큼 aggregate 된 값을 update 한다.
 *
 *  partition by order by 의 의미는 Window의 Range의 개념을 가지고 있다.
 *
 *  그래서 Aggrete 된 값을 모두 같은 값으로 update하는데 Rownumber는 같은 값일 지라도
 *  증가하기 때문에 update 바로전에 Aggregate를 수행해서 update를 수행한다.
 *
 *  다른 함수의 경우 이미 Aggregate 된 값을 aExecAggrCount 만큼 update 한다.
 */
IDE_RC qmnWNST::updateAggrRows( qcTemplate * aTemplate,
                                qmndWNST   * aDataPlan,
                                qmdMtrNode * aAggrResultNode,
                                qmcRowFlag * aFlag,
                                SLong        aExecAggrCount )
{
    qmdMtrNode * sNode;
    SLong        sUpdateCount = 0;
    qmcRowFlag   sFlag        = QMC_ROW_INITIALIZE;

    aDataPlan->mtrRowIdx = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                          &aDataPlan->cursorInfo )
              != IDE_SUCCESS );

    aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

    IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
              != IDE_SUCCESS );

    while ( sUpdateCount < aExecAggrCount )
    {
        for ( sNode = aAggrResultNode;
              sNode != NULL;
              sNode = sNode->next )
        {
            /* mtfRowNumber의 Aggregate를 수행한다 */
            if ( ( sNode->srcNode->node.module == &mtfRowNumber ) ||
                 ( sNode->srcNode->node.module == &mtfRowNumberLimit ) )
            {
                IDE_TEST( qtc::aggregate( sNode->srcNode,
                                          aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            
            IDE_TEST( sNode->func.setMtr( aTemplate,
                                          sNode,
                                          aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        /* SortTemp 에 Update를 수행한다 */
        IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                  != IDE_SUCCESS );
        sUpdateCount++;

        if ( sUpdateCount < aExecAggrCount )
        {
            aDataPlan->mtrRowIdx = 1;
            aDataPlan->plan.myTuple->row =
                aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        }
        else
        {
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row =
                aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        }

        IDE_TEST( getNextRecord( aTemplate,
                                 aDataPlan,
                                 &sFlag )
                  != IDE_SUCCESS );
    }

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothign to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 *  Order By Aggregation
 *
 *   order by RANGE betwwen UNBOUNDED PRECEDING and CURRENT ROW
 *   와 같음. 하지만 이렇게 윈도우 구문에서는 Ranking관련 함수는 쓸수 없음.
 */
IDE_RC qmnWNST::orderByAggregation( qcTemplate  * aTemplate,
                                    qmndWNST    * aDataPlan,
                                    qmdMtrNode  * aOverColumnNode,
                                    qmdAggrNode * aAggrNode,
                                    qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    mtcRankValueType   sRankValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        do
        {
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            sExecAggrCnt = 0;
            sRankValue   = MTC_RANK_VALUE_FIRST;

            do
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           (void*)&sRankValue,
                                           aDataPlan->distNode,
                                           aDataPlan->distNodeCnt )
                          != IDE_SUCCESS );

                sExecAggrCnt++;
                aDataPlan->mtrRowIdx = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }

                if ( (sFlag & QMC_ROW_COMPARE_MASK) == QMC_ROW_COMPARE_SAME )
                {
                    sRankValue = MTC_RANK_VALUE_SAME;
                }
                else
                {
                    sRankValue = MTC_RANK_VALUE_DIFF;
                }

            } while ( ( sFlag & QMC_ROW_COMPARE_MASK ) == QMC_ROW_COMPARE_SAME );

            if ( sExecAggrCnt > 0 )
            {
                IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                          != IDE_SUCCESS );

                IDE_TEST( updateAggrRows( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag,
                                          sExecAggrCnt )
                          != IDE_SUCCESS );
                sExecAggrCnt = 0;
            }
            else
            {
                /* Nothing to do */
            }
        } while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST );

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
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
qmnWNST::clearDistNode( qmdDistNode * aDistNode,
                        const UInt    aDistNodeCnt )
{
/***********************************************************************
 *
 * Description :
 *    Distinct Column을 위한 Temp Table을 Clear
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt  i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDistNode;
          i < aDistNodeCnt;
          i++, sDistNode++ )
    {
        IDE_TEST( qmcHashTemp::clear( & sDistNode->hashMgr )
                  != IDE_SUCCESS );
        sDistNode->mtrRow = sDistNode->dstTuple->row;
        sDistNode->isDistinct = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::initAggregation( qcTemplate        * aTemplate,
                          const qmdAggrNode * aAggrNode )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column을 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmdAggrNode * sNode;
    
    for ( sNode = aAggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::initialize( sNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::execAggregation( qcTemplate         * aTemplate,
                          const qmdAggrNode  * aAggrNode,
                          void               * aAggrInfo,
                          qmdDistNode        * aDistNode,
                          const UInt           aDistNodeCnt )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation을 수행
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmdAggrNode * sAggrNode;

    // BUG-42277
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // set distinct column and insert to hash(DISTINCT)
    IDE_TEST( setDistMtrColumns( aTemplate, aDistNode, aDistNodeCnt )
              != IDE_SUCCESS );

    for ( sAggrNode = aAggrNode;
          sAggrNode != NULL;
          sAggrNode = sAggrNode->next )
    {
        if ( ( sAggrNode->dstNode->node.module == &mtfRowNumber ) ||
             ( sAggrNode->dstNode->node.module == &mtfRowNumberLimit ) )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        
        if ( sAggrNode->myDist == NULL )
        {
            // Non Distinct Aggregation인 경우
            IDE_TEST( qtc::aggregateWithInfo( sAggrNode->dstNode,
                                              aAggrInfo,
                                              aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            // Distinct Aggregation인 경우
            if ( sAggrNode->myDist->isDistinct == ID_TRUE )
            {
                // Distinct Argument인 경우
                IDE_TEST( qtc::aggregateWithInfo( sAggrNode->dstNode,
                                                  aAggrInfo,
                                                  aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                // Non-Distinct Argument인 경우
                // Aggregation을 수행하지 않는다.
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::setDistMtrColumns( qcTemplate        * aTemplate,
                            qmdDistNode       * aDistNode,
                            const UInt          aDistNodeCnt )
{
/***********************************************************************
 *
 * Description :
 *     Distinct Column을 구성한다.
 *
 * Implementation :
 *     Memory 공간을 할당 받고, Distinct Column을 구성
 *     Hash Temp Table에 삽입을 시도한다.
 *
 ***********************************************************************/
    UInt i;
    qmdDistNode * sDistNode;

    for ( i = 0, sDistNode = aDistNode;
          i < aDistNodeCnt;
          i++, sDistNode++ )
    {
        if ( sDistNode->isDistinct == ID_TRUE )
        {
            // 새로운 메모리 공간을 할당
            // Memory Temp Table인 경우에만 새로운 공간을 할당받는다.
            IDE_TEST( qmcHashTemp::alloc( & sDistNode->hashMgr,
                                          & sDistNode->mtrRow )
                      != IDE_SUCCESS );

            sDistNode->dstTuple->row = sDistNode->mtrRow;
        }
        else
        {
            // To Fix PR-8556
            // 이전 메모리를 그대로 사용할 수 있는 경우
            sDistNode->mtrRow = sDistNode->dstTuple->row;
        }

        // Distinct Column을 구성
        IDE_TEST( sDistNode->func.setMtr( aTemplate,
                                          (qmdMtrNode*) sDistNode,
                                          sDistNode->mtrRow ) != IDE_SUCCESS );

        // Hash Temp Table에 삽입
        // Is Distinct의 결과로 삽입 성공 여부를 판단할 수 있다.
        IDE_TEST( qmcHashTemp::addDistRow( & sDistNode->hashMgr,
                                           & sDistNode->mtrRow,
                                           & sDistNode->isDistinct )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC
qmnWNST::finiAggregation( qcTemplate        * aTemplate,
                          const qmdAggrNode * aAggrNode )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation을 마무리
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmdAggrNode * sAggrNode;

    for ( sAggrNode = aAggrNode;
          sAggrNode != NULL;
          sAggrNode = sAggrNode->next )
    {
        /* BUG-43087 support ratio_to_report
         * RATIO_TO_REPORT 함수는 finalize에서 비율을 결정하기 때문에
         * Aggretation의 Result를 복사하기 전 현제 row를 구할때 finalize
         * 를 수행한다.
         */
        if ( sAggrNode->dstNode->node.module != &mtfRatioToReport )
        {
            IDE_TEST( qtc::finalize( sAggrNode->dstNode, aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::compareRows( const qmndWNST   * aDataPlan,
                      const qmdMtrNode * aMtrNode,
                      qmcRowFlag       * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Window Sort의 정보를 출력함
 *
 * Implementation :
 *    1. over column에서 partition by column들로 동일 파티션을 비교한다.
 *    2. over column에서 order by column들로 동일 값을 비교한다.
 *
 ***********************************************************************/
    const qmdMtrNode * sNode;
    SInt               sPartCompResult;
    SInt               sOrderCompResult;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    //------------------------
    // Partition Compare
    //------------------------

    sPartCompResult  = -1;
    sOrderCompResult = -1;
    
    for( sNode = aMtrNode;
         sNode != NULL;
         sNode = sNode->next )
    {
        sValueInfo1.value  = sNode->func.getRow( (qmdMtrNode*)sNode, aDataPlan->mtrRow[0]);
        sValueInfo1.column = (const mtcColumn *) sNode->func.compareColumn;
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.value  = sNode->func.getRow( (qmdMtrNode*)sNode, aDataPlan->mtrRow[1]);
        sValueInfo2.column = (const mtcColumn *) sNode->func.compareColumn;
        sValueInfo2.flag   = MTD_OFFSET_USE;

        if ( (sNode->myNode->flag & QMC_MTR_SORT_ORDER_FIXED_MASK)
             == QMC_MTR_SORT_ORDER_FIXED_FALSE )
        {
            // partition by column
            sPartCompResult = sNode->func.compare( &sValueInfo1,
                                                   &sValueInfo2 );

            if( sPartCompResult != 0 )
            {
                break;
            }
        }
        else
        {
            // order by column
            sOrderCompResult = sNode->func.compare( &sValueInfo1,
                                                    &sValueInfo2 );

            if( sOrderCompResult != 0 )
            {
                break;
            }
        }
    }
    
    if( sPartCompResult == 0 )
    {
        *aFlag &= ~QMC_ROW_GROUP_MASK;
        *aFlag |= QMC_ROW_GROUP_SAME;
    }
    else
    {
        *aFlag &= ~QMC_ROW_GROUP_MASK;
        *aFlag |= QMC_ROW_GROUP_NULL;
    }

    if( sOrderCompResult == 0 )
    {
        *aFlag &= ~QMC_ROW_COMPARE_MASK;
        *aFlag |= QMC_ROW_COMPARE_SAME;
    }
    else
    {
        *aFlag &= ~QMC_ROW_COMPARE_MASK;
        *aFlag |= QMC_ROW_COMPARE_DIFF;
    }
    
    return IDE_SUCCESS;
}

IDE_RC
qmnWNST::allocMtrRow( qcTemplate     * aTemplate,
                      const qmncWNST * aCodePlan,
                      const qmndWNST * aDataPlan,
                      void           * aMtrRow[2] )
{
/***********************************************************************
 *
 * Description :
 *    MTR ROW를 할당
 *
 * Implementation :
 *    Sort Temp Table에서 작업하므로 다음과 같이 할당한다.
 *    1. Memory Sort Temp: 공간을 할당할 필요가 없음
 *    2. Disk Sort Temp: 이미 DataPlan->plan.myTuple->row에 할당될 것이 있으므로
 *                       추가로 하나의 mtrRowSize크기를 할당 받아 사용
 *
 ***********************************************************************/
    iduMemory * sMemory;

    sMemory = aTemplate->stmt->qmxMem;

    //-------------------------------------------
    // 두 Row의 비교를 위한 공간 할당
    //-------------------------------------------
    
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        // nothing to do
    }
    else
    {
        // 이미 할당된 공간이 있으므로 이를 이용
        IDE_DASSERT( aDataPlan->plan.myTuple->row != NULL );
        aMtrRow[0] = aDataPlan->plan.myTuple->row;

        // 비교를 위하 추가로 필요한 공간을 할당
        IDE_TEST( sMemory->alloc( aDataPlan->mtrRowSize,
                                  (void**)&(aMtrRow[1]))
                  != IDE_SUCCESS);
        IDE_TEST_RAISE( aMtrRow[1] == NULL, err_mem_alloc );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mem_alloc );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MEMORY_ALLOCATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::getFirstRecord( qcTemplate  * aTemplate,
                         qmndWNST    * aDataPlan,
                         qmcRowFlag  * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    현재 Temp Table의 첫 번째 레코드를 가져옴
 *
 * Implementation :
 *
 ***********************************************************************/
    void       * sOrgRow;
    void       * sSearchRow;
    
    // 첫 번째 레코드를 가져 옴
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    
    IDE_TEST( qmcSortTemp::getFirstSequence( aDataPlan->sortMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // 검색된 row로 mtrRow 변경
    aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

    // Row 존재 유무 설정 및 Tuple Set 복원
    if ( sSearchRow != NULL )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        // Data가 존재할 경우 Tuple Set 복원
        IDE_TEST( setTupleSet( aTemplate,
                               aDataPlan->mtrNode,
                               aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

        // ????
        aDataPlan->plan.myTuple->modify++;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnWNST::getNextRecord( qcTemplate  * aTemplate,
                        qmndWNST    * aDataPlan,
                        qmcRowFlag  * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    getFirstRecord 이후 반복적으로 호출되며 레코드를 순서대로 하나씩 가져옴
 *
 * Implementation :
 *
 ***********************************************************************/
    void       * sOrgRow;
    void       * sSearchRow;
    
    // 첫 번째 레코드를 가져 옴
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    
    IDE_TEST( qmcSortTemp::getNextSequence( aDataPlan->sortMgr,
                                            & sSearchRow )
              != IDE_SUCCESS );
    
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // 검색된 row로 mtrRow 변경
    aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

    // Row 존재 유무 설정 및 Tuple Set 복원
    if ( sSearchRow != NULL )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        // Data가 존재할 경우 Tuple Set 복원
        IDE_TEST( setTupleSet( aTemplate,
                               aDataPlan->mtrNode,
                               aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

        // ????
        aDataPlan->plan.myTuple->modify++;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }    
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::printAnalyticFunctionInfo( qcTemplate     * aTemplate,
                                    const qmncWNST * aCodePlan,
                                          qmndWNST * aDataPlan,
                                    ULong            aDepth,
                                    iduVarString   * aString,
                                    qmnDisplay       aMode )
{
/***********************************************************************
 *
 * Description :
 *    Window Sort의 정보를 출력함
 *
 * Implementation :
 *
 ***********************************************************************/
    ULong    i;
    UInt     sSortKeyIdx;

    //-----------------------------
    // 첫 번째 정렬키를 출력
    //-----------------------------

    for( sSortKeyIdx = 0;
         sSortKeyIdx < aCodePlan->sortKeyCnt;
         sSortKeyIdx++ )
    {
        //-----------------------------    
        // 1. 정렬키를 출력
        //-----------------------------    
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString, " " );
        }
    
        iduVarStringAppendFormat( aString, "SORT_KEY[%"ID_UINT32_FMT"]: (", sSortKeyIdx );

        if ( aMode == QMN_DISPLAY_ALL )
        {
            // explain plan = on; 인 경우
            if ( (*aDataPlan->flag & QMND_WNST_INIT_DONE_MASK)
                 == QMND_WNST_INIT_DONE_TRUE )
            {
                IDE_TEST( printLinkedColumns( aTemplate,
                                              aDataPlan->sortNode[sSortKeyIdx],
                                              aString )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( printLinkedColumns( aTemplate,
                                              aCodePlan->sortNode[sSortKeyIdx],
                                              aString )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // explain plan = only; 인 경우
            IDE_TEST( printLinkedColumns( aTemplate,
                                          aCodePlan->sortNode[sSortKeyIdx],
                                          aString )
                      != IDE_SUCCESS );
        }
        
        if ( ( sSortKeyIdx == 0 ) &&
             ( ( ( aCodePlan->flag & QMNC_WNST_STORE_MASK )
                   == QMNC_WNST_STORE_PRESERVED_ORDER ) ||
                 ( ( aCodePlan->flag & QMNC_WNST_STORE_MASK )
                   == QMNC_WNST_STORE_LIMIT_PRESERVED_ORDER ) ) )
        {
            // 첫 번째 정렬키가 PRESERVED ORDER인 경우
            // 이를 출력하고 줄바꿈
            iduVarStringAppend( aString, ") PRESERVED ORDER\n" );
        }
        else
        {
            // 그 외의 경우 그냥 줄바꿈
            iduVarStringAppend( aString, ")\n" );  
        }

        //-----------------------------    
        // 2. 관련된  Analytic Function을 모두 출력
        //-----------------------------    
        IDE_TEST( printWindowNode( aTemplate,
                                   aCodePlan->wndNode[sSortKeyIdx],
                                   aDepth+1,
                                   aString )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::printLinkedColumns( qcTemplate       * aTemplate,
                             const qmcMtrNode * aNode,
                             iduVarString     * aString )
{
/***********************************************************************
 *
 * Description :
 *    연결된 칼럼정보(qmcMtrNode)를 쉼표로 연결하여 출력함
 *
 * Implementation :
 *
 ***********************************************************************/
    const qmcMtrNode  * sNode;

   // 연결된 칼럼의 출력
    for( sNode = aNode;
         sNode != NULL;
         sNode = sNode->next )
    {
        IDE_TEST( qmoUtil::printExpressionInPlan( aTemplate,
                                                  aString,
                                                  sNode->srcNode,
                                                  QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );

        // BUG-33663
        if ( (sNode->flag & QMC_MTR_SORT_ORDER_MASK)
             == QMC_MTR_SORT_DESCENDING )
        {
            iduVarStringAppendLength( aString,
                                      " DESC",
                                      5 );
        }
        else
        {
            // Nothing to do.
        }
        
        if( sNode->next != NULL )
        {
            // 쉼표 출력
            iduVarStringAppend( aString, "," );                    
        }
        else
        {
            // 마지막 칼럼
            break;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::printLinkedColumns( qcTemplate    * aTemplate,
                             qmdMtrNode    * aNode,
                             iduVarString  * aString )
{
/***********************************************************************
 *
 * Description :
 *    연결된 칼럼정보(qmcMtrNode)를 쉼표로 연결하여 출력함
 *
 * Implementation :
 *
 ***********************************************************************/
    qmdMtrNode  * sNode;

   // 연결된 칼럼의 출력
    for( sNode = aNode;
         sNode != NULL;
         sNode = sNode->next )
    {
        IDE_TEST( qmoUtil::printExpressionInPlan( aTemplate,
                                                  aString,
                                                  sNode->srcNode,
                                                  QMO_PRINT_UPPER_NODE_NORMAL )
                  != IDE_SUCCESS );
        
        // BUG-33663
        if ( (sNode->flag & QMC_MTR_SORT_ORDER_MASK)
             == QMC_MTR_SORT_DESCENDING )
        {
            iduVarStringAppendLength( aString,
                                      " DESC",
                                      5 );
        }
        else
        {
            // Nothing to do.
        }
        
        if( sNode->next != NULL )
        {
            // 쉼표 출력
            iduVarStringAppend( aString, "," );                    
        }
        else
        {
            // 마지막 칼럼
            break;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnWNST::printWindowNode( qcTemplate       * aTemplate,
                          const qmcWndNode * aWndNode,
                          ULong              aDepth,
                          iduVarString     * aString )
{
/***********************************************************************
 *
 * Description :
 *    연결된 Window Node와 각각의 Analytic Function 정보를
 *    aDepth만큼 들여써서 출력함
 *
 * Implementation :
 *
 ***********************************************************************/
    ULong                  i;
    const qmcWndNode     * sWndNode;
    const qmcMtrNode     * sNode;
    
    for( sWndNode = aWndNode;
         sWndNode != NULL;
         sWndNode = sWndNode->next )
    {
        for( sNode = sWndNode->aggrNode;
             sNode != NULL;
             sNode = sNode->next )
        {
            for ( i = 0; i < aDepth; i++ )
            {
                iduVarStringAppend( aString, " " );
            }

            IDE_TEST( qmoUtil::printExpressionInPlan( aTemplate,
                                                      aString,
                                                      sNode->srcNode,
                                                      QMO_PRINT_UPPER_NODE_NORMAL )
                      != IDE_SUCCESS );

            iduVarStringAppend( aString, "\n" );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * windowAggregation
 *
 * 윈도우에 사용된 옵션에 따라 적절한 동작을 하는 함수를 호출한다.
 */
IDE_RC qmnWNST::windowAggregation( qcTemplate   * aTemplate,
                                   qmndWNST     * aDataPlan,
                                   qmcWndWindow * aWindow,
                                   qmdMtrNode   * aOverColumnNode,
                                   qmdMtrNode   * aOrderByColumn,
                                   qmdAggrNode  * aAggrNode,
                                   qmdMtrNode   * aAggrResultNode,
                                   idBool         aIsPartition )
{

    if ( aWindow->rowsOrRange == QTC_OVER_WINODW_ROWS )
    {
        switch ( aWindow->startOpt ) /* Start Point */
        {
            case QTC_OVER_WINODW_OPT_UNBOUNDED_PRECEDING:
            {
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedUnFollowRows( aTemplate,
                                                                     aDataPlan,
                                                                     aOverColumnNode,
                                                                     aAggrNode,
                                                                     aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedUnFollowRows( aTemplate,
                                                                 aDataPlan,
                                                                 aAggrNode,
                                                                 aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedCurrentRows( aTemplate,
                                                                    aDataPlan,
                                                                    aOverColumnNode,
                                                                    aAggrNode,
                                                                    aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedCurrentRows( aTemplate,
                                                                aDataPlan,
                                                                aAggrNode,
                                                                aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedPrecedRows( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aWindow->endValue.number,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedPrecedRows( aTemplate,
                                                               aDataPlan,
                                                               aWindow->endValue.number,
                                                               aAggrNode,
                                                               aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedFollowRows( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aWindow->endValue.number,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedFollowRows( aTemplate,
                                                               aDataPlan,
                                                               aWindow->endValue.number,
                                                               aAggrNode,
                                                               aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            case QTC_OVER_WINODW_OPT_CURRENT_ROW:
            {
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionCurrentUnFollowRows( aTemplate,
                                                                    aDataPlan,
                                                                    aOverColumnNode,
                                                                    aAggrNode,
                                                                    aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderCurrentUnFollowRows( aTemplate,
                                                                aDataPlan,
                                                                aAggrNode,
                                                                aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        IDE_TEST( currentCurrentRows( aTemplate,
                                                      aDataPlan,
                                                      aAggrNode,
                                                      aAggrResultNode )
                                  != IDE_SUCCESS );
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionCurrentFollowRows( aTemplate,
                                                                  aDataPlan,
                                                                  aOverColumnNode,
                                                                  aWindow->endValue.number,
                                                                  aAggrNode,
                                                                  aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderCurrentFollowRows( aTemplate,
                                                              aDataPlan,
                                                              aWindow->endValue.number,
                                                              aAggrNode,
                                                              aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            case QTC_OVER_WINODW_OPT_N_PRECEDING:
            {
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedUnFollowRows( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aWindow->startValue.number,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedUnFollowRows( aTemplate,
                                                               aDataPlan,
                                                               aWindow->startValue.number,
                                                               aAggrNode,
                                                               aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedCurrentRows( aTemplate,
                                                                  aDataPlan,
                                                                  aOverColumnNode,
                                                                  aWindow->startValue.number,
                                                                  aAggrNode,
                                                                  aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedCurrentRows( aTemplate,
                                                              aDataPlan,
                                                              aWindow->startValue.number,
                                                              aAggrNode,
                                                              aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedPrecedRows( aTemplate,
                                                                 aDataPlan,
                                                                 aOverColumnNode,
                                                                 aWindow->startValue.number,
                                                                 aWindow->endValue.number,
                                                                 aAggrNode,
                                                                 aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedPrecedRows( aTemplate,
                                                             aDataPlan,
                                                             aWindow->startValue.number,
                                                             aWindow->endValue.number,
                                                             aAggrNode,
                                                             aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedFollowRows( aTemplate,
                                                                 aDataPlan,
                                                                 aOverColumnNode,
                                                                 aWindow->startValue.number,
                                                                 aWindow->endValue.number,
                                                                 aAggrNode,
                                                                 aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedFollowRows( aTemplate,
                                                             aDataPlan,
                                                             aWindow->startValue.number,
                                                             aWindow->endValue.number,
                                                             aAggrNode,
                                                             aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            case QTC_OVER_WINODW_OPT_N_FOLLOWING:
            {
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionFollowUnFollowRows( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aWindow->startValue.number,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderFollowUnFollowRows( aTemplate,
                                                               aDataPlan,
                                                               aWindow->startValue.number,
                                                               aAggrNode,
                                                               aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionFollowFollowRows( aTemplate,
                                                                 aDataPlan,
                                                                 aOverColumnNode,
                                                                 aWindow->startValue.number,
                                                                 aWindow->endValue.number,
                                                                 aAggrNode,
                                                                 aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderFollowFollowRows( aTemplate,
                                                             aDataPlan,
                                                             aWindow->startValue.number,
                                                             aWindow->endValue.number,
                                                             aAggrNode,
                                                             aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            default:
                IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                break;
        }
    }
    else if ( aWindow->rowsOrRange == QTC_OVER_WINODW_RANGE )
    {
        switch ( aWindow->startOpt ) /* Start Point */
        {
            case QTC_OVER_WINODW_OPT_UNBOUNDED_PRECEDING:
            {
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedUnFollowRows( aTemplate,
                                                                     aDataPlan,
                                                                     aOverColumnNode,
                                                                     aAggrNode,
                                                                     aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedUnFollowRows( aTemplate,
                                                                 aDataPlan,
                                                                 aAggrNode,
                                                                 aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionOrderByAggregation( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderByAggregation( aTemplate,
                                                          aDataPlan,
                                                          aOverColumnNode,
                                                          aAggrNode,
                                                          aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        IDE_TEST_RAISE( aOrderByColumn->next != NULL,
                                        ERR_INVALID_WINDOW_SPECIFICATION );
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedPrecedFollowRange( aTemplate,
                                                                          aDataPlan,
                                                                          aOverColumnNode,
                                                                          aOrderByColumn,
                                                                          aWindow->endValue.number,
                                                                          aWindow->endValue.type,
                                                                          aAggrNode,
                                                                          aAggrResultNode,
                                                                          ID_TRUE )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedPrecedFollowRange( aTemplate,
                                                                      aDataPlan,
                                                                      aOrderByColumn,
                                                                      aWindow->endValue.number,
                                                                      aWindow->endValue.type,
                                                                      aAggrNode,
                                                                      aAggrResultNode,
                                                                      ID_TRUE )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        IDE_TEST_RAISE( aOrderByColumn->next != NULL,
                                        ERR_INVALID_WINDOW_SPECIFICATION );

                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionUnPrecedPrecedFollowRange( aTemplate,
                                                                          aDataPlan,
                                                                          aOverColumnNode,
                                                                          aOrderByColumn,
                                                                          aWindow->endValue.number,
                                                                          aWindow->endValue.type,
                                                                          aAggrNode,
                                                                          aAggrResultNode,
                                                                          ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderUnPrecedPrecedFollowRange( aTemplate,
                                                                      aDataPlan,
                                                                      aOrderByColumn,
                                                                      aWindow->endValue.number,
                                                                      aWindow->endValue.type,
                                                                      aAggrNode,
                                                                      aAggrResultNode,
                                                                      ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            case QTC_OVER_WINODW_OPT_CURRENT_ROW:
            {
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionCurrentUnFollowRange( aTemplate,
                                                                     aDataPlan,
                                                                     aOverColumnNode,
                                                                     aAggrNode,
                                                                     aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderCurrentUnFollowRange( aTemplate,
                                                                 aDataPlan,
                                                                 aOverColumnNode,
                                                                 aAggrNode,
                                                                 aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        IDE_TEST( currentCurrentRange( aTemplate,
                                                       aDataPlan,
                                                       aOverColumnNode,
                                                       aAggrNode,
                                                       aAggrResultNode )
                                  != IDE_SUCCESS );
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        IDE_TEST_RAISE( aOrderByColumn->next != NULL,
                                        ERR_INVALID_WINDOW_SPECIFICATION );

                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionCurrentFollowRange( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aOrderByColumn,
                                                                   aWindow->endValue.number,
                                                                   aWindow->endValue.type,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderCurrentFollowRange( aTemplate,
                                                               aDataPlan,
                                                               aOrderByColumn,
                                                               aWindow->endValue.number,
                                                               aWindow->endValue.type,
                                                               aAggrNode,
                                                               aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            case QTC_OVER_WINODW_OPT_N_PRECEDING:
            {
                IDE_TEST_RAISE( aOrderByColumn->next != NULL,
                                ERR_INVALID_WINDOW_SPECIFICATION );
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedFollowUnFollowRange( aTemplate,
                                                                          aDataPlan,
                                                                          aOverColumnNode,
                                                                          aOrderByColumn,
                                                                          aWindow->startValue.number,
                                                                          aWindow->startValue.type,
                                                                          aAggrNode,
                                                                          aAggrResultNode,
                                                                          ID_TRUE )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedFollowUnFollowRange( aTemplate,
                                                                      aDataPlan,
                                                                      aOrderByColumn,
                                                                      aWindow->startValue.number,
                                                                      aWindow->startValue.type,
                                                                      aAggrNode,
                                                                      aAggrResultNode,
                                                                      ID_TRUE )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedCurrentRange( aTemplate,
                                                                   aDataPlan,
                                                                   aOverColumnNode,
                                                                   aOrderByColumn,
                                                                   aWindow->startValue.number,
                                                                   aWindow->startValue.type,
                                                                   aAggrNode,
                                                                   aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedCurrentRange( aTemplate,
                                                               aDataPlan,
                                                               aOrderByColumn,
                                                               aWindow->startValue.number,
                                                               aWindow->startValue.type,
                                                               aAggrNode,
                                                               aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedPrecedRange( aTemplate,
                                                                  aDataPlan,
                                                                  aOverColumnNode,
                                                                  aOrderByColumn,
                                                                  aWindow->startValue.number,
                                                                  aWindow->startValue.type,
                                                                  aWindow->endValue.number,
                                                                  aWindow->endValue.type,
                                                                  aAggrNode,
                                                                  aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedPrecedRange( aTemplate,
                                                              aDataPlan,
                                                              aOrderByColumn,
                                                              aWindow->startValue.number,
                                                              aWindow->startValue.type,
                                                              aWindow->endValue.number,
                                                              aWindow->endValue.type,
                                                              aAggrNode,
                                                              aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedFollowRange( aTemplate,
                                                                  aDataPlan,
                                                                  aOverColumnNode,
                                                                  aOrderByColumn,
                                                                  aWindow->startValue.number,
                                                                  aWindow->startValue.type,
                                                                  aWindow->endValue.number,
                                                                  aWindow->endValue.type,
                                                                  aAggrNode,
                                                                  aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedFollowRange( aTemplate,
                                                              aDataPlan,
                                                              aOrderByColumn,
                                                              aWindow->startValue.number,
                                                              aWindow->startValue.type,
                                                              aWindow->endValue.number,
                                                              aWindow->endValue.type,
                                                              aAggrNode,
                                                              aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            case QTC_OVER_WINODW_OPT_N_FOLLOWING:
            {
                IDE_TEST_RAISE( aOrderByColumn->next != NULL,
                                ERR_INVALID_WINDOW_SPECIFICATION );
                switch ( aWindow->endOpt )
                {
                    case QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionPrecedFollowUnFollowRange( aTemplate,
                                                                          aDataPlan,
                                                                          aOverColumnNode,
                                                                          aOrderByColumn,
                                                                          aWindow->startValue.number,
                                                                          aWindow->startValue.type,
                                                                          aAggrNode,
                                                                          aAggrResultNode,
                                                                          ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderPrecedFollowUnFollowRange( aTemplate,
                                                                      aDataPlan,
                                                                      aOrderByColumn,
                                                                      aWindow->startValue.number,
                                                                      aWindow->startValue.type,
                                                                      aAggrNode,
                                                                      aAggrResultNode,
                                                                      ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                        break;
                    case QTC_OVER_WINODW_OPT_CURRENT_ROW:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                    case QTC_OVER_WINODW_OPT_N_PRECEDING:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                    case QTC_OVER_WINODW_OPT_N_FOLLOWING:
                        if ( aIsPartition == ID_TRUE )
                        {
                            IDE_TEST( partitionFollowFollowRange( aTemplate,
                                                                  aDataPlan,
                                                                  aOverColumnNode,
                                                                  aOrderByColumn,
                                                                  aWindow->startValue.number,
                                                                  aWindow->startValue.type,
                                                                  aWindow->endValue.number,
                                                                  aWindow->endValue.type,
                                                                  aAggrNode,
                                                                  aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            IDE_TEST( orderFollowFollowRange( aTemplate,
                                                              aDataPlan,
                                                              aOverColumnNode,
                                                              aWindow->startValue.number,
                                                              aWindow->startValue.type,
                                                              aWindow->endValue.number,
                                                              aWindow->endValue.type,
                                                              aAggrNode,
                                                              aAggrResultNode )
                                      != IDE_SUCCESS );
                        }
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                        break;
                }
                break;
            }
            default:
                IDE_RAISE( ERR_INVALID_EXEC_METHOD );
                break;
        }
    }
    else
    {
        IDE_RAISE( ERR_INVALID_EXEC_METHOD );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_EXEC_METHOD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::windowAggregation",
                                  "Invalid exec method" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_WINDOW_SPECIFICATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By UNBOUNDED PRECEDING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 파티션의 처음부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션의 마지막 까지를 범위로 한다.
 *
 *  Temp Table에서 첫 Row를 읽어서 계속 다음 Row를 읽어들이면서 두 Row가 같은 파티션인지를
 *  검사한다. 같은 파티션인 경우 Aggregation을 수행하고 아닌경우 Aggregation을 마무리하고
 *  해당 Temp Table의 Aggretaion Column에 Update 를 수행한다.
 */
IDE_RC qmnWNST::partitionUnPrecedUnFollowRows( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag       sFlag        = QMC_ROW_INITIALIZE;
    SLong            sExecAggrCnt = 0;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 초기화 과정 및 커서를 저장한다 */
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );
        sExecAggrCnt = 0;

        do
        {
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );
            sExecAggrCnt++;
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 저장된 Row와 현재 Row가 같은 파치션인지 비교한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
            /* 같은 파티션인 경우에만 Aggregation을 수행한다. */
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

        /* 같은 파티션이 아닌경우 Aggregatino을 finiAggregation 을 한다.*/
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        /* Store된 커서로 이동해서 같은 그룹 Row의 값을 update한다 */
        IDE_TEST( updateAggrRows( aTemplate,
                                  aDataPlan,
                                  aAggrResultNode,
                                  &sFlag,
                                  sExecAggrCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By UNBOUNDED PRECEDING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 처음부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 마지막 까지를 범위로 한다.
 *
 *  TempTalbe의 처음부터 끝까지 읽어들이면서 Aggregation을 수행하고 다 된후 처음 부터 끝까지
 *  Update를 수행한다.
 */
IDE_RC qmnWNST::orderUnPrecedUnFollowRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag       sFlag        = QMC_ROW_INITIALIZE;
    SLong            sExecAggrCnt = 0;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    /* Row 가 있다면 초기화를 수행한다 */
    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* Row가 있다면 마지막까지 Aggregation을 수행한다. */
    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( execAggregation( aTemplate,
                                   aAggrNode,
                                   NULL,
                                   NULL,
                                   0 )
                  != IDE_SUCCESS );
        sExecAggrCnt++;
        aDataPlan->mtrRowIdx    = 1;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );
    }

    if ( sExecAggrCnt > 0 )
    {
        /* finilization */
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* Table의 처음으로 이동후 update를 수행한다. */
        IDE_TEST( updateAggrRows( aTemplate,
                                  aDataPlan,
                                  aAggrResultNode,
                                  &sFlag,
                                  sExecAggrCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By UNBOUNDED PRECEDING - CURRENT ROW
 *
 *  Start Point 가 UNBOUNDED PRECEDING 파티션의 처음부터 시작한다.
 *  End   Point 는 CURRENT ROW 현재 Row 까지를 범위로 한다.
 *
 *  파티션의 처음부터 현재 Row까지 Aggregation을 수행하므로 처음부터 Aggregation을 수행하면서
 *  update를 수행하고 만약 다른 그룹이라면 Aggregation을 초기화하고 수행한다.
 */
IDE_RC qmnWNST::partitionUnPrecedCurrentRows( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    qmdMtrNode       * sNode;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        do
        {
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            for ( sNode = ( qmdMtrNode *)aAggrResultNode;
                  sNode != NULL;
                  sNode = sNode->next )
            {
                IDE_TEST( sNode->func.setMtr( aTemplate,
                                              sNode,
                                              aDataPlan->plan.myTuple->row )
                          != IDE_SUCCESS );
            }

            /* 지금까지 계산된 Aggregation을 update한다. */
            IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                      != IDE_SUCCESS );

            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx = 1;
            }
            else
            {
                aDataPlan->mtrRowIdx = 0;
            }
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate,
                                     aDataPlan,
                                     &sFlag )
                      != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                /* 저장된 Row와 현재 Row가 같은 파치션인지 비교한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
            /* 같은 파티션인 경우에만 Aggregation을 수행한다. */
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By UNBOUNDED PRECEDING - CURRENT ROW
 *
 *  Start Point 가 UNBOUNDED PRECEDING 처음부터 시작한다.
 *  End   Point 는 CURRENT ROW 현재 Row 까지를 범위로 한다.
 *
 *  처음부터 현재 Row까지 Aggregation을 수행하므로 처음부터 Aggregation을 수행하면서
 *  update를 수행한다.
 */
IDE_RC qmnWNST::orderUnPrecedCurrentRows( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    qmdMtrNode       * sNode;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );
    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( execAggregation( aTemplate,
                                   aAggrNode,
                                   NULL,
                                   NULL,
                                   0 )
                  != IDE_SUCCESS );

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        for ( sNode = ( qmdMtrNode *)aAggrResultNode;
              sNode != NULL;
              sNode = sNode->next )
        {
            IDE_TEST( sNode->func.setMtr( aTemplate,
                                          sNode,
                                          aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }

        /* 지금까지 계산된 Aggregation을 update한다. */
        IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        if ( aDataPlan->mtrRowIdx == 0 )
        {
            aDataPlan->mtrRowIdx = 1;
        }
        else
        {
            aDataPlan->mtrRowIdx = 0;
        }
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( getNextRecord( aTemplate,
                                 aDataPlan,
                                 &sFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By UNBOUNDED PRECEDING - N PRECEDING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 파티션의 처음부터 시작한다.
 *  End   Point 는 N         PRECEDING 현재 Row부터 N 전의 Row까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 현재 Row가 몇번째인지를
 *  저장하면서 이보다 N 개 전까지를 Aggregation을 수행한뒤에 현재 Row로 Restore한뒤에
 *  Update를 수행한다.
 */
IDE_RC qmnWNST::partitionUnPrecedPrecedRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aEndValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sEndPoint    = aEndValue;
    SLong              sWindowPos;
    SLong              sCount;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sWindowPos = 0;

        /* 파티션의 처음의 cursor를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row의 cursor를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 Cursor로 이동한다. */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

            /* 처음부터 현재 Row의 N개 전까지 Record를 읽으면서 Aggregation 을 수행한다 */
            for ( sCount = sWindowPos - sEndPoint;
                  sCount >= 0;
                  sCount-- )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) != QMC_ROW_DATA_EXIST )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 Row로 커서를 원복한다 */
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );
            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
            /* 같은 파티션인 경우에만 Aggregation을 수행한다. */
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By UNBOUNDED PRECEDING - N PRECEDING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 처음부터 시작한다.
 *  End   Point 는 N         PRECEDING 마지막 까지를 범위로 한다.
 *
 *  현재 Row를 Cursor로 저장한다. 현재 Row가 몇번째인지를
 *  저장하면서 이보다 N 개 전까지를 Aggregation을 수행한뒤에 현재 Row로 Restore한뒤에
 *  Update를 수행한다.
 */
IDE_RC qmnWNST::orderUnPrecedPrecedRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aEndValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sWindowPos   = 0;
    SLong              sCount       = 0;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row의 cursor를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 항상 처음 Row의 Record를 읽는다. */
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
        IDE_TEST( setTupleSet( aTemplate,
                               aDataPlan->mtrNode,
                               aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

        /* 처음부터 현재 Row의 N개 전까지 Record를 읽으면서 Aggregation 을 수행한다 */
        for ( sCount = sWindowPos - sEndPoint;
              sCount >= 0;
              sCount-- )
        {
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) != QMC_ROW_DATA_EXIST )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row로 커서를 원복한다 */
        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* N 개 전까지 Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        ++sWindowPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By UNBOUNDED PRECEDING - N FOLLOWING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 파티션의 처음부터 시작한다.
 *  End   Point 는 N         FOLLOWING 현재 Row부터 N 후의 Row까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 현재 Row가 몇번째인지를
 *  저장하면서 이보다 N 개 후 까지를 Aggregation을 수행한뒤에 현재 Row로 Restore한뒤에
 *  Update를 수행한다.
 */
IDE_RC qmnWNST::partitionUnPrecedFollowRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aEndValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음의 cursor를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        sWindowPos = 0;

        do
        {
            /* 현재 Row의 cursor를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 Cursor로 이동한다. */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

            /* 처음부터 현재 Row의 N개 후까지 Record를 읽으면서 Aggregation 을 수행한다 */
            for ( sCount = sWindowPos + sEndPoint;
                  sCount >= 0;
                  sCount-- )
            {

                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                    if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    break;
                }
            }
            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 Row로 커서를 원복한다 */
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By UNBOUNDED PRECEDING - N FOLLOWING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 처음부터 시작한다.
 *  End   Point 는 N         FOLLOWING 마지막 까지를 범위로 한다.
 *
 *  현재 Row를 Cursor로 저장한다. 현재 Row가 몇번째인지를
 *  저장하면서 이보다 N 개 후 까지를 Aggregation을 수행한뒤에 현재 Row로 Restore한뒤에
 *  Update를 수행한다.
 */
IDE_RC qmnWNST::orderUnPrecedFollowRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aEndValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sWindowPos   = 0;
    SLong              sCount       = 0;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row의 cursor를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 항상 처음 Row의 Record를 읽는다. */
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
        IDE_TEST( setTupleSet( aTemplate,
                               aDataPlan->mtrNode,
                               aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

        /* 처음부터 현재 Row의 N개 후까지 Record를 읽으면서 Aggregation 을 수행한다 */
        for ( sCount = sWindowPos + sEndPoint;
              sCount >= 0;
              sCount-- )
        {

            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) != QMC_ROW_DATA_EXIST )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row로 커서를 원복한다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        ++sWindowPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By CURRENT ROW - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 CURRENT   ROW 현재 Row 부터  시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션의 마지막 Row까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다. 현재 Row 부터 파티션의 끝까지 Aggregation을
 *  수행한 뒤에 현재 Row로 restore후에 update한 뒤에 다음 row를 수행한다.
 */
IDE_RC qmnWNST::partitionCurrentUnFollowRows( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag       sFlag        = QMC_ROW_INITIALIZE;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        do
        {
            /* Aggr초기화 및 현재 Row의 cursor를 저장한다. */
            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            /* 같은 파티션일 경우 모두 Aggregation을 수행한다 */
            do
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 Row로 restore한뒤에 update 그리고 Row를 읽는다. */
            IDE_TEST( updateAggrRows( aTemplate,
                                      aDataPlan,
                                      aAggrResultNode,
                                      &sFlag,
                                      1 )
                      != IDE_SUCCESS );
        } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By CURRENT ROW - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 CURRENT   ROW 현재 Row 부터  시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션의 마지막 Row까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다. 현재 Row 부터 끝까지 Aggregation을
 *  수행한 뒤에 현재 Row로 restore후에 update한 뒤에 다음 row를 수행한다.
 */
IDE_RC qmnWNST::orderCurrentUnFollowRows( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag       sFlag        = QMC_ROW_INITIALIZE;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        do
        {
            /* Aggr초기화 및 현재 Row의 cursor를 저장한다. */
            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            /* 끝까지 Aggregation을 수행한다 */
            do
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Nothing to do */
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );
            /* 현재 Row로 restore한뒤에 update 그리고 다음 Row를 읽는다. */
            IDE_TEST( updateAggrRows( aTemplate,
                                      aDataPlan,
                                      aAggrResultNode,
                                      &sFlag,
                                      1 )
                      != IDE_SUCCESS );
        } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By CURRENT ROW - CURRENT ROW
 *
 *  Start Point 가 CURRENT   ROW 현재 Row 부터  시작한다.
 *  End   Point 는 CURRENT   ROW 현재 Row까지이다.
 *
 *  현재 ROW만 계산해서 UPDATE 한다.
 */
IDE_RC qmnWNST::currentCurrentRows( qcTemplate  * aTemplate,
                                    qmndWNST    * aDataPlan,
                                    qmdAggrNode * aAggrNode,
                                    qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;
    qmdMtrNode * sNode;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        IDE_TEST( execAggregation( aTemplate,
                                   aAggrNode,
                                   NULL,
                                   NULL,
                                   0 )
                  != IDE_SUCCESS );

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        for ( sNode = ( qmdMtrNode *)aAggrResultNode;
              sNode != NULL;
              sNode = sNode->next )
        {
            IDE_TEST( sNode->func.setMtr( aTemplate,
                                          sNode,
                                          aDataPlan->plan.myTuple->row)
                      != IDE_SUCCESS );
        }

        IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        if ( aDataPlan->mtrRowIdx == 0 )
        {
            aDataPlan->mtrRowIdx = 1;
        }
        else
        {
            aDataPlan->mtrRowIdx = 0;
        }
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( getNextRecord( aTemplate,
                                 aDataPlan,
                                 &sFlag )
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By CURRENT ROW - N FOLLOWING
 *
 *  Start Point 가 CURRENT   ROW 현재 Row 부터  시작한다.
 *  End   Point 는 N         FOLLOWING 파티션의 마지막 Row까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다. 현재 Row 부터 N개 후까지 Aggregation을
 *  수행한 뒤에 현재 Row로 restore후에 update한 뒤에 다음 row를 수행한다.
 */
IDE_RC qmnWNST::partitionCurrentFollowRows( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdMtrNode  * aOverColumnNode,
                                            SLong         aEndValue,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 부터 N개 후까지 Record를 읽으면서 계산을 수행한다 */
        for ( sExecAggrCnt = sEndPoint;
              sExecAggrCnt >= 0;
              sExecAggrCnt-- )
        {
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );
            if ( sExecAggrCnt == 0 )
            {
                break;
            }
            else
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                    if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    break;
                }
            }
        }
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row로 restore한뒤에 update 그리고 Row를 읽는다. */
        IDE_TEST( updateAggrRows( aTemplate,
                                  aDataPlan,
                                  aAggrResultNode,
                                  &sFlag,
                                  1 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By CURRENT ROW - N FOLLOWING
 *
 *  Start Point 가 CURRENT ROW 현재 Row 부터  시작한다.
 *  End   Point 는 N       FOLLOWING 파티션의 마지막 Row까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다. 현재 Row 부터 N개 후 까지의 Aggregation을
 *  수행한 뒤에 현재 Row로 restore후에 update한 뒤에 다음 row를 수행한다.
 */
IDE_RC qmnWNST::orderCurrentFollowRows( qcTemplate  * aTemplate,
                                        qmndWNST    * aDataPlan,
                                        SLong         aEndValue,
                                        qmdAggrNode * aAggrNode,
                                        qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row의 N개 후까지 Record를 읽으면서 Aggregation 을 수행한다 */
        for ( sExecAggrCnt = sEndPoint;
              sExecAggrCnt >= 0;
              sExecAggrCnt-- )
        {

            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );
            if ( sExecAggrCnt == 0 )
            {
                break;
            }
            else
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Nothing to do */
                }
                else
                {
                    break;
                }
            }
        }
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row로 restore한뒤에 update 그리고 Row를 읽는다. */
        IDE_TEST( updateAggrRows( aTemplate,
                                  aDataPlan,
                                  aAggrResultNode,
                                  &sFlag,
                                  1 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N PRECEDING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 N         PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션의 마지막 Row까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 파티션의 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 파티션의 끝까지 Aggregation 수행
 */
IDE_RC qmnWNST::partitionPrecedUnFollowRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aStartValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag sFlag        = QMC_ROW_INITIALIZE;
    SLong      sCount       = 0;
    SLong      sWindowPos   = 0;
    SLong      sStartPoint  = aStartValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        sWindowPos = 0;

        do
        {
            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 돌아간다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

            /* 파티션의 처음에서 현재 Row에서 N개 전까지 SKIP 한다 */
            for ( sCount = sWindowPos - sStartPoint;
                  sCount > 0;
                  sCount-- )
            {
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
            }

            /* N개 전 Row부터 파티션의 끝까지 Aggregation을 수행한다 */
            do
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By N PRECEDING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 N         PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션의 마지막 Row까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 파티션의 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 끝까지 Aggregation 수행
 */
IDE_RC qmnWNST::orderPrecedUnFollowRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aStartValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        /* 파티션의 처음에서 현재 Row에서 N개 전까지 SKIP 한다 */
        for ( sCount = sWindowPos - sStartPoint;
              sCount > 0;
              sCount-- )
        {
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothign to do */
            }
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        /* N개 전 Row부터 끝까지 Aggregation을 수행한다 */
        do
        {
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );

            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothign to do */
            }
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        } while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST );

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        ++sWindowPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N PRECEDING - CURRENT ROW
 *
 *  Start Point 가 N       PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 CURRENT ROW 현재 Row까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 파티션의 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 현재 Row까지 Aggregation 수행
 */
IDE_RC qmnWNST::partitionPrecedCurrentRows( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdMtrNode  * aOverColumnNode,
                                            SLong         aStartValue,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        sWindowPos = 0;
        do
        {
            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 돌아간다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );


            /* 파티션의 처음에서 현재 Row에서 N개 전까지 SKIP 한다 */
            for ( sCount = sWindowPos - sStartPoint, sExecAggrCnt = 0;
                  sCount > 0;
                  sCount--, sExecAggrCnt++ )
            {
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
            }

            do
            {
                /* N개 전 Row부터 현재 Row까지 Aggregation을 수행한다 */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );

                if ( sWindowPos <= sExecAggrCnt )
                {
                    break;
                }
                else
                {
                    ++sExecAggrCnt;
                }
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N PRECEDING - CURRENT ROW
 *
 *  Start Point 가 N       PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 CURRENT ROW 현재 Row까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 파티션의 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 현재 Row까지 Aggregation 수행
 */
IDE_RC qmnWNST::orderPrecedCurrentRows( qcTemplate  * aTemplate,
                                        qmndWNST    * aDataPlan,
                                        SLong         aStartValue,
                                        qmdAggrNode * aAggrNode,
                                        qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 처음으로 돌아간다 */
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        /* 처음에서 현재 Row에서 N개 전까지 SKIP 한다 */
        for ( sCount = sWindowPos - sStartPoint, sExecAggrCnt = 0;
              sCount > 0;
              sCount--, sExecAggrCnt++ )
        {
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        do
        {
            /* N개 전 Row부터 현재 Row까지 Aggregation을 수행한다 */
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );

            if ( sWindowPos <= sExecAggrCnt )
            {
                break;
            }
            else
            {
                ++sExecAggrCnt;
            }
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        ++sWindowPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N PRECEDING - N PRECEDING
 *
 *  Start Point 가 N PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N PRECEDING 현재 Row의 N 개 전 까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 파티션의 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 현재 Row까지 Aggregation 수행
 */
IDE_RC qmnWNST::partitionPrecedPrecedRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOverColumnNode,
                                           SLong         aStartValue,
                                           SLong         aEndValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        sWindowPos = 0;
        do
        {
            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            if ( sStartPoint < sEndPoint )
            {
                /* EndPoint값이 클경우 계산을 마친다 */
                sWindowPos = sStartPoint;
            }
            else
            {
                /* 파티션의 처음으로 Restore한다 */
                aDataPlan->mtrRowIdx = 0;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                      & aDataPlan->partitionCursorInfo )
                          != IDE_SUCCESS );

                aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
                IDE_TEST( setTupleSet( aTemplate,
                                       aDataPlan->mtrNode,
                                       aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );
            }

            if ( sWindowPos < sEndPoint )
            {
                /* EndPoint값이 클경우 계산을 마친다 */
                IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* 파티션의 처음부터 StartPoint의 N개 전까지 SKIP한다 */
                for ( sCount = sWindowPos - sStartPoint, sExecAggrCnt = 0;
                      sCount > 0;
                      sCount--, sExecAggrCnt++ )
                {
                    if ( aDataPlan->mtrRowIdx == 0 )
                    {
                        aDataPlan->mtrRowIdx    = 1;
                        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                              != IDE_SUCCESS );
                }

                do
                {
                    /* StartPoint의 N개 전부터 EndPoint의 N개 전가지 Aggr한다. */
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );

                    if ( sWindowPos <= sExecAggrCnt + sEndPoint )
                    {
                        break;
                    }
                    else
                    {
                        ++sExecAggrCnt;
                    }

                    if ( aDataPlan->mtrRowIdx == 0)
                    {
                        aDataPlan->mtrRowIdx    = 1;
                        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                              != IDE_SUCCESS );
                    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                    {
                        /* 같은 파티션인지를 체크한다 */
                        IDE_TEST( compareRows( aDataPlan,
                                               aOverColumnNode,
                                               &sFlag )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        break;
                    }
                } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

                IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                          != IDE_SUCCESS );

                aDataPlan->mtrRowIdx = 0;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                /* 현재 Row 위치로 되돌아온다 */
                IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                      &aDataPlan->cursorInfo )
                          != IDE_SUCCESS );

                aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

                IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                          != IDE_SUCCESS );

            }

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By N PRECEDING - N PRECEDING
 *
 *  Start Point 가 N       PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N       PRECEDING 현재 Row의 N 개 전 까지이다.
 *
 *  처음 시작하면 현재 Row의 위치를 Cursor로 저장한다. 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 현재 Row까지 Aggregation 수행
 */
IDE_RC qmnWNST::orderPrecedPrecedRows( qcTemplate  * aTemplate,
                                       qmndWNST    * aDataPlan,
                                       SLong         aStartValue,
                                       SLong         aEndValue,
                                       qmdAggrNode * aAggrNode,
                                       qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        if ( sStartPoint < sEndPoint )
        {
            /* EndPoint값이 클경우 계산을 마친다 */
            sWindowPos = sStartPoint;
        }
        else
        {
            /* 처음 Row를 읽는다 */
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        if ( sWindowPos < sEndPoint )
        {
            /* EndPoint값이 클경우 계산을 마친다 */
            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 처음부터 StartPoint의 N개 전까지 SKIP한다 */
            for ( sCount = sWindowPos - sStartPoint, sExecAggrCnt = 0;
                  sCount > 0;
                  sCount--, sExecAggrCnt++ )
            {
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
            }

            do
            {
                /* StartPoint의 N개 전부터 EndPoint의 N개 전가지 Aggr한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );

                if ( sWindowPos <= sExecAggrCnt + sEndPoint )
                {
                    break;
                }
                else
                {
                    ++sExecAggrCnt;
                }
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

            } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        ++sWindowPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N PRECEDING - N PRECEDING
 *
 *  Start Point 가 N  PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N  FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row를 Cursor로 저장한다. 파티션의 처음으로
 *  돌아가서 현재 ROW의 N개 전까지는 SKIP 하고 그 뒤 부터 현재 Row까지 Aggregation 수행
 */
IDE_RC qmnWNST::partitionPrecedFollowRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOverColumnNode,
                                           SLong         aStartValue,
                                           SLong         aEndValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        sWindowPos = 0;
        do
        {
            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 Restore한다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

            /* 파티션의 처음부터 StartPoint의 N개 전까지 SKIP한다 */
            for ( sCount = sWindowPos - sStartPoint, sExecAggrCnt = 0;
                  sCount > 0;
                  sCount--, sExecAggrCnt++ )
            {
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
            }

            do
            {
                /* StartPoint의 N개 전부터 EndPoint의 N개 후 까지 Aggr한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );

                if ( sWindowPos + sEndPoint <= sExecAggrCnt )
                {
                    break;
                }
                else
                {
                    ++sExecAggrCnt;
                }

                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By N PRECEDING - N FOLLOWING
 *
 *  Start Point 가 N  PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N  FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다. 처음으로 돌아가서 현재 ROW의 N개 전까지는 SKIP
 *  하고 그 뒤 부터 현재 Row에서 N개 후까지 Aggregation 수행
 */
IDE_RC qmnWNST::orderPrecedFollowRows( qcTemplate  * aTemplate,
                                       qmndWNST    * aDataPlan,
                                       SLong         aStartValue,
                                       SLong         aEndValue,
                                       qmdAggrNode * aAggrNode,
                                       qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sExecAggrCnt = 0;
    SLong              sCount       = 0;
    SLong              sWindowPos   = 0;
    SLong              sStartPoint  = aStartValue;
    SLong              sEndPoint    = aEndValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        /* 파티션의 처음부터 StartPoint의 N개 전까지 SKIP한다 */
        for ( sCount = sWindowPos - sStartPoint, sExecAggrCnt = 0;
              sCount > 0;
              sCount--, sExecAggrCnt++ )
        {
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        do
        {
            /* StartPoint의 N개 전부터 EndPoint의 N개 후 까지 Aggr한다. */
            IDE_TEST( execAggregation( aTemplate,
                                       aAggrNode,
                                       NULL,
                                       NULL,
                                       0 )
                      != IDE_SUCCESS );

            if ( sWindowPos + sEndPoint <= sExecAggrCnt )
            {
                break;
            }
            else
            {
                ++sExecAggrCnt;
            }
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        ++sWindowPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N FOLLOWING - UNBOUNDED FOLLWOING
 *
 *  Start Point 가 N         FOLLOWING 현재 Row의 N 개 후부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 현재 파티션의 마지막 까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다.
 *  현재 ROW의 N개 후 까지는 SKIP 하고 그 뒤 부터 파티션의 마지막까지 Aggregation 수행
 */
IDE_RC qmnWNST::partitionFollowUnFollowRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aStartValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    qmdMtrNode       * sMtrNode;
    SLong              sCount       = 0;
    SLong              sStartPoint  = aStartValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        /* 현재 위치에서 N개 후 까지 SKIP한다. 이때 다른 파티션인 경우를 조사해서 Skip을 멈춘다 */
        for ( sCount = sStartPoint;
              sCount > 0;
              sCount-- )
        {
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );

                if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                break;
            }
        }

        if ( sCount <= 0 )
        {
            /* N 개 후부터 파티션의 마지막까지 계산을 한다. */
            while ( 1 )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );

                    if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        for ( sMtrNode = aAggrResultNode;
              sMtrNode != NULL;
              sMtrNode = sMtrNode->next )
        {
            IDE_TEST( sMtrNode->func.setMtr( aTemplate,
                                             sMtrNode,
                                             aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        /* Update를 수행한다. */
        IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        if ( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME )
        {
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        }
        else
        {
            aDataPlan->mtrRowIdx    = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        }

        /* 다음 Record를 가져온다 */
        IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By N FOLLOWING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 N         FOLLOWING 현재 Row의 N 개 후부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 현재 파티션의 마지막 까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다.
 *  현재 ROW의 N개 후까지는 SKIP 하고 그 뒤 부터 끝까지 Aggregation 수행
 */
IDE_RC qmnWNST::orderFollowUnFollowRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aStartValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sCount       = 0;
    SLong              sStartPoint  = aStartValue;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        /* 현재 위치에서 N개 후 까지 SKIP한다. */
        for ( sCount = sStartPoint;
              sCount > 0;
              sCount-- )
        {
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* Nothing to do */
            }
            else
            {
                break;
            }
        }

        if ( sCount <= 0 )
        {
            /* N 개 후부터 마지막까지 계산을 한다. */
            while ( 1 )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Nothing to do */
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        aDataPlan->mtrRowIdx    = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row로 되돌아 온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Partition By Order By N FOLLOWING - N FOLLWOING
 *
 *  Start Point 가 N FOLLOWING 현재 Row의 N 개 후부터 시작한다.
 *  End   Point 는 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다.
 *  현재 ROW의 Start N개 전까지는 SKIP 하고 그 뒤 부터 End N 개 후까지 Aggregation 수행
 */
IDE_RC qmnWNST::partitionFollowFollowRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOverColumnNode,
                                           SLong         aStartValue,
                                           SLong         aEndValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    qmdMtrNode       * sMtrNode;
    SLong              sCount       = 0;
    SLong              sStartPoint  = aStartValue;
    SLong              sEndPoint    = aEndValue;
    SLong              sExecCount   = 0;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        /* 현재 위치에서 N개 후 까지 SKIP한다. 이때 다른 파티션인 경우를 조사해서 Skip을 멈춘다 */
        for ( sCount = sStartPoint;
              sCount > 0;
              sCount-- )
        {
            if ( sCount > sEndPoint )
            {
                break;
            }
            else
            {
                /* Nothin to do */
            }
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );

                if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                break;
            }
        }

        if ( sCount <= 0 )
        {
            /* N 개 후까지 계산을 한다. 이때 다른 파티션인지를 조사한다 */
            for ( sExecCount = sEndPoint - sStartPoint;
                  sExecCount >= 0;
                  sExecCount-- )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* 같은 파티션인지를 체크한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );

                    if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        for ( sMtrNode = aAggrResultNode;
              sMtrNode != NULL;
              sMtrNode = sMtrNode->next )
        {
            IDE_TEST( sMtrNode->func.setMtr( aTemplate,
                                             sMtrNode,
                                             aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        /* Update를 수행한다. */
        IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        if ( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME )
        {
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        }
        else
        {
            aDataPlan->mtrRowIdx    = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        }

        /* 다음 Record를 가져온다 */
        IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * ROWS Order By N FOLLOWING - N FOLLWOING
 *
 *  Start Point 가 N FOLLOWING 현재 Row의 N 개 후부터 시작한다.
 *  End   Point 는 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  처음 시작하면 현재 Row를 Cursor로 저장한다.
 *  현재 ROW의 Start N개 전까지는 SKIP 하고 그 뒤 부터 End N 개 후까지 Aggregation 수행
 */
IDE_RC qmnWNST::orderFollowFollowRows( qcTemplate  * aTemplate,
                                       qmndWNST    * aDataPlan,
                                       SLong         aStartValue,
                                       SLong         aEndValue,
                                       qmdAggrNode * aAggrNode,
                                       qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag        = QMC_ROW_INITIALIZE;
    SLong              sCount       = 0;
    SLong              sStartPoint  = aStartValue;
    SLong              sEndPoint    = aEndValue;
    SLong              sExecCount   = 0;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        /* 현재 위치에서 N개 후 까지 SKIP한다. */
        for ( sCount = sStartPoint;
              sCount > 0;
              sCount-- )
        {
            if ( sCount > sEndPoint )
            {
                break;
            }
            else
            {
                /* Nothin to do */
            }
            if ( aDataPlan->mtrRowIdx == 0 )
            {
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* Nothing to do */
            }
            else
            {
                break;
            }
        }

        if ( sCount <= 0 )
        {
            for ( sExecCount = sEndPoint - sStartPoint;
                  sExecCount >= 0;
                  sExecCount-- )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                if ( aDataPlan->mtrRowIdx == 0 )
                {
                    aDataPlan->mtrRowIdx    = 1;
                    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Nothing to do */
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row로 되돌아 온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By CURRENT ROW - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 CURRENT   ROW       현재 Row 부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션이 마지막 까지이다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 파티션의 끝까지 Aggr을 계산하는데
 *  이 때 현재 Row와 Logical하게 같은 즉 Order by 구문의 컬럼까지 같은 Row를 세서 그만큼
 *  Update를 수행하고 다음 Record를 읽는다.
 */
IDE_RC qmnWNST::partitionCurrentUnFollowRange( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag       sFlag        = QMC_ROW_INITIALIZE;
    SLong            sExecAggrCnt;
    SLong            sSameAggrCnt;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        while ( 1 )
        {
            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );
            sExecAggrCnt = 0;
            sSameAggrCnt = 1;

            do
            {
                /* 현재 Row 부터 파티션의 끝까지 Aggr을 수행한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                sExecAggrCnt++;
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }

                /* Partitioy By 컬럼과 Order By 컬럼이 모두 같은 경우 이다 */
                if ( ( sFlag & QMC_ROW_COMPARE_MASK ) == QMC_ROW_COMPARE_SAME )
                {
                    ++sSameAggrCnt;
                }
                else
                {
                    /* Nothin to do */
                }

            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 위치로 돌아가서 Order By 컬럼까지 같은 경우 까지 Update를 수행하고
             * 다음 Record를 읽는다.
             */
            IDE_TEST( updateAggrRows( aTemplate,
                                      aDataPlan,
                                      aAggrResultNode,
                                      &sFlag,
                                      sSameAggrCnt )
                      != IDE_SUCCESS );

            if ( ( sFlag & QMC_ROW_DATA_MASK ) != QMC_ROW_DATA_EXIST )
            {
                if ( sExecAggrCnt <= sSameAggrCnt )
                {
                    break;
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
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By CURRENT ROW- UNBOUNDED FOLLOWING
 *
 *  Start Point 가 CURRENT   ROW       현재 Row 부터 시작한다.
 *  End   Point 는 UNBOUNDED FOLLOWING 마지막 까지이다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 파티션의 끝까지 Aggr을 계산하는데
 *  이 때 현재 Row와 Logical하게 같은 즉 Order by 구문의 컬럼까지 같은 Row를 세서 그만큼
 *  Update를 수행하고 다음 Record를 읽는다.
 */
IDE_RC qmnWNST::orderCurrentUnFollowRange( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOverColumnNode,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag       sFlag = QMC_ROW_INITIALIZE;
    SLong            sExecAggrCnt;
    SLong            sSameAggrCnt;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        do
        {
            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );
            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            sExecAggrCnt = 0;
            sSameAggrCnt = 1;

            do
            {
                /* 현재 Row부터 마지막 Row가지 Aggr을 수행한다 */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                sExecAggrCnt++;
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }

                /* Partitioy By 컬럼과 Order By 컬럼이 모두 같은 경우 이다 */
                if ( (sFlag & QMC_ROW_COMPARE_MASK) == QMC_ROW_COMPARE_SAME )
                {
                    ++sSameAggrCnt;
                }
                else
                {
                    /* Nothing to do */
                }
            } while ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 위치로 돌아가서 Order By 컬럼까지 같은 경우 까지 Update를 수행하고
             * 다음 Record를 읽는다.
             */
            IDE_TEST( updateAggrRows( aTemplate,
                                      aDataPlan,
                                      aAggrResultNode,
                                      &sFlag,
                                      sSameAggrCnt )
                      != IDE_SUCCESS );
        } while ( sExecAggrCnt > sSameAggrCnt );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE CURRENT ROW - CURRENT ROW
 *
 *  Start Point 가 CURRENT ROW 현재 Row 부터 시작한다.
 *  End   Point 는 CURRENT ROW 현재 Row 까지이다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 현재 Row와 Logical하게 같은
 *  즉 Order by 구문의 컬럼까지 같은 Row를 세서 그만큼 Update를 수행하고 다음 Record를 읽는다.
 */
IDE_RC qmnWNST::currentCurrentRange( qcTemplate  * aTemplate,
                                     qmndWNST    * aDataPlan,
                                     qmdMtrNode  * aOverColumnNode,
                                     qmdAggrNode * aAggrNode,
                                     qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag sFlag        = QMC_ROW_INITIALIZE;
    SLong      sExecAggrCnt;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        do
        {
            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );
            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );
            sExecAggrCnt = 0;

            do
            {
                /* 현재 Row에서 Order by 구문의 컬럼까지 같은 Row를 Aggr 한다 */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                sExecAggrCnt++;
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Logical하게 같은 지 비교한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_COMPARE_MASK ) == QMC_ROW_COMPARE_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 현재 위치로 돌아가서 Order By 컬럼까지 같은 경우 까지 Update를 수행하고
             * 다음 Record를 읽는다.
             */
            IDE_TEST( updateAggrRows( aTemplate,
                                      aDataPlan,
                                      aAggrResultNode,
                                      &sFlag,
                                      sExecAggrCnt )
                      != IDE_SUCCESS );
        } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Calculate Interval
 *
 *   Range 구문에서만 사용되며 N PRECEDING, N FOLLOWING에서 사용되는 N에 대해서
 *   현재 Row와 계산을 수행한다.
 *
 *   항상 ORDER BY에 사용되는 컬럼이 1 개 여야만 한다.
 *
 *   aInterval     - N을 의미한다. Range에서는 N의 Logical한 간격을 의히함다.
 *   aIntervalType - N은 정수형만 가능한데 Order By 컬럼이 숫자이거나 Date인경우만
 *                   가능하다.
 *   aValue        - 현재 Row에서 Interval 값만큼 빼거나 더한 값을 여기로 넘겨준다.
 *   aIsPreceding  - PRECEDING 인 경우에는 현재 Row에서 값을 뺀값이고,
 *                   FOLLOWING 인 경우에는 현재 Row에서 값을 더하는것을 의미한다.
 *
 *   현재 ROW의 ORDER BY 컴럼의 컬럼 Type과 value를 얻는다.
 *   값이 NULL 인경우에는 계산하지 않고 NULL Type임을 명시만 한다.
 */
IDE_RC qmnWNST::calculateInterval( qcTemplate        * aTemplate,
                                   qmcdSortTemp      * aTempTable,
                                   qmdMtrNode        * aNode,
                                   void              * aRow,
                                   SLong               aInterval,
                                   UInt                aIntervalType,
                                   qmcWndWindowValue * aValue,
                                   idBool              aIsPreceding )
{
    SLong            sLong     = 0;
    SDouble          sDouble   = 0;
    mtdDateType      sDateType;
    SLong            sInterval = 0;
    const void     * sRowValue;
    mtdNumericType * sNumeric1 = NULL;
    SChar            sBuffer[MTD_NUMERIC_SIZE_MAXIMUM];
    mtcColumn      * sColumn;

    /* Temp Table 이 Value 인 경우 */
    if ( ( ( aNode->myNode->flag & QMC_MTR_TYPE_MASK )
           == QMC_MTR_TYPE_COPY_VALUE ) ||
         ( ( ( aNode->myNode->flag & QMC_MTR_TYPE_MASK )
             == QMC_MTR_TYPE_MEMORY_KEY_COLUMN ) &&
           ( ( aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE )
             == QMCD_SORT_TMP_STORAGE_DISK ) ) ||
         ( ( aNode->myNode->flag & QMC_MTR_TYPE_MASK )
           == QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN ) ) /* PROJ-2464 hybrid partitioned table 지원 */
    {
        sColumn = aNode->dstColumn;
        
        /* Value인 경우 Dst 컬럼에서 읽는다 */
        sRowValue = mtc::value( sColumn, aNode->dstTuple->row, MTD_OFFSET_USE );

        if ( sColumn->module->isNull( sColumn, sRowValue )
             == ID_TRUE )
        {
            /* NULL 인경우 계산하지 않는다. */
            aValue->type = QMC_WND_WINDOW_VALUE_NULL;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Value가 아닌 경우 원복 후에 srcColumn에서 읽는다. */
        IDE_TEST( aNode->func.setTuple( aTemplate, aNode, aRow ) != IDE_SUCCESS );

        sColumn = aNode->srcColumn;
        
        sRowValue = mtc::value( sColumn, aNode->srcTuple->row, MTD_OFFSET_USE );

        if ( sColumn->module->isNull( sColumn, sRowValue )
             == ID_TRUE )
        {
            /* NULL 인경우 계산하지 않는다. */
            aValue->type = QMC_WND_WINDOW_VALUE_NULL;
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sColumn->module->id == MTD_SMALLINT_ID )
    {
        /* SMALLINT 인경우 LONG으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_LONG;
        sLong = (SLong)(*(mtdSmallintType*)sRowValue);
        IDE_TEST_RAISE( MTD_SMALLINT_MAXIMUM < aInterval,
                        ERR_INVALID_WINDOW_SPECIFICATION );
    }
    else if ( sColumn->module->id == MTD_INTEGER_ID )
    {
        /* INTEGER 인경우 LONG으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_LONG;
        sLong = (SLong)(*(mtdIntegerType*)sRowValue);
        IDE_TEST_RAISE( MTD_INTEGER_MAXIMUM < aInterval,
                        ERR_INVALID_WINDOW_SPECIFICATION );
    }
    else if ( sColumn->module->id == MTD_BIGINT_ID )
    {
        /* BIGINT 인경우 LONG으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_LONG;
        sLong = (SLong)(*(mtdBigintType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_DOUBLE_ID )
    {
        /* DOUBLE 인경우 DOUBLE으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_DOUBLE;
        sDouble = (SDouble)(*(mtdDoubleType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_REAL_ID )
    {
        /* REAL 인경우 DOUBLE으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_DOUBLE;
        sDouble = (SDouble)(*(mtdRealType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_DATE_ID )
    {
        /* DATE 인경우 DATE으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_DATE;
        sDateType = (*(mtdDateType*)sRowValue);
    }
    else if ( ( sColumn->module->id == MTD_FLOAT_ID )   ||
              ( sColumn->module->id == MTD_NUMERIC_ID ) ||
              ( sColumn->module->id == MTD_NUMBER_ID ) )
    {
        /* Float, Numeric, Number는 NUMERIC으로 처리한다 */
        aValue->type = QMC_WND_WINDOW_VALUE_NUMERIC;
        sNumeric1 = (mtdNumericType*)sRowValue;
        IDE_TEST( mtv::nativeN2Numeric( aInterval, (mtdNumericType * )sBuffer )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
    }

    /* PRECEDING 인 경우 현재 ROW부터 뺄셈을 수행한다 */
    if ( aIsPreceding == ID_TRUE )
    {
        switch ( aValue->type )
        {
            case QMC_WND_WINDOW_VALUE_LONG:
                aValue->longType = sLong - aInterval;
                break;
            case QMC_WND_WINDOW_VALUE_DOUBLE:
                aValue->doubleType = sDouble - (SDouble)aInterval;
                break;
            case QMC_WND_WINDOW_VALUE_DATE:
                switch ( aIntervalType )
                {
                    case QTC_OVER_WINDOW_VALUE_TYPE_YEAR:
                        if ( aInterval > 0 )
                        {
                            sInterval = 12 * aInterval;
                            IDE_TEST( mtdDateInterface::addMonth( &aValue->dateType,
                                                                  &sDateType,
                                                                  -sInterval )
                                      != IDE_SUCCESS );
                            aValue->dateField = MTD_DATE_DIFF_YEAR;
                        }
                        else
                        {
                            aValue->dateType = sDateType;
                            aValue->dateField = MTD_DATE_DIFF_DAY;
                        }
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_MONTH:
                        if ( aInterval > 0 )
                        {
                            sInterval = aInterval;
                            IDE_TEST( mtdDateInterface::addMonth( &aValue->dateType,
                                                                  &sDateType,
                                                                  -sInterval )
                                      != IDE_SUCCESS );
                            aValue->dateField = MTD_DATE_DIFF_MONTH;
                        }
                        else
                        {
                            aValue->dateType = sDateType;
                            aValue->dateField = MTD_DATE_DIFF_DAY;
                        }
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_NUMBER:
                    case QTC_OVER_WINDOW_VALUE_TYPE_DAY:
                        sInterval = aInterval;
                        IDE_TEST( mtdDateInterface::addDay( &aValue->dateType,
                                                            &sDateType,
                                                            -sInterval )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_DAY;
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_HOUR:
                        sInterval = 3600 * aInterval;
                        IDE_TEST( mtdDateInterface::addSecond( &aValue->dateType,
                                                               &sDateType,
                                                               -sInterval,
                                                               0 )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_HOUR;
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_MINUTE:
                        sInterval = 60 * aInterval;
                        IDE_TEST( mtdDateInterface::addSecond( &aValue->dateType,
                                                               &sDateType,
                                                               -sInterval,
                                                               0 )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_MINUTE;
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_SECOND:
                        sInterval = aInterval;
                        IDE_TEST( mtdDateInterface::addSecond( &aValue->dateType,
                                                               &sDateType,
                                                               -sInterval,
                                                               0 )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_SECOND;
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                }
                break;
            case QMC_WND_WINDOW_VALUE_NUMERIC:
                IDE_TEST( mtc::subtractFloat( (mtdNumericType * )aValue->numericType,
                                              MTD_FLOAT_PRECISION_MAXIMUM,
                                              sNumeric1,
                                              (mtdNumericType * )sBuffer )
                          != IDE_SUCCESS );
                break;
            default:
                break;
        }
    }
    else /* FOLLOWING 인 경우 현재 ROW부터 덧셈을 수행한다 */
    {
        switch ( aValue->type )
        {
            case QMC_WND_WINDOW_VALUE_LONG:
                aValue->longType = sLong + aInterval;
                break;
            case QMC_WND_WINDOW_VALUE_DOUBLE:
                aValue->doubleType = sDouble + (SDouble)aInterval;
                break;
            case QMC_WND_WINDOW_VALUE_DATE:
                switch ( aIntervalType )
                {
                    case QTC_OVER_WINDOW_VALUE_TYPE_YEAR:
                        if ( aInterval > 0 )
                        {
                            sInterval = 12 * aInterval;
                            IDE_TEST( mtdDateInterface::addMonth( &aValue->dateType,
                                                                  &sDateType,
                                                                  sInterval )
                                      != IDE_SUCCESS );
                            aValue->dateField = MTD_DATE_DIFF_YEAR;
                        }
                        else
                        {
                            aValue->dateType  = sDateType;
                            aValue->dateField = MTD_DATE_DIFF_DAY;
                        }
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_MONTH:
                        if ( aInterval > 0 )
                        {
                            sInterval = aInterval;
                            IDE_TEST( mtdDateInterface::addMonth( &aValue->dateType,
                                                                  &sDateType,
                                                                  sInterval )
                                      != IDE_SUCCESS );
                            aValue->dateField = MTD_DATE_DIFF_MONTH;
                        }
                        else
                        {
                            aValue->dateType  = sDateType;
                            aValue->dateField = MTD_DATE_DIFF_DAY;
                        }
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_NUMBER:
                    case QTC_OVER_WINDOW_VALUE_TYPE_DAY:
                        sInterval = aInterval;
                        IDE_TEST( mtdDateInterface::addDay( &aValue->dateType,
                                                            &sDateType,
                                                            sInterval )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_DAY;
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_HOUR:
                        sInterval = 3600 * aInterval;
                        IDE_TEST( mtdDateInterface::addSecond( &aValue->dateType,
                                                               &sDateType,
                                                               sInterval,
                                                               0 )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_HOUR;
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_MINUTE:
                        sInterval = 60 * aInterval;
                        IDE_TEST( mtdDateInterface::addSecond( &aValue->dateType,
                                                               &sDateType,
                                                               sInterval,
                                                               0 )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_MINUTE;
                        break;
                    case QTC_OVER_WINDOW_VALUE_TYPE_SECOND:
                        sInterval = aInterval;
                        IDE_TEST( mtdDateInterface::addSecond( &aValue->dateType,
                                                               &sDateType,
                                                               sInterval,
                                                               0 )
                                  != IDE_SUCCESS );
                        aValue->dateField = MTD_DATE_DIFF_SECOND;
                        break;
                    default:
                        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
                        break;
                }
                break;
            case QMC_WND_WINDOW_VALUE_NUMERIC:
                IDE_TEST( mtc::addFloat( (mtdNumericType * )aValue->numericType,
                                         MTD_FLOAT_PRECISION_MAXIMUM,
                                         sNumeric1,
                                         (mtdNumericType * )sBuffer )
                          != IDE_SUCCESS );
                break;
            default:
                break;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WINDOW_SPECIFICATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Compare RANGE Value
 *
 *  RANGE에만 사용된다. calculate된 value 와 새로 읽은 Record와 비교를 해서 이 Reocrd가
 *  범위에 드는 Record인지를 판단한다.
 *
 *  aRow             - 새로 읽은 Row이다.
 *  aValue           - 비교해야할 값이다.
 *  aIsLessThanEqual - ORDER BY 구문이 ASC, 인지 DESC인지에 따라 작거나 같은걸
 *                     참으로 해야할지 크거나 같은걸 참으로 해야할지가 달라진다.
 *  aResult          - 윈도의 영역인지 아닌 지를 판단한다.
 *
 */
IDE_RC qmnWNST::compareRangeValue( qcTemplate        * aTemplate,
                                   qmcdSortTemp      * aTempTable,
                                   qmdMtrNode        * aNode,
                                   void              * aRow,
                                   qmcWndWindowValue * aValue,
                                   idBool              aIsLessThanEqual,
                                   idBool            * aResult )
{
    SLong            sLong     = 0;
    SDouble          sDouble   = 0;
    mtdDateType      sDateType;
    const void     * sRowValue;
    mtdNumericType * sNumeric1 = NULL;
    mtdNumericType * sNumeric2 = NULL;
    SChar            sBuffer[MTD_NUMERIC_SIZE_MAXIMUM];
    idBool           sResult = ID_FALSE;
    mtcColumn      * sColumn;

    /* Temp Table 이 Value 인 경우 */
    if ( ( ( aNode->myNode->flag & QMC_MTR_TYPE_MASK )
           == QMC_MTR_TYPE_COPY_VALUE ) ||
         ( ( ( aNode->myNode->flag & QMC_MTR_TYPE_MASK )
             == QMC_MTR_TYPE_MEMORY_KEY_COLUMN ) &&
           ( ( aTempTable->flag & QMCD_SORT_TMP_STORAGE_TYPE )
             == QMCD_SORT_TMP_STORAGE_DISK ) ) ||
         ( ( aNode->myNode->flag & QMC_MTR_TYPE_MASK )
           == QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN ) ) /* PROJ-2464 hybrid partitioned table 지원 */
    {
        sColumn = aNode->dstColumn;
        
        /* Value인 경우 Dst 컬럼에서 읽는다 */
        sRowValue = mtc::value( sColumn, aNode->dstTuple->row, MTD_OFFSET_USE );

        if ( sColumn->module->isNull( sColumn, sRowValue )
             == ID_TRUE )
        {
            /* NULL 인 경우 비교하지 않고 FALSE이다.. */
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Value가 아닌 경우 원복 후에 srcColumn에서 읽는다. */
        IDE_TEST( aNode->func.setTuple( aTemplate, aNode, aRow ) != IDE_SUCCESS );
        
        sColumn = aNode->srcColumn;
        
        sRowValue = mtc::value( sColumn, aNode->srcTuple->row, MTD_OFFSET_USE );

        if ( sColumn->module->isNull( sColumn, sRowValue )
             == ID_TRUE )
        {
            /* NULL 인 경우 비교하지 않고 FALSE이다. */
            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sColumn->module->id == MTD_SMALLINT_ID )
    {
        /* SMALLINT 인경우 LONG으로 처리한다 */
        sLong = (SLong)(*(mtdSmallintType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_INTEGER_ID )
    {
        /* INTEGER 인경우 LONG으로 처리한다 */
        sLong = (SLong)(*(mtdIntegerType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_BIGINT_ID )
    {
        /* BIGINT 인경우 LONG으로 처리한다 */
        sLong = (SLong)(*(mtdBigintType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_DOUBLE_ID )
    {
        /* DOUBLE 인경우 DOUBLE으로 처리한다 */
        sDouble = (SDouble)(*(mtdDoubleType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_REAL_ID )
    {
        /* REAL 인경우 DOUBLE으로 처리한다 */
        sDouble = (SDouble)(*(mtdRealType*)sRowValue);
    }
    else if ( sColumn->module->id == MTD_DATE_ID )
    {
        /* DATE 인경우 DATE으로 처리한다 */
        sDateType = (*(mtdDateType*)sRowValue);
    }
    else if ( ( sColumn->module->id == MTD_FLOAT_ID )   ||
              ( sColumn->module->id == MTD_NUMERIC_ID ) ||
              ( sColumn->module->id == MTD_NUMBER_ID ) )
    {
        /* Float, Numeric, Number는 NUMERIC으로 처리한다 */
        sNumeric1 = (mtdNumericType*)sRowValue;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_WINDOW_SPECIFICATION );
    }

    if ( aIsLessThanEqual == ID_FALSE )
    {
        switch ( aValue->type )
        {
            case QMC_WND_WINDOW_VALUE_LONG:
                if ( aValue->longType >= sLong )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_DOUBLE:
                if ( aValue->doubleType >= sDouble )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_DATE:
                IDE_TEST( mtdDateInterface::dateDiff( &sLong,
                                                      &sDateType,
                                                      &aValue->dateType,
                                                      aValue->dateField )
                          != IDE_SUCCESS );
                if ( sLong >= 0 )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_NUMERIC:
                sNumeric2 = (mtdNumericType * )sBuffer;
                IDE_TEST( mtc::subtractFloat( sNumeric2,
                                              MTD_FLOAT_PRECISION_MAXIMUM,
                                              (mtdNumericType * )aValue->numericType,
                                              sNumeric1 )
                          != IDE_SUCCESS );
                if ( MTD_NUMERIC_IS_POSITIVE( sNumeric2 ) == ID_TRUE )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_NULL:
                sResult = ID_TRUE;
                break;
            default:
                break;
        }
    }
    else
    {
        switch ( aValue->type )
        {
            case QMC_WND_WINDOW_VALUE_LONG:
                if ( aValue->longType <= sLong )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_DOUBLE:
                if ( aValue->doubleType <= sDouble )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_DATE:
                IDE_TEST( mtdDateInterface::dateDiff( &sLong,
                                                      &sDateType,
                                                      &aValue->dateType,
                                                      aValue->dateField )
                          != IDE_SUCCESS );
                if ( sLong <= 0 )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_NUMERIC:
                sNumeric2 = (mtdNumericType * )sBuffer;
                IDE_TEST( mtc::subtractFloat( sNumeric2,
                                              MTD_FLOAT_PRECISION_MAXIMUM,
                                              sNumeric1,
                                              (mtdNumericType * )aValue->numericType )
                          != IDE_SUCCESS );
                if ( MTD_NUMERIC_IS_POSITIVE( sNumeric2 ) == ID_TRUE )
                {
                    sResult = ID_TRUE;
                }
                else
                {
                    sResult = ID_FALSE;
                }
                break;
            case QMC_WND_WINDOW_VALUE_NULL:
                sResult = ID_TRUE;
                break;
            default:
                break;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aResult = sResult;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WINDOW_SPECIFICATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Update One ROW and NEXT Record
 *
 *   Temp Table에서 Update가 필요한 한 Row의 컬럼을 UPDATE 하고 다음 레코드를 읽는다.
 */
IDE_RC qmnWNST::updateOneRowNextRecord( qcTemplate * aTemplate,
                                        qmndWNST   * aDataPlan,
                                        qmdMtrNode * aAggrResultNode,
                                        qmcRowFlag * aFlag )
{
    qmdMtrNode  * sMtrNode;
    qmcRowFlag    sFlag = QMC_ROW_INITIALIZE;

    for ( sMtrNode = aAggrResultNode;
          sMtrNode != NULL;
          sMtrNode = sMtrNode->next )
    {
        IDE_TEST( sMtrNode->func.setMtr( aTemplate,
                                         sMtrNode,
                                         aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    IDE_TEST( qmcSortTemp::updateRow( aDataPlan->sortMgr )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 1;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By UNBOUNDED PRECEDING - N PRECEDING or N FOLLOWING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 파티션의 처음 부터 시작한다.
 *  End   Point 는 N PRECEDING 현재 Row의 N 개 전 까지이다.
 *                 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 파치션의 처음과 현재 Row의 위치를 저장한다. 현재 Row에서 EndPoint
 *  값의 Value를 얻는이 이를 통해서 파티션의 처음부터 N PRECEDING이라면 N 전 값까지 계산하고
 *  N FOLLOWING 이라면 N 후 값까지 계산한다.
 */
IDE_RC qmnWNST::partitionUnPrecedPrecedFollowRange( qcTemplate  * aTemplate,
                                                    qmndWNST    * aDataPlan,
                                                    qmdMtrNode  * aOverColumnNode,
                                                    qmdMtrNode  * aOrderByColumn,
                                                    SLong         aEndValue,
                                                    SInt          aEndType,
                                                    qmdAggrNode * aAggrNode,
                                                    qmdMtrNode  * aAggrResultNode,
                                                    idBool        aIsPreceding )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    idBool             sResult;
    idBool             sIsLess;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess = ID_FALSE;
        sIsPreceding = aIsPreceding;
    }
    else
    {
        sIsLess = ID_TRUE;
        sIsPreceding = ( aIsPreceding == ID_TRUE ? ID_FALSE : ID_TRUE );
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row로 부터 End Point의 N 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aEndValue,
                                         aEndType,
                                         &sValue1,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            do
            {
                /* 현재 값과 EndPoint의 값과 비교한다. */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult )
                           != IDE_SUCCESS );
                if ( sResult == ID_TRUE )
                {
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                /* 다음 Row를 얻는다 */
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Row를 비교한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By UNBOUNDED PRECEDING - N PRECEDING or N FOLLOWING
 *
 *  Start Point 가 UNBOUNDED PRECEDING 파티션의 처음 부터 시작한다.
 *  End   Point 는 N PRECEDING 현재 Row의 N 개 전 까지이다.
 *                 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 EndPoint
 *  값의 Value를 얻는이 이를 통해서 처음부터 현재 Row를 기준으로 N PRECEDING이라면
 *  N 전 값까지 계산하고 N FOLLOWING 이라면 N 후 값까지 계산한다.
 */
IDE_RC qmnWNST::orderUnPrecedPrecedFollowRange( qcTemplate  * aTemplate,
                                                qmndWNST    * aDataPlan,
                                                qmdMtrNode  * aOrderByColumn,
                                                SLong         aEndValue,
                                                SInt          aEndType,
                                                qmdAggrNode * aAggrNode,
                                                qmdMtrNode  * aAggrResultNode,
                                                idBool        aIsPreceding )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    idBool             sResult;
    idBool             sIsLess;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_FALSE;
        sIsPreceding = aIsPreceding;
    }
    else
    {
        sIsLess      = ID_TRUE;
        sIsPreceding = ( aIsPreceding == ID_TRUE ? ID_FALSE : ID_TRUE );
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row로 부터 End Point의 N개 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aEndValue,
                                     aEndType,
                                     &sValue1,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 처음으로 되돌아 온다 */
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        while( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            /* 현재 값과 EndPoint의 값과 비교한다. */
            IDE_TEST( compareRangeValue( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         &sValue1,
                                         sIsLess,
                                         &sResult )
                      != IDE_SUCCESS );
            if ( sResult == ID_TRUE )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 다음 Row를 얻는다 */
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By CURRENT ROW - N FOLLOWING
 *
 *  Start Point 가 CURRENT ROW       현재 Row부터 시작한다.
 *  End   Point 는 N       FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 */
IDE_RC qmnWNST::partitionCurrentFollowRange( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             qmdMtrNode  * aOrderByColumn,
                                             SLong         aEndValue,
                                             SInt          aEndType,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag          sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue   sValue1;
    idBool              sResult;
    idBool              sIsLess;
    idBool              sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_FALSE;
        sIsPreceding = ID_FALSE;
    }
    else
    {
        sIsLess      = ID_TRUE;
        sIsPreceding = ID_TRUE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row로 부터 End Point의 N 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aEndValue,
                                     aEndType,
                                     &sValue1,
                                     sIsPreceding )
                   != IDE_SUCCESS );

        IDE_TEST( execAggregation( aTemplate,
                                   aAggrNode,
                                   NULL,
                                   NULL,
                                   0 )
                  != IDE_SUCCESS );

        while ( 1 )
        {
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 다음 Record를 읽는다 */
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 레코드를 비교해된다. */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );

                /* 같은 파티션에 속한지 체크해본다 */
                if ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME )
                {
                    /* EndPoint에 의 계산된 결과와 비교해본다. */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue1,
                                                 sIsLess,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_TRUE )
                    {
                        IDE_TEST( execAggregation( aTemplate,
                                                   aAggrNode,
                                                   NULL,
                                                   NULL,
                                                   0 )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( compareRows( aDataPlan,
                                   aOverColumnNode,
                                   &sFlag )
                      != IDE_SUCCESS );
            while ( ( sFlag & QMC_ROW_COMPARE_MASK ) == QMC_ROW_COMPARE_SAME )
            {
                IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                    &aDataPlan->partitionCursorInfo )
                          != IDE_SUCCESS );

                IDE_TEST( updateOneRowNextRecord( aTemplate,
                                                  aDataPlan,
                                                  aAggrResultNode,
                                                  &sFlag )
                           != IDE_SUCCESS );
                if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            }
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By CURRENT ROW - N FOLLOWING
 *
 *  Start Point 가 CURRENT ROW       현재 Row부터 시작한다.
 *  End   Point 는 N       FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 */
IDE_RC qmnWNST::orderCurrentFollowRange( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         qmdMtrNode  * aOrderByColumn,
                                         SLong         aEndValue,
                                         SInt          aEndType,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag          sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue   sValue1;
    idBool              sResult;
    idBool              sIsLess;
    idBool              sIsPreceding;

    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_FALSE;
        sIsPreceding = ID_FALSE;
    }
    else
    {
        sIsLess      = ID_TRUE;
        sIsPreceding = ID_TRUE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aEndValue,
                                     aEndType,
                                     &sValue1,
                                     sIsPreceding )
                   != IDE_SUCCESS );

        IDE_TEST( execAggregation( aTemplate,
                                   aAggrNode,
                                   NULL,
                                   NULL,
                                   0 )
                  != IDE_SUCCESS );

        while ( 1 )
        {
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult )
                         != IDE_SUCCESS );
                if ( sResult == ID_TRUE )
                {
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( compareRows( aDataPlan,
                                   aOrderByColumn,
                                   &sFlag )
                      != IDE_SUCCESS );
            while ( ( sFlag & QMC_ROW_COMPARE_MASK ) == QMC_ROW_COMPARE_SAME )
            {
                IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                    &aDataPlan->partitionCursorInfo )
                          != IDE_SUCCESS );

                IDE_TEST( updateOneRowNextRecord( aTemplate,
                                                  aDataPlan,
                                                  aAggrResultNode,
                                                  &sFlag )
                           != IDE_SUCCESS );
                if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOrderByColumn,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            }
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By N PRECEDING or N FOLLOWING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 N         PRECEDING 현재 Row 부터 N 값을 뺀 값 부터 시작하거나 혹은
 *                 N         FOLLOWINg 현재 Row 부터 N 값을 더한 값 부터 시작해서
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션이 마지막 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *  PRECEDING이 사용되었는지 FOLLOWING이 사용되었는지에 따라 계산이 달라진다.
 *
 *  처음 시작 하면 파티션의 처음과 현재 Row의 위치를 저장한다.
 *  현재 Row에서 START Point의 value를 얻는다. 그 후 파티션의 처음으로 돌아가 이 value와
 *  비교해서 Skip한뒤에 파티션의 끝까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::partitionPrecedFollowUnFollowRange( qcTemplate  * aTemplate,
                                                    qmndWNST    * aDataPlan,
                                                    qmdMtrNode  * aOverColumnNode,
                                                    qmdMtrNode  * aOrderByColumn,
                                                    SLong         aStartValue,
                                                    SInt          aStartType,
                                                    qmdAggrNode * aAggrNode,
                                                    qmdMtrNode  * aAggrResultNode,
                                                    idBool        aIsPreceding )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsPreceding = aIsPreceding;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsPreceding = ( aIsPreceding == ID_TRUE ? ID_FALSE : ID_TRUE );
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row로 부터 Start Point의 N 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aStartValue,
                                         aStartType,
                                         &sValue1,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음으로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );
            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            sSkipEnd = ID_FALSE;
            do
            {
                if ( sSkipEnd == ID_FALSE )
                {
                    /* 현재 값과 StartPoint의 값과 비교한다. */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue1,
                                                 sIsLess,
                                                 &sResult )
                              != IDE_SUCCESS );
                }
                else
                {
                    sResult = ID_TRUE;
                }
                if ( sResult == ID_TRUE )
                {
                    /* Start 값 이후이므로 계산을 수행한다. */
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                    sSkipEnd = ID_TRUE;
                }
                else
                {
                    /* Start 값 전이므로 Skip한다 */
                    sSkipEnd = ID_FALSE;
                }
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                /* 다음 Row를 읽는다 */
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Row를 비교한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By N PRECEDING or N FOLLOWING - UNBOUNDED FOLLOWING
 *
 *  Start Point 가 N         PRECEDING 현재 Row 부터 N 값을 뺀 값 부터 시작하거나 혹은
 *                 N         FOLLOWINg 현재 Row 부터 N 값을 더한 값 부터 시작해서
 *  End   Point 는 UNBOUNDED FOLLOWING 파티션이 마지막 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *  PRECEDING이 사용되었는지 FOLLOWING이 사용되었는지에 따라 계산이 달라진다.
 *
 *  처음 현재 Row의 위치를 저장한다.
 *  현재 Row에서 START Point의 value를 얻는다. 그 후 처음으로 돌아가 이 value와
 *  비교해서 Skip한뒤에 끝까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::orderPrecedFollowUnFollowRange( qcTemplate  * aTemplate,
                                                qmndWNST    * aDataPlan,
                                                qmdMtrNode  * aOrderByColumn,
                                                SLong         aStartValue,
                                                SInt          aStartType,
                                                qmdAggrNode * aAggrNode,
                                                qmdMtrNode  * aAggrResultNode,
                                                idBool        aIsPreceding )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsPreceding = aIsPreceding;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsPreceding = ( aIsPreceding == ID_TRUE ? ID_FALSE : ID_TRUE );
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aStartValue,
                                     aStartType,
                                     &sValue1,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 Row의 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        sSkipEnd                = ID_FALSE;
        aDataPlan->mtrRowIdx    = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            if ( sSkipEnd == ID_FALSE )
            {
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult )
                          != IDE_SUCCESS );
            }
            else
            {
                sResult = ID_TRUE;
            }
            if ( sResult == ID_TRUE )
            {
                /* Start 값 이후이므로 계산을 수행한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                sSkipEnd = ID_TRUE;
            }
            else
            {
                /* Start 값 이전 이므로 Skip한다. */
                sSkipEnd = ID_FALSE;
            }
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 위치로 되돌아 온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By N PRECEDING - CURRENT ROW
 *
 *  Start Point 가 N       PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 CURRENT ROW       현재 Row 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint
 *  값의 Value를 얻는이 이를 통해서 파티션의 처음부터 N개 전 Value까지 Skip하고 현재 Row의
 *  값 까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::partitionPrecedCurrentRange( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             qmdMtrNode  * aOrderByColumn,
                                             SLong         aStartValue,
                                             SInt          aStartType,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_TRUE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_FALSE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aStartValue,
                                         aStartType,
                                         &sValue1,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 Row로 부터 CURRENT ROW 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         0,
                                         aStartType,
                                         &sValue2,
                                         ID_TRUE )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음위치로 이동한다. */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );
            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            sSkipEnd = ID_FALSE;
            do
            {
                if ( sSkipEnd == ID_FALSE )
                {
                    /* 현재 값과 StartPoint의 값과 비교한다. */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue1,
                                                 sIsLess,
                                                 &sResult )
                               != IDE_SUCCESS );
                }
                else
                {
                    sResult = ID_TRUE;
                }
                if ( sResult == ID_TRUE )
                {
                    /* Start 값 이후이므로 계산을 수행한다. */
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                    sSkipEnd = ID_TRUE;
                }
                else
                {
                    /* Start 값 이전 이므로 Skip한다. */
                    sSkipEnd = ID_FALSE;
                }
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }

                if ( sSkipEnd == ID_TRUE )
                {
                    /* 다음 Row가 현재 Row의 Value에 속한지 비교해본다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
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
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By N PRECEDING - CURRENT ROW
 *
 *  Start Point 가 N       PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 CURRENT ROW       현재 Row 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint
 *  값의 Value를 얻는이 이를 통해서 처음부터 N개 전 Value까지 Skip하고 현재 Row의
 *  값 까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::orderPrecedCurrentRange( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         qmdMtrNode  * aOrderByColumn,
                                         SLong         aStartValue,
                                         SInt          aStartType,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_TRUE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_FALSE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aStartValue,
                                     aStartType,
                                     &sValue1,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 Row로 부터 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     0,
                                     aStartType,
                                     &sValue2,
                                     ID_TRUE )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        sSkipEnd                = ID_FALSE;
        aDataPlan->mtrRowIdx    = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 처음 Record를 가져온다 */
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            if ( sSkipEnd == ID_FALSE )
            {
                /* 현재 값과 StartPoint의 값과 비교한다. */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult )
                          != IDE_SUCCESS );
            }
            else
            {
                sResult = ID_TRUE;
            }
            if ( sResult == ID_TRUE )
            {
                /* Start 값 이후이므로 계산을 수행한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                sSkipEnd = ID_TRUE;
            }
            else
            {
                /* Start 값 이전 이므로 Skip한다. */
                sSkipEnd = ID_FALSE;
            }
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                if ( sSkipEnd == ID_TRUE )
                {
                    /* 다음 Row가 현재 Value에 속한지 비교해본다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
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
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By N PRECEDING - N PRECEDING
 *
 *  Start Point 가 N PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N PRECEDING 현재 Row의 N 개 전 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint,EndPoint
 *  값의 Value를 얻는이 이를 통해서 파티션의 처음부터 N개 전 Value까지 Skip하고 N개 전 Value
 *  까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::partitionPrecedPrecedRange( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdMtrNode  * aOverColumnNode,
                                            qmdMtrNode  * aOrderByColumn,
                                            SLong         aStartValue,
                                            SInt          aStartType,
                                            SLong         aEndValue,
                                            SInt          aEndType,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
           QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_TRUE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_FALSE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aStartValue,
                                         aStartType,
                                         &sValue1,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 Row로 부터 End Point이 N개 전 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aEndValue,
                                         aEndType,
                                         &sValue2,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음위치로 이동한다. */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );
            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            sSkipEnd = ID_FALSE;

            do
            {
                /* 시작 값보다 마지막이 크다면 멈춘다 */
                if ( aStartValue < aEndValue )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
                if ( sSkipEnd == ID_FALSE )
                {
                    /* 현재 값과 EndPoint의 값과 비교해서 현재 Row게 End값에 속하지 않는다면 끝낸다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    /* 현재 값과 StartPoint의 값과 비교한다. */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue1,
                                                 sIsLess,
                                                 &sResult )
                              != IDE_SUCCESS );
                }
                else
                {
                    sResult = ID_TRUE;
                }
                if ( sResult == ID_TRUE )
                {
                    /* Start 값 이후이므로 계산을 수행한다. */
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                    sSkipEnd = ID_TRUE;
                }
                else
                {
                    /* Start 값 이전 이므로 Skip한다. */
                    sSkipEnd = ID_FALSE;
                }
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                /* 다음 Row를 읽는다 */
                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    /* Row를 비교한다 */
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
                if ( sSkipEnd == ID_TRUE )
                {
                    /* 다음 Row가 EndPoint의 Value에 속한지 비교해본다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                               != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
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
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By N PRECEDING - N PRECEDING
 *
 *  Start Point 가 N PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N PRECEDING 현재 Row의 N 개 전 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint,EndPoint
 *  값의 Value를 얻는이 이를 통해서 파티션의 처음부터 N개 전 Value까지 Skip하고 N개 전 Value
 *  까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::orderPrecedPrecedRange( qcTemplate  * aTemplate,
                                        qmndWNST    * aDataPlan,
                                        qmdMtrNode  * aOrderByColumn,
                                        SLong         aStartValue,
                                        SInt          aStartType,
                                        SLong         aEndValue,
                                        SInt          aEndType,
                                        qmdAggrNode * aAggrNode,
                                        qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_TRUE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_FALSE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aStartValue,
                                     aStartType,
                                     &sValue1,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 Row로 부터 End Point이 N개 전 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aEndValue,
                                     aEndType,
                                     &sValue2,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        sSkipEnd                = ID_FALSE;
        aDataPlan->mtrRowIdx    = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            /* 시작 값보다 마지막이 크다면 멈춘다 */
            if ( aStartValue < aEndValue )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            if ( sSkipEnd == ID_FALSE )
            {
                /* 현재 값과 EndPoint의 값과 비교해서 현재 Row게 End값에 속하지 않는다면 끝낸다 */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue2,
                                             sIsMore,
                                             &sResult )
                          != IDE_SUCCESS );
                if ( sResult == ID_FALSE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                /* 현재 값과 StartPoint의 값과 비교한다. */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult )
                          != IDE_SUCCESS );
            }
            else
            {
                sResult = ID_TRUE;
            }
            if ( sResult == ID_TRUE )
            {
                /* Start 값 이후이므로 계산을 수행한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                sSkipEnd = ID_TRUE;
            }
            else
            {
                /* Start 값 이전 이므로 SkIP한다 */
                sSkipEnd = ID_FALSE;
            }
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 다음 Row를 읽는다 */
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                if ( sSkipEnd == ID_TRUE )
                {
                    /* 다음 Row가 EndPoint의 Value에 속한지 비교해본다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
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
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By N PRECEDING - N FOLLOWING
 *
 *  Start Point 가 N PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint,EndPoint
 *  값의 Value를 얻는이 이를 통해서 파티션의 처음부터 N개 전 Value까지 Skip하고 N개 후 Value
 *  까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::partitionPrecedFollowRange( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdMtrNode  * aOverColumnNode,
                                            qmdMtrNode  * aOrderByColumn,
                                            SLong         aStartValue,
                                            SInt          aStartType,
                                            SLong         aEndValue,
                                            SInt          aEndType,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;
    idBool             sIsFollowing;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_TRUE;
        sIsFollowing = ID_FALSE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_FALSE;
        sIsFollowing = ID_TRUE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aStartValue,
                                         aStartType,
                                         &sValue1,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 Row로 부터 End Point이 N개 후 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aEndValue,
                                         aEndType,
                                         &sValue2,
                                         sIsFollowing )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음위치로 이동한다. */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );
            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            sSkipEnd = ID_FALSE;
            do
            {
                if ( sSkipEnd == ID_FALSE )
                {
                    /* 현재 값과 StartPoint의 값과 비교한다. */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue1,
                                                 sIsLess,
                                                 &sResult )
                              != IDE_SUCCESS );
                }
                else
                {
                    sResult = ID_TRUE;
                }
                if ( sResult == ID_TRUE )
                {
                    /* Start 값 이후이므로 계산을 수행한다. */
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                    sSkipEnd = ID_TRUE;
                }
                else
                {
                    /* Start 값 이전 이므로 Skip한다. */
                    sSkipEnd = ID_FALSE;
                }
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
                if ( sSkipEnd == ID_TRUE )
                {
                    /* 다음 Row가 EndPoint의 Value에 속한지 비교해본다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
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
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By N PRECEDING - N FOLLOWING
 *
 *  Start Point 가 N PRECEDING 현재 Row의 N 개 전부터 시작한다.
 *  End   Point 는 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint,EndPoint
 *  값의 Value를 얻는이 이를 통해서 처음부터 N개 전 Value까지 Skip하고 N개 후 Value
 *  까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::orderPrecedFollowRange( qcTemplate  * aTemplate,
                                        qmndWNST    * aDataPlan,
                                        qmdMtrNode  * aOrderByColumn,
                                        SLong         aStartValue,
                                        SInt          aStartType,
                                        SLong         aEndValue,
                                        SInt          aEndType,
                                        qmdAggrNode * aAggrNode,
                                        qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;
    idBool             sIsFollowing;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_TRUE;
        sIsFollowing = ID_FALSE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_FALSE;
        sIsFollowing = ID_TRUE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row로 부터 Start Point이 N개 전 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aStartValue,
                                     aStartType,
                                     &sValue1,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 Row로 부터 End Point이 N개 후 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aEndValue,
                                     aEndType,
                                     &sValue2,
                                     sIsFollowing )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        sSkipEnd = ID_FALSE;

        aDataPlan->mtrRowIdx    = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            if ( sSkipEnd == ID_FALSE )
            {
                /* 현재 값과 StartPoint의 값과 비교한다. */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult )
                          != IDE_SUCCESS );
            }
            else
            {
                sResult = ID_TRUE;
            }
            if ( sResult == ID_TRUE )
            {
                /* Start 값 이후이므로 계산을 수행한다. */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                sSkipEnd = ID_TRUE;
            }
            else
            {
                /* Start 값 이전 이므로 SKIP한다 */
                sSkipEnd = ID_FALSE;
            }
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );

            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                if ( sSkipEnd == ID_TRUE )
                {
                    /* 다음 Row가 EndPoint의 Value에 속한지 비교해본다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue2,
                                                 sIsMore,
                                                 &sResult )
                              != IDE_SUCCESS );
                    if ( sResult == ID_FALSE )
                    {
                        break;
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
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Partition By Order By N FOLLOWING - N FOLLOWING
 *
 *  Start Point 가 N FOLLOWING 현재 Row의 N 개 후 부터 시작한다.
 *  End   Point 는 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 파티션의 처음과 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint,EndPoint
 *  값의 Value를 얻는이 이를 통해서 현재부터 Start Value의  N개 후 값부터 End Value의 N개 후
 *  값 까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::partitionFollowFollowRange( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdMtrNode  * aOverColumnNode,
                                            qmdMtrNode  * aOrderByColumn,
                                            SLong         aStartValue,
                                            SInt          aStartType,
                                            SLong         aEndValue,
                                            SInt          aEndType,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult1;
    idBool             sResult2;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
           QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_FALSE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_TRUE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        do
        {
            /* 현재 Row로 부터 Start Point이 N개 후 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aStartValue,
                                         aStartType,
                                         &sValue1,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 Row로 부터 End Point이 N개 후 값을 계산한다. */
            IDE_TEST( calculateInterval( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         aEndValue,
                                         aEndType,
                                         &sValue2,
                                         sIsPreceding )
                      != IDE_SUCCESS );

            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 파티션의 처음위치로 이동한다. */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );
            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            sSkipEnd = ID_FALSE;
            do
            {
                /* 시작 값이 마지막값 보다 크다면 멈춘다 */
                if ( aStartValue > aEndValue )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
                if ( sSkipEnd == ID_FALSE )
                {
                    /* 현재 Row와 StartPoint 값과 비교한다 */
                    IDE_TEST( compareRangeValue( aTemplate,
                                                 aDataPlan->sortMgr,
                                                 aOrderByColumn,
                                                 aDataPlan->plan.myTuple->row,
                                                 &sValue1,
                                                 sIsLess,
                                                 &sResult1 )
                              != IDE_SUCCESS );
                }
                else
                {
                    sResult1 = ID_TRUE;
                }

                /* 현재 Row과 EndPoint값과 비교한다 */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue2,
                                             sIsMore,
                                             &sResult2 )
                          != IDE_SUCCESS );
                if ( sResult2 == ID_FALSE )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }

                if ( sResult1 == ID_TRUE )
                {
                    /* Start값 이후이므로 Aggregation한다 */
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
                    sSkipEnd = ID_TRUE;
                }
                else
                {
                    /* Start값 이전 이므로 SKIP한다 */
                    sSkipEnd = ID_FALSE;
                }
                aDataPlan->mtrRowIdx    = 1;
                aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           &sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row로 되돌아 온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * RANGE Order By N FOLLOWING - N FOLLOWING
 *
 *  Start Point 가 N FOLLOWING 현재 Row의 N 개 후 부터 시작한다.
 *  End   Point 는 N FOLLOWING 현재 Row의 N 개 후 까지이다.
 *
 *  ORDER BY 구문에 사용된 컬럼이 ASC인지 DESC인지를 조사해서 비교해야할 옵션을 선택한다.
 *
 *  처음 시작하면 현재 Row의 위치를 저장한다. 현재 Row에서 StartPoint,EndPoint
 *  값의 Value를 얻는이 이를 통해서 현재부터 Start Value의  N개 후 값부터 End Value의 N개 후
 *  값 까지 Aggregation을 수행한다.
 */
IDE_RC qmnWNST::orderFollowFollowRange( qcTemplate  * aTemplate,
                                        qmndWNST    * aDataPlan,
                                        qmdMtrNode  * aOrderByColumn,
                                        SLong         aStartValue,
                                        SInt          aStartType,
                                        SLong         aEndValue,
                                        SInt          aEndType,
                                        qmdAggrNode * aAggrNode,
                                        qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag         sFlag   = QMC_ROW_INITIALIZE;
    qmcWndWindowValue  sValue1;
    qmcWndWindowValue  sValue2;
    idBool             sResult1;
    idBool             sResult2;
    idBool             sSkipEnd;
    idBool             sIsLess;
    idBool             sIsMore;
    idBool             sIsPreceding;

    /* ORDER BY 구문에 ASC인지 DESC인지에 따라 계산 이 달라진다 */
    if ( ( aOrderByColumn->myNode->flag & QMC_MTR_SORT_ORDER_MASK ) ==
            QMC_MTR_SORT_ASCENDING )
    {
        sIsLess      = ID_TRUE;
        sIsMore      = ID_FALSE;
        sIsPreceding = ID_FALSE;
    }
    else
    {
        sIsLess      = ID_FALSE;
        sIsMore      = ID_TRUE;
        sIsPreceding = ID_TRUE;
    }

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );

    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 Row로 부터 Start Point이 N개 후 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aStartValue,
                                     aStartType,
                                     &sValue1,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 Row로 부터 End Point이 N개 후 값을 계산한다. */
        IDE_TEST( calculateInterval( aTemplate,
                                     aDataPlan->sortMgr,
                                     aOrderByColumn,
                                     aDataPlan->plan.myTuple->row,
                                     aEndValue,
                                     aEndType,
                                     &sValue2,
                                     sIsPreceding )
                  != IDE_SUCCESS );

        /* 현재 위치를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        sSkipEnd                = ID_FALSE;
        aDataPlan->mtrRowIdx    = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            /* 시작 값이 마지막값 보다 크다면 멈춘다 */
            if ( aStartValue > aEndValue )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            if ( sSkipEnd == ID_FALSE )
            {
                /* 현재 Row와 StartPoint 값과 비교한다 */
                IDE_TEST( compareRangeValue( aTemplate,
                                             aDataPlan->sortMgr,
                                             aOrderByColumn,
                                             aDataPlan->plan.myTuple->row,
                                             &sValue1,
                                             sIsLess,
                                             &sResult1 )
                          != IDE_SUCCESS );
            }
            else
            {
                sResult1 = ID_TRUE;
            }

            /* 현재 Row과 EndPoint값과 비교한다 */
            IDE_TEST( compareRangeValue( aTemplate,
                                         aDataPlan->sortMgr,
                                         aOrderByColumn,
                                         aDataPlan->plan.myTuple->row,
                                         &sValue2,
                                         sIsMore,
                                         &sResult2 )
                      != IDE_SUCCESS );
            if ( sResult2 == ID_FALSE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            if ( sResult1 == ID_TRUE )
            {
                /* Start값 이후이므로 Aggregation한다 */
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
                sSkipEnd = ID_TRUE;
            }
            else
            {
                /* Start값 이전 이므로 SKIP한다 */
                sSkipEnd = ID_FALSE;
            }
            aDataPlan->mtrRowIdx    = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx = 0;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        /* 현재 Row로 되돌아 온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnWNST::getPositionValue( qcTemplate  * aTemplate,
                                  qmdAggrNode * aAggrNode,
                                  SLong       * aNumber )
{
    mtcStack  * sStack;
    mtcNode   * sArg1;
    mtcNode   * sArg2;
    SLong       sNumberValue = 0;

    /* lag, lead 함수는 next가 없다. */
    IDE_DASSERT( aAggrNode->next == NULL );
    
    sArg1 = aAggrNode->dstNode->node.arguments;
    sArg2 = sArg1->next;

    if ( sArg2 != NULL )
    {
        IDE_TEST( qtc::calculate( (qtcNode*)sArg2, aTemplate )
                  != IDE_SUCCESS );

        sStack = aTemplate->tmplate.stack;
        
        /* bigint로 받았음 */
        IDE_TEST_RAISE( sStack->column->module->id != MTD_BIGINT_ID,
                        ERR_INVALID_WINDOW_SPECIFICATION );
        
        sNumberValue = (SLong)(*(mtdBigintType*)sStack->value);
        
        IDE_TEST_RAISE( sNumberValue < 0, ERR_INVALID_WINDOW_SPECIFICATION );

        *aNumber = sNumberValue;
    }
    else
    {
        *aNumber = 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WINDOW_SPECIFICATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnWNST::checkNullAggregation( qcTemplate  * aTemplate,
                                      qmdAggrNode * aAggrNode,
                                      idBool      * aIsNull )
{
    mtcStack  * sStack;
    mtcNode   * sArg1;

    /* lag, lead 함수는 next가 없다. */
    IDE_DASSERT( aAggrNode->next == NULL );
    
    sArg1 = aAggrNode->dstNode->node.arguments;

    IDE_DASSERT( sArg1 != NULL );
    
    IDE_TEST( qtc::calculate( (qtcNode*)sArg1, aTemplate )
              != IDE_SUCCESS );

    sStack = aTemplate->tmplate.stack;
    
    if ( sStack->column->module->isNull( sStack->column,
                                         sStack->value ) == ID_TRUE )
    {
        *aIsNull = ID_TRUE;
    }
    else
    {
        *aIsNull = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 *  PROJ-1804 Ranking Function
 */
IDE_RC qmnWNST::partitionOrderByLagAggr( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         qmdMtrNode  * aOverColumnNode,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode )
{
    SLong       sLagPoint    = 0;
    qmcRowFlag  sFlag        = QMC_ROW_INITIALIZE;
    SLong       sCount       = 0;
    SLong       sWindowPos   = 0;
    idBool      sIsNull      = ID_FALSE;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        sWindowPos = 0;

        do
        {
            /* 현재 위치를 저장한다. */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );
            
            IDE_TEST( getPositionValue( aTemplate, aAggrNode, &sLagPoint )
                      != IDE_SUCCESS );
        
            IDE_TEST( initAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            /* 파티션의 처음으로 돌아간다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  & aDataPlan->partitionCursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;
            IDE_TEST( setTupleSet( aTemplate,
                                   aDataPlan->mtrNode,
                                   aDataPlan->plan.myTuple->row ) != IDE_SUCCESS );

            /* 파티션의 처음에서 현재 Row에서 N개 전까지 SKIP 한다 */
            for ( sCount = sWindowPos - sLagPoint;
                  sCount > 0;
                  sCount-- )
            {
                /* BUG-40279 lead, lag with ignore nulls */
                /* ignore nulls를 고려하여 이전까지의 null이 아닌 값들을 취한다. */
                if ( aAggrNode->dstNode->node.module == &mtfLagIgnoreNulls )
                {
                    if ( sWindowPos >= sLagPoint )
                    {
                        IDE_TEST( checkNullAggregation( aTemplate, aAggrNode, &sIsNull )
                                  != IDE_SUCCESS );

                        if ( sIsNull == ID_FALSE )
                        {
                            IDE_TEST( execAggregation( aTemplate,
                                                       aAggrNode,
                                                       NULL,
                                                       NULL,
                                                       0 )
                                      != IDE_SUCCESS );
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
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );
            }

            if ( sWindowPos >= sLagPoint )
            {
                /* BUG-40279 lead, lag with ignore nulls */
                if ( aAggrNode->dstNode->node.module == &mtfLagIgnoreNulls )
                {
                    IDE_TEST( checkNullAggregation( aTemplate, aAggrNode, &sIsNull )
                              != IDE_SUCCESS );
                }
                else
                {
                    sIsNull = ID_FALSE;
                }

                if ( sIsNull == ID_FALSE )
                {
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );
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

            IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                      != IDE_SUCCESS );

            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
            IDE_TEST( updateOneRowNextRecord( aTemplate,
                                              aDataPlan,
                                              aAggrResultNode,
                                              &sFlag )
                       != IDE_SUCCESS );

            ++sWindowPos;
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                break;
            }
        } while ( ( sFlag & QMC_ROW_GROUP_MASK ) == QMC_ROW_GROUP_SAME );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1804 Ranking Function */
IDE_RC qmnWNST::orderByLagAggr( qcTemplate  * aTemplate,
                                qmndWNST    * aDataPlan,
                                qmdAggrNode * aAggrNode,
                                qmdMtrNode  * aAggrResultNode )
{
    SLong       sLagPoint    = 0;
    qmcRowFlag  sFlag        = QMC_ROW_INITIALIZE;
    SLong       sCount       = 0;
    SLong       sWindowPos   = 0;
    idBool      sIsNull      = ID_FALSE;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    sWindowPos = 0;
    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( getPositionValue( aTemplate, aAggrNode, &sLagPoint )
                  != IDE_SUCCESS );
        
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 처음으로 돌아간다 */
        IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                  != IDE_SUCCESS );

        /* 파티션의 처음에서 현재 Row에서 N개 전까지 SKIP 한다 */
        for ( sCount = sWindowPos - sLagPoint;
              sCount > 0;
              sCount-- )
        {
            /* BUG-40279 lead, lag with ignore nulls */
            /* ignore nulls를 고려하여 이전까지의 null이 아닌 값들을 취한다. */
            if ( aAggrNode->dstNode->node.module == &mtfLagIgnoreNulls )
            {
                if ( sWindowPos >= sLagPoint )
                {
                    IDE_TEST( checkNullAggregation( aTemplate, aAggrNode, &sIsNull )
                              != IDE_SUCCESS );

                    if ( sIsNull == ID_FALSE )
                    {
                        IDE_TEST( execAggregation( aTemplate,
                                                   aAggrNode,
                                                   NULL,
                                                   NULL,
                                                   0 )
                                  != IDE_SUCCESS );
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
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
        }

        if ( sWindowPos >= sLagPoint )
        {
            /* BUG-40279 lead, lag with ignore nulls */
            if ( aAggrNode->dstNode->node.module == &mtfLagIgnoreNulls )
            {
                IDE_TEST( checkNullAggregation( aTemplate, aAggrNode, &sIsNull )
                          != IDE_SUCCESS );
            }
            else
            {
                sIsNull = ID_FALSE;
            }

            if ( sIsNull == ID_FALSE )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
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

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );

        ++sWindowPos;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1804 Ranking Function */
IDE_RC qmnWNST::partitionOrderByLeadAggr( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          qmdMtrNode  * aOverColumnNode,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode )
{
    SLong       sLeadPoint = 0;
    qmcRowFlag  sFlag      = QMC_ROW_INITIALIZE;
    SLong       sCount     = 0;
    idBool      sIsNull    = ID_FALSE;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( getPositionValue( aTemplate, aAggrNode, &sLeadPoint )
                  != IDE_SUCCESS );
        
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        aDataPlan->mtrRowIdx    = 1;
        aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

        for ( sCount = sLeadPoint; sCount > 0; sCount-- )
        {
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
            if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                /* 같은 파티션인지를 체크한다 */
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       &sFlag )
                          != IDE_SUCCESS );
                if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                {
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                break;
            }
        }

        if ( sCount == 0 )
        {
            /* BUG-40279 lead, lag with ignore nulls */
            /* ignore nulls를 고려하여 이후의 null이 아닌 값들을 찾는다. */
            if ( aAggrNode->dstNode->node.module == &mtfLeadIgnoreNulls )
            {
                while ( 1 )
                {
                    IDE_TEST( checkNullAggregation( aTemplate, aAggrNode, &sIsNull )
                              != IDE_SUCCESS );
                    if ( sIsNull == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                
                    IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                              != IDE_SUCCESS );
                    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                    {
                        /* 같은 파티션인지를 체크한다 */
                        IDE_TEST( compareRows( aDataPlan,
                                               aOverColumnNode,
                                               &sFlag )
                                  != IDE_SUCCESS );
                        if ( ( sFlag & QMC_ROW_GROUP_MASK ) != QMC_ROW_GROUP_SAME )
                        {
                            break;
                        }
                        else
                        {
                            /* Nothing to do */
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                sIsNull = ID_FALSE;
            }

            if ( sIsNull == ID_FALSE )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
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

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
        IDE_TEST( updateAggrRows( aTemplate,
                                  aDataPlan,
                                  aAggrResultNode,
                                  &sFlag,
                                  1 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1804 Ranking Function */
IDE_RC qmnWNST::orderByLeadAggr( qcTemplate  * aTemplate,
                                 qmndWNST    * aDataPlan,
                                 qmdAggrNode * aAggrNode,
                                 qmdMtrNode  * aAggrResultNode )
{
    SLong       sLeadPoint = 0;
    qmcRowFlag  sFlag      = QMC_ROW_INITIALIZE;
    SLong       sCount     = 0;
    idBool      sIsNull    = ID_FALSE;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 현재 위치를 저장한다. */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        IDE_TEST( getPositionValue( aTemplate, aAggrNode, &sLeadPoint )
                  != IDE_SUCCESS );
        
        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        for ( sCount = sLeadPoint; sCount > 0; sCount-- )
        {
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
            if ( ( sFlag & QMC_ROW_DATA_MASK ) != QMC_ROW_DATA_EXIST )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sCount == 0 )
        {
            /* BUG-40279 lead, lag with ignore nulls */
            /* ignore nulls를 고려하여 이후의 null이 아닌 값들을 찾는다. */
            if ( aAggrNode->dstNode->node.module == &mtfLeadIgnoreNulls )
            {
                while ( 1 )
                {
                    IDE_TEST( checkNullAggregation( aTemplate, aAggrNode, &sIsNull )
                              != IDE_SUCCESS );
                    if ( sIsNull == ID_FALSE )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                
                    IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                              != IDE_SUCCESS );
                    if ( ( sFlag & QMC_ROW_DATA_MASK ) != QMC_ROW_DATA_EXIST )
                    {
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            else
            {
                sIsNull = ID_FALSE;
            }

            if ( sIsNull == ID_FALSE )
            {
                IDE_TEST( execAggregation( aTemplate,
                                           aAggrNode,
                                           NULL,
                                           NULL,
                                           0 )
                          != IDE_SUCCESS );
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

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 현재 Row 위치로 되돌아온다 */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              &aDataPlan->cursorInfo )
                  != IDE_SUCCESS );

        aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

        IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
        IDE_TEST( updateOneRowNextRecord( aTemplate,
                                          aDataPlan,
                                          aAggrResultNode,
                                          &sFlag )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnWNST::getMinLimitValue( qcTemplate * aTemplate,
                                  qmdWndNode * aWndNode,
                                  SLong      * aNumber )
{
    qmdWndNode   * sWndNode;
    qmdAggrNode  * sAggrNode;
    mtcStack     * sStack;
    mtcNode      * sArg1;
    SLong          sNumberValue;
    SLong          sMaxNumberValue = ID_SLONG_MAX;

    for( sWndNode = aWndNode;
         sWndNode != NULL;
         sWndNode = sWndNode->next )
    {
        for ( sAggrNode = sWndNode->aggrNode;
              sAggrNode != NULL;
              sAggrNode = sAggrNode->next )
        {
            /* row_number_limit */
            IDE_TEST_RAISE( sAggrNode->dstNode->node.module != &mtfRowNumberLimit,
                            ERR_INVALID_FUNCTION );
            
            sArg1 = sAggrNode->dstNode->node.arguments;

            if ( sArg1 != NULL )
            {
                IDE_TEST( qtc::calculate( (qtcNode*)sArg1, aTemplate )
                          != IDE_SUCCESS );

                sStack = aTemplate->tmplate.stack;

                /* bigint로 받았음 */
                IDE_TEST_RAISE( sStack->column->module->id != MTD_BIGINT_ID,
                                ERR_INVALID_WINDOW_SPECIFICATION );

                sNumberValue = (SLong)(*(mtdBigintType*)sStack->value);

                if ( ( sNumberValue > 0 ) && ( sNumberValue < sMaxNumberValue ) )
                {
                    sMaxNumberValue = sNumberValue;
                }
                else
                {
                    /* Nothing to do */
                    // sMaxNumberValue = ID_SLONG_MAX;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    *aNumber = sMaxNumberValue;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnWNST::getMinLimitValue",
                                  "Invalid analytic function" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_WINDOW_SPECIFICATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-43086 support Ntile analytic function */
IDE_RC qmnWNST::getNtileValue( qcTemplate  * aTemplate,
                               qmdAggrNode * aAggrNode,
                               SLong       * aNumber )
{
    mtcStack  * sStack;
    mtcNode   * sArg1;
    SLong       sNumberValue = 0;

    sArg1 = aAggrNode->dstNode->node.arguments;

    IDE_TEST( qtc::calculate( (qtcNode*)sArg1, aTemplate )
            != IDE_SUCCESS );

    sStack = aTemplate->tmplate.stack;

    /* bigint로 받았음 */
    IDE_TEST_RAISE( sStack->column->module->id != MTD_BIGINT_ID,
            ERR_INVALID_WINDOW_SPECIFICATION );

    sNumberValue = (SLong)(*(mtdBigintType*)sStack->value);

    IDE_TEST_RAISE( ( sNumberValue <= 0 ) && ( sNumberValue != MTD_BIGINT_NULL ),
                    ERR_INVALID_WINDOW_SPECIFICATION );

    *aNumber = sNumberValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_WINDOW_SPECIFICATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_WINDOW_INVALID_AGGREGATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-43086 support Ntile analytic function */
IDE_RC qmnWNST::partitionOrderByNtileAggr( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOverColumnNode,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag  sFlag       = QMC_ROW_INITIALIZE;
    SLong       sSkipCount  = 0;
    SLong       sNtileValue = 0;
    SLong       sRowCount   = 0;
    SLong       sQuotient   = 0;
    SLong       sRemainder  = 0;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );
    
    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 파티션의 처음의 cursor를 저장한다 */
        IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                            &aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );
        
        IDE_TEST( getNtileValue( aTemplate, aAggrNode, &sNtileValue )
                  != IDE_SUCCESS );

        sRowCount   = 0;
        do
        {
            sRowCount++;
            aDataPlan->mtrRowIdx = 1;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];
            IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                      != IDE_SUCCESS );
            if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( compareRows( aDataPlan,
                                       aOverColumnNode,
                                       & sFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                // Data가 없으면 종료
                break;
            }
        } while( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME );

        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            /* 현재 위치를 저장한다 */
            IDE_TEST( qmcSortTemp::storeCursor( aDataPlan->sortMgr,
                                                &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( initAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        /* 파티션의 처음으로 Cursor로 이동한다. */
        IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                              & aDataPlan->partitionCursorInfo )
                  != IDE_SUCCESS );

        sFlag = 0;
        if ( sNtileValue == MTD_BIGINT_NULL )
        {
            while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( updateOneRowNextRecord( aTemplate,
                                                  aDataPlan,
                                                  aAggrResultNode,
                                                  &sFlag )
                           != IDE_SUCCESS );
                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           & sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Data가 없으면 종료
                    break;
                }
            }
        }
        else
        {
            sQuotient  = sRowCount / sNtileValue;
            sRemainder = sRowCount % sNtileValue;

            do
            {
                if ( sSkipCount < 1 )
                {
                    IDE_TEST( execAggregation( aTemplate,
                                               aAggrNode,
                                               NULL,
                                               NULL,
                                               0 )
                              != IDE_SUCCESS );

                    if ( ( sRemainder > 0 ) && ( sQuotient > 0 ) )
                    {
                        sRemainder--;
                        sSkipCount = sQuotient;
                    }
                    else
                    {
                        sSkipCount = sQuotient - 1;
                    }
                }
                else
                {
                    sSkipCount--;
                }

                IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                          != IDE_SUCCESS );
                aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

                /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
                IDE_TEST( updateOneRowNextRecord( aTemplate,
                                                  aDataPlan,
                                                  aAggrResultNode,
                                                  &sFlag )
                           != IDE_SUCCESS );

                if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
                {
                    IDE_TEST( compareRows( aDataPlan,
                                           aOverColumnNode,
                                           & sFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Data가 없으면 종료
                    break;
                }
            } while ( (sFlag & QMC_ROW_GROUP_MASK) == QMC_ROW_GROUP_SAME );
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );

        if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            aDataPlan->mtrRowIdx = 0;
            aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

            /* 현재 Row 위치로 되돌아온다 */
            IDE_TEST( qmcSortTemp::restoreCursor( aDataPlan->sortMgr,
                                                  &aDataPlan->cursorInfo )
                      != IDE_SUCCESS );

            aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

            IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-43086 support Ntile analytic function */
IDE_RC qmnWNST::orderByNtileAggr( qcTemplate  * aTemplate,
                                  qmndWNST    * aDataPlan,
                                  qmdAggrNode * aAggrNode,
                                  qmdMtrNode  * aAggrResultNode )
{
    qmcRowFlag  sFlag       = QMC_ROW_INITIALIZE;
    SLong       sSkipCount  = 0;
    SLong       sNtileValue = 0;
    SLong       sRowCount   = 0;
    SLong       sQuotient   = 0;
    SLong       sRemainder  = 0;

    IDE_TEST( qmcSortTemp::setUpdateColumnList( aDataPlan->sortMgr,
                                                aAggrResultNode )
              != IDE_SUCCESS );
    aDataPlan->mtrRowIdx    = 0;
    aDataPlan->plan.myTuple->row = aDataPlan->mtrRow[aDataPlan->mtrRowIdx];

    IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
              != IDE_SUCCESS );

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( getNtileValue( aTemplate, aAggrNode, &sNtileValue )
                  != IDE_SUCCESS );
            
        IDE_TEST( initAggregation( aTemplate,
                                   aAggrNode )
                  != IDE_SUCCESS );

        if ( sNtileValue == MTD_BIGINT_NULL )
        {
            while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( updateOneRowNextRecord( aTemplate,
                                                  aDataPlan,
                                                  aAggrResultNode,
                                                  &sFlag )
                           != IDE_SUCCESS );
            }
        }
        else
        {
            while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
            {
                while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                {
                    sRowCount++;
                    IDE_TEST( getNextRecord( aTemplate, aDataPlan, &sFlag )
                              != IDE_SUCCESS );
                }

                sQuotient  = sRowCount / sNtileValue;
                sRemainder = sRowCount % sNtileValue;

                /* 처음 Row 위치로 되돌아온다 */
                IDE_TEST( getFirstRecord( aTemplate, aDataPlan, &sFlag )
                          != IDE_SUCCESS );

                while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
                {
                    if ( sSkipCount < 1 )
                    {
                        IDE_TEST( execAggregation( aTemplate,
                                                   aAggrNode,
                                                   NULL,
                                                   NULL,
                                                   0 )
                                  != IDE_SUCCESS );

                        if ( ( sRemainder > 0 ) && ( sQuotient > 0 ) )
                        {
                            sRemainder--;
                            sSkipCount = sQuotient;
                        }
                        else
                        {
                            sSkipCount = sQuotient - 1;
                        }
                    }
                    else
                    {
                        sSkipCount--;
                    }

                    IDE_TEST( setTupleSet( aTemplate, aDataPlan->mtrNode, aDataPlan->plan.myTuple->row )
                              != IDE_SUCCESS );
                    aDataPlan->mtrRow[aDataPlan->mtrRowIdx] = aDataPlan->plan.myTuple->row;

                    /* Aggregation된 값을 update하고 다음 Row를 읽는다. */
                    IDE_TEST( updateOneRowNextRecord( aTemplate,
                                                      aDataPlan,
                                                      aAggrResultNode,
                                                      &sFlag )
                               != IDE_SUCCESS );
                }
            }
        }

        IDE_TEST( finiAggregation( aTemplate, aAggrNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

