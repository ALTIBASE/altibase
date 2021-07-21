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
 * $Id $
 *
 * Description :
 *     CMTR(Connect By MaTeRialization) Node
 *
 *     관계형 모델에서 Hierarchy를 위해 Materialization을 수행하는 Node이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnConnectByMTR.h>
#include <qmxResultCache.h>

IDE_RC qmnCMTR::init( qcTemplate * aTemplate, qmnPlan * aPlan )
{
    qmncCMTR * sCodePlan = (qmncCMTR *) aPlan;
    qmndCMTR * sDataPlan = (qmndCMTR *) (aTemplate->tmplate.data +
                                         aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    /* CMTR은 Memory Sort Temp 만 가능하다 */
    IDE_TEST_RAISE( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                    != QMN_PLAN_STORAGE_MEMORY,
                    ERR_STORAGE_TYPE );

    /* first initialization */
    if ( (*sDataPlan->flag & QMND_CMTR_INIT_DONE_MASK)
         == QMND_CMTR_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STORAGE_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnCMTR::init",
                                  "only memory sort temp is available" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnCMTR::doIt( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* aFlag */ )
{
    return IDE_FAILURE;
}

IDE_RC qmnCMTR::padNull( qcTemplate * /* aTemplate */,
                         qmnPlan    * /* aPlan */)
{
    return IDE_FAILURE;
}

IDE_RC qmnCMTR::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
    qmncCMTR * sCodePlan = (qmncCMTR*) aPlan;
    qmndCMTR * sDataPlan =
        (qmndCMTR*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndCMTR * sCacheDataPlan = NULL;

    ULong sPageCount;
    SLong sRecordCount;
    idBool sIsInit = ID_FALSE;
    ULong i;

    if ( ( *sDataPlan->flag & QMND_CMTR_PRINTED_MASK )
         == QMND_CMTR_PRINTED_FALSE )
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
            if ( (*sDataPlan->flag & QMND_CMTR_INIT_DONE_MASK)
                 == QMND_CMTR_INIT_DONE_TRUE )
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
                            sDataPlan->rowSize,
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
                    /* Nothing to do */
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
            /* PROJ-2462 Result Cache */
            if ( ( sCodePlan->componentInfo != NULL ) &&
                 ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
                   == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
                 ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
                   == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
            {
                sCacheDataPlan = (qmndCMTR *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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
                               ID_USHORT_MAX,
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
                                          aMode )
                  != IDE_SUCCESS );

        *sDataPlan->flag &= ~QMND_CMTR_PRINTED_MASK;
        *sDataPlan->flag |= QMND_CMTR_PRINTED_TRUE;
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

IDE_RC qmnCMTR::getNullRow( qcTemplate       * aTemplate,
                            qmnPlan          * aPlan,
                            void             * aRow )
{
    qmndCMTR * sDataPlan =
        (qmndCMTR*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->mtrRow = sDataPlan->mtrNode->dstTuple->row;

    IDE_TEST( qmcSortTemp::getNullRow( sDataPlan->sortMgr,
                                       & sDataPlan->mtrRow )
              != IDE_SUCCESS );

    IDE_DASSERT( sDataPlan->mtrRow != NULL );

    idlOS::memcpy( aRow,
                   sDataPlan->mtrRow,
                   sDataPlan->sortMgr->nullRowSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnCMTR::getNullRowSize( qcTemplate       * aTemplate,
                                qmnPlan          * aPlan,
                                UInt             * aRowSize )
{
    qmndCMTR * sDataPlan =
        (qmndCMTR*) (aTemplate->tmplate.data + aPlan->offset);

    *aRowSize = sDataPlan->sortMgr->nullRowSize;

    return IDE_SUCCESS;
}

IDE_RC qmnCMTR::getTuple( qcTemplate       * aTemplate,
                          qmnPlan          * aPlan,
                          mtcTuple        ** aTuple )
{
    qmndCMTR * sDataPlan =
        (qmndCMTR*) (aTemplate->tmplate.data + aPlan->offset);

    *aTuple = sDataPlan->mtrNode->dstTuple;

    return IDE_SUCCESS;
}

IDE_RC qmnCMTR::setPriorNode( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qtcNode    * aPrior )
{
    qmdMtrNode  * sNode = NULL;
    qtcNode     * sTmp  = NULL;
    qmndCMTR    * sDataPlan =
                    (qmndCMTR*) (aTemplate->tmplate.data + aPlan->offset);
    UInt          sSize = 0;
    UInt          i     = 0;

    sDataPlan->priorCount = 0;
    for ( sTmp = aPrior; sTmp != NULL; sTmp = (qtcNode *)sTmp->node.next )
    {
        sDataPlan->priorCount++;
    }

    if ( sDataPlan->priorCount > 0 )
    {
        sSize = ID_SIZEOF( qmdMtrNode * ) * sDataPlan->priorCount;

        IDE_TEST( aTemplate->stmt->qmxMem->cralloc( sSize,
                                                    (void **)&sDataPlan->priorNode )
                  != IDE_SUCCESS );
        i = 0;

        for ( sTmp = aPrior; sTmp != NULL; sTmp = (qtcNode *)sTmp->node.next )
        {
            for ( sNode = sDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
            {
                if ( sNode->dstNode->node.column == sTmp->node.column )
                {
                    sDataPlan->priorNode[i] = sNode;
                    i++;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
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

IDE_RC qmnCMTR::comparePriorNode( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan,
                                  void       * aPriorRow,
                                  void       * aSearchRow,
                                  idBool     * aResult )
{
    SInt           sResult     = -1;
    mtdValueInfo   sValue1;
    mtdValueInfo   sValue2;
    qmdMtrNode   * sNode;
    qmndCMTR     * sDataPlan   =
            (qmndCMTR*) (aTemplate->tmplate.data + aPlan->offset);
    UInt           i;

    for ( i = 0; i < sDataPlan->priorCount; i ++ )
    {
        sNode = sDataPlan->priorNode[i];
        
        sValue1.value  = sNode->func.getRow( sNode, aPriorRow );
        sValue1.column = (const mtcColumn *)sNode->func.compareColumn;
        sValue1.flag   = MTD_OFFSET_USE;

        sValue2.value  = sNode->func.getRow( sNode, aSearchRow );
        sValue2.column = (const mtcColumn *)sNode->func.compareColumn;
        sValue2.flag   = MTD_OFFSET_USE;

        IDE_TEST_RAISE( sNode->func.compare == & mtd::compareNA,
                        ERR_INTERNAL );

        sResult = sNode->func.compare( &sValue1,
                                       &sValue2 );

        if ( sResult != 0 )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sResult == 0 )
    {
        *aResult = ID_TRUE;
    }
    else
    {
        *aResult = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnCMTR::comparePriorNode",
                                  "compare func is null" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnCMTR::firstInit( qcTemplate * aTemplate,
                           qmncCMTR   * aCodePlan,
                           qmndCMTR   * aDataPlan )
{
    idBool     sDep = ID_FALSE;
    qmndCMTR * sCacheDataPlan = NULL;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndCMTR *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
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

    /* 저장 Column의 초기화 */
    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );
    
    /* Temp Table의 초기화 */
    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    /* View Row의 크기 초기화 */
    aDataPlan->rowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    /* View Row의 저장 공간 초기화 Memory Temp Table을 사용할 경우 의미 없음 */
    aDataPlan->mtrRow = aDataPlan->mtrNode->dstTuple->row;
    aDataPlan->priorCount = 0;

    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        sDep = ID_TRUE;
    }
    else
    {
        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
             == QMX_RESULT_CACHE_STORED_TRUE )
        {
            sDep = ID_FALSE;
        }
        else
        {
            sDep = ID_TRUE;
        }
    }

    if ( sDep == ID_TRUE )
    {
        /* Child의 결과를 저장함 */
        IDE_TEST( storeChild( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 초기화 완료를 표기 */
    *aDataPlan->flag &= ~QMND_CMTR_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_CMTR_INIT_DONE_TRUE;

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


IDE_RC qmnCMTR::initMtrNode( qcTemplate * aTemplate,
                             qmncCMTR   * aCodePlan,
                             qmndCMTR   * aDataPlan )
{
    UInt         sHeaderSize;

    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );
    
    /* Store And Search와 동일한 저장 방식을 취한다. */
    /* PROJ-2469 View Materialization Optimize */
    /* 상위 Plan에서 사용하지 않는 Node Type - QMC_MTR_TYPE_USELESS_COLUMN 일 수 있다. */
    IDE_DASSERT( ( ( aCodePlan->myNode->flag & QMC_MTR_TYPE_MASK )
                   == QMC_MTR_TYPE_COPY_VALUE ) ||
                 ( ( aCodePlan->myNode->flag & QMC_MTR_TYPE_MASK )
                   == QMC_MTR_TYPE_USELESS_COLUMN )
                 );

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        aDataPlan->mtrNode = ( qmdMtrNode * )( aTemplate->resultCache.data +
                                               aCodePlan->mtrNodeOffset );
    }
    else
    {
        aDataPlan->mtrNode =
            (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);
    }

    /*
     * 저장 Column의 초기화
     * 1.  저장 Column의 연결 정보 생성
     * 2.  저장 Column의 초기화
     * 3.  저장 Column의 offset을 재조정
     * 4.  Row Size의 계산
     *     - Disk Temp Table의 경우 Row를 위한 Memory도 할당받음.
     */

    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode )
              != IDE_SUCCESS );

    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                0 ) // Base Table 존재하지 않음
              != IDE_SUCCESS );

    sHeaderSize = QMC_MEMSORT_TEMPHEADER_SIZE;
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  sHeaderSize )
              != IDE_SUCCESS );

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               &aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    aDataPlan->priorNode = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnCMTR::initTempTable( qcTemplate * aTemplate,
                               qmncCMTR   * aCodePlan,
                               qmndCMTR   * aDataPlan )
{
    UInt       sFlag = 0;
    qmndCMTR * sCacheDataPlan = NULL;

    sFlag = QMCD_SORT_TMP_STORAGE_MEMORY;

    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnCMTR::initTempTable::qmxAlloc::sortMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                  (void **)&aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        /* Temp Table의 초기화 */
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
        sCacheDataPlan = (qmndCMTR *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnCMTR::initTempTable::qrcAlloc::sortMgr",
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

IDE_RC qmnCMTR::storeChild( qcTemplate * aTemplate,
                            qmncCMTR   * aCodePlan,
                            qmndCMTR   * aDataPlan )
{
    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    /* Child Plan의 초기화 */
    IDE_TEST( aCodePlan->plan.left->init( aTemplate,
                                          aCodePlan->plan.left )
              != IDE_SUCCESS);

    /* Child Plan의 결과를 저장 */
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          &sFlag )
              != IDE_SUCCESS );

    while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        /* 저장 공간의 할당 */
        aDataPlan->mtrRow = aDataPlan->mtrNode->dstTuple->row;
        IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                      &aDataPlan->mtrRow )
                  != IDE_SUCCESS );

        /* Record 구성 */
        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        /* Temp Table에 삽입 */
        IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                       aDataPlan->mtrRow )
                  != IDE_SUCCESS );

        /* Left Child의 수행 */
        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              &sFlag )
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


IDE_RC qmnCMTR::setMtrRow( qcTemplate * aTemplate,
                           qmndCMTR   * aDataPlan )
{
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
}

