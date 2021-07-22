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
 * $Id: qmnSetDifference.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     SDIF(Set DIFference) Node
 *
 *     관계형 모델에서 hash-based set difference 연산을
 *     수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnSetDifference.h>
#include <qmxResultCache.h>

IDE_RC
qmnSDIF::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SDIF 노드의 초기화
 *
 * Implementation :
 *    Left Dependent가 발생한 경우 저장된 정보를 모두 버려야 함.
 *    Right Dependent가 발생한 경우 저장된 정보는 그대로 사용할 수
 *    있으며, Hit Flag만 clear하여 right를 재수행할 수 있다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSDIF * sCodePlan = (qmncSDIF *) aPlan;
    qmndSDIF * sDataPlan =
        (qmndSDIF *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    idBool sLeftDependency;
    idBool sRightDependency;
    idBool sIsSkip = ID_FALSE;

    sDataPlan->doIt = qmnSDIF::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_SDIF_INIT_DONE_MASK)
         == QMND_SDIF_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( checkLeftDependency( sDataPlan,
                                   &sLeftDependency ) != IDE_SUCCESS );

    if ( sLeftDependency == ID_TRUE )
    {
        /* PROJ-2462 Result Cache */
        if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
        {
            sIsSkip = ID_FALSE;
        }
        else
        {
            sDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[sCodePlan->planID];
            if ( ( ( *sDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
                   == QMX_RESULT_CACHE_STORED_TRUE ) &&
                 ( sDataPlan->leftDepValue == QMN_PLAN_DEFAULT_DEPENDENCY_VALUE ) )
            {
                sIsSkip = ID_TRUE;
            }
            else
            {
                sIsSkip = ID_FALSE;
            }
        }

        if ( sIsSkip == ID_FALSE )
        {
            //----------------------------------------
            // Left Dependent Row가 변경된 경우
            // 저장 Row를 재구성한다.
            //----------------------------------------

            // 1. Temp Table Clear
            IDE_TEST( qmcHashTemp::clear( sDataPlan->hashMgr )
                      != IDE_SUCCESS );

            // 2. Left 수행하여 저장
            IDE_TEST( storeLeft( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );

            // 3. Right 수행하여 differenced row 결정
            IDE_TEST( setDifferencedRows( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );

            // PROJ-2462 Result Cache
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
            /* Nothing to do */
        }

    }
    else
    {
        IDE_TEST( checkRightDependency( sDataPlan,
                                        & sRightDependency ) != IDE_SUCCESS );

        if ( sRightDependency == ID_TRUE )
        {
            //----------------------------------------
            // Right Dependent Row만 변경된 경우
            // Differenced Row만 재구성한다.
            //----------------------------------------

            // 1. Hit Flag 제거
            // 2. Right를 수행하여 differenced row 결정

            IDE_TEST( qmcHashTemp::clearHitFlag( sDataPlan->hashMgr )
                      != IDE_SUCCESS );

            IDE_TEST( setDifferencedRows( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );
        }
        else
        {
            // nothing to do
        }
    }

    sDataPlan->doIt = qmnSDIF::doItFirst;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnSDIF::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SDIF 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *    SDIF의 상위 노드는 항상 VIEW이다.
 *    따라서, 결과가 존재할 경우 VIEW에서 처리할 수 있도록
 *    그 결과를 Stack에 설정한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSDIF * sCodePlan = (qmncSDIF *) aPlan;
    qmndSDIF * sDataPlan =
        (qmndSDIF*) (aTemplate->tmplate.data + aPlan->offset);

    qmdMtrNode * sNode;
    void       * sRow;

    mtcStack   * sStack;
    SInt         sRemain;

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // 상위 VIEW 노드를 위한 Stack 설정
    //-----------------------------------

    sStack  = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        sRow = sDataPlan->plan.myTuple->row;

        for ( sNode = sDataPlan->mtrNode;
              sNode != NULL;
              sNode = sNode->next,
                  aTemplate->tmplate.stack++,
                  aTemplate->tmplate.stackRemain-- )
        {
            IDE_TEST_RAISE(aTemplate->tmplate.stackRemain < 1,
                           ERR_STACK_OVERFLOW);

            aTemplate->tmplate.stack->value =
                (void*)( (UChar*)sRow + sNode->dstColumn->column.offset);
            aTemplate->tmplate.stack->column = sNode->dstColumn;
        }

        aTemplate->tmplate.stack = sStack;
        aTemplate->tmplate.stackRemain = sRemain;

    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

        aTemplate->tmplate.stack = sStack;
        aTemplate->tmplate.stackRemain = sRemain;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnSDIF::padNull( qcTemplate * /* aTemplate */,
                  qmnPlan    * /* aPlan */)
{
/***********************************************************************
 *
 * Description :
 *    호출되어서는 안됨.
 *    상위 Node는 반드시 VIEW노드이며,
 *    View는 자신의 Null Row만을 설정하기 때문이다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSDIF * sCodePlan = (qmncSDIF*) aPlan;
    qmndSDIF * sDataPlan =
        (qmndSDIF*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndSDIF * sCacheDataPlan = NULL;
    idBool     sIsInit       = ID_FALSE;

    SLong sRecordCnt;
    ULong sPageCnt;
    UInt  sBucketCnt;

    ULong i;

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

        if ( (*sDataPlan->flag & QMND_SDIF_INIT_DONE_MASK)
             == QMND_SDIF_INIT_DONE_TRUE )
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
                        "SET-DIFFERENCE ( "
                        "ITEM_SIZE: %"ID_UINT32_FMT", "
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
                        "SET-DIFFERENCE ( "
                        "ITEM_SIZE: BLOCKED, "
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
                        "SET-DIFFERENCE ( "
                        "ITEM_SIZE: %"ID_UINT32_FMT", "
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
                        "SET-DIFFERENCE ( "
                        "ITEM_SIZE: BLOCKED, "
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
                                      "SET-DIFFERENCE ( "
                                      "ITEM_SIZE: 0, "
                                      "ITEM_COUNT: 0, "
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
                                  "SET-DIFFERENCE ( "
                                  "ITEM_SIZE: ??, "
                                  "ITEM_COUNT: ??, "
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
        /* PROJ-2462 Result Cache */
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndSDIF *)(aTemplate->resultCache.data + sCodePlan->plan.offset);
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
                           sCodePlan->leftDepTupleRowID,
                           sCodePlan->rightDepTupleRowID );
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

    IDE_TEST( aPlan->right->printPlan( aTemplate,
                                       aPlan->right,
                                       aDepth + 1,
                                       aString,
                                       aMode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
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

#define IDE_FN "qmnSDIF::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    최초 수행 함수
 *
 * Implementation :
 *    Hash Temp Table로부터
 *    Differenced Row(Hit 되지 않은 Row)를 얻어온다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSDIF * sCodePlan = (qmncSDIF *) aPlan;
    qmndSDIF * sDataPlan =
        (qmndSDIF *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    //--------------------------------
    // Differenced Row 추출
    //--------------------------------

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getFirstNonHit( sDataPlan->hashMgr,
                                           & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    if ( sSearchRow != NULL )
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;

        sDataPlan->plan.myTuple->modify++;

        sDataPlan->doIt = qmnSDIF::doItNext;
    }
    else
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 수행 함수
 *
 * Implementation :
 *    Hash Temp Table로부터
 *    Differenced Row(Hit 되지 않은 Row)를 얻어온다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSDIF * sCodePlan = (qmncSDIF *) aPlan;
    qmndSDIF * sDataPlan =
        (qmndSDIF *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    //--------------------------------
    // Differenced Row 추출
    //--------------------------------

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getNextNonHit( sDataPlan->hashMgr,
                                          & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    if ( sSearchRow != NULL )
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;

        sDataPlan->plan.myTuple->modify++;
    }
    else
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_NONE;

        sDataPlan->doIt = qmnSDIF::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::firstInit( qcTemplate * aTemplate,
                    qmncSDIF   * aCodePlan,
                    qmndSDIF   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Data 영역에 대한 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndSDIF * sCacheDataPlan = NULL;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    //---------------------------------
    // SDIF 고유 정보의 초기화
    //---------------------------------

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndSDIF *)(aTemplate->resultCache.data + aCodePlan->plan.offset);
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

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // To Fix PR-8060
    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    aDataPlan->leftDepTuple =
        & aTemplate->tmplate.rows[aCodePlan->leftDepTupleRowID];
    aDataPlan->leftDepValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    aDataPlan->rightDepTuple =
        & aTemplate->tmplate.rows[aCodePlan->rightDepTupleRowID];
    aDataPlan->rightDepValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------

    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_SDIF_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_SDIF_INIT_DONE_TRUE;

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
qmnSDIF::initMtrNode( qcTemplate * aTemplate,
                      qmncSDIF   * aCodePlan,
                      qmndSDIF   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 관리를 위한 노드를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt sHeaderSize = 0;

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

    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode, aDataPlan->mtrNode )
              != IDE_SUCCESS );

    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                0 ) != IDE_SUCCESS );

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
qmnSDIF::initTempTable( qcTemplate * aTemplate,
                        qmncSDIF   * aCodePlan,
                        qmndSDIF   * aDataPlan )
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
    qmndSDIF  * sCacheDataPlan = NULL;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    // 모든 Column이 Hashing 대상이다.
    IDE_DASSERT( (aDataPlan->mtrNode->flag & QMC_MTR_HASH_NEED_MASK )
                 == QMC_MTR_HASH_NEED_TRUE );

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

    //-----------------------------
    // Temp Table 초기화
    //-----------------------------
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnSDIF::initTempTable::qmxAlloc:hashMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdHashTemp ),
                                                  (void **)&aDataPlan->hashMgr )
                  != IDE_SUCCESS );
        IDE_TEST( qmcHashTemp::init( aDataPlan->hashMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->mtrNode,
                                     NULL,  // Aggregation Column없음
                                     aCodePlan->bucketCnt,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndSDIF *)(aTemplate->resultCache.data +
                                     aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnSDIF::initTempTable::qrcAlloc:hashMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdHashTemp ),
                                                               (void **)&aDataPlan->hashMgr )
                      != IDE_SUCCESS );

            IDE_TEST( qmcHashTemp::init( aDataPlan->hashMgr,
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         aDataPlan->mtrNode,
                                         NULL,  // Aggregation Column없음
                                         aCodePlan->bucketCnt,
                                         sFlag )
                      != IDE_SUCCESS );
            sCacheDataPlan->hashMgr = aDataPlan->hashMgr;
        }
        else
        {
            aDataPlan->hashMgr = sCacheDataPlan->hashMgr;
            aDataPlan->temporaryRightRow = sCacheDataPlan->temporaryRightRow;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::checkLeftDependency( qmndSDIF   * aDataPlan,
                              idBool     * aDependent )
{
/***********************************************************************
 *
 * Description :
 *    Left Dependent Tuple의 변경 여부 검사
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( aDataPlan->leftDepValue != aDataPlan->leftDepTuple->modify )
    {
        *aDependent = ID_TRUE;
    }
    else
    {
        *aDependent = ID_FALSE;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmnSDIF::checkRightDependency( qmndSDIF   * aDataPlan,
                               idBool     * aDependent )
{
/***********************************************************************
 *
 * Description :
 *     Right Dependent Tuple의 변경 여부 검사
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN ""
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( aDataPlan->rightDepValue != aDataPlan->rightDepTuple->modify )
    {
        *aDependent = ID_TRUE;
    }
    else
    {
        *aDependent = ID_FALSE;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC
qmnSDIF::storeLeft( qcTemplate * aTemplate,
                    qmncSDIF   * aCodePlan,
                    qmndSDIF   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Left를 수행하여 distinct hashing으로 저장
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::storeLeft"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;
    idBool     sInserted;
    qmndSDIF * sCacheDataPlan = NULL;

    //---------------------------------------
    // Left Child 수행
    //---------------------------------------

    IDE_TEST( aCodePlan->plan.left->init( aTemplate,
                                          aCodePlan->plan.left )
              != IDE_SUCCESS);

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    //---------------------------------------
    // 반복 수행하여 Temp Table 구성
    //---------------------------------------

    sInserted = ID_TRUE;

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //---------------------------------------
        // 1.  저장 Row를 위한 공간 할당
        //---------------------------------------

        if ( sInserted == ID_TRUE )
        {
            IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                          & aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {
            // 삽입이 실패한 경우로 이미 할당받은 공간을 사용한다.
            // 따라서, 별도로 공간을 할당 받을 필요가 없다.
        }

        //---------------------------------------
        // 2.  저장 Row의 구성
        //---------------------------------------

        IDE_TEST( setMtrRow( aTemplate,
                             aDataPlan ) != IDE_SUCCESS );

        //---------------------------------------
        // 3.  저장 Row의 삽입
        //---------------------------------------

        IDE_TEST( qmcHashTemp::addDistRow( aDataPlan->hashMgr,
                                           & aDataPlan->plan.myTuple->row,
                                           & sInserted )
                  != IDE_SUCCESS );

        //---------------------------------------
        // 4.  Left Child 수행
        //---------------------------------------

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    //---------------------------------------
    // Right 처리를 위한 메모리 공간 확보
    //---------------------------------------

    if ( sInserted == ID_TRUE )
    {
        IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                      & aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }
    else
    {
        // 이미 공간이 남아 있음
    }

    aDataPlan->leftDepValue = aDataPlan->leftDepTuple->modify;
    // jhseong, PR-10107
    aDataPlan->temporaryRightRow = aDataPlan->plan.myTuple->row;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndSDIF *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->temporaryRightRow = aDataPlan->temporaryRightRow;
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
qmnSDIF::setRightChildMtrRow( qcTemplate * aTemplate,
                              qmndSDIF   * aDataPlan,
                              idBool     * aIsSetMtrRow )
{
/***********************************************************************
 *
 * Description :
 *    Right Child의 결과를 저장 Row에 구성
 *
 * Implementation :
 *    하위 노드가 반드시 PROJ 노드이기 때문에 Stack에 쌓인 결과를
 *    사용하여 저장 Row를 구성한다.
 *
 * BUG-24190
 * select i1(varchar(30)) from t1 minus select i1(varchar(250)) from t2;
 * 수행시 서버 비정상종료.
 *
 *  right child의 actualsize가 큰 경우 skip 한다. 
 *
 *                [ SET-DIFFERENCE ] ( 컬럼사이즈 : 30 )
 *                         |
 *           -----------------------------
 *           |                            |
 *         [ T1 : varchar(30) ]    [ T2 : varchar(250) ]
 *
 ***********************************************************************/

    qmdMtrNode * sNode;

    mtcStack   * sStack;
    SInt         sRemain;

    UInt         sActualSize;

    sStack = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    *aIsSetMtrRow = ID_TRUE;    

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next,
              aTemplate->tmplate.stack++,
              aTemplate->tmplate.stackRemain-- )
    {
        IDE_TEST_RAISE(aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW);

        sActualSize = aTemplate->tmplate.stack->column->module->actualSize(
            aTemplate->tmplate.stack->column,
            aTemplate->tmplate.stack->value );

        if ( sActualSize > aDataPlan->plan.myTuple->columns[sNode->dstNode->node.column].column.size )
        {
            *aIsSetMtrRow = ID_FALSE;
            break;            
        }
        else
        {
            // Nothing To Do 
        }
    }

    aTemplate->tmplate.stack = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    if( *aIsSetMtrRow == ID_TRUE )
    {
        // To Fix PR-8060
        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do 
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

        // fix BUG-33678
        aTemplate->tmplate.stack = sStack;
        aTemplate->tmplate.stackRemain = sRemain;
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC
qmnSDIF::setMtrRow( qcTemplate * aTemplate,
                    qmndSDIF   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Child의 결과를 저장 Row에 구성
 *
 * Implementation :
 *    하위 노드가 반드시 PROJ 노드이기 때문에 Stack에 쌓인 결과를
 *    사용하여 저장 Row를 구성한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::setMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;
    void       * sRow;

    mtcStack   * sStack;
    SInt         sRemain;

    sStack = aTemplate->tmplate.stack;
    sRemain = aTemplate->tmplate.stackRemain;

    sRow = aDataPlan->plan.myTuple->row;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next,
              aTemplate->tmplate.stack++,
              aTemplate->tmplate.stackRemain-- )
    {
        IDE_TEST_RAISE(aTemplate->tmplate.stackRemain < 1, ERR_STACK_OVERFLOW);
        idlOS::memcpy(
            (SChar*) sRow + sNode->dstColumn->column.offset,
            (SChar*) aTemplate->tmplate.stack->value,
            aTemplate->tmplate.stack->column->module->actualSize(
                aTemplate->tmplate.stack->column,
                aTemplate->tmplate.stack->value ) );
    }

    aTemplate->tmplate.stack = sStack;
    aTemplate->tmplate.stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

        // fix BUG-33678
        aTemplate->tmplate.stack = sStack;
        aTemplate->tmplate.stackRemain = sRemain;
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSDIF::setDifferencedRows( qcTemplate * aTemplate,
                             qmncSDIF   * aCodePlan,
                             qmndSDIF   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Hash Temp Table을 이용하여 diffrenced row를 생성
 *
 * Implementation :
 *    Right를 위한 저장 Row를 구성하고,
 *    Hash Temp Table을 이용하여 diffrenced row를 생성한다.
 *
 *    Right Row를 구성하고 Hash Temp Table에서 동일한 Row를 검색하여
 *    결과로 선택되지 않도록 Hit을 설정한다.
 *
 *    즉, Intersected Row들을 모두 결정한 후에
 *    수행 과정에서는 Hit되지 않은 Row만을 선택하게 된다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSDIF::setDifferencedRows"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;
    void     * sOrgRow;
    void     * sSearchRow;
    idBool     sIsSetMtrRow;        

    IDE_TEST( aCodePlan->plan.right->init( aTemplate,
                                           aCodePlan->plan.right )
              != IDE_SUCCESS );

    //------------------------------
    // Right를 위한 저장 Row 구성
    //------------------------------

    IDE_TEST( aCodePlan->plan.right->doIt( aTemplate,
                                           aCodePlan->plan.right,
                                           & sFlag ) != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        //------------------------------
        // Right를 위한 저장 Row 구성
        //------------------------------

        // jhseong, PR-10107
        aDataPlan->plan.myTuple->row = aDataPlan->temporaryRightRow;

        // To Fix PR-8060
        // PR-24190
        // select i1(varchar(30)) from t1 minus select i1(varchar(250)) from t2;
        // 수행시 서버 비정상종료        
        IDE_TEST( setRightChildMtrRow( aTemplate, aDataPlan, &sIsSetMtrRow )
                  != IDE_SUCCESS );

        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
        
        if( sIsSetMtrRow == ID_TRUE )
        {
            //------------------------------
            // Hash Temp Table을 이용한 intersected row 검색
            //------------------------------
            
            IDE_TEST( qmcHashTemp::getSameRowAndNonHit( aDataPlan->hashMgr,
                                                        aDataPlan->plan.myTuple->row,
                                                        & sSearchRow )
                      != IDE_SUCCESS );
        }
        else
        {
            sSearchRow = NULL;            
        }
            
        aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

        if ( sSearchRow != NULL )
        {
            //------------------------------
            // 선택되지 않도록 Hit Flag Setting
            //------------------------------

            IDE_TEST( qmcHashTemp::setHitFlag( aDataPlan->hashMgr )
                      != IDE_SUCCESS );
        }
        else
        {
            // To Fix PR-8060
            // 아무런 처리를 할 필요가 없다.

            // Nothing To Do
        }

        IDE_TEST( aCodePlan->plan.right->doIt( aTemplate,
                                               aCodePlan->plan.right,
                                               & sFlag ) != IDE_SUCCESS );
    }

    aDataPlan->rightDepValue = aDataPlan->rightDepTuple->modify;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
