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
 * $Id: qmnPPCRD.cpp 90438 2021-04-02 08:20:57Z ahra.cho $
 *
 * Description :
 *     Parallel Partition Coordinator(PPCRD) Node
 **********************************************************************/

#include <ide.h>
#include <qmnPPCRD.h>
#include <qmnPRLQ.h>
#include <qmnScan.h>
#include <qmoUtil.h>
#include <qmoPartition.h>
#include <qcg.h>

/***********************************************************************
 *
 * Description :
 *    PPCRD 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::init( qcTemplate * aTemplate,
                       qmnPlan    * aPlan )
{
    qmncPPCRD * sCodePlan = (qmncPPCRD *) aPlan;
    qmndPPCRD * sDataPlan =
        (qmndPPCRD *) (aTemplate->tmplate.data + aPlan->offset);

    idBool        sJudge;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aPlan->left  == NULL );
    IDE_DASSERT( aPlan->right == NULL );

    //---------------------------------
    // 기본 초기화
    //---------------------------------

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    sDataPlan->doIt = qmnPPCRD::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_PPCRD_INIT_DONE_MASK)
         == QMND_PPCRD_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    if ( sCodePlan->selectedPartitionCount > 0 )
    {
        IDE_DASSERT( sCodePlan->plan.children != NULL );

        sDataPlan->selectedChildrenCount = sCodePlan->selectedPartitionCount;

        sJudge = ID_TRUE;
    }
    else
    {
        sJudge = ID_FALSE;
    }

    if ( sJudge == ID_TRUE )
    {
        if ( sCodePlan->constantFilter != NULL )
        {
            IDE_TEST( qtc::judge( &sJudge,
                                  sCodePlan->constantFilter,
                                  aTemplate )
                      != IDE_SUCCESS );
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

    if ( sJudge == ID_TRUE )
    {
        //------------------------------------------------
        // 수행 함수 결정
        //------------------------------------------------
        sDataPlan->doIt = qmnPPCRD::doItFirst;

        *sDataPlan->flag &= ~QMND_PPCRD_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_PPCRD_ALL_FALSE_FALSE;

        // TODO1502:
        //  update, delete를 위한 flag설정해야 함.
    }
    else
    {
        sDataPlan->doIt = qmnPPCRD::doItAllFalse;

        *sDataPlan->flag &= ~QMND_PPCRD_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_PPCRD_ALL_FALSE_TRUE;

        // update, delete를 위한 flag설정해야함.
    }

    //------------------------------------------------
    // 가변 Data Flag 의 초기화
    //------------------------------------------------

    // Subquery내부에 포함되어 있을 경우 초기화하여
    // In Subquery Key Range를 다시 생성할 수 있도록 한다.
    *sDataPlan->flag &= ~QMND_PPCRD_INSUBQ_RANGE_BUILD_MASK;
    *sDataPlan->flag |= QMND_PPCRD_INSUBQ_RANGE_BUILD_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    PPCRD의 고유 기능을 수행한다.
 *
 * Implementation :
 *    현재 Child Plan 를 수행하고, 없을 경우 다음 child plan을 수행
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::doIt( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
    qmndPPCRD * sDataPlan =
        (qmndPPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );
}

IDE_RC qmnPPCRD::padNull( qcTemplate * aTemplate,
                          qmnPlan    * aPlan )
{
    qmncPPCRD * sCodePlan = (qmncPPCRD*) aPlan;
    qmndPPCRD * sDataPlan =
        (qmndPPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    // 초기화가 수행되지 않은 경우 초기화를 수행
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_PPCRD_INIT_DONE_MASK)
         == QMND_PPCRD_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    qmx::copyMtcTupleForPartitionedTable( sDataPlan->plan.myTuple,
                                          &(aTemplate->tmplate.rows[sCodePlan->partitionedTupleID]) );

    if ( ( sDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK )
                                        == MTC_TUPLE_STORAGE_DISK )
    {
        //-----------------------------------
        // Disk Table인 경우
        //-----------------------------------

        // Record 저장을 위한 공간은 하나만 존재하며,
        // 이에 대한 pointer는 항상 유지되어야 한다.

        if ( sDataPlan->nullRow == NULL )
        {
            //-----------------------------------
            // Null Row를 가져온 적이 없는 경우
            //-----------------------------------

            // 적합성 검사
            IDE_DASSERT( sDataPlan->plan.myTuple->rowOffset > 0 );

            IDU_FIT_POINT( "qmnPPCRD::padNull::cralloc::nullRow",
                            idERR_ABORT_InsufficientMemory );

            // Null Row를 위한 공간 할당
            IDE_TEST( aTemplate->stmt->qmxMem->cralloc(
                    sDataPlan->plan.myTuple->rowOffset,
                    (void**)&sDataPlan->nullRow ) != IDE_SUCCESS);

            // PROJ-1705
            // 디스크테이블의 null row는 qp에서 생성/저장해두고 사용한다.
            IDE_TEST( qmn::makeNullRow( sDataPlan->plan.myTuple,
                                        sDataPlan->nullRow )
                      != IDE_SUCCESS );
        
            SMI_MAKE_VIRTUAL_NULL_GRID( sDataPlan->nullRID );
        }
        else
        {
            // 이미 Null Row를 가져왔음.
            // Nothing To Do
        }

        sDataPlan->plan.myTuple->row = sDataPlan->diskRow;

        // Null Row 복사
        idlOS::memcpy( sDataPlan->plan.myTuple->row,
                       sDataPlan->nullRow,
                       sDataPlan->plan.myTuple->rowOffset );

        // Null RID의 복사
        idlOS::memcpy( & sDataPlan->plan.myTuple->rid,
                       & sDataPlan->nullRID,
                       ID_SIZEOF(scGRID) );
    }
    else
    {
        //-----------------------------------
        // Memory Table인 경우
        //-----------------------------------

        // NULL ROW를 가져온다.
        IDE_TEST( smiGetTableNullRow( sDataPlan->plan.myTuple->tableHandle,
                                      (void **) & sDataPlan->plan.myTuple->row,
                                      & sDataPlan->plan.myTuple->rid )
                  != IDE_SUCCESS );

        // 적합성 검사
        IDE_DASSERT( sDataPlan->plan.myTuple->row != NULL );

    }

    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    수행 정보를 출력한다.
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::printPlan( qcTemplate   * aTemplate,
                            qmnPlan      * aPlan,
                            ULong          aDepth,
                            iduVarString * aString,
                            qmnDisplay     aMode )
{
    qmncPPCRD   * sCodePlan;
    qmndPPCRD   * sDataPlan;

    qmnChildren * sChildren;
    qcmIndex    * sIndex;
    qcTemplate  * sTemplate;
    UInt          i;

    sCodePlan = (qmncPPCRD*)aPlan;
    sDataPlan = (qmndPPCRD*)(aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = &aTemplate->planFlag[sCodePlan->planID];

    //----------------------------
    // Display 위치 결정
    //----------------------------

    qmn::printSpaceDepth(aString, aDepth);

    //----------------------------
    // PartSCAN 노드 표시
    //----------------------------

    iduVarStringAppendLength( aString, "PARTITION-COORDINATOR ( TABLE: ", 31 );

    if ( ( sCodePlan->tableOwnerName.name != NULL ) &&
         ( sCodePlan->tableOwnerName.size > 0 ) )
    {
        iduVarStringAppendLength( aString,
                                  sCodePlan->tableOwnerName.name,
                                  sCodePlan->tableOwnerName.size );
        iduVarStringAppendLength( aString, ".", 1 );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------
    // Table Name 출력
    //----------------------------

    if ( ( sCodePlan->tableName.size <= QC_MAX_OBJECT_NAME_LEN ) &&
         ( sCodePlan->tableName.name != NULL ) &&
         ( sCodePlan->tableName.size > 0 ) )
    {
        iduVarStringAppendLength( aString,
                                  sCodePlan->tableName.name,
                                  sCodePlan->tableName.size );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------
    // Alias Name 출력
    //----------------------------

    if ( sCodePlan->aliasName.name != NULL &&
         sCodePlan->aliasName.size > 0  &&
         sCodePlan->aliasName.name != sCodePlan->tableName.name )
    {
        // Table 이름 정보와 Alias 이름 정보가 다를 경우
        // (alias name)
        iduVarStringAppendLength( aString, " ", 1 );

        if ( sCodePlan->aliasName.size <= QC_MAX_OBJECT_NAME_LEN )
        {
            iduVarStringAppendLength( aString,
                                      sCodePlan->aliasName.name,
                                      sCodePlan->aliasName.size );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Alias 이름 정보가 없거나 Table 이름 정보가 동일한 경우
        // Nothing To Do
    }

    //----------------------------
    // PARALLEL 출력.
    //----------------------------
    iduVarStringAppendLength( aString, ", PARALLEL", 10 );

    //----------------------------
    // PARTITION 정보 출력.
    //----------------------------
    iduVarStringAppendFormat( aString,
                              ", PARTITION: %"ID_UINT32_FMT"/%"ID_UINT32_FMT"",
                              sCodePlan->selectedPartitionCount,
                              sCodePlan->partitionCount );

    IDE_DASSERT((sCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK) ==
                QMNC_SCAN_INDEX_TABLE_SCAN_FALSE);

    //----------------------------
    // Access Method 출력
    //----------------------------

    // BUG-38192 : 선택된 index 출력

    sIndex = sCodePlan->index;

    if ( sIndex != NULL )
    {
        // Index가 사용된 경우
        iduVarStringAppend( aString, ", INDEX: " );

        iduVarStringAppend( aString, sIndex->userName );
        iduVarStringAppend( aString, "." );
        iduVarStringAppend( aString, sIndex->name );
    }
    else
    {
        // Full Scan인 경우
        iduVarStringAppend(aString, ", FULL SCAN");
    }

    IDE_TEST( printAccessInfo( sDataPlan,
                               aString,
                               aMode ) != IDE_SUCCESS );

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString, sCodePlan->plan.qmgAllCost );

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        qmn::printSpaceDepth(aString, aDepth);

        // sCodePlan 의 값을 출력하기때문에 qmn::printMTRinfo을 사용하지 못한다.
        iduVarStringAppendFormat( aString,
                                  "[ SELF NODE INFO, "
                                  "SELF: %"ID_INT32_FMT" ]\n",
                                  (SInt)sCodePlan->tupleRowID );
    }

    //----------------------------
    // Predicate 정보의 상세 출력
    //----------------------------

    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        IDE_TEST( printPredicateInfo( aTemplate,
                                      sCodePlan,
                                      aDepth,
                                      aString ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // subquery 정보의 출력.
    // subquery는 constant filter, nnf filter, subquery filter에만 있다.
    // Constant Filter의 Subquery 정보 출력
    if ( sCodePlan->constantFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->constantFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    // Subquery Filter의 Subquery 정보 출력
    if ( sCodePlan->subqueryFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->subqueryFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    // NNF Filter의 Subquery 정보 출력
    if ( sCodePlan->nnfFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->nnfFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    if ( QCU_TRCLOG_DISPLAY_CHILDREN == 1 )
    {
        // BUG-38192

        //----------------------------
        // Child PRLQ Plan의 정보 출력
        //----------------------------

        for ( sChildren = sCodePlan->plan.childrenPRLQ;
              sChildren != NULL;
              sChildren = sChildren->next )
        {
            IDE_TEST( sChildren->childPlan->printPlan( aTemplate,
                                                       sChildren->childPlan,
                                                       aDepth + 1,
                                                       aString,
                                                       aMode )
                      != IDE_SUCCESS );
        }

        //----------------------------
        // Child Plan의 정보 출력
        //----------------------------

        for ( sChildren = sCodePlan->plan.children;
              sChildren != NULL;
              sChildren = sChildren->next )
        {
            sTemplate = aTemplate;

            /*
             * BUG-38294
             * SCAN 이 실행된 template 을 넘겨주어야 한다.
             */
            if ((*sDataPlan->flag & QMND_PPCRD_INIT_DONE_MASK) ==
                QMND_PPCRD_INIT_DONE_TRUE)
            {
                for (i = 0; i < sDataPlan->mSCANToPRLQMapCount; i++)
                {
                    if (sDataPlan->mSCANToPRLQMap[i].mSCAN == sChildren->childPlan)
                    {
                        sTemplate =
                            qmnPRLQ::getTemplate(aTemplate,
                                                 sDataPlan->mSCANToPRLQMap[i].mPRLQ);
                        break;
                    }
                }
            }
            else
            {
                /* nothing to do */
            }
            IDE_TEST( sChildren->childPlan->printPlan( sTemplate,
                                                       sChildren->childPlan,
                                                       aDepth + 1,
                                                       aString,
                                                       aMode )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPPCRD::doItDefault( qcTemplate * /* aTemplate */,
                              qmnPlan    * /* aPlan */,
                              qmcRowFlag * /* aFlag */)
{
    /* 호출되어서는 안됨 */

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    partition pruning에 의해 만족하는 partition이 하나도 없는 경우 사용
 *    Filter 조건을 만족하는 Record가 하나도 없는 경우 사용
 *
 *    Constant Filter 검사후에 결정되는 함수로 절대 만족하는
 *    Record가 존재하지 않는다.
 *
 * Implementation :
 *    항상 record 없음을 리턴한다.
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::doItAllFalse( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag )
{
    qmncPPCRD * sCodePlan = (qmncPPCRD*) aPlan;
    qmndPPCRD * sDataPlan =
        (qmndPPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    // BUG-48800
    if ( sCodePlan->mPrePruningPartHandle != NULL )
    {
        IDE_TEST( smiValidateObjects( sCodePlan->mPrePruningPartHandle,
                                      sCodePlan->mPrePruningPartSCN )
                  != IDE_SUCCESS );
    }

    // BUG-47599 emptyPartiton에 lock을 걸면 add partition op와 lock 충돌이 발생합니다.
    if ( sCodePlan->tableRef->mEmptyPartRef != NULL )
    {
        IDE_TEST( smiValidateObjects( sCodePlan->tableRef->mEmptyPartRef->partitionHandle,
                                      sCodePlan->tableRef->mEmptyPartRef->partitionSCN )
                  != IDE_SUCCESS);
    }

    // 적합성 검사
    IDE_DASSERT( ( sCodePlan->selectedPartitionCount == 0 ) ||
                 ( sCodePlan->constantFilter != NULL ) );
    IDE_DASSERT( ( *sDataPlan->flag & QMND_PPCRD_ALL_FALSE_MASK )
                 == QMND_PPCRD_ALL_FALSE_TRUE );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Data 영역에 대한 초기화
 *
 *    firstInit()을 호출할 때 Partitioned Table Tuple에 들어있는 값이,
 *    최초에는 Partitioned Table의 것이나, 이후에는 Table Partition의 것이다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::firstInit( qcTemplate * aTemplate,
                            qmncPPCRD  * aCodePlan,
                            qmndPPCRD  * aDataPlan )
{
    iduMemory* sMemory;
    UInt       sRowOffset;

    sMemory = aTemplate->stmt->qmxMem;

    // Tuple 위치의 결정
    aDataPlan->plan.myTuple = &aTemplate->tmplate.rows[aCodePlan->tupleRowID];

    // 적합성 검사
    // 저장 매체에 대한 정보가 Tuple 정보와 Plan의 정보가 동일하게
    // 설정되어 있는 지 검사
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_DISK )
    {
        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_DISK );
    }
    else
    {
        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_MEMORY );
    }

    IDE_DASSERT( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                 != QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

    aDataPlan->indexTuple = NULL;
    
    // fix BUG-9052
    aDataPlan->subQFilterDepCnt = 0;

    //---------------------------------
    // Predicate 종류의 초기화
    //---------------------------------
    
    // partition filter 영역의 할당

    IDE_TEST( allocPartitionFilter( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    aDataPlan->partitionFilter = NULL;
    
    //---------------------------------
    // Partitioned Table 관련 정보의 초기화
    //---------------------------------
    
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aCodePlan->tupleRowID )
              != IDE_SUCCESS );

    aDataPlan->nullRow = NULL;
    aDataPlan->lastChildNo = ID_UINT_MAX;
    aDataPlan->selectedChildrenCount = 0;

    if ( aCodePlan->selectedPartitionCount > 0 )
    {
        // children area를 생성.
        IDU_FIT_POINT( "qmnPPCRD::firstInit::alloc::childrenSCANArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(sMemory->alloc(aCodePlan->selectedPartitionCount *
                                ID_SIZEOF(qmnChildren*),
                                (void**)&aDataPlan->childrenSCANArea)
                 != IDE_SUCCESS);

        // children PRLQ area를 생성.
        IDU_FIT_POINT( "qmnPPCRD::firstInit::alloc::childrenPRLQArea", 
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST(sMemory->alloc(aCodePlan->PRLQCount *
                                ID_SIZEOF(qmnChildren),
                                (void**)&aDataPlan->childrenPRLQArea)
                 != IDE_SUCCESS);

        // intersect count array create
        IDU_FIT_POINT( "qmnPPCRD::firstInit::alloc::rangeIntersectCountArray", 
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(sMemory->cralloc(aCodePlan->selectedPartitionCount *
                                  ID_SIZEOF(UInt),
                                  (void**)&aDataPlan->rangeIntersectCountArray)
                 != IDE_SUCCESS);

        IDE_DASSERT( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                     != QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

        aDataPlan->childrenIndex = NULL;

        /* BUG-38294 */
        IDU_FIT_POINT( "qmnPPCRD::firstInit::alloc::SCANToPRLQMap", 
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(sMemory->alloc(aCodePlan->selectedPartitionCount *
                                ID_SIZEOF(qmnPRLQChildMap),
                                (void**)&aDataPlan->mSCANToPRLQMap)
                 != IDE_SUCCESS);
        aDataPlan->mSCANToPRLQMapCount = 0;
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------
    // cursor 정보 설정
    //---------------------------------
    
    // PROJ-2205 rownum in DML
    // row movement update인 경우
    IDE_DASSERT( aDataPlan->plan.myTuple->cursorInfo == NULL );

    aDataPlan->updateColumnList = NULL;
    aDataPlan->cursorType       = SMI_SELECT_CURSOR;
    aDataPlan->inplaceUpdate    = ID_FALSE;
    aDataPlan->lockMode         = SMI_LOCK_READ;

    aDataPlan->isRowMovementUpdate = ID_FALSE;

    aDataPlan->isNeedAllFetchColumn = ID_FALSE;

    /* PROJ-2464 hybrid partitioned table 지원
     *  Partitioned Tuple이 Disk인 경우, Row Offset 만큼 메모리를 할당한다.
     */
    if ( ( aTemplate->tmplate.rows[aCodePlan->tupleRowID].lflag & MTC_TUPLE_STORAGE_MASK )
                                                              == MTC_TUPLE_STORAGE_DISK )
    {
        sRowOffset = aTemplate->tmplate.rows[aCodePlan->partitionedTupleID].rowOffset;

        // 적합성 검사
        IDE_DASSERT( sRowOffset > 0 );

        // Disk Row를 위한 공간 할당
        IDU_FIT_POINT( "qmnPPCRD::firstInit::alloc::diskRow", idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( sRowOffset,
                                                  (void **) &aDataPlan->diskRow )
                  != IDE_SUCCESS );

        /* BUG-42380 setTuple에서, 별도로 만든 Partitioned Table의 Tuple에 접근한다. */
        aTemplate->tmplate.rows[aCodePlan->partitionedTupleID].row = aDataPlan->diskRow;
    }
    else
    {
        aDataPlan->diskRow = NULL;
    }

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------
    
    *aDataPlan->flag &= ~QMND_PPCRD_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_PPCRD_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *     Predicate의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::printPredicateInfo( qcTemplate   * aTemplate,
                                     qmncPPCRD     * aCodePlan,
                                     ULong          aDepth,
                                     iduVarString * aString )
{
    // partition Key Range 정보의 출력
    IDE_TEST( printPartKeyRangeInfo( aTemplate,
                                     aCodePlan,
                                     aDepth,
                                     aString ) != IDE_SUCCESS );

    // partition Filter 정보의 출력
    IDE_TEST( printPartFilterInfo( aTemplate,
                                   aCodePlan,
                                   aDepth,
                                   aString ) != IDE_SUCCESS );

    // Filter 정보의 출력
    IDE_TEST( printFilterInfo( aTemplate,
                               aCodePlan,
                               aDepth,
                               aString ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *     partition Key Range의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::printPartKeyRangeInfo( qcTemplate   * aTemplate,
                                        qmncPPCRD    * aCodePlan,
                                        ULong          aDepth,
                                        iduVarString * aString )
{
    // Fixed Key Range 출력
    if (aCodePlan->partitionKeyRange != NULL)
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppend( aString, " [ PARTITION KEY ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->partitionKeyRange)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *     partition Filter의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::printPartFilterInfo( qcTemplate   * aTemplate,
                                      qmncPPCRD    * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString )
{
    // Fixed Key Filter 출력
    if (aCodePlan->partitionFilter != NULL)
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppend( aString, " [ PARTITION FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->partitionFilter )
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *     Filter의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::printFilterInfo( qcTemplate   * aTemplate,
                                  qmncPPCRD    * aCodePlan,
                                  ULong          aDepth,
                                  iduVarString * aString )
{
    // Constant Filter 출력
    if (aCodePlan->constantFilter != NULL)
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppend( aString, " [ CONSTANT FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->constantFilter )
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Subquery Filter 출력
    if (aCodePlan->subqueryFilter != NULL)
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppend( aString, " [ SUBQUERY FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->subqueryFilter )
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // NNF Filter 출력
    if (aCodePlan->nnfFilter != NULL)
    {
        qmn::printSpaceDepth(aString, aDepth);
        iduVarStringAppend( aString, " [ NOT-NORMAL-FORM FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->nnfFilter )
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *    Partition Filter를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::allocPartitionFilter( qcTemplate * aTemplate,
                                       qmncPPCRD  * aCodePlan,
                                       qmndPPCRD  * aDataPlan )
{
    iduMemory * sMemory;

    if ( aCodePlan->partitionFilter != NULL )
    {
        IDE_TEST(qmoKeyRange::estimateKeyRange(aTemplate,
                                               aCodePlan->partitionFilter,
                                               &aDataPlan->partitionFilterSize)
                 != IDE_SUCCESS);

        IDE_DASSERT( aDataPlan->partitionFilterSize > 0 );

        sMemory = aTemplate->stmt->qmxMem;

        IDU_FIT_POINT( "qmnPPCRD::allocPartitionFilter::alloc::partitionFilterArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(sMemory->cralloc(aDataPlan->partitionFilterSize,
                                  (void**)&aDataPlan->partitionFilterArea)
                 != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->partitionFilterSize = 0;
        aDataPlan->partitionFilterArea = NULL;
        aDataPlan->partitionFilter     = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPPCRD::doItFirst( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag )
{
    qmncPPCRD  * sCodePlan;
    qmndPPCRD  * sDataPlan;
    qmnPlan    * sChildPRLQPlan;
    qmnPlan    * sChildSCANPlan;
    qmndPRLQ   * sChildPRLQDataPlan;
    qmndPlan   * sChildSCANDataPlan;
    qmnChildren* sChild;
    UInt         sDiskRowOffset = 0;
    UInt         i;
    idBool       sExistDisk = ID_FALSE;

    sCodePlan = (qmncPPCRD*)aPlan;
    sDataPlan = (qmndPPCRD*)(aTemplate->tmplate.data + aPlan->offset);

    // Table에 IS Lock을 건다.
    IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aTemplate->stmt))->getTrans(),
                                        sCodePlan->table,
                                        sCodePlan->tableSCN,
                                        SMI_TBSLV_DDL_DML,
                                        SMI_TABLE_LOCK_IS,
                                        ID_ULONG_MAX,
                                        ID_FALSE )
             != IDE_SUCCESS);

    // BUG-47599
    // emptyPartiton에 lock을 걸면 add partition op와 lock 충돌이 발생합니다.
    if ( sCodePlan->tableRef->mEmptyPartRef != NULL )
    {
        IDE_TEST( smiValidateObjects( sCodePlan->tableRef->mEmptyPartRef->partitionHandle,
                                      sCodePlan->tableRef->mEmptyPartRef->partitionSCN )
                  != IDE_SUCCESS);
    }

    // 비정상 종료 검사
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    /*
     * PROJ-2205 rownum in DML
     * row movement update인 경우
     * first doIt시 child의 모든 cursor를 열어둬야 하는데..
     * parallel query 에서는 DML 지원 안하므로 항상 false
     */
    IDE_DASSERT( sDataPlan->isRowMovementUpdate == ID_FALSE);

    if ( sCodePlan->partitionFilter != NULL )
    {
        // partition filter 구성
        IDE_TEST( makePartitionFilter( aTemplate,
                                       sCodePlan,
                                       sDataPlan ) != IDE_SUCCESS );

        if ( sDataPlan->partitionFilter != NULL )
        {
            IDE_TEST( partitionFiltering( aTemplate,
                                          sCodePlan,
                                          sDataPlan )
                      != IDE_SUCCESS );
        }
        else
        {
            /*
             * partition filter를 만들다가 실패할 수 있다.
             * host 변수의 경우 conversion등으로 안될 수 있음.
             * 이때는 filtering을 하지 않는다.
             * => 여기로 올리 없을듯..
             */
            IDE_DASSERT(0);
            makeChildrenSCANArea( aTemplate, aPlan );
        }
    }
    else
    {
        makeChildrenSCANArea( aTemplate, aPlan );
    }

    makeChildrenPRLQArea( aTemplate, aPlan );

    /*
     * PRLQ init,
     * PRLQ->mCurrentNode 에 SCAN 연결
     */
    for ( i = 0; 
          (i < sCodePlan->PRLQCount)  &&
          (i < sDataPlan->selectedChildrenCount);
          i++ )
    {
        sChild = &sDataPlan->childrenPRLQArea[i];
        IDE_TEST( sChild->childPlan->init( aTemplate,
                                           sChild->childPlan )
                  != IDE_SUCCESS );

        /*
         * PRLQ 에 SCAN 을 연결한다.
         *
         * sChild->childPlan : PRLQ
         * sDataPlan->childrenSCANArea[i]->childPlan : SCAN
         */
        qmnPRLQ::setChildNode( aTemplate,
                               sChild->childPlan,
                               sDataPlan->childrenSCANArea[i]->childPlan );

        /* BUG-38294 */
        sDataPlan->mSCANToPRLQMap[sDataPlan->mSCANToPRLQMapCount].mSCAN =
            sDataPlan->childrenSCANArea[i]->childPlan;
        sDataPlan->mSCANToPRLQMap[sDataPlan->mSCANToPRLQMapCount].mPRLQ =
            sChild->childPlan;
        sDataPlan->mSCANToPRLQMapCount++;

        /* PRLQ 에 연결한 마지막 SCAN 의 번호 */
        sDataPlan->lastChildNo = i;
    }

    // 선택된 children이 있는 경우만 수행.
    if ( sDataPlan->selectedChildrenCount > 0 )
    {
        /* SCAN init */
        for ( i = 0; i < sDataPlan->selectedChildrenCount; i++ )
        {
            sChild = sDataPlan->childrenSCANArea[i];
            IDE_TEST( sChild->childPlan->init( aTemplate,
                                               sChild->childPlan )
                      != IDE_SUCCESS );

            /* PROJ-2464 hybrid partitioned table 지원
             *  - PRLQ의 메모리 할당을 최적화하기 위해서, Disk Partition이 포함되었는지 검사한다.
             *    1. 하부 PLAN에 Disk가 포함되었는지 검사한다.
             *    2. PRLQ에 추가적인 정보를 전달한다.
             */
            /* 1. 하부 PLAN에 Disk가 포함되었는지 검사한다. */
            if ( ( sChild->childPlan->flag & QMN_PLAN_STORAGE_MASK ) == QMN_PLAN_STORAGE_DISK )
            {
                sExistDisk         = ID_TRUE;
                sChildSCANDataPlan = (qmndPlan *)(aTemplate->tmplate.data + sChild->childPlan->offset);
                sDiskRowOffset     = sChildSCANDataPlan->myTuple->rowOffset;
            }
            else
            {
                /* Nothing to do */
            }
        }

        for (i = 0; (i < sCodePlan->PRLQCount) && (i < sDataPlan->selectedChildrenCount); i++)
        {
            sChild = &sDataPlan->childrenPRLQArea[i];

            /* 2. PRLQ에 추가적인 정보를 전달한다. */
            qmnPRLQ::setPRLQInfo( aTemplate,
                                  sChild->childPlan,
                                  sExistDisk,
                                  sDiskRowOffset );

            // BUG-41931
            // child의 doIt시에도 plan.myTuple을 참조할 수 있으므로
            // myTuple의 columns / partitionTupleID를 child의 것으로 갱신해야 한다.
            // PPCRD에서는, 이 작업이 doItNext() 에서만 이루어지고 있었다.
            sChildPRLQPlan     = sChild->childPlan;
            sChildPRLQDataPlan = (qmndPRLQ*)( aTemplate->tmplate.data +
                                              sChildPRLQPlan->offset );

            // SCAN 플랜의 정보를 직접 접근해야함
            if ( sChildPRLQDataPlan->mCurrentNode->type == QMN_GRAG )
            {
                sChildSCANPlan = sChildPRLQDataPlan->mCurrentNode->left;
            }
            else
            {
                sChildSCANPlan = sChildPRLQDataPlan->mCurrentNode;
            }

            sChildSCANDataPlan = (qmndPlan*)( aTemplate->tmplate.data +
                                              sChildSCANPlan->offset );

            sDataPlan->plan.myTuple->columns = 
                sChildSCANDataPlan->myTuple->columns;

            sDataPlan->plan.myTuple->partitionTupleID =
                ((qmncSCAN*)sChildSCANPlan)->tupleRowID;

            sDataPlan->plan.myTuple->tableHandle = (void*)((qmncSCAN*)sChildSCANPlan)->table;

            /* PROJ-2464 hybrid partitioned table 지원 */
            sDataPlan->plan.myTuple->rowOffset  = sChildSCANDataPlan->myTuple->rowOffset;
            sDataPlan->plan.myTuple->rowMaximum = sChildSCANDataPlan->myTuple->rowMaximum;

            IDE_TEST(qmnPRLQ::startIt(aTemplate,
                                      sChild->childPlan,
                                      NULL)
                     != IDE_SUCCESS);
        }

        sDataPlan->doIt = qmnPPCRD::doItNext;

        // Record를 획득한다.
        IDE_TEST(sDataPlan->doIt(aTemplate, aPlan, aFlag) != IDE_SUCCESS);
    }
    else
    {
        // 선택된 children이 없다면 데이터는 없다.
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPPCRD::makePartitionFilter( qcTemplate * aTemplate,
                                      qmncPPCRD   * aCodePlan,
                                      qmndPPCRD   * aDataPlan )
{
    qmsTableRef * sTableRef;
    UInt          sCompareType;

    sTableRef = aCodePlan->tableRef;

    // PROJ-1872
    // Partition Key는 Partition Table 결정할때 쓰임
    // (Partition을 나누는 기준값)과 (Predicate의 value)와
    // 비교하므로 MTD Value간의 compare를 사용함
    // ex) i1 < 10 => Partition P1
    //     i1 < 20 => Partition P2
    //     i1 < 30 => Partition P3
    // SELECT ... FROM ... WHERE i1 = 5
    // P1, P2, P3에서 P1 Partition을 선택하기 위해
    // Partition Key Range를 사용함
    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;
    
    IDE_TEST( qmoKeyRange::makePartKeyRange(
                  aTemplate,
                  aCodePlan->partitionFilter,
                  sTableRef->tableInfo->partKeyColCount,
                  sTableRef->tableInfo->partKeyColBasicInfo,
                  sTableRef->tableInfo->partKeyColsFlag,
                  sCompareType,
                  aDataPlan->partitionFilterArea,
                  aDataPlan->partitionFilterSize,
                  &aDataPlan->partitionFilter )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------------------------
 * doIt function
 * ------------------------------------------------------------------
 * Queue 에 레코드가 들어있는 childrenPRLQArea 를 실행한다.
 * Queue 가 비어있으면 다음 childrenPRLQArea 를 실행하고,
 * 실행이 끝났으면 childrenPRLQArea 에서 제거한다.
 * ------------------------------------------------------------------
 */
IDE_RC qmnPPCRD::doItNext( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
    idBool    sJudge;
    qmnPlan * sChildPRLQPlan;
    qmnPlan * sChildSCANPlan;
    qmndPRLQ* sChildPRLQDataPlan;
    qmndPlan* sChildSCANDataPlan;
    mtcTuple* sPartitionTuple;

    qmncPPCRD* sCodePlan = (qmncPPCRD*)aPlan;
    qmndPPCRD* sDataPlan = (qmndPPCRD*)(aTemplate->tmplate.data + aPlan->offset);

    while ( 1 )
    {
        //----------------------------
        // 현재 Child를 수행
        //----------------------------

        sChildPRLQPlan = sDataPlan->curPRLQ->childPlan;
        sChildPRLQDataPlan = (qmndPRLQ*)( aTemplate->tmplate.data +
                                          sChildPRLQPlan->offset );

        // PROJ-2444
        // SCAN 플랜의 정보를 직접 접근해야함
        if ( sChildPRLQDataPlan->mCurrentNode->type == QMN_GRAG )
        {
            sChildSCANPlan = sChildPRLQDataPlan->mCurrentNode->left;
        }
        else
        {
            sChildSCANPlan = sChildPRLQDataPlan->mCurrentNode;
        }

        sChildSCANDataPlan = (qmndPlan*)( aTemplate->tmplate.data +
                                          sChildSCANPlan->offset );

        // isNeedAllFetchColumn 는 row movement update 시에 ID_TRUE 로 설정된다.
        // Parallel query 는 row movement update 를 지원하지 않기 때문에
        // isNeedAllFetchColumn 은 항상 기본값인 ID_FALSE 이다.
        // 따라서 하위 SCAN 노드로 전달하는 과정은 생략하도록 한다.
        // sChildSCANDataPlan->isNeedAllFetchColumn = aDataPlan->isNeedAllFetchColumn;

        // PROJ-2002 Column Security
        // child의 doIt시에도 plan.myTuple을 참조할 수 있다.
        // BUGBUG1701
        // 여기서 PPCRD 의 plan.myTuple 을 수정해도
        // Thread 는 최초 실행되는 시점의 plan.myTuple 을 복사해서 참조하므로
        // 의도한대로 작동하지 않을 수 있다.
        // 다만 여기서 하고자 하는 작업은 SCAN 노드에서 doIt 시
        //  상위 노드인 PPCRD 의 plan.myTuple 이 SCAN 의 columns 와
        //  partitionTupleID 를 가지도록 하는 것이므로 thread 에서
        //  template 복사 시에 이 부분을 유의하면 이 로직은
        //  해결 될 것으로 보인다.
        sDataPlan->plan.myTuple->columns = sChildSCANDataPlan->myTuple->columns;

        sDataPlan->plan.myTuple->partitionTupleID =
            ((qmncSCAN*)sChildSCANPlan)->tupleRowID;

        // PROJ-2444 Parallel Aggreagtion
        sPartitionTuple = &aTemplate->tmplate.rows[sDataPlan->plan.myTuple->partitionTupleID];

        /* BUG-39073 */
        sDataPlan->plan.myTuple->tableHandle = (void*)((qmncSCAN*)sChildSCANPlan)->table;

        /* PROJ-2464 hybrid partitioned table 지원 */
        sDataPlan->plan.myTuple->rowOffset  = sChildSCANDataPlan->myTuple->rowOffset;
        sDataPlan->plan.myTuple->rowMaximum = sChildSCANDataPlan->myTuple->rowMaximum;

        IDE_TEST( sChildPRLQPlan->doIt( aTemplate, sChildPRLQPlan, aFlag )
                  != IDE_SUCCESS );

        //----------------------------
        // Data 존재 여부에 따른 처리
        //----------------------------

        sJudge = ID_FALSE;

        if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            //----------------------------
            // 현재 Child에 Data가 존재하는 경우
            //----------------------------

            // PROJ-2444
            // 자식 PRLQ 의 데이타를 직접 읽어온다.
            sDataPlan->plan.myTuple->rid = sChildPRLQDataPlan->mRid;
            sDataPlan->plan.myTuple->row = sChildPRLQDataPlan->mRow;

            // 파티션의 튜플에도 값을 저장해야 한다.
            // lob 레코드를 읽을때 partitionTupleID 를 이용한다.
            sPartitionTuple->rid = sChildPRLQDataPlan->mRid;
            sPartitionTuple->row = sChildPRLQDataPlan->mRow;

            sDataPlan->plan.myTuple->modify++;

            // judge
            if ( sCodePlan->subqueryFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      sCodePlan->subqueryFilter,
                                      aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                sJudge = ID_TRUE;
            }

            if ( sCodePlan->nnfFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      sCodePlan->nnfFilter,
                                      aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            if ( sJudge == ID_TRUE )
            {
                if ( ( *aFlag & QMC_ROW_QUEUE_EMPTY_MASK )
                    == QMC_ROW_QUEUE_EMPTY_TRUE )
                {
                    // Record exists and Thread stoped
                    // OR Record exists and Serial executed

                    // BUGBUG1071: All rows read = Serial executed
                    // Serial execute 만을 나타내는 flag 가 있으면 해결될 듯
                    // 다음 PRLQ 를 읽는다.
                    sDataPlan->prevPRLQ = sDataPlan->curPRLQ;
                    sDataPlan->curPRLQ  = sDataPlan->curPRLQ->next;
                }
                else
                {
                    // Record exists and Thread still running
                    // Nothing to do

                    // BUGBUG1071
                    // Tuning point;
                    // 특정 record 수로 적정값을 찾아내거나
                    // 일정 비율을 사용해서 PRLQ 를 고르게 실행하도록 하면
                    // 성능 향상에 도움이 될 듯 하다.
                    if ( sDataPlan->plan.myTuple->modify % 1000 == 0 )
                    {
                        sDataPlan->prevPRLQ = sDataPlan->curPRLQ;
                        sDataPlan->curPRLQ  = sDataPlan->curPRLQ->next;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }

                *aFlag = QMC_ROW_DATA_EXIST;
                break;
            }
            else
            {
                /* judge fail, try next row */
            }
        }
        else
        {
            /* QMC_ROW_DATA_NONE */

            if( ( *aFlag & QMC_ROW_QUEUE_EMPTY_MASK ) ==
                QMC_ROW_QUEUE_EMPTY_TRUE )
            {
                /* All rows read */

                if ( sDataPlan->lastChildNo + 1 ==
                     sDataPlan->selectedChildrenCount )
                {
                    // 해당 PRLQ 는 레코드도 없고
                    // 더 이상 실행할 SCAN 노드도 없다.
                    // 따라서 실행 대상에서 제거한다.

                    if ( sChildPRLQDataPlan->mThrObj != NULL )
                    {
                        // PRLQ 의 thread 를 반환한다.
                        qmnPRLQ::returnThread( aTemplate,
                                               sDataPlan->curPRLQ->childPlan );
                    }

                    if ( sDataPlan->prevPRLQ == sDataPlan->curPRLQ )
                    {
                        // 더 이상 실행할 PRLQ 가 없는 경우
                        sDataPlan->curPRLQ = NULL;
                        sDataPlan->prevPRLQ = NULL;

                        *aFlag = QMC_ROW_DATA_NONE;
                        break;
                    }
                    else
                    {
                        // 현재 PRLQ node 를 list 에서 제거한다.
                        sDataPlan->prevPRLQ->next = sDataPlan->curPRLQ->next;
                        sDataPlan->curPRLQ        = sDataPlan->curPRLQ->next;
                    }
                }
                else
                {
                    // 해당 PRLQ 는 레코드가 없다.
                    // 따라서 다음 SCAN 노드를 할당하고 실행한다.
                    sDataPlan->lastChildNo++;
                    qmnPRLQ::setChildNode( aTemplate,
                                           sDataPlan->curPRLQ->childPlan,
                                           sDataPlan->childrenSCANArea[sDataPlan->lastChildNo]->childPlan );

                    /* BUG-38294 */
                    sDataPlan->mSCANToPRLQMap[sDataPlan->mSCANToPRLQMapCount].mSCAN =
                        sDataPlan->childrenSCANArea[sDataPlan->lastChildNo]->childPlan;
                    sDataPlan->mSCANToPRLQMap[sDataPlan->mSCANToPRLQMapCount].mPRLQ =
                        sDataPlan->curPRLQ->childPlan;
                    sDataPlan->mSCANToPRLQMapCount++;
                }
            }
            else
            {
                // SCAN 작업이 남아있지만 잠시 queue 가 비어있다.
                // 다음 PRLQ 를 읽는다.
                sDataPlan->prevPRLQ = sDataPlan->curPRLQ;
                sDataPlan->curPRLQ  = sDataPlan->curPRLQ->next;
            }
        }
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnPPCRD::doItFirst;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *     Access 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
IDE_RC qmnPPCRD::printAccessInfo( qmndPPCRD    * aDataPlan,
                                  iduVarString * aString,
                                  qmnDisplay     aMode )
{
    IDE_DASSERT( aDataPlan != NULL );
    IDE_DASSERT( aString   != NULL );

    if ( aMode == QMN_DISPLAY_ALL )
    {
        //----------------------------
        // explain plan = on; 인 경우
        //----------------------------

        // 수행 횟수 출력
        if ( (*aDataPlan->flag & QMND_PPCRD_INIT_DONE_MASK)
             == QMND_PPCRD_INIT_DONE_TRUE )
        {
            // fix BUG-9052
            iduVarStringAppendFormat( aString,
                                      ", ACCESS: %"ID_UINT32_FMT"",
                                      (aDataPlan->plan.myTuple->modify
                                       - aDataPlan->subQFilterDepCnt) );
        }
        else
        {
            iduVarStringAppend( aString, ", ACCESS: 0" );
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------

        iduVarStringAppend( aString, ", ACCESS: ??" );
    }

    return IDE_SUCCESS;
}

IDE_RC qmnPPCRD::partitionFiltering( qcTemplate * aTemplate,
                                     qmncPPCRD  * aCodePlan,
                                     qmndPPCRD  * aDataPlan )
{
    return qmoPartition::partitionFilteringWithPartitionFilter(
        aTemplate->stmt,
        aCodePlan->rangeSortedChildrenArray,
        aDataPlan->rangeIntersectCountArray,
        aCodePlan->selectedPartitionCount,
        aCodePlan->partitionCount,
        aCodePlan->plan.children,
        aCodePlan->tableRef->tableInfo->partitionMethod,
        aDataPlan->partitionFilter,
        aDataPlan->childrenSCANArea,
        &aDataPlan->selectedChildrenCount);
}

void qmnPPCRD::makeChildrenSCANArea( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan )
{
    qmncPPCRD  * sCodePlan;
    qmndPPCRD  * sDataPlan;
    qmnChildren* sChild;
    qmsTableRef* sTableRef;
    UInt         i;

    sCodePlan = (qmncPPCRD*)aPlan;
    sDataPlan = (qmndPPCRD*)(aTemplate->tmplate.data + aPlan->offset);
    sTableRef = sCodePlan->tableRef;

    /* PROJ-2249 method range 이면 정렬된 children을 할당 한다. */
    if ( ( sTableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE ) ||
         ( sTableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
    {
        for ( i = 0; i < sDataPlan->selectedChildrenCount; i++ )
        {
            sDataPlan->childrenSCANArea[i] =
                sCodePlan->rangeSortedChildrenArray[i].children;
        }
    }
    else
    {
        for ( i = 0, sChild = sCodePlan->plan.children;
              ( i < sDataPlan->selectedChildrenCount ) && ( sChild != NULL );
              i++, sChild = sChild->next )
        {
            sDataPlan->childrenSCANArea[i] = sChild;
        }
    }
}

void qmnPPCRD::makeChildrenPRLQArea( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan )
{
    qmncPPCRD  * sCodePlan;
    qmndPPCRD  * sDataPlan;
    qmnChildren* sChild;
    qmnChildren* sPrev;
    UInt         i;

    sCodePlan = (qmncPPCRD*)aPlan;
    sDataPlan = (qmndPPCRD*)(aTemplate->tmplate.data + aPlan->offset);
    sChild    = NULL;
    sPrev     = NULL;

    if ( (sCodePlan->plan.childrenPRLQ != NULL) &&
         (sDataPlan->selectedChildrenCount > 0) )
    {
        for ( i = 0, sChild = sCodePlan->plan.childrenPRLQ;
              ( i < sCodePlan->PRLQCount ) &&
              ( i < sDataPlan->selectedChildrenCount ) &&
              ( sChild != NULL );
              i++, sChild = sChild->next )
        {
            sDataPlan->childrenPRLQArea[i].childPlan = sChild->childPlan;
            if ( sPrev == NULL )
            {
                sPrev = &sDataPlan->childrenPRLQArea[i];
            }
            else
            {
                sPrev->next = &sDataPlan->childrenPRLQArea[i];
                sPrev       = &sDataPlan->childrenPRLQArea[i];
            }
        }

        // PRLQ 가 하나도 없다면 실행이 불가능하다.
        IDE_DASSERT( sPrev != NULL );

        // 원형 리스트로 만든다.
        sPrev->next = sDataPlan->childrenPRLQArea;

        // curPRLQ, prevPRLQ 를 초기화
        sDataPlan->prevPRLQ = sPrev;
        sDataPlan->curPRLQ  = sDataPlan->childrenPRLQArea;
    }
    else
    {
        // 실행할 child 노드가 없다.
        sDataPlan->childrenPRLQArea = NULL;
        sDataPlan->prevPRLQ = NULL;
        sDataPlan->curPRLQ  = NULL;
    }
}
