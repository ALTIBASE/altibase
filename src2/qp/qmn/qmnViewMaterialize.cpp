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
 * $Id: qmnViewMaterialize.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     VMTR(View MaTeRialization) Node
 *
 *     관계형 모델에서 View에 대한 Materialization을 수행하는 Node이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnViewMaterialize.h>
#include <qmxResultCache.h>

IDE_RC qmnVMTR::checkDependency( qcTemplate * aTemplate,
                                 qmncVMTR   * aCodePlan,
                                 qmndVMTR   * aDataPlan,
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
qmnVMTR::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    VMTR 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVMTR * sCodePlan = (qmncVMTR *) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR *) (aTemplate->tmplate.data + aPlan->offset);

    idBool     sDependency;

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    // first initialization
    if ( (*sDataPlan->flag & QMND_VMTR_INIT_DONE_MASK)
         == QMND_VMTR_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }

    IDE_TEST( checkDependency( aTemplate,
                               sCodePlan,
                               sDataPlan,
                               &sDependency ) != IDE_SUCCESS );
    
    if ( sDependency == ID_TRUE )
    {
        //----------------------------------------
        // Temp Table 구축 전 초기화
        //----------------------------------------
        
        IDE_TEST( qmcSortTemp::clear( sDataPlan->sortMgr ) != IDE_SUCCESS );
   
        IDE_TEST( storeChild( aTemplate, sCodePlan, sDataPlan ) != IDE_SUCCESS );

        //----------------------------------------
        // Temp Table 구축 후 초기화
        //----------------------------------------

        sDataPlan->depValue = sDataPlan->depTuple->modify;
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
qmnVMTR::doIt( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnVMTR::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVMTR::padNull( qcTemplate * /* aTemplate */,
                  qmnPlan    * /* aPlan */)
{
/***********************************************************************
 *
 * Description :
 *    이 함수가 수행되면 안됨.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVMTR::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *   VMTR 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVMTR * sCodePlan = (qmncVMTR*) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndVMTR * sCacheDataPlan = NULL;
    idBool     sIsInit       = ID_FALSE;

    ULong sPageCount;
    SLong sRecordCount;

    ULong i;

    if ( ( *sDataPlan->flag & QMND_VMTR_PRINTED_MASK )
         == QMND_VMTR_PRINTED_FALSE )
    {
        // VMTR 노드는 여러 개의 상위 Plan Node를 가진다.
        // 따라서, 한 번만 출력되도록 한다.

        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

        if ( aMode == QMN_DISPLAY_ALL )
        {
            if ( (*sDataPlan->flag & QMND_VMTR_INIT_DONE_MASK)
                 == QMND_VMTR_INIT_DONE_TRUE )
            {
                sIsInit = ID_TRUE;
                IDE_TEST( qmcSortTemp::getDisplayInfo( sDataPlan->sortMgr,
                                                       & sPageCount,
                                                       & sRecordCount )
                          != IDE_SUCCESS );

                if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                     == QMN_PLAN_STORAGE_MEMORY )
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "MATERIALIZATION ( "
                            "ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT,
                            sDataPlan->viewRowSize,
                            sRecordCount );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "MATERIALIZATION ( "
                            "ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT,
                            sRecordCount );
                    }
                }
                else
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "MATERIALIZATION ( "
                            "ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: %"ID_UINT64_FMT,
                            sDataPlan->viewRowSize,
                            sRecordCount,
                            sPageCount );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE, DISK_PAGE_COUNT 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "MATERIALIZATION ( "
                            "ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: BLOCKED",
                            sRecordCount );
                    }
                }
            }
            else
            {
                iduVarStringAppend( aString,
                                    "MATERIALIZATION ( "
                                    "ITEM_SIZE: 0, ITEM_COUNT: 0" );

            }
        }
        else
        {
            iduVarStringAppend( aString,
                                "MATERIALIZATION ( "
                                "ITEM_SIZE: ??, ITEM_COUNT: ??" );

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
                sCacheDataPlan = (qmndVMTR *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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
                               sCodePlan->depTupleID,
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

        IDE_TEST( aPlan->left->printPlan( aTemplate,
                                          aPlan->left,
                                          aDepth + 1,
                                          aString,
                                          aMode ) != IDE_SUCCESS );

        *sDataPlan->flag &= ~QMND_VMTR_PRINTED_MASK;
        *sDataPlan->flag |= QMND_VMTR_PRINTED_TRUE;
    }
    else
    {
        // 이미 Plan정보가 출력된 상태임
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVMTR::getCursorInfo( qcTemplate       * aTemplate,
                        qmnPlan          * aPlan,
                        void            ** aTableHandle,
                        void            ** aIndexHandle,
                        qmcdMemSortTemp ** aMemSortTemp,
                        qmdMtrNode      ** aMemSortRecord )
{
/***********************************************************************
 *
 * Description :
 *    VSCN에서만 호출할 수 있으며, Temp Table에 대한 접근을
 *    위한 정보를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::getCursorInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncVMTR * sCodePlan = (qmncVMTR*) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( qmcSortTemp::getCursorInfo( sDataPlan->sortMgr,
                                          aTableHandle,
                                          aIndexHandle,
                                          aMemSortTemp,
                                          aMemSortRecord )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnVMTR::getNullRowDisk( qcTemplate       * aTemplate,
                                qmnPlan          * aPlan,
                                void             * aRow,
                                scGRID           * aRowRid )
{
/***********************************************************************
 *
 * Description :
 *    VSCN에서만 호출할 수 있으며, Temp Table로부터 Null Row를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->mtrRow = sDataPlan->mtrNode->dstTuple->row;

    IDE_TEST( qmcSortTemp::getNullRow( sDataPlan->sortMgr,
                                       & sDataPlan->mtrRow )
              != IDE_SUCCESS );

    IDE_DASSERT( sDataPlan->mtrRow != NULL );

    idlOS::memcpy( aRow,
                   sDataPlan->mtrRow,
                   sDataPlan->sortMgr->nullRowSize );
    idlOS::memcpy( aRowRid,
                   & sDataPlan->mtrNode->dstTuple->rid,
                   ID_SIZEOF(scGRID) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/**
 * PROJ-2462 ResultCache
 *
 * Memory Temp의 NullRow같은 경우 그대로 전달하면된다. 
 */
IDE_RC qmnVMTR::getNullRowMemory( qcTemplate *  aTemplate,
                                  qmnPlan    *  aPlan,
                                  void       ** aRow )
{
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->mtrRow = sDataPlan->mtrNode->dstTuple->row;

    IDE_TEST( qmcSortTemp::getNullRow( sDataPlan->sortMgr,
                                       & sDataPlan->mtrRow )
              != IDE_SUCCESS );

    IDE_DASSERT( sDataPlan->mtrRow != NULL );

    *aRow = sDataPlan->mtrRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnVMTR::getNullRowSize( qcTemplate       * aTemplate,
                         qmnPlan          * aPlan,
                         UInt             * aRowSize )
{
/***********************************************************************
 *
 * Description :
 *    VSCN에서만 호출할 수 있으며, Row Size를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::getNullRowSize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncVMTR * sCodePlan = (qmncVMTR*) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    *aRowSize = sDataPlan->sortMgr->nullRowSize;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnVMTR::getTuple( qcTemplate       * aTemplate,
                   qmnPlan          * aPlan,
                   mtcTuple        ** aTuple )
{
/***********************************************************************
 *
 * Description :
 *    VSCN에서만 호출할 수 있으며, Tuple을 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::getRowSize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncVMTR * sCodePlan = (qmncVMTR*) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    *aTuple = sDataPlan->mtrNode->dstTuple;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qmnVMTR::getMtrNode( qcTemplate       * aTemplate,
                            qmnPlan          * aPlan,
                            qmdMtrNode      ** aNode )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *    SREC에서만 호출할 수 있으며, mtrNode를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // qmncVMTR * sCodePlan = (qmncVMTR*) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    *aNode = sDataPlan->mtrNode;

    return IDE_SUCCESS;
    
}

IDE_RC
qmnVMTR::firstInit( qcTemplate * aTemplate,
                    qmncVMTR   * aCodePlan,
                    qmndVMTR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    VMTR node의 Data 영역의 멤버에 대한 초기화를 수행하고,
 *    Child의 수행 결과를 저장함.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndVMTR * sCacheDataPlan = NULL;

    //---------------------------------
    // VMTR 고유 정보의 초기화
    //---------------------------------

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndVMTR *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
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

    // 저장 Column의 초기화
    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );
    
    // Temp Table의 초기화
    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // View Row의 크기 초기화
    aDataPlan->viewRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    // View Row의 저장 공간 초기화
    //    - Memory Temp Table을 사용할 경우 의미 없음
    aDataPlan->mtrRow = aDataPlan->mtrNode->dstTuple->row;

    aDataPlan->myTuple  = aDataPlan->mtrNode->dstTuple;
    aDataPlan->depTuple = &aTemplate->tmplate.rows[ aCodePlan->depTupleID ];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;
    
    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_VMTR_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_VMTR_INIT_DONE_TRUE;

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
qmnVMTR::initMtrNode( qcTemplate * aTemplate,
                      qmncVMTR   * aCodePlan,
                      qmndVMTR   * aDataPlan )
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
    
    // Store And Search와 동일한 저장 방식을 취한다.
    // PROJ-2469 Optimize View Materialization
    // 상위 Plan에서 사용하지 않는 MtrNode Type - QMC_MTR_TYPE_USELESS_COLUMN 일 수 있다.
    IDE_DASSERT( ( ( aCodePlan->myNode->flag & QMC_MTR_TYPE_MASK )
                   == QMC_MTR_TYPE_COPY_VALUE ) ||
                 ( ( aCodePlan->myNode->flag & QMC_MTR_TYPE_MASK )
                   == QMC_MTR_TYPE_USELESS_COLUMN )
                 );    

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
                                0 ) // Base Table 존재하지 않음
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
qmnVMTR::initTempTable( qcTemplate * aTemplate,
                        qmncVMTR   * aCodePlan,
                        qmndVMTR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt       sFlag          = 0;
    qmndVMTR * sCacheDataPlan = NULL;

    //---------------------------------
    // 저장 관리를 위한 정보의 초기화
    //---------------------------------

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sFlag = QMCD_SORT_TMP_STORAGE_MEMORY;
    }
    else
    {
        sFlag = QMCD_SORT_TMP_STORAGE_DISK;
    }

    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnVMTR::initTempTable::qmxAlloc:sortMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                  (void **)&aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     NULL,
                                     0,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndVMTR *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnVMTR::initTempTable::qrcAlloc:sortMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                               (void **)&aDataPlan->sortMgr )
                      != IDE_SUCCESS );

            IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         NULL,
                                         0,
                                         sFlag )
                      != IDE_SUCCESS );
            sCacheDataPlan->sortMgr = aDataPlan->sortMgr;
        }
        else
        {
            aDataPlan->sortMgr = sCacheDataPlan->sortMgr;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnVMTR::storeChild( qcTemplate * aTemplate,
                     qmncVMTR   * aCodePlan,
                     qmndVMTR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Child Plan의 결과를 저장함.
 *
 * Implementation :
 *    Child를 반복적으로 수행하여 그 결과를 저장함.
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::storeChild"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    //---------------------------------
    // Child Plan의 초기화
    //---------------------------------

    IDE_TEST( aCodePlan->plan.left->init( aTemplate,
                                          aCodePlan->plan.left )
              != IDE_SUCCESS);

    //---------------------------------
    // Child Plan의 결과를 저장
    //---------------------------------

    // doIt left child
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        // 저장 공간의 할당
        aDataPlan->mtrRow = aDataPlan->mtrNode->dstTuple->row;
        IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                      & aDataPlan->mtrRow )
                  != IDE_SUCCESS );

        // Record 구성
        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        // Temp Table에 삽입
        IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                       aDataPlan->mtrRow )
                  != IDE_SUCCESS );

        // Left Child의 수행
        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );

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

#undef IDE_FN
}


IDE_RC
qmnVMTR::setMtrRow( qcTemplate * aTemplate,
                    qmndVMTR   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Row를 구성함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVMTR::setMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setMtr( aTemplate,
                                      sNode,
                                      aDataPlan->mtrRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnVMTR::resetDependency( qcTemplate  * aTemplate,
                                 qmnPlan     * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *    VSCN에서만 호출할 수 있으며, dependency를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // qmncVMTR * sCodePlan = (qmncVMTR*) aPlan;
    qmndVMTR * sDataPlan =
        (qmndVMTR*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    return IDE_SUCCESS;
}

