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
 * $Id: qmnLimitSort.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     LMST(LiMit SorT) Node
 *
 *     관계형 모델에서 sorting 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Limit Order By
 *             : SELECT * FROM T1 ORDER BY I1 LIMIT 10;
 *         - Store And Search
 *             : MIN, MAX 값만을 저장 관리
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcuProperty.h>
#include <qmnLimitSort.h>
#include <qmxResultCache.h>

IDE_RC
qmnLMST::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    LMST 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLMST * sCodePlan = (qmncLMST *) aPlan;
    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    idBool sDependency;

    sDataPlan->doIt        = qmnLMST::doItDefault;

    //----------------------------------------
    // 최초 초기화 수행
    //----------------------------------------

    if ( (*sDataPlan->flag & QMND_LMST_INIT_DONE_MASK)
         == QMND_LMST_INIT_DONE_FALSE )
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

        sDataPlan->mtrTotalCnt = 0;
        sDataPlan->isNullStore = ID_FALSE;

        IDE_TEST( qmcSortTemp::clear( sDataPlan->sortMgr ) != IDE_SUCCESS );

        //----------------------------------------
        // 용도에 따른 Temp Table을 구축
        //----------------------------------------

        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);

        // BUG-37681
        if( sCodePlan->limitCnt != 0 )
        {
            if ( (sCodePlan->flag & QMNC_LMST_USE_MASK ) == QMNC_LMST_USE_ORDERBY )
            {
                // Limit Order By를 위한 저장
                IDE_TEST( store4OrderBy( aTemplate, sCodePlan, sDataPlan )
                        != IDE_SUCCESS );
            }
            else
            {
                // Store And Search를 위한 저장
                IDE_TEST( store4StoreSearch( aTemplate, sCodePlan, sDataPlan )
                        != IDE_SUCCESS );
            }
        }
        else
        {
            // Nothing To Do
        }

        //----------------------------------------
        // Temp Table 구축 후 초기화
        //----------------------------------------

        sDataPlan->depValue = sDataPlan->depTuple->modify;
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------------
    // 수행 함수 결정
    //----------------------------------------

    // BUG-37681 limit 0 일때 별도의 doit 함수를 수행하도록 한다.
    if( sCodePlan->limitCnt != 0 )
    {
        if ( (sCodePlan->flag & QMNC_LMST_USE_MASK ) == QMNC_LMST_USE_ORDERBY )
        {
            sDataPlan->doIt = qmnLMST::doItFirstOrderBy;
        }
        else
        {
            sDataPlan->doIt = qmnLMST::doItFirstStoreSearch;
        }
    }
    else
    {
        sDataPlan->doIt = qmnLMST::doItAllFalse;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnLMST::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    LMST 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncLMST * sCodePlan = (qmncLMST *) aPlan;
    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnLMST::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    LMST 노드의 Tuple에 Null Row를 설정한다.
 *
 * Implementation :
 *    Child Plan의 Null Padding을 수행하고,
 *    자신의 Null Row를 Temp Table로부터 획득한다.
 *
 ***********************************************************************/

#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLMST * sCodePlan = (qmncLMST *) aPlan;
    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_LMST_INIT_DONE_MASK)
         == QMND_LMST_INIT_DONE_FALSE )
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
qmnLMST::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    LMST 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncLMST * sCodePlan = (qmncLMST *) aPlan;
    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndLMST * sCacheDataPlan = NULL;
    idBool     sIsInit       = ID_FALSE;
    ULong  i;
    ULong  sDiskPageCnt;
    SLong  sRecordCnt;

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

        if ( (*sDataPlan->flag & QMND_LMST_INIT_DONE_MASK)
             == QMND_LMST_INIT_DONE_TRUE )
        {
            sIsInit = ID_TRUE;
            // 수행 정보 획득
            IDE_TEST( qmcSortTemp::getDisplayInfo( sDataPlan->sortMgr,
                                                   & sDiskPageCnt,
                                                   & sRecordCnt )
                      != IDE_SUCCESS );
            IDE_DASSERT( sDiskPageCnt == 0 );

            if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
            {
                iduVarStringAppendFormat(
                    aString,
                    "LIMIT-SORT ( ITEM_SIZE: %"ID_UINT32_FMT", "
                    "ITEM_COUNT: %"ID_UINT64_FMT", "
                    "STORE_COUNT: %"ID_INT64_FMT", "
                    "ACCESS: %"ID_UINT32_FMT,
                    sDataPlan->mtrRowSize,
                    sDataPlan->mtrTotalCnt,
                    sRecordCnt,
                    sDataPlan->plan.myTuple->modify );
            }
            else
            {
                // BUG-29209
                // ITEM_SIZE 정보 보여주지 않음
                iduVarStringAppendFormat(
                    aString,
                    "LIMIT-SORT ( ITEM_SIZE: BLOCKED, "
                    "ITEM_COUNT: %"ID_UINT64_FMT", "
                    "STORE_COUNT: %"ID_INT64_FMT", "
                    "ACCESS: %"ID_UINT32_FMT,
                    sDataPlan->mtrTotalCnt,
                    sRecordCnt,
                    sDataPlan->plan.myTuple->modify );
            }
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "LIMIT-SORT ( ITEM_SIZE: 0, "
                                      "ITEM_COUNT: 0, "
                                      "STORE_COUNT: 0, "
                                      "ACCESS: 0" );
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------

        iduVarStringAppendFormat( aString,
                                  "LIMIT-SORT ( ITEM_SIZE: ??, "
                                  "ITEM_COUNT: ??, "
                                  "STORE_COUNT: ??, "
                                  "ACCESS: ??" );

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
            sCacheDataPlan = (qmndLMST *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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

IDE_RC
qmnLMST::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnLMST::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnLMST::doItFirstOrderBy( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    LIMIT ORDER BY를 위한 최초 수행
 *
 * Implementation :
 *    Temp Table을 이용하여 순차 검색을 한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::doItFirstOrderBy"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstSequence( sDataPlan->sortMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Data가 존재할 경우 Tuple Set 복원
    if ( sSearchRow != NULL )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );

        sDataPlan->doIt = qmnLMST::doItNextOrderBy;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnLMST::doItNextOrderBy( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    LIMIT ORDER BY를 위한 다음 수행
 *
 * Implementation :
 *    Temp Table을 이용하여 순차 검색을 한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::doItNextOrderBy"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getNextSequence( sDataPlan->sortMgr,
                                            & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Data가 존재할 경우 Tuple Set 복원
    if ( sSearchRow != NULL )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
        sDataPlan->doIt = qmnLMST::doItFirstOrderBy;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnLMST::doItFirstStoreSearch( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Store And Search를 위한 첫 번째 검색을 수행한다.
 *
 * Implementation :
 *     LMST가 Store And Search를 위해 사용되는 경우는
 *     다음과 같은 Subquery Predicate이 사용되는 경우이다.
 *     Predicate과 필요로 하는 Data의 종류는 다음과 같다.
 *     ------------------------------------------------------
 *       Predicate          |   필요 Data
 *     ------------------------------------------------------
 *       >ANY, >=ANY        |  MIN Value, NULL Value
 *       <ANY, <=ANY        |  MAX Value, NULL Value
 *       >ALL, >=ALL        |  MAX Value, NULL Value
 *       <ALL, <=ALL        |  MIN Value, NULL Value
 *       =ALL               |  MIN Value, MAX Value, NULL Value
 *       !=ALL              |  MIN Value, MAX Value, NULL Value
 *     ------------------------------------------------------
 *
 *    그러나, Predicate에 따른 필요 Data를 일일이 결정하지 않고
 *    다음과 같은 순서에 의하여 Data가 Return 될 수 있도록 하여
 *    복잡도를 낮춘다.
 *        MIN Value -> MAX Value -> NULL Value -> Data 없음
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::doItFirstStoreSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    //------------------------------------
    // Left-Most 값 추출
    //------------------------------------

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstSequence( sDataPlan->sortMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    if ( sSearchRow != NULL )
    {
        //------------------------------------
        // Data가 존재할 경우 Tuple Set 복원
        //------------------------------------

        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );

        sDataPlan->doIt = qmnLMST::doItNextStoreSearch;
    }
    else
    {
        //------------------------------------
        // Data가 없는 경우 NULL 존재 유무 확인
        //------------------------------------

        if ( sDataPlan->isNullStore == ID_TRUE )
        {
            // NULL Row를 만들어 줌
            *aFlag = QMC_ROW_DATA_EXIST;

            IDE_TEST( qmnLMST::padNull( aTemplate, aPlan ) != IDE_SUCCESS );

            sDataPlan->doIt = qmnLMST::doItLastStoreSearch;
        }
        else
        {
            // 원하는 결과가 없음
            *aFlag = QMC_ROW_DATA_NONE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnLMST::doItNextStoreSearch( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Store And Search를 위한 다음 검색을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::doItNextStoreSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    //------------------------------------
    // Right-Most 값 추출
    //------------------------------------

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getNextSequence( sDataPlan->sortMgr,
                                            & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    if ( sSearchRow != NULL )
    {
        //------------------------------------
        // Data가 존재할 경우 Tuple Set 복원
        //------------------------------------

        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------
        // Data가 없는 경우 NULL 존재 유무 확인
        //------------------------------------

        if ( sDataPlan->isNullStore == ID_TRUE )
        {
            // NULL Row를 만들어 줌
            *aFlag = QMC_ROW_DATA_EXIST;

            IDE_TEST( qmnLMST::padNull( aTemplate, aPlan ) != IDE_SUCCESS );

            sDataPlan->doIt = qmnLMST::doItLastStoreSearch;
        }
        else
        {
            // 원하는 결과가 없음
            *aFlag = QMC_ROW_DATA_NONE;
            sDataPlan->doIt = qmnLMST::doItFirstStoreSearch;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnLMST::doItLastStoreSearch( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Store And Search를 위한 마지막 검색을 수행한다.
 *
 * Implementation :
 *     데이터 없음을 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::doItLastStoreSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncLMST * sCodePlan = (qmncLMST *) aPlan;
    qmndLMST * sDataPlan =
        (qmndLMST *) (aTemplate->tmplate.data + aPlan->offset);

    *aFlag = QMC_ROW_DATA_NONE;
    sDataPlan->doIt = qmnLMST::doItFirstStoreSearch;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnLMST::doItAllFalse( qcTemplate * /*aTemplate*/,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
    qmncLMST * sCodePlan = (qmncLMST *) aPlan;

    IDE_DASSERT( sCodePlan->limitCnt == 0 );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;

    return IDE_SUCCESS;
}

IDE_RC
qmnLMST::firstInit( qcTemplate * aTemplate,
                    qmncLMST   * aCodePlan,
                    qmndLMST   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    LMST node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndLMST * sCacheDataPlan = NULL;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                 == QMN_PLAN_STORAGE_MEMORY );

    IDE_DASSERT( aCodePlan->limitCnt <= QMN_LMST_MAXIMUM_LIMIT_CNT );

    //---------------------------------
    // LMST 고유 정보의 초기화
    //---------------------------------

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndLMST *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
        aDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
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

    *aDataPlan->flag &= ~QMND_LMST_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_LMST_INIT_DONE_TRUE;

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
qmnLMST::initMtrNode( qcTemplate * aTemplate,
                      qmncLMST   * aCodePlan,
                      qmndLMST   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 관리를 위한 노드를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );

    aDataPlan->mtrNode =
        (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);

    if ( (aCodePlan->flag & QMNC_LMST_USE_MASK ) == QMNC_LMST_USE_ORDERBY )
    {
        // Nothing To Do
    }
    else
    {
        // Store And Search로 사용되는 경우 Base Table은 저장하지 않는다.
        IDE_DASSERT( aCodePlan->baseTableCount == 0 );
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

    /* PROJ-2462 Result Cache */
    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------

    // 1.  저장 Column의 연결 정보 생성
    // 2.  저장 Column의 초기화
    // 3.  저장 Column의 offset을 재조정
    // 4.  Row Size의 계산

    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                aCodePlan->baseTableCount )
              != IDE_SUCCESS );

    // Memory Temp Table만을 사용함
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  QMC_MEMSORT_TEMPHEADER_SIZE )
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
qmnLMST::initSortNode( qmncLMST   * aCodePlan,
                       qmndLMST   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     저장 Column의 정보 중 정렬 Column의 시작 위치를 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::initSortNode"
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

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    if ( (aCodePlan->flag & QMNC_LMST_USE_MASK ) == QMNC_LMST_USE_ORDERBY )
    {
        // 반드시 정렬 Column이 있어야 한다.
        IDE_DASSERT( aDataPlan->sortNode != NULL );
    }
    else
    {
        // Store And Search로 사용되는 경우
        IDE_DASSERT( aDataPlan->sortNode == aDataPlan->mtrNode );
    }

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC
qmnLMST::initTempTable( qcTemplate * aTemplate,
                        qmncLMST   * aCodePlan,
                        qmndLMST   * aDataPlan )
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
    qmndLMST * sCacheDataPlan = NULL;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    // Memory Temp Table만을 사용함
    IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK )
                 == MTC_TUPLE_STORAGE_MEMORY );

    //-----------------------------
    // Flag 정보 초기화
    //-----------------------------

    sFlag = QMCD_SORT_TMP_STORAGE_MEMORY;

    //-----------------------------
    // Temp Table 초기화
    //-----------------------------

    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnLMST::initTempTable::qmxAlloc:sortMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                  (void **)&aDataPlan->sortMgr )
                  != IDE_SUCCESS );
        IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->sortNode,
                                     0,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndLMST *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnLMST::initTempTable::qrcAlloc:sortMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                               (void **)&aDataPlan->sortMgr )
                      != IDE_SUCCESS );

            IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         aDataPlan->sortNode,
                                         0,
                                         sFlag )
                      != IDE_SUCCESS );
            sCacheDataPlan->sortMgr = aDataPlan->sortMgr;
        }
        else
        {
            aDataPlan->sortMgr     = sCacheDataPlan->sortMgr;
            aDataPlan->mtrTotalCnt = sCacheDataPlan->mtrTotalCnt;
            aDataPlan->isNullStore = sCacheDataPlan->isNullStore;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnLMST::checkDependency( qcTemplate * aTemplate,
                                 qmncLMST   * aCodePlan,
                                 qmndLMST   * aDataPlan,
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
qmnLMST::store4OrderBy( qcTemplate * aTemplate,
                        qmncLMST   * aCodePlan,
                        qmndLMST   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     ORDER BY를 위한 저장
 *
 * Implementation :
 *     다음 조건까지 Child를 반복 수행하여 저장
 *          - Data가 존재하지 않을 경우
 *          - Limit보다 많이 삽입되는 경우
 *     Sorting 수행
 *     Data가 더 존재할 경우
 *          - 삽입 여부 결정 및 삽입 수행
 *          - Memory를 항상 재사용하게 되므로, 추가적인 메모리 할당은
 *            하지 않는다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::store4OrderBy"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;
    qmndLMST * sCacheDataPlan = NULL;

    //------------------------------
    // Child Record의 저장
    //------------------------------

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    while ( ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST) &&
            ( aDataPlan->mtrTotalCnt < aCodePlan->limitCnt ) )
    {
        aDataPlan->mtrTotalCnt++;

        IDE_TEST( addOneRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    //------------------------------
    // 정렬 수행
    //------------------------------
    // SORT와 달리 LMST의 경우, 항상 정렬을 수행함
    IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr ) != IDE_SUCCESS );

    //------------------------------
    // Limit Sorting 수행
    //------------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                      & aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );

        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            aDataPlan->mtrTotalCnt++;

            IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                      != IDE_SUCCESS );

            IDE_TEST( qmcSortTemp::shiftAndAppend( aDataPlan->sortMgr,
                                                   aDataPlan->plan.myTuple->row,
                                                   & aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                                  aCodePlan->plan.left,
                                                  & sFlag ) != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-2462 Result Cache
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndLMST *)(aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->mtrTotalCnt = aDataPlan->mtrTotalCnt;
        sCacheDataPlan->isNullStore = aDataPlan->isNullStore;

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

#undef IDE_FN
}


IDE_RC
qmnLMST::store4StoreSearch( qcTemplate * aTemplate,
                            qmncLMST   * aCodePlan,
                            qmndLMST   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Store And Search를 위한 저장
 *     MIN, MAX값만 저장하고 NULL존재 여부만 판단한다.
 *
 * Implementation :
 *     2건만을 저장
 *        - MIN, MAX 처리를 위해 두 건만을 저장
 *        - NULL 존재 유무를 검사하여 NULL이 아닌 경우만 삽입
 *     2건 저장 시 정렬 수행
 *     이 후 저장하는 Column은 NULL이 아닌 경우에 한하여
 *          - Min, Max값을 교환하여 저장
 *          - Memory를 항상 재사용하게 되므로, 추가적인 메모리 할당은
 *            하지 않는다.
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::store4StoreSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;
    idBool     sIsNull;
    ULong      sRowCnt;
    qmndLMST * sCacheDataPlan = NULL;

    //------------------------------
    // 적합성 검사
    //------------------------------

    IDE_DASSERT( aCodePlan->limitCnt == 2 );

    //------------------------------
    // Child Record의 저장
    //------------------------------

    IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                  & aDataPlan->plan.myTuple->row )
              != IDE_SUCCESS );

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    sRowCnt = 0;
    while ( ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST) &&
            sRowCnt < aCodePlan->limitCnt )
    {
        aDataPlan->mtrTotalCnt++;

        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        sIsNull = aDataPlan->sortNode->func.isNull( aDataPlan->sortNode,
                                                    aDataPlan->plan.myTuple->row );
        if ( sIsNull == ID_TRUE )
        {
            aDataPlan->isNullStore = ID_TRUE;
        }
        else
        {
            sRowCnt++;
            IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                           aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                          & aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    //------------------------------
    // 정렬 수행
    //------------------------------

    IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr ) != IDE_SUCCESS );

    //------------------------------
    // MIN-MAX 교환 수행
    //------------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            aDataPlan->mtrTotalCnt++;

            IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                      != IDE_SUCCESS );

            sIsNull =
                aDataPlan->sortNode->func.isNull( aDataPlan->sortNode,
                                                  aDataPlan->plan.myTuple->row );
            if ( sIsNull == ID_TRUE )
            {
                aDataPlan->isNullStore = ID_TRUE;
            }
            else
            {
                IDE_TEST(
                    qmcSortTemp::changeMinMax( aDataPlan->sortMgr,
                                               aDataPlan->plan.myTuple->row,
                                               & aDataPlan->plan.myTuple->row )
                    != IDE_SUCCESS );
            }

            IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                                  aCodePlan->plan.left,
                                                  & sFlag ) != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-2462 Result Cache
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndLMST *)(aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->mtrTotalCnt = aDataPlan->mtrTotalCnt;
        sCacheDataPlan->isNullStore = aDataPlan->isNullStore;

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

#undef IDE_FN
}

IDE_RC
qmnLMST::addOneRow( qcTemplate * aTemplate,
                    qmndLMST   * aDataPlan )
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

#define IDE_FN "qmnLMST::addOneRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                  & aDataPlan->plan.myTuple->row )
              != IDE_SUCCESS );

    IDE_TEST( setMtrRow( aTemplate,
                         aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                   aDataPlan->plan.myTuple->row )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnLMST::setMtrRow( qcTemplate * aTemplate,
                    qmndLMST   * aDataPlan )
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

#define IDE_FN "qmnLMST::setMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
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
qmnLMST::setTupleSet( qcTemplate * aTemplate,
                      qmndLMST   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     검색된 저장 Row를 기준으로 Tuple Set을 복원한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnLMST::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate, sNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    // To Fix PR-8017
    aDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
