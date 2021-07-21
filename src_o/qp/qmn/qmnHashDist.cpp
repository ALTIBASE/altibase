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
 * $Id: qmnHashDist.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     HSDS(HaSh DiStinct) Node
 *
 *     관계형 모델에서 hash-based distinction 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Hash-based Distinction
 *         - Set Union
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnHashDist.h>
#include <qmxResultCache.h>

IDE_RC
qmnHSDS::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    HSDS 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHSDS * sCodePlan = (qmncHSDS *) aPlan;
    qmndHSDS * sDataPlan =
        (qmndHSDS *) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;
    idBool     sDependency;

    sDataPlan->doIt = qmnHSDS::doItDefault;

    //----------------------------------------
    // 최초 초기화 수행
    //----------------------------------------

    if ( (*sDataPlan->flag & QMND_HSDS_INIT_DONE_MASK)
         == QMND_HSDS_INIT_DONE_FALSE )
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

        IDE_TEST( qmcHashTemp::clear( sDataPlan->hashMgr )
                  != IDE_SUCCESS );

        //----------------------------------------
        // Child를 반복 수행하여 Temp Table을 구축
        //----------------------------------------

        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);

        if ( ( sCodePlan->flag & QMNC_HSDS_IN_TOP_MASK )
             == QMNC_HSDS_IN_TOP_FALSE )
        {
            IDE_TEST( qmnHSDS::doItDependent( aTemplate, aPlan, & sFlag )
                      != IDE_SUCCESS );

            while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
            {
                IDE_TEST( qmnHSDS::doItDependent( aTemplate, aPlan, & sFlag )
                          != IDE_SUCCESS );
            }

            /* PROJ-2462 Result Cache */
            if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
                 == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
            {
                *sDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_STORED_MASK;
                *sDataPlan->resultData.flag |= QMX_RESULT_CACHE_STORED_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            // Top Query에서 사용되는 경우 초기화 과정에서 Data를 구축하지
            // 않고 수행 과정에서 Data 구축 및 결과 리턴을 수행한다.
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

    if ( ( sCodePlan->flag & QMNC_HSDS_IN_TOP_MASK )
         == QMNC_HSDS_IN_TOP_FALSE )
    {
        // 이미 구성된 결과를 리턴한다.
        sDataPlan->doIt = qmnHSDS::doItFirstIndependent;
    }
    else
    {
        if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
        {
            if ( ( *sDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
                 == QMX_RESULT_CACHE_STORED_TRUE )
            {
                // 이미 구성된 결과를 리턴한다.
                sDataPlan->doIt = qmnHSDS::doItFirstIndependent;
            }
            else
            {
                // 수행 과정 중에 Temp Table을 구성하고,
                // 결과 생성시마다 리턴한다.
                sDataPlan->doIt = qmnHSDS::doItDependent;
            }
        }
        else
        {
            // 수행 과정 중에 Temp Table을 구성하고,
            // 결과 생성시마다 리턴한다.
            sDataPlan->doIt = qmnHSDS::doItDependent;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnHSDS::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    HSDS 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncHSDS * sCodePlan = (qmncHSDS *) aPlan;
    qmndHSDS * sDataPlan =
        (qmndHSDS*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHSDS::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    HSDS 노드의 Tuple에 Null Row를 설정한다.
 *
 * Implementation :
 *    Child Plan의 Null Padding을 수행하고,
 *    자신의 Null Row를 Temp Table로부터 획득한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHSDS * sCodePlan = (qmncHSDS *) aPlan;
    qmndHSDS * sDataPlan =
        (qmndHSDS *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_HSDS_INIT_DONE_MASK)
         == QMND_HSDS_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }


    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    IDE_TEST( qmcHashTemp::getNullRow( sDataPlan->hashMgr,
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
qmnHSDS::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    HSDS 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHSDS * sCodePlan = (qmncHSDS*) aPlan;
    qmndHSDS * sDataPlan =
        (qmndHSDS*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndHSDS * sCacheDataPlan = NULL;
    idBool     sIsInit       = ID_FALSE;
    SLong sRecordCnt;
    ULong sPageCnt;
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

        if ( (*sDataPlan->flag & QMND_HSDS_INIT_DONE_MASK)
             == QMND_HSDS_INIT_DONE_TRUE )
        {
            sIsInit = ID_TRUE;
            // 수행 정보 획득
            IDE_TEST( qmcHashTemp::getDisplayInfo( sDataPlan->hashMgr,
                                                   & sPageCnt,
                                                   & sRecordCnt,
                                                   & sBucketCnt )
                      != IDE_SUCCESS );

            if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                 == QMN_PLAN_STORAGE_MEMORY )
            {
                if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                {
                    iduVarStringAppendFormat(
                        aString,
                        "DISTINCT ( ITEM_SIZE: %"ID_UINT32_FMT", "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
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
                        "DISTINCT ( ITEM_SIZE: BLOCKED, "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
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
                        "DISTINCT ( ITEM_SIZE: %"ID_UINT32_FMT", "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
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
                        "DISTINCT ( ITEM_SIZE: BLOCKED, "
                        "ITEM_COUNT: %"ID_INT64_FMT", "
                        "DISK_PAGE_COUNT: BLOCKED, "
                        "ACCESS: %"ID_UINT32_FMT,
                        sRecordCnt,
                        sDataPlan->plan.myTuple->modify );
                }
            }
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "DISTINCT ( ITEM_SIZE: 0, ITEM_COUNT: 0, "
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
                                  "DISTINCT ( ITEM_SIZE: ??, ITEM_COUNT: ??, "
                                  "BUCKET_COUNT: %"ID_UINT32_FMT", "
                                  "ACCESS: ??",
                                  sCodePlan->bucketCnt );
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
            sCacheDataPlan = (qmndHSDS *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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

#undef IDE_FN
}

IDE_RC
qmnHSDS::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnHSDS::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHSDS::doItDependent( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    새로운 Row 출현 시 바로 결과를 리턴
 *    Top Query에서 수행될 경우 호출된다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::doItDependent"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHSDS * sCodePlan = (qmncHSDS *) aPlan;
    qmndHSDS * sDataPlan =
        (qmndHSDS *) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag sFlag     = QMC_ROW_INITIALIZE;
    idBool     sInserted;

    sInserted = ID_TRUE;

    //------------------------------
    // Child Record의 저장
    //------------------------------

    IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                          sCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //---------------------------------------
        // 1.  저장 Row를 위한 공간 할당
        // 2.  저장 Row의 구성
        // 3.  저장 Row의 삽입
        // 4.  삽입 성공 여부에 따른 결과 리턴
        //---------------------------------------

        // 1.  저장 Row를 위한 공간 할당
        if ( sInserted == ID_TRUE )
        {
            IDE_TEST( qmcHashTemp::alloc( sDataPlan->hashMgr,
                                          & sDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {
            // 삽입이 실패한 경우로 이미 할당받은 공간을 사용한다.
            // 따라서, 별도로 공간을 할당 받을 필요가 없다.
        }

        // 2.  저장 Row의 구성
        IDE_TEST( setMtrRow( aTemplate,
                             sDataPlan ) != IDE_SUCCESS );

        // 3.  저장 Row의 삽입
        IDE_TEST( qmcHashTemp::addDistRow( sDataPlan->hashMgr,
                                           & sDataPlan->plan.myTuple->row,
                                           & sInserted )
                  != IDE_SUCCESS );

        // 4.  삽입 성공 여부에 따른 결과 리턴
        if ( sInserted == ID_TRUE )
        {
            // 삽입이 성공한 경우 Distinct Row이다.
            // 따라서, 해당 결과를 리턴한다.
            break;
        }
        else
        {
            // 삽입이 실패한 경우 동일한 Row가 존재하고 있다.
            // 따라서, Child를 다시 수행한다.
            IDE_TEST( sCodePlan->plan.left->doIt( aTemplate,
                                                  sCodePlan->plan.left,
                                                  & sFlag ) != IDE_SUCCESS );
        }
    }

    //------------------------------
    // 결과 리턴
    //------------------------------

    if ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS);
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }

    /* PROJ-2462 Result Cache */
    if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        *sDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_STORED_MASK;
        *sDataPlan->resultData.flag |= QMX_RESULT_CACHE_STORED_TRUE;
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
qmnHSDS::doItFirstIndependent( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Subquery 내에서 사용될 경우의 최초 수행 함수
 *
 * Implementation :
 *    이미 구성된 Temp Table로부터 한 껀씩 얻어온다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::doItFirstIndependent"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndHSDS * sDataPlan =
        (qmndHSDS *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getFirstSequence( sDataPlan->hashMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );

        sDataPlan->doIt = qmnHSDS::doItNextIndependent;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHSDS::doItNextIndependent( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Subquery 내에서 사용될 경우의 다음 수행 함수
 *
 * Implementation :
 *    이미 구성된 Temp Table로부터 한 껀씩 얻어온다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::doItNextIndependent"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndHSDS * sDataPlan =
        (qmndHSDS *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getNextSequence( sDataPlan->hashMgr,
                                            & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
        sDataPlan->doIt = qmnHSDS::doItFirstIndependent;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;

        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHSDS::firstInit( qcTemplate * aTemplate,
                    qmncHSDS   * aCodePlan,
                    qmndHSDS   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    HSDS node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndHSDS * sCacheDataPlan = NULL;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    //---------------------------------
    // HSDS 고유 정보의 초기화
    //---------------------------------
    //
    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndHSDS *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
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

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( initHashNode( aDataPlan ) != IDE_SUCCESS );

    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    aDataPlan->depTuple =
        & aTemplate->tmplate.rows[aCodePlan->depTupleRowID];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------

    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_HSDS_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_HSDS_INIT_DONE_TRUE;

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
qmnHSDS::initMtrNode( qcTemplate * aTemplate,
                      qmncHSDS   * aCodePlan,
                      qmndHSDS   * aDataPlan )
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
qmnHSDS::initHashNode( qmndHSDS   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Hashing Column의 시작 위치
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::initHashNode"
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

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnHSDS::initTempTable( qcTemplate * aTemplate,
                        qmncHSDS   * aCodePlan,
                        qmndHSDS   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt       sFlag          = 0;
    qmndHSDS * sCacheDataPlan = NULL;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    //-----------------------------
    // Flag 정보 초기화
    //-----------------------------

    sFlag = QMCD_HASH_TMP_DISTINCT_TRUE | QMCD_HASH_TMP_PRIMARY_TRUE;

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

    // BUG-31997: When using temporary tables by RID, RID refers to
    // the invalid row.
    /* QMNC_HSDS_IN_TOP_TRUE 일때는 temp table 를 완전이 구성하지 않고
     * 1 row 를 insert 하고 상위 플랜에서 insert 한 rid 를 그대로 저장하게 된다.
     * 하지만 hash temp table 은 index로 만들어져서 rid 가 변경될수 있다.
     * 따라서 SM 에게 rid 를 변경하지 않도록 요청을 해야 한다. */
    if ( (aCodePlan->flag & QMNC_HSDS_IN_TOP_MASK) == QMNC_HSDS_IN_TOP_TRUE )
    {
        if ( (aCodePlan->plan.flag & QMN_PLAN_TEMP_FIXED_RID_MASK)
             == QMN_PLAN_TEMP_FIXED_RID_TRUE )
        {
            sFlag &= ~QMCD_HASH_TMP_FIXED_RID_MASK;
            sFlag |= QMCD_HASH_TMP_FIXED_RID_TRUE;
        }
    }

    //-----------------------------
    // Temp Table 초기화
    //-----------------------------
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnHSDS::initTempTable::qmxAlloc:hashMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdHashTemp ),
                                                  (void **)&aDataPlan->hashMgr )
                  != IDE_SUCCESS );
        IDE_TEST( qmcHashTemp::init( aDataPlan->hashMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->hashNode,
                                     NULL,  // Aggregation Column없음
                                     aCodePlan->bucketCnt,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndHSDS *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnHSDS::initTempTable::qrcAlloc:hashMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdHashTemp ),
                                                               (void **)&aDataPlan->hashMgr )
                      != IDE_SUCCESS );

            IDE_TEST( qmcHashTemp::init( aDataPlan->hashMgr,
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         aDataPlan->hashNode,
                                         NULL,  // Aggregation Column없음
                                         aCodePlan->bucketCnt,
                                         sFlag )
                      != IDE_SUCCESS );
            sCacheDataPlan->hashMgr = aDataPlan->hashMgr;
        }
        else
        {
            aDataPlan->hashMgr = sCacheDataPlan->hashMgr;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnHSDS::checkDependency( qcTemplate * aTemplate,
                                 qmncHSDS   * aCodePlan,
                                 qmndHSDS   * aDataPlan,
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
qmnHSDS::setMtrRow( qcTemplate * aTemplate,
                    qmndHSDS   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Row의 구성
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::setMtrRow"
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
qmnHSDS::setTupleSet( qcTemplate * aTemplate,
                      qmndHSDS   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     검색된 저장 Row를 이용하여 Tuple Set을 복원한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHSDS::setTupleSet"
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
