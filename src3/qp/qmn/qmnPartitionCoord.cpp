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
 * $Id: qmnPartitionCoord.cpp 20233 2007-02-01 01:58:21Z shsuh $
 *
 * Description :
 *     Partition Coordinator(PCRD) Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmnPartitionCoord.h>
#include <qmnScan.h>
#include <qmoUtil.h>
#include <qcuProperty.h>
#include <qmoPartition.h>
#include <qcg.h>

extern "C" int
compareChildrenIndex( const void* aElem1, const void* aElem2 )
{
/***********************************************************************
 *
 * Description :
 *    compare qcmColumn
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_DASSERT( aElem1 != NULL );
    IDE_DASSERT( aElem2 != NULL );

    //--------------------------------
    // compare partition OID
    //--------------------------------

    if( ((qmnChildrenIndex*)aElem1)->childOID >
        ((qmnChildrenIndex*)aElem2)->childOID )
    {
        return 1;
    }
    else if( ((qmnChildrenIndex*)aElem1)->childOID <
             ((qmnChildrenIndex*)aElem2)->childOID )
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

IDE_RC
qmnPCRD::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    PCRD 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncPCRD * sCodePlan = (qmncPCRD *) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD *) (aTemplate->tmplate.data + aPlan->offset);

    idBool        sJudge;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aPlan->left     == NULL );
    IDE_DASSERT( aPlan->right    == NULL );

    //---------------------------------
    // 기본 초기화
    //---------------------------------

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    sDataPlan->doIt = qmnPCRD::doItDefault;

    // first initialization
    if ( (*sDataPlan->flag & QMND_PCRD_INIT_DONE_MASK)
         == QMND_PCRD_INIT_DONE_FALSE )
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

    if( sCodePlan->selectedPartitionCount > 0 )
    {
        IDE_DASSERT( sCodePlan->plan.children != NULL );

        sDataPlan->selectedChildrenCount = sCodePlan->selectedPartitionCount;

        sJudge = ID_TRUE;
    }
    else
    {
        sJudge = ID_FALSE;
    }

    if( sJudge == ID_TRUE )
    {
        if( sCodePlan->constantFilter != NULL )
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

    if( sJudge == ID_TRUE )
    {
        //------------------------------------------------
        // 수행 함수 결정
        //------------------------------------------------
        sDataPlan->doIt = qmnPCRD::doItFirst;

        *sDataPlan->flag &= ~QMND_PCRD_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_PCRD_ALL_FALSE_FALSE;

        // TODO1502:
        //  update, delete를 위한 flag설정해야 함.
    }
    else
    {
        sDataPlan->doIt = qmnPCRD::doItAllFalse;

        *sDataPlan->flag &= ~QMND_PCRD_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_PCRD_ALL_FALSE_TRUE;

        // update, delete를 위한 flag설정해야함.
    }

    //------------------------------------------------
    // 가변 Data Flag 의 초기화
    //------------------------------------------------

    // Subquery내부에 포함되어 있을 경우 초기화하여
    // In Subquery Key Range를 다시 생성할 수 있도록 한다.
    *sDataPlan->flag &= ~QMND_PCRD_INSUBQ_RANGE_BUILD_MASK;
    *sDataPlan->flag |= QMND_PCRD_INSUBQ_RANGE_BUILD_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    PCRD의 고유 기능을 수행한다.
 *
 * Implementation :
 *    현재 Child Plan 를 수행하고, 없을 경우 다음 child plan을 수행
 *
 ***********************************************************************/

    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );
}

IDE_RC
qmnPCRD::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    if ( ( sCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
         == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE )
    {
        // 다음 Record를 읽는다.
        IDE_TEST( readRowWithIndex( aTemplate, sCodePlan, sDataPlan, aFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        // 다음 Record를 읽는다.
        IDE_TEST( readRow( aTemplate, sCodePlan, sDataPlan, aFlag )
                  != IDE_SUCCESS );
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnPCRD::doItFirst;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    // 초기화가 수행되지 않은 경우 초기화를 수행
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_PCRD_INIT_DONE_MASK)
         == QMND_PCRD_INIT_DONE_FALSE )
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

            // Null Row를 위한 공간 할당
            IDE_TEST( aTemplate->stmt->qmxMem->cralloc(
                          sDataPlan->plan.myTuple->rowOffset,
                          (void**) & sDataPlan->nullRow ) != IDE_SUCCESS );

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

IDE_RC
qmnPCRD::printPlan( qcTemplate   * aTemplate,
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

    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong i;

    qmnChildren * sChildren;
    qcmIndex    * sIndex;

    //----------------------------
    // Display 위치 결정
    //----------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // PartSCAN 노드 표시
    //----------------------------

    iduVarStringAppend( aString,
                        "PARTITION-COORDINATOR ( TABLE: " );

    if ( ( sCodePlan->tableOwnerName.name != NULL ) &&
         ( sCodePlan->tableOwnerName.size > 0 ) )
    {
        iduVarStringAppendLength( aString,
                                  sCodePlan->tableOwnerName.name,
                                  sCodePlan->tableOwnerName.size );
        iduVarStringAppend( aString, "." );
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
        iduVarStringAppend( aString, " " );

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
    // PARTITION 정보 출력.
    //----------------------------
    iduVarStringAppendFormat( aString,
                              ", PARTITION: %"ID_UINT32_FMT"/%"ID_UINT32_FMT"",
                              sCodePlan->selectedPartitionCount,
                              sCodePlan->partitionCount );

    //----------------------------
    // Access Method 출력
    //----------------------------

    // BUG-38192
    // global, local 구분없이 선택된 index 를 출력

    sIndex = sCodePlan->index;

    if ( sIndex != NULL )
    {
        // To fix BUG-12742
        // explain plan = on인 경우 실행 이후 plan을 출력하므로
        // dataPlan의 keyrange를 보고 index출력 여부를 결정.
        // explain plan = only인 경우 optimize이후 plan을 출력하므로
        // codePlan의 fix/variable keyrange를 보고 index출력 여부를 결정.

        if ( aMode == QMN_DISPLAY_ALL )
        {
            //----------------------------
            // explain plan = on; 인 경우
            //----------------------------

            // IN SUBQUERY KEYRANGE가 있는 경우:
            // keyrange만들다 실패하면 fetch종료이므로,
            // keyrange가 없더라도 index는 출력한다.
        }
        else
        {
            //----------------------------
            // explain plan = only; 인 경우
            //----------------------------

            if ( ( ( sCodePlan->flag & QMNC_SCAN_FORCE_INDEX_SCAN_MASK )
                   == QMNC_SCAN_FORCE_INDEX_SCAN_FALSE ) &&
                 ( sCodePlan->fixKeyRange == NULL ) &&
                 ( sCodePlan->varKeyRange == NULL ) )
            {
                sIndex = NULL;
            }
            else
            {
                // Nothing to do.
            }
        }

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
            // Nothing to do.
        }
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
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

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

    // Variable Key Range의 Subquery 정보 출력
    if ( sCodePlan->varKeyRange != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->varKeyRange,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    // Variable Key Filter의 Subquery 정보 출력
    if ( sCodePlan->varKeyFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sCodePlan->varKeyFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
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

    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    sCodePlan->plan.resultDesc )
                  != IDE_SUCCESS );
    }

    //----------------------------
    // Child Plan의 정보 출력
    //----------------------------

    // BUG-38192

    if ( QCU_TRCLOG_DISPLAY_CHILDREN == 1 )
    {
        for ( sChildren = sCodePlan->plan.children;
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
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    호출되어서는 안됨
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::doItAllFalse( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
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

    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    // BUG-48800
    if ( sCodePlan->mPrePruningPartHandle != NULL )
    {
        IDE_TEST( smiValidateObjects( sCodePlan->mPrePruningPartHandle,
                                      sCodePlan->mPrePruningPartSCN )
                  != IDE_SUCCESS );
    }

    // BUG-47599
    // emptyPartiton에 lock을 걸면 add partition op와 lock 충돌이 발생합니다.
    if ( sCodePlan->tableRef->mEmptyPartRef != NULL )
    {
        IDE_TEST( smiValidateObjects( sCodePlan->tableRef->mEmptyPartRef->partitionHandle,
                                      sCodePlan->tableRef->mEmptyPartRef->partitionSCN )
                  != IDE_SUCCESS);
    }

    // 적합성 검사
    IDE_DASSERT( ( sCodePlan->selectedPartitionCount == 0 ) ||
                 ( sCodePlan->constantFilter != NULL ) );
    IDE_DASSERT( ( *sDataPlan->flag & QMND_PCRD_ALL_FALSE_MASK )
                 == QMND_PCRD_ALL_FALSE_TRUE );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::firstInit( qcTemplate * aTemplate,
                    qmncPCRD   * aCodePlan,
                    qmndPCRD   * aDataPlan )
{
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

    qmnCursorInfo       * sCursorInfo;
    UInt                  sRowOffset;

    //--------------------------------
    // 적합성 검사
    //--------------------------------

    // Key Range는 Fixed KeyRange와 Variable KeyRange가 혼용될 수 없다.
    IDE_DASSERT( (aCodePlan->fixKeyRange == NULL) ||
                 (aCodePlan->varKeyRange == NULL) );

    // Key Filter는 Fixed KeyFilter와 Variable KeyFilter가 혼용될 수 없다.
    IDE_DASSERT( (aCodePlan->fixKeyFilter == NULL) ||
                 (aCodePlan->varKeyFilter == NULL) );

    //---------------------------------
    // PCRD 고유 정보의 초기화
    //---------------------------------

    // Tuple 위치의 결정
    aDataPlan->plan.myTuple =
        & aTemplate->tmplate.rows[aCodePlan->tupleRowID];

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

    if ( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
         == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE )
    {
        // Index Tuple 위치의 결정
        aDataPlan->indexTuple =
            & aTemplate->tmplate.rows[aCodePlan->indexTupleRowID];

        // Cursor Property의 설정
        // Session Event 정보를 설정하기 위하여 복사해야 한다.
        SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &(aDataPlan->cursorProperty), NULL );

        aDataPlan->cursorProperty.mStatistics = aTemplate->stmt->mStatistics;
    }
    else
    {
        aDataPlan->indexTuple = NULL;
    }

    // fix BUG-9052
    aDataPlan->subQFilterDepCnt = 0;

    //---------------------------------
    // Predicate 종류의 초기화
    //---------------------------------

    // partition filter 영역의 할당

    IDE_TEST( allocPartitionFilter( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // Fixed Key Range 영역의 할당
    IDE_TEST( allocFixKeyRange( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // Fixed Key Filter 영역의 할당
    IDE_TEST( allocFixKeyFilter( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // Variable Key Range 영역의 할당
    IDE_TEST( allocVarKeyRange( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // Variable Key Filter 영역의 할당
    IDE_TEST( allocVarKeyFilter( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // Not Null Key Range 영역의 할당
    IDE_TEST( allocNotNullKeyRange( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    aDataPlan->partitionFilter = NULL;
    aDataPlan->keyRange = NULL;
    aDataPlan->keyFilter = NULL;

    //---------------------------------
    // Partitioned Table 관련 정보의 초기화
    //---------------------------------

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aCodePlan->tupleRowID )
              != IDE_SUCCESS );

    aDataPlan->nullRow = NULL;
    aDataPlan->curChildNo = 0;
    aDataPlan->selectedChildrenCount = 0;

    if( aCodePlan->selectedPartitionCount > 0 )
    {
        // children area를 생성.
        IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                      aCodePlan->selectedPartitionCount * ID_SIZEOF(qmnChildren*),
                      (void**)&aDataPlan->childrenArea )
                  != IDE_SUCCESS );

        // intersect count array create
        IDE_TEST( aTemplate->stmt->qmxMem->cralloc(
                      aCodePlan->selectedPartitionCount * ID_SIZEOF(UInt),
                      (void**)&aDataPlan->rangeIntersectCountArray)
                  != IDE_SUCCESS );

        if ( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
             == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE )
        {
            // children index를 생성.
            IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                          aCodePlan->selectedPartitionCount * ID_SIZEOF(qmnChildrenIndex),
                          (void**)&aDataPlan->childrenIndex )
                      != IDE_SUCCESS );
        }
        else
        {
            aDataPlan->childrenIndex = NULL;
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
         == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE )
    {
        // index table scan을 위한 row 할당
        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   & aTemplate->tmplate,
                                   aCodePlan->indexTupleRowID )
                  != IDE_SUCCESS );
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
    if ( aDataPlan->plan.myTuple->cursorInfo != NULL )
    {
        // DML로 생성된 노드인 경우
        sCursorInfo = (qmnCursorInfo*) aDataPlan->plan.myTuple->cursorInfo;

        aDataPlan->updateColumnList = sCursorInfo->updateColumnList;
        aDataPlan->cursorType       = sCursorInfo->cursorType;

        /* PROJ-2626 Snapshot Export */
        if ( aTemplate->stmt->mInplaceUpdateDisableFlag == ID_TRUE )
        {
            aDataPlan->inplaceUpdate = ID_FALSE;
        }
        else
        {
            aDataPlan->inplaceUpdate = sCursorInfo->inplaceUpdate;
        }

        aDataPlan->lockMode         = sCursorInfo->lockMode;

        aDataPlan->isRowMovementUpdate = sCursorInfo->isRowMovementUpdate;
    }
    else
    {
        aDataPlan->updateColumnList = NULL;
        aDataPlan->cursorType       = SMI_SELECT_CURSOR;
        aDataPlan->inplaceUpdate    = ID_FALSE;
        aDataPlan->lockMode         = SMI_LOCK_READ;

        aDataPlan->isRowMovementUpdate = ID_FALSE;
    }

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
        IDU_FIT_POINT( "qmnPCRD::firstInit::alloc::diskRow", idERR_ABORT_InsufficientMemory );
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

    *aDataPlan->flag &= ~QMND_PCRD_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_PCRD_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::printPredicateInfo( qcTemplate   * aTemplate,
                             qmncPCRD     * aCodePlan,
                             ULong          aDepth,
                             iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     Predicate의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

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

    // Key Range 정보의 출력
    IDE_TEST( printKeyRangeInfo( aTemplate,
                                 aCodePlan,
                                 aDepth,
                                 aString ) != IDE_SUCCESS );

    // Key Filter 정보의 출력
    IDE_TEST( printKeyFilterInfo( aTemplate,
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

IDE_RC
qmnPCRD::printPartKeyRangeInfo( qcTemplate   * aTemplate,
                                qmncPCRD     * aCodePlan,
                                ULong          aDepth,
                                iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     partition Key Range의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt i;

    // Fixed Key Range 출력
    if (aCodePlan->partitionKeyRange != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ PARTITION KEY ]\n" );
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

IDE_RC
qmnPCRD::printPartFilterInfo( qcTemplate   * aTemplate,
                              qmncPCRD     * aCodePlan,
                              ULong          aDepth,
                              iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     partition Filter의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt i;

    // Fixed Key Filter 출력
    if (aCodePlan->partitionFilter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ PARTITION FILTER ]\n" );
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

IDE_RC
qmnPCRD::printKeyRangeInfo( qcTemplate   * aTemplate,
                            qmncPCRD     * aCodePlan,
                            ULong          aDepth,
                            iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     Key Range의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt i;

    // Fixed Key Range 출력
    if (aCodePlan->fixKeyRange4Print != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ FIXED KEY ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->fixKeyRange4Print)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Variable Key Range 출력
    if (aCodePlan->varKeyRange != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

        // BUG-18300
        if ( ( aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK )
             == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE )
        {
            iduVarStringAppend( aString,
                                " [ IN-SUBQUERY VARIABLE KEY ]\n" );
        }
        else
        {
            iduVarStringAppend( aString,
                                " [ VARIABLE KEY ]\n" );
        }

        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->varKeyRange)
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

IDE_RC
qmnPCRD::printKeyFilterInfo( qcTemplate   * aTemplate,
                             qmncPCRD     * aCodePlan,
                             ULong          aDepth,
                             iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     Key Filter의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt i;

    // Fixed Key Filter 출력
    if (aCodePlan->fixKeyFilter4Print != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ FIXED KEY FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->fixKeyFilter4Print)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Variable Key Filter 출력
    if (aCodePlan->varKeyFilter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ VARIABLE KEY FILTER ]\n" );
        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          aCodePlan->varKeyFilter )
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


IDE_RC
qmnPCRD::printFilterInfo( qcTemplate   * aTemplate,
                          qmncPCRD     * aCodePlan,
                          ULong          aDepth,
                          iduVarString * aString )
{
/***********************************************************************
 *
 * Description :
 *     Filter의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt i;

    // Constant Filter 출력
    if (aCodePlan->constantFilter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ CONSTANT FILTER ]\n" );
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

    // Normal Filter 출력
    if (aCodePlan->filter != NULL)
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
                                          aCodePlan->filter)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Subquery Filter 출력
    if (aCodePlan->subqueryFilter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ SUBQUERY FILTER ]\n" );
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
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppend( aString,
                            " [ NOT-NORMAL-FORM FILTER ]\n" );
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

IDE_RC
qmnPCRD::allocPartitionFilter( qcTemplate * aTemplate,
                               qmncPCRD   * aCodePlan,
                               qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Partition Filter를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    iduMemory * sMemory;

    if ( aCodePlan->partitionFilter != NULL )
    {
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           aCodePlan->partitionFilter,
                                           & aDataPlan->partitionFilterSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->partitionFilterSize > 0 );

        sMemory = aTemplate->stmt->qmxMem;

        IDE_TEST( sMemory->cralloc( aDataPlan->partitionFilterSize,
                                    (void**) &aDataPlan->partitionFilterArea )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->partitionFilterSize = 0;
        aDataPlan->partitionFilterArea = NULL;
        aDataPlan->partitionFilter = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::allocFixKeyRange( qcTemplate * aTemplate,
                           qmncPCRD   * aCodePlan,
                           qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-1413
 *    Fixed Key Range를 위한 공간을 할당받는다.
 *    Fixed Key Range라 하더라도 plan 공유에 의해 fixed key range가
 *    공유되므로 variable key range와 마찬가지로 자신만의 공간에
 *    공유되지 않은 key range를 가져야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    iduMemory * sMemory;

    if ( aCodePlan->fixKeyRange != NULL )
    {
        IDE_DASSERT( aCodePlan->index != NULL );

        // Fixed Key Range의 크기 계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           aCodePlan->fixKeyRange,
                                           & aDataPlan->fixKeyRangeSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->fixKeyRangeSize > 0 );

        // Fixed Key Range를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;
        IDE_TEST( sMemory->cralloc( aDataPlan->fixKeyRangeSize,
                                    (void**) & aDataPlan->fixKeyRangeArea )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->fixKeyRangeSize = 0;
        aDataPlan->fixKeyRangeArea = NULL;
        aDataPlan->fixKeyRange = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::allocFixKeyFilter( qcTemplate * aTemplate,
                            qmncPCRD   * aCodePlan,
                            qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-1436
 *    Fixed Key Filter를 위한 공간을 할당받는다.
 *    Fixed Key Filter라 하더라도 plan 공유에 의해 fixed key filter가
 *    공유되므로 variable key filter와 마찬가지로 자신만의 공간에
 *    공유되지 않은 key filter를 가져야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    iduMemory * sMemory;

    if ( ( aCodePlan->fixKeyFilter != NULL ) &&
         ( aCodePlan->fixKeyRange != NULL ) )
    {
        IDE_DASSERT( aCodePlan->index != NULL );

        // Fixed Key Filter의 크기 계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           aCodePlan->fixKeyFilter,
                                           & aDataPlan->fixKeyFilterSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->fixKeyFilterSize > 0 );

        // Fixed Key Filter를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;
        IDE_TEST( sMemory->cralloc( aDataPlan->fixKeyFilterSize,
                                    (void**) & aDataPlan->fixKeyFilterArea )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->fixKeyFilterSize = 0;
        aDataPlan->fixKeyFilterArea = NULL;
        aDataPlan->fixKeyFilter = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::allocVarKeyRange( qcTemplate * aTemplate,
                           qmncPCRD   * aCodePlan,
                           qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Variable Key Range를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    iduMemory * sMemory;

    if ( aCodePlan->varKeyRange != NULL )
    {
        IDE_DASSERT( aCodePlan->index != NULL );

        // Variable Key Range의 크기 계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           aCodePlan->varKeyRange,
                                           & aDataPlan->varKeyRangeSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->varKeyRangeSize > 0 );

        // Variable Key Range를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;
        IDE_TEST( sMemory->cralloc( aDataPlan->varKeyRangeSize,
                                    (void**) & aDataPlan->varKeyRangeArea )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->varKeyRangeSize = 0;
        aDataPlan->varKeyRangeArea = NULL;
        aDataPlan->varKeyRange = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::allocVarKeyFilter( qcTemplate * aTemplate,
                            qmncPCRD   * aCodePlan,
                            qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Variable Key Filter를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    iduMemory * sMemory;

    if ( ( aCodePlan->varKeyFilter != NULL ) &&
         ( (aCodePlan->varKeyRange != NULL) ||
           (aCodePlan->fixKeyRange != NULL) ) ) // BUG-20679
    {
        IDE_DASSERT( aCodePlan->index != NULL );

        // Variable Key Filter의 크기 계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           aCodePlan->varKeyFilter,
                                           & aDataPlan->varKeyFilterSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->varKeyFilterSize > 0 );

        // Variable Key Filter를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;
        IDE_TEST( sMemory->cralloc( aDataPlan->varKeyFilterSize,
                                    (void**) & aDataPlan->varKeyFilterArea )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->varKeyFilterSize = 0;
        aDataPlan->varKeyFilterArea = NULL;
        aDataPlan->varKeyFilter = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::allocNotNullKeyRange( qcTemplate * aTemplate,
                               qmncPCRD   * aCodePlan,
                               qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    NotNull Key Range를 위한 공간을 할당받는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    iduMemory * sMemory;
    UInt        sSize;

    if ( ( (aCodePlan->flag & QMNC_SCAN_NOTNULL_RANGE_MASK) ==
           QMNC_SCAN_NOTNULL_RANGE_TRUE ) &&
         ( aCodePlan->fixKeyRange == NULL ) &&
         ( aCodePlan->varKeyRange == NULL ) )
    {
        // keyRange 적용을 위한 size 구하기
        IDE_TEST( mtk::estimateRangeDefault( NULL,
                                             NULL,
                                             0,
                                             &sSize )
                  != IDE_SUCCESS );

        IDE_DASSERT( sSize > 0 );

        // Fixed Key Range를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;
        IDE_TEST( sMemory->cralloc( sSize,
                                    (void**) & aDataPlan->notNullKeyRange )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->notNullKeyRange = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD *) (aTemplate->tmplate.data + aPlan->offset);

    qmnChildren         * sChild;
    qmndSCAN            * sChildDataScan;
    UInt                  i;

    // DML이 아닌 경우
    if ( sDataPlan->plan.myTuple->cursorInfo == NULL )
    {
        // Table에 IS Lock을 건다.
        IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aTemplate->stmt))->getTrans(),
                                            sCodePlan->table,
                                            sCodePlan->tableSCN,
                                            SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                            SMI_TABLE_LOCK_IS,
                                            ID_ULONG_MAX,
                                            ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                 != IDE_SUCCESS);
    }
    else
    {
        // BUG-42952 DML인 경우 IX Lock을 건다.
        IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aTemplate->stmt))->getTrans(),
                                            sCodePlan->table,
                                            sCodePlan->tableSCN,
                                            SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                            SMI_TABLE_LOCK_IX,
                                            ID_ULONG_MAX,
                                            ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                 != IDE_SUCCESS);
    }

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

    // PROJ-2205 rownum in DML
    // row movement update인 경우 first doIt시 child의 모든 cursor를 열어둔다.
    if ( sDataPlan->isRowMovementUpdate == ID_TRUE )
    {
        sDataPlan->isNeedAllFetchColumn = ID_TRUE;

        IDE_TEST( makeChildrenArea( aTemplate, aPlan )
                  != IDE_SUCCESS );

        // 모든 child plan의 cursor를 미리 전부 열어두어야 한다.
        for( i = 0; i < sDataPlan->selectedChildrenCount; i++ )
        {
            sChild = sDataPlan->childrenArea[i];

            IDE_TEST( sChild->childPlan->init( aTemplate,
                                               sChild->childPlan )
                      != IDE_SUCCESS );

            sChildDataScan =
                (qmndSCAN*) (aTemplate->tmplate.data + sChild->childPlan->offset);

            IDE_TEST( qmnSCAN::openCursorForPartition(
                          aTemplate,
                          (qmncSCAN*)(sChild->childPlan),
                          sChildDataScan )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        if( sCodePlan->partitionFilter != NULL )
        {
            // partition filter 구성
            IDE_TEST( makePartitionFilter( aTemplate,
                                           sCodePlan,
                                           sDataPlan ) != IDE_SUCCESS );

            // partition filter를 만들다가 실패할 수 있다.
            // host 변수의 경우 conversion등으로 안될 수 있음.
            // 이때는 filtering을 하지 않는다.
            if( sDataPlan->partitionFilter != NULL )
            {
                IDE_TEST( partitionFiltering( aTemplate,
                                              sCodePlan,
                                              sDataPlan )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( makeChildrenArea( aTemplate, aPlan )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( makeChildrenArea( aTemplate, aPlan )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < sDataPlan->selectedChildrenCount; i++ )
        {
            sChild = sDataPlan->childrenArea[i];
            IDE_TEST( sChild->childPlan->init( aTemplate,
                                               sChild->childPlan )
                      != IDE_SUCCESS );
        }
    }

    // 선택된 children이 있는 경우만 수행.
    if( sDataPlan->selectedChildrenCount > 0 )
    {
        if ( ( sCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
             == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE )
        {
            // make children index
            IDE_TEST( makeChildrenIndex( sDataPlan )
                      != IDE_SUCCESS );

            // KeyRange, KeyFilter, Filter 구성
            IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                             sCodePlan,
                                             sDataPlan ) != IDE_SUCCESS );

            // Cursor를 연다
            if ( ( *sDataPlan->flag & QMND_PCRD_INDEX_CURSOR_MASK )
                 == QMND_PCRD_INDEX_CURSOR_OPEN )
            {
                // 이미 열려 있는 경우
                IDE_TEST( restartIndexCursor( sCodePlan, sDataPlan )
                          != IDE_SUCCESS );
            }
            else
            {
                // 처음 여는 경우
                IDE_TEST( openIndexCursor( aTemplate, sCodePlan, sDataPlan )
                          != IDE_SUCCESS );
            }

            // Record를 획득한다.
            IDE_TEST( readRowWithIndex( aTemplate, sCodePlan, sDataPlan, aFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            sDataPlan->curChildNo = 0;

            // Record를 획득한다.
            IDE_TEST( readRow( aTemplate, sCodePlan, sDataPlan, aFlag )
                      != IDE_SUCCESS );
        }

        if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            sDataPlan->doIt = qmnPCRD::doItNext;
        }
        else
        {
            // Nothing To Do
        }
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

IDE_RC
qmnPCRD::makePartitionFilter( qcTemplate * aTemplate,
                              qmncPCRD   * aCodePlan,
                              qmndPCRD   * aDataPlan )
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

IDE_RC
qmnPCRD::readRow( qcTemplate * aTemplate,
                  qmncPCRD   * aCodePlan,
                  qmndPCRD   * aDataPlan,
                  qmcRowFlag * aFlag )
{
    idBool sJudge;

    qmnPlan       * sChildPlan;
    qmndSCAN      * sChildDataPlan;

    //--------------------------------------------------
    // Data가 존재할 때까지 모든 Child를 수행한다.
    //--------------------------------------------------

    while ( 1 )
    {
        //----------------------------
        // 현재 Child를 수행
        //----------------------------

        sChildPlan = aDataPlan->childrenArea[aDataPlan->curChildNo]->childPlan;

        sChildDataPlan = (qmndSCAN*)(aTemplate->tmplate.data +
                                         sChildPlan->offset );

        sChildDataPlan->isNeedAllFetchColumn = aDataPlan->isNeedAllFetchColumn;

        // PROJ-2002 Column Security
        // child의 doIt시에도 plan.myTuple을 참조할 수 있다.
        aDataPlan->plan.myTuple->columns = sChildDataPlan->plan.myTuple->columns;
        aDataPlan->plan.myTuple->partitionTupleID =
            ((qmncSCAN*)sChildPlan)->tupleRowID;

        // PROJ-1502 PARTITIONED DISK TABLE
        aDataPlan->plan.myTuple->tableHandle = (void *)((qmncSCAN*)sChildPlan)->table;

        /* PROJ-2464 hybrid partitioned table 지원 */
        aDataPlan->plan.myTuple->rowOffset  = sChildDataPlan->plan.myTuple->rowOffset;
        aDataPlan->plan.myTuple->rowMaximum = sChildDataPlan->plan.myTuple->rowMaximum;

        IDE_TEST( sChildPlan->doIt( aTemplate, sChildPlan, aFlag )
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

            aDataPlan->plan.myTuple->row = sChildDataPlan->plan.myTuple->row;
            aDataPlan->plan.myTuple->rid = sChildDataPlan->plan.myTuple->rid;
            aDataPlan->plan.myTuple->modify++;

            // judge등등..
            if( aCodePlan->subqueryFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      aCodePlan->subqueryFilter,
                                      aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                sJudge = ID_TRUE;
            }

            if ( aCodePlan->nnfFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      aCodePlan->nnfFilter,
                                      aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            if( sJudge == ID_TRUE )
            {
                *aFlag = QMC_ROW_DATA_EXIST;
                break;
            }
            else
            {
                // 굳이 flag를 none으로 세팅할 이유는 없으나,
                // 레코드가 없음을 강조.
                *aFlag = QMC_ROW_DATA_NONE;
            }
        }
        else
        {
            //----------------------------
            // 현재 Child에 Data가 없는 경우
            //----------------------------

            aDataPlan->curChildNo++;

            if ( aDataPlan->curChildNo == aDataPlan->selectedChildrenCount )
            {
                // 모든 Child를 수행한 경우
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::printAccessInfo( qmndPCRD     * aDataPlan,
                          iduVarString * aString,
                          qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *     Access 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_DASSERT( aDataPlan != NULL );
    IDE_DASSERT( aString   != NULL );

    if ( aMode == QMN_DISPLAY_ALL )
    {
        //----------------------------
        // explain plan = on; 인 경우
        //----------------------------

        // 수행 횟수 출력
        if ( (*aDataPlan->flag & QMND_PCRD_INIT_DONE_MASK)
             == QMND_PCRD_INIT_DONE_TRUE )
        {
            // fix BUG-9052
            iduVarStringAppendFormat( aString,
                                      ", ACCESS: %"ID_UINT32_FMT"",
                                      (aDataPlan->plan.myTuple->modify
                                       - aDataPlan->subQFilterDepCnt) );
        }
        else
        {
            iduVarStringAppend( aString,
                                ", ACCESS: 0" );
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------

        iduVarStringAppend( aString,
                            ", ACCESS: ??" );
    }

    return IDE_SUCCESS;
}

IDE_RC
qmnPCRD::partitionFiltering( qcTemplate * aTemplate,
                             qmncPCRD   * aCodePlan,
                             qmndPCRD   * aDataPlan )
{
    qmsTableRef * sTableRef;

    sTableRef = aCodePlan->tableRef;

    IDE_TEST( qmoPartition::partitionFilteringWithPartitionFilter(
                  aTemplate->stmt,
                  aCodePlan->rangeSortedChildrenArray,
                  aDataPlan->rangeIntersectCountArray,
                  aCodePlan->selectedPartitionCount,
                  aCodePlan->partitionCount,
                  aCodePlan->plan.children,
                  sTableRef->tableInfo->partitionMethod,
                  aDataPlan->partitionFilter,
                  aDataPlan->childrenArea,
                  &aDataPlan->selectedChildrenCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::makeChildrenArea( qcTemplate * aTemplate,
                           qmnPlan    * aPlan )
{
    qmncPCRD    * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD    * sDataPlan =
        (qmndPCRD *) (aTemplate->tmplate.data + aPlan->offset);

    qmnChildren * sChild;
    UInt          i;

    qmsTableRef * sTableRef;
    sTableRef = sCodePlan->tableRef;

    /* PROJ-2249 method range 이면 정렬된 children을 할당 한다. */
    if ( ( sTableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE ) ||
         ( sTableRef->tableInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH ) )
    {
        for ( i = 0; i < sDataPlan->selectedChildrenCount; i++ )
        {
            sDataPlan->childrenArea[i] = sCodePlan->rangeSortedChildrenArray[i].children;
        }
    }
    else
    {
        for ( i = 0,
                 sChild = sCodePlan->plan.children;
             ( i < sDataPlan->selectedChildrenCount ) &&
                 ( sChild != NULL );
             i++,
                 sChild = sChild->next )
        {
            sDataPlan->childrenArea[i] = sChild;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmnPCRD::makeChildrenIndex( qmndPCRD   * aDataPlan )
{
    qmncSCAN  * sChildPlan;
    UInt        i;

    for ( i = 0; i < aDataPlan->selectedChildrenCount; i++ )
    {
        sChildPlan = (qmncSCAN*) aDataPlan->childrenArea[i]->childPlan;

        aDataPlan->childrenIndex[i].childOID  =
            sChildPlan->partitionRef->partitionOID;
        aDataPlan->childrenIndex[i].childPlan =
            (qmnPlan*) sChildPlan;
    }

    if ( aDataPlan->selectedChildrenCount > 1 )
    {
        idlOS::qsort( aDataPlan->childrenIndex,
                      aDataPlan->selectedChildrenCount,
                      ID_SIZEOF(qmnChildrenIndex),
                      compareChildrenIndex );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

IDE_RC
qmnPCRD::storeCursor( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    현재 Cursor의 위치를 저장한다.
 *    Merge Join을 위해 사용된다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    // 적합성 검사
    IDE_DASSERT( ( sCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                 == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

    IDE_DASSERT( (*sDataPlan->flag & QMND_PCRD_INDEX_CURSOR_MASK)
                 == QMND_PCRD_INDEX_CURSOR_OPEN );

    // Index Cursor 정보를 저장함.
    IDE_TEST( sDataPlan->cursor.getCurPos( & sDataPlan->cursorInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::restoreCursor( qcTemplate * aTemplate,
                        qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    이미 저장된 Cursor를 이용하여 Cursor 위치를 복원시킴
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncPCRD * sCodePlan = (qmncPCRD*) aPlan;
    qmndPCRD * sDataPlan =
        (qmndPCRD*) (aTemplate->tmplate.data + aPlan->offset);

    void       * sOrgRow;
    void       * sIndexRow;
    qmcRowFlag   sRowFlag = QMC_ROW_INITIALIZE;

    // 적합성 검사
    IDE_DASSERT( ( sCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                 == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

    IDE_DASSERT( (*sDataPlan->flag & QMND_PCRD_INDEX_CURSOR_MASK)
                 == QMND_PCRD_INDEX_CURSOR_OPEN );

    //-----------------------------
    // Index Cursor의 복원
    //-----------------------------
    
    IDE_TEST( sDataPlan->cursor.setCurPos( & sDataPlan->cursorInfo )
              != IDE_SUCCESS );
    
    //-----------------------------
    // Index Record의 복원
    //-----------------------------

    // To Fix PR-8110
    // 저장된 Cursor를 복원하면, 저장 위치의 이전으로 이동한다.
    // 따라서, 단순히 Cursor를 Read하면 된다.
    sOrgRow = sIndexRow = sDataPlan->indexTuple->row;

    IDE_TEST(
        sDataPlan->cursor.readRow( (const void**) & sIndexRow,
                                   & sDataPlan->indexTuple->rid,
                                   SMI_FIND_NEXT )
        != IDE_SUCCESS );

    sDataPlan->indexTuple->row =
        (sIndexRow == NULL) ? sOrgRow : sIndexRow;

    // 반드시 저장된 Cursor에는 Row가 존재해야 한다.
    IDE_ASSERT( sIndexRow != NULL );

    //-----------------------------
    // Table Record의 복원
    //-----------------------------

    // Index Row로 Table Row 읽기
    IDE_TEST( readRowWithIndexRow( aTemplate,
                                   sDataPlan,
                                   & sRowFlag )
              != IDE_SUCCESS );

    // 반드시 존재해야한다.
    IDE_TEST_RAISE( ( sRowFlag & QMC_ROW_DATA_MASK )
                    == QMC_ROW_DATA_NONE,
                    ERR_ABORT_RECORD_NOT_FOUND );

    // 커서의 복원 후의 수행 함수
    sDataPlan->doIt = qmnPCRD::doItNext;

    // Modify값 증가, record가 변경된 경우임
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_RECORD_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnPCRD::restoreCursor",
                                  "record not found" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::makeKeyRangeAndFilter( qcTemplate * aTemplate,
                                qmncPCRD   * aCodePlan,
                                qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 열기 위한 Key Range, Key Filter, Filter를 구성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmnCursorPredicate  sPredicateInfo;

    //-------------------------------------
    // 적합성 검사
    //-------------------------------------

    IDE_DASSERT( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                 == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

    if ( ( aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK )
         == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE )
    {
        // IN Subquery Key Range가 있을 경우 다음 조건을 만족해야 함
        IDE_DASSERT( aCodePlan->fixKeyRange == NULL );
        IDE_DASSERT( aCodePlan->varKeyRange != NULL );
        IDE_DASSERT( aCodePlan->fixKeyFilter == NULL );
        IDE_DASSERT( aCodePlan->varKeyFilter == NULL );
    }

    //-------------------------------------
    // Predicate 정보의 구성
    //-------------------------------------

    sPredicateInfo.index = aCodePlan->index;
    sPredicateInfo.tupleRowID = aCodePlan->tupleRowID;  // partitioned table tuple id

    // Fixed Key Range 정보의 구성
    sPredicateInfo.fixKeyRangeArea = aDataPlan->fixKeyRangeArea;
    sPredicateInfo.fixKeyRange = aDataPlan->fixKeyRange;
    sPredicateInfo.fixKeyRangeOrg = aCodePlan->fixKeyRange;
    sPredicateInfo.fixKeyRangeSize = aDataPlan->fixKeyRangeSize;

    // Variable Key Range 정보의 구성
    sPredicateInfo.varKeyRangeArea = aDataPlan->varKeyRangeArea;
    sPredicateInfo.varKeyRange = aDataPlan->varKeyRange;
    sPredicateInfo.varKeyRangeOrg = aCodePlan->varKeyRange;
    sPredicateInfo.varKeyRange4FilterOrg = aCodePlan->varKeyRange4Filter;
    sPredicateInfo.varKeyRangeSize = aDataPlan->varKeyRangeSize;

    // Fixed Key Filter 정보의 구성
    sPredicateInfo.fixKeyFilterArea = aDataPlan->fixKeyFilterArea;
    sPredicateInfo.fixKeyFilter = aDataPlan->fixKeyFilter;
    sPredicateInfo.fixKeyFilterOrg = aCodePlan->fixKeyFilter;
    sPredicateInfo.fixKeyFilterSize = aDataPlan->fixKeyFilterSize;

    // Variable Key Filter 정보의 구성
    sPredicateInfo.varKeyFilterArea = aDataPlan->varKeyFilterArea;
    sPredicateInfo.varKeyFilter = aDataPlan->varKeyFilter;
    sPredicateInfo.varKeyFilterOrg = aCodePlan->varKeyFilter;
    sPredicateInfo.varKeyFilter4FilterOrg = aCodePlan->varKeyFilter4Filter;
    sPredicateInfo.varKeyFilterSize = aDataPlan->varKeyFilterSize;

    // Not Null Key Range 정보의 구성
    sPredicateInfo.notNullKeyRange = aDataPlan->notNullKeyRange;

    // Filter 정보의 구성
    sPredicateInfo.filter = aCodePlan->filter;

    sPredicateInfo.filterCallBack = & aDataPlan->callBack;
    sPredicateInfo.callBackDataAnd = & aDataPlan->callBackDataAnd;
    sPredicateInfo.callBackData = aDataPlan->callBackData;

    /* PROJ-2632 */
    sPredicateInfo.mSerialFilterInfo  = NULL;
    sPredicateInfo.mSerialExecuteData = NULL;

    //-------------------------------------
    // Key Range, Key Filter, Filter의 생성
    //-------------------------------------

    IDE_TEST( qmn::makeKeyRangeAndFilter( aTemplate,
                                          & sPredicateInfo )
              != IDE_SUCCESS );

    aDataPlan->keyRange = sPredicateInfo.keyRange;
    aDataPlan->keyFilter = sPredicateInfo.keyFilter;

    //-------------------------------------
    // IN SUBQUERY KEY RANGE 검사
    //-------------------------------------

    if ( ( (aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
           == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
         ( aDataPlan->keyRange == smiGetDefaultKeyRange() ) )
    {
        // IN SUBQUERY Key Range가 있는 경우라면
        // 더 이상 Record가 존재하지 않아 Key Range를 생성하지
        // 못하는 경우이다.  절대 Type Conversion등으로 인하여
        // Key Range를 생성하지 못하는 경우는 없다.
        // 따라서, 더 이상 record를 검색해서는 안된다.
        *aDataPlan->flag &= ~QMND_PCRD_INSUBQ_RANGE_BUILD_MASK;
        *aDataPlan->flag |= QMND_PCRD_INSUBQ_RANGE_BUILD_FAILURE;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::openIndexCursor( qcTemplate * aTemplate,
                          qmncPCRD   * aCodePlan,
                          qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 연다.
 *
 * Implementation :
 *    Cursor를 열기 위한 정보를 구성한다.
 *    Cursor를 열고 Cursor Manager에 등록한다.
 *    Cursor를 최초 위치로 이동시킨다.
 *
 ***********************************************************************/

    UInt   sTraverse;
    UInt   sPrevious;
    UInt   sCursorFlag;
    UInt   sInplaceUpdate;
    void * sOrgRow;

    smiFetchColumnList   * sFetchColumnList = NULL;
    qmnCursorInfo        * sCursorInfo;
    idBool                 sIsMutexLock = ID_FALSE;

    // 적합성 검사
    IDE_DASSERT( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                 == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

    if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
          == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
         ((*aDataPlan->flag & QMND_PCRD_INSUBQ_RANGE_BUILD_MASK)
          == QMND_PCRD_INSUBQ_RANGE_BUILD_FAILURE ) )
    {
        // Cursor를 열지 않는다.
        // 더 이상 IN SUBQUERY Key Range를 생성할 수 없는 경우로
        // record를 fetch하지 않아야 한다.
        // Nothing To Do
    }
    else
    {
        //-------------------------------------------------------
        //  Cursor를 위한 정보 구성
        //-------------------------------------------------------

        //---------------------------
        // Flag 정보의 구성
        //---------------------------

        // Traverse 방향의 결정
        if ( ( aCodePlan->flag & QMNC_SCAN_TRAVERSE_MASK )
             == QMNC_SCAN_TRAVERSE_FORWARD )
        {
            sTraverse = SMI_TRAVERSE_FORWARD;
        }
        else
        {
            sTraverse = SMI_TRAVERSE_BACKWARD;
        }

        // Previous 사용 여부 결정
        if ( ( aCodePlan->flag & QMNC_SCAN_PREVIOUS_ENABLE_MASK )
             == QMNC_SCAN_PREVIOUS_ENABLE_TRUE )
        {
            sPrevious = SMI_PREVIOUS_ENABLE;
        }
        else
        {
            sPrevious = SMI_PREVIOUS_DISABLE;
        }

        // PROJ-1509
        // inplace update 여부 결정
        // MEMORY table에서는,
        // trigger or foreign key가 있는 경우,
        // 갱신 이전/이후 값을 읽기 위해서는
        // inplace update가 되지 않도록 해야 한다.
        // 이 정보는 update cursor에서만 필요한 것이지만,
        // qp에서 이를 cursor별로 설정하면 코드가 복잡해져,
        // insert, update, delete cursor에 관계없이 정보를 sm으로 넘긴다.
        // < 참고 > sm팀과의 협의사항이며,
        //          sm에서 이 정보는 update cursor에서만
        //          사용되기 때문에 문제 없다고 함.
        if( aDataPlan->inplaceUpdate == ID_TRUE )
        {
            sInplaceUpdate = SMI_INPLACE_UPDATE_ENABLE;
        }
        else
        {
            sInplaceUpdate = SMI_INPLACE_UPDATE_DISABLE;
        }

        sCursorFlag =
            aDataPlan->lockMode | sTraverse | sPrevious | sInplaceUpdate;

        //-------------------------------------------------------
        //  Cursor를 연다.
        //-------------------------------------------------------
        
        // Cursor의 초기화
        aDataPlan->cursor.initialize();

        // PROJ-1705
        // 레코드패치시 복사되어야 할 컬럼정보생성
        IDE_DASSERT( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                     == QMN_PLAN_STORAGE_DISK );

        IDE_TEST( qdbCommon::makeFetchColumnList4TupleID(
                      aTemplate,
                      aCodePlan->indexTupleRowID,
                      ID_TRUE,
                      aCodePlan->indexTableIndex,
                      ID_TRUE,
                      & sFetchColumnList )
                  != IDE_SUCCESS );

        aDataPlan->cursorProperty.mFetchColumnList = sFetchColumnList;

        /* BUG-39836 : repeatable read에서 Disk DB에서  B-tree index가 설정된 
         * partition table에서 _PROWID Fetch를 수행 할 때 mLockRowBuffer 값이 
         * NULL이기 때문에 assert되어 natc에서 FATAL 발생. 
         * qmnSCAN::openCursor에서 다음의 주석을 참조하여 mLockRowBuffer에 
         * 값을 할당.  
         *
         * - select for update와 repeatable read 처리를 위해 sm에 qp에서 할당한 
         * 메모리영역을 내려준다.
         * sm에서 select for update와 repeatable read처리시 메모리 잠시 사용. 
         */
        aDataPlan->cursorProperty.mLockRowBuffer = (UChar*)aDataPlan->plan.myTuple->row;
        aDataPlan->cursorProperty.mLockRowBufferSize =
            aTemplate->tmplate.rows[aCodePlan->tupleRowID].rowOffset;       

        aDataPlan->cursorProperty.mIndexTypeID =
                (UChar)aCodePlan->indexTableIndex->indexTypeId;

        /* BUG-38290
         * Cursor open 은 동시성 제어가 필요하다.
         * Cursor 는 SM 에서 open 시 transaction 을 사용하는데,
         * transaction 은 동시성 제어가 고려되어 있지 않으므로
         * QP 에서 동시성 제어를 해야 한다.
         * 또, 이미 open 된 cursor 를 cursro manager 에 추가하는
         * addOpendCursor 함수도 동시성 제어를 해야 한다.
         *
         * 이는 cursor open 이 SCAN 과 PCRD, CUNT 등에서 사용되는데,
         * 특히 SCAN 노드가 parallel query 로 실행 시에 worker thread 에
         * 의해 동시에 실행될 가능성이 높기 때문이다.
         *
         * 따라서 동시성 제어를 위해 cursor manager 의 mutex 로 동시성을
         * 제어 한다.
         */
        IDE_TEST( aTemplate->stmt->mCursorMutex.lock(NULL) != IDE_SUCCESS );
        sIsMutexLock = ID_TRUE;

        IDE_TEST( aDataPlan->cursor.open( QC_SMI_STMT( aTemplate->stmt ),
                                          aCodePlan->indexTableHandle,
                                          aCodePlan->indexTableIndex->indexHandle,
                                          aCodePlan->indexTableSCN,
                                          aDataPlan->updateColumnList,
                                          aDataPlan->keyRange,
                                          aDataPlan->keyFilter,
                                          smiGetDefaultFilter(),    // filter는 직접 처리한다.
                                          sCursorFlag,
                                          aDataPlan->cursorType,
                                          & aDataPlan->cursorProperty,
                                          ID_FALSE )
                  != IDE_SUCCESS );

        // Cursor를 등록
        IDE_TEST( aTemplate->cursorMgr->addOpenedCursor(
                      aTemplate->stmt->qmxMem,
                      aCodePlan->indexTupleRowID,
                      & aDataPlan->cursor )
                  != IDE_SUCCESS );

        sIsMutexLock = ID_FALSE;
        IDE_TEST( aTemplate->stmt->mCursorMutex.unlock() != IDE_SUCCESS );

        // Cursor가 열렸음을 표기
        *aDataPlan->flag &= ~QMND_PCRD_INDEX_CURSOR_MASK;
        *aDataPlan->flag |= QMND_PCRD_INDEX_CURSOR_OPEN;

        //-------------------------------------------------------
        //  Cursor를 최초 위치로 이동
        //-------------------------------------------------------

        // Disk Table의 경우
        // Key Range 검색으로 인해 해당 row의 공간을 상실할 수 있다.
        // 이를 위해 저장 공간의 위치를 저장하고 이를 복원한다.
        sOrgRow = aDataPlan->indexTuple->row;
        
        IDE_TEST( aDataPlan->cursor.beforeFirst() != IDE_SUCCESS );
        
        aDataPlan->indexTuple->row = sOrgRow;

        //---------------------------------
        // cursor 정보 설정
        //---------------------------------

        // PROJ-2205 rownum in DML
        // row movement update인 경우
        if ( aDataPlan->plan.myTuple->cursorInfo != NULL )
        {
            // DML로 생성된 노드인 경우
            sCursorInfo = (qmnCursorInfo*) aDataPlan->plan.myTuple->cursorInfo;

            sCursorInfo->cursor = & aDataPlan->cursor;
            
            // selected index
            sCursorInfo->selectedIndex = aCodePlan->index;
            sCursorInfo->selectedIndexTuple = aDataPlan->indexTuple;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsMutexLock == ID_TRUE )
    {
        (void)aTemplate->stmt->mCursorMutex.unlock();
        sIsMutexLock = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::restartIndexCursor( qmncPCRD   * aCodePlan,
                             qmndPCRD   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 Restart한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void * sOrgRow;

    // 적합성 검사
    IDE_DASSERT( ( aCodePlan->flag & QMNC_SCAN_INDEX_TABLE_SCAN_MASK )
                 == QMNC_SCAN_INDEX_TABLE_SCAN_TRUE );

    if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
          == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
         ((*aDataPlan->flag & QMND_PCRD_INSUBQ_RANGE_BUILD_MASK)
          == QMND_PCRD_INSUBQ_RANGE_BUILD_FAILURE ) )
    {
        // Cursor를 열지 않는다.
        // 더 이상 IN SUBQUERY Key Range를 생성할 수 없는 경우로
        // record를 fetch하지 않아야 한다.
        // Nothing To Do
    }
    else
    {
        sOrgRow = aDataPlan->indexTuple->row;

        // Disk Table의 경우
        // Key Range 검색으로 인해 해당 row의 공간을 상실할 수 있다.
        // 이를 위해 저장 공간의 위치를 저장하고 이를 복원한다.
        IDE_TEST( aDataPlan->
                  cursor.restart( aDataPlan->keyRange,
                                  aDataPlan->keyFilter,
                                  smiGetDefaultFilter() )   // filter는 직접 처리한다.
                  != IDE_SUCCESS);
        
        IDE_TEST( aDataPlan->cursor.beforeFirst() != IDE_SUCCESS );

        aDataPlan->indexTuple->row = sOrgRow;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnPCRD::readRowWithIndex( qcTemplate * aTemplate,
                                  qmncPCRD   * aCodePlan,
                                  qmndPCRD   * aDataPlan,
                                  qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Index Cursor로부터 조건에 맞는 Record를 읽고
 *    OID, RID로 partition cursor에서 record를 읽는다.
 *
 * Implementation :
 *    Cursor를 이용하여 KeyRange, KeyFilter, Filter를 만족하는
 *    Record를 읽는다.
 *    이후, subquery filter를 적용하여 이를 만족하면 record를 리턴한다.
 *
 ***********************************************************************/

    idBool       sJudge;
    idBool       sResult;
    void       * sOrgRow;
    void       * sIndexRow;
    qmcRowFlag   sRowFlag = QMC_ROW_INITIALIZE;

    //----------------------------------------------
    // 1. Cursor를 이용하여 Record를 읽음
    // 2. Subquery Filter 적용
    // 3. Subquery Filter 를 만족하면 결과 리턴
    //----------------------------------------------

    sJudge = ID_FALSE;

    while ( sJudge == ID_FALSE )
    {
        //----------------------------------------
        // Index Cursor를 이용하여 Index Row를 얻음
        //----------------------------------------

        if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
              == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
             ((*aDataPlan->flag & QMND_PCRD_INSUBQ_RANGE_BUILD_MASK)
              == QMND_PCRD_INSUBQ_RANGE_BUILD_FAILURE ) )
        {
            // Subquery 결과의 부재로
            // IN SUBQUERY KEYRANGE를 위한 RANGE 생성을 실패한 경우
            sIndexRow = NULL;
        }
        else
        {
            sOrgRow = sIndexRow = aDataPlan->indexTuple->row;

            IDE_TEST(
                aDataPlan->cursor.readRow( (const void**) & sIndexRow,
                                           & aDataPlan->indexTuple->rid,
                                           SMI_FIND_NEXT )
                != IDE_SUCCESS );

            aDataPlan->indexTuple->row =
                (sIndexRow == NULL) ? sOrgRow : sIndexRow;
        }

        //----------------------------------------
        // IN SUBQUERY KEY RANGE시 재시도
        //----------------------------------------

        if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
              == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
             (sIndexRow == NULL) )
        {
            // IN SUBQUERY KEYRANGE인 경우 다시 시도한다.
            IDE_TEST( reReadIndexRow4InSubRange( aTemplate,
                                                 aCodePlan,
                                                 aDataPlan,
                                                 & sIndexRow )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        if ( sIndexRow == NULL )
        {
            sJudge = ID_FALSE;
            break;
        }
        else
        {
            //----------------------------------------
            // Index Row로 Table Row 읽기
            //----------------------------------------

            IDE_TEST( readRowWithIndexRow( aTemplate,
                                           aDataPlan,
                                           & sRowFlag )
                      != IDE_SUCCESS );

            if ( ( sRowFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
            {
                continue;
            }
            else
            {
                // Nothing to do.
            }

            //----------------------------------------
            // FILTER 처리
            //----------------------------------------

            if ( aDataPlan->callBack.callback != smiGetDefaultFilter()->callback )
            {
                IDE_TEST( qtc::smiCallBack(
                              & sResult,
                              aDataPlan->plan.myTuple->row,
                              NULL,
                              0,
                              aDataPlan->plan.myTuple->rid,
                              aDataPlan->callBack.data )
                          != IDE_SUCCESS );

                if ( sResult == ID_FALSE )
                {
                    continue;
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

            //----------------------------------------
            // SUBQUERY FILTER 처리
            //----------------------------------------

            // modify값 보정
            if ( aCodePlan->filter == NULL )
            {
                // modify값 증가.
                aDataPlan->plan.myTuple->modify++;
            }
            else
            {
                // SM에 의하여 filter적용 시 modify 값이 증가함.

                // fix BUG-9052 BUG-9248

                // BUG-9052
                // SELECT * FROM T1 WHERE T1.I2 IN ( SELECT MAX(T3.I2)
                //                       FROM T2, T3
                //                       WHERE T2.I1 = T3.I1
                //                       AND T3.I1 = T1.I1
                //                       GROUP BY T2.I2 ) AND T1.I1 > 0;
                // subquery filter가 outer column 참조시
                // outer column을 참조한 store and search를
                // 재수행하도록 하기 위해
                // aDataPlan->plan.myTuple->modify와
                // aDataPlan->subQFilterDepCnt의 값을 증가시킨다.
                // printPlan()내에서 ACCESS count display시
                // DataPlan->plan.myTuple->modify에서
                // DataPlan->subQFilterDepCnt 값을 빼주도록 한다.

                // BUG-9248
                // subquery filter이외에도 modify count값을 참조해서
                // 중간결과를 재저장해야하는 경우가 존재

                aDataPlan->plan.myTuple->modify++;
                aDataPlan->subQFilterDepCnt++;
            }

            // Subquery Filter를 적용
            if ( aCodePlan->subqueryFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      aCodePlan->subqueryFilter,
                                      aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                sJudge = ID_TRUE;
            }

            // NNF Filter를 적용
            if ( aCodePlan->nnfFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      aCodePlan->nnfFilter,
                                      aTemplate )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing To Do
            }
        }
    }

    if ( sJudge == ID_TRUE )
    {
        // 만족하는 Record 있음
        *aFlag = QMC_ROW_DATA_EXIST;
    }
    else
    {
        // 만족하는 Record 없음
        *aFlag = QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::reReadIndexRow4InSubRange( qcTemplate * aTemplate,
                                    qmncPCRD   * aCodePlan,
                                    qmndPCRD   * aDataPlan,
                                    void      ** aIndexRow )
{
/***********************************************************************
 *
 * Description :
 *     IN SUBQUERY KEYRANGE가 있을 경우 Record Read를 다시 시도한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void       * sIndexRow;
    void       * sOrgRow;

    sIndexRow = NULL;

    if ( (*aDataPlan->flag & QMND_PCRD_INSUBQ_RANGE_BUILD_MASK)
         == QMND_PCRD_INSUBQ_RANGE_BUILD_SUCCESS )
    {
        //---------------------------------------------------------
        // [IN SUBQUERY KEY RANGE]
        // 검색된 Record는 없으나, Subquery의 결과가 아직 존재하는 경우
        //     - Key Range를 다시 생성한다.
        //     - Record를 Fetch한다.
        //     - 위의 과정을 Fetch한 Record가 있거나,
        //       Key Range 생성을 실패할 때까지 반복한다.
        //---------------------------------------------------------

        while ( (sIndexRow == NULL) &&
                ((*aDataPlan->flag & QMND_PCRD_INSUBQ_RANGE_BUILD_MASK)
                 == QMND_PCRD_INSUBQ_RANGE_BUILD_SUCCESS ) )
        {
            // Key Range를 재생성한다.
            IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                             aCodePlan,
                                             aDataPlan ) != IDE_SUCCESS );


            // Cursor를 연다.
            // 이미 Open되어 있으므로, Restart한다.
            IDE_TEST( restartIndexCursor( aCodePlan,
                                          aDataPlan ) != IDE_SUCCESS );


            if ( (*aDataPlan->flag & QMND_PCRD_INSUBQ_RANGE_BUILD_MASK)
                 == QMND_PCRD_INSUBQ_RANGE_BUILD_SUCCESS )
            {
                // Key Range 생성을 성공한 경우 Record를 읽는다.
                sOrgRow = sIndexRow = aDataPlan->indexTuple->row;

                IDE_TEST(
                    aDataPlan->cursor.readRow( (const void**) & sIndexRow,
                                               & aDataPlan->indexTuple->rid,
                                               SMI_FIND_NEXT )
                    != IDE_SUCCESS );

                aDataPlan->indexTuple->row =
                    (sIndexRow == NULL) ? sOrgRow : sIndexRow;
            }
            else
            {
                sIndexRow = NULL;
            }
        }
    }
    else
    {
        // sIndexRow = NULL;
    }

    *aIndexRow = sIndexRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnPCRD::readRowWithIndexRow( qcTemplate * aTemplate,
                              qmndPCRD   * aDataPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     oid, rid가 기록된 index row로 table row를 읽는다.
 *
 * Implementation :
 *     - oid에 해당하는 child init
 *     - readRowFromGRID on child
 *
 ***********************************************************************/

    UInt               sColumnCount;
    mtcColumn        * sColumn;
    smOID              sPartOID;
    scGRID             sRowGRID;
    qmnChildrenIndex   sFindIndex;
    qmnChildrenIndex * sFound;
    qmncSCAN         * sChildPlan;
    qmndSCAN         * sChildDataPlan;

    //----------------------------
    // get oid, rid
    //----------------------------

    IDE_DASSERT( aDataPlan->indexTuple->columnCount >= 3 );

    sColumnCount = aDataPlan->indexTuple->columnCount;

    // oid
    sColumn = & aDataPlan->indexTuple->columns[sColumnCount - 2];
    sPartOID = *(smOID*)( (UChar*)aDataPlan->indexTuple->row + sColumn->column.offset );

    // rid
    sColumn = & aDataPlan->indexTuple->columns[sColumnCount - 1];
    sRowGRID = *(scGRID*)( (UChar*)aDataPlan->indexTuple->row + sColumn->column.offset );

    //----------------------------
    // get child with oid
    //----------------------------

    sFindIndex.childOID = sPartOID;

    sFound = (qmnChildrenIndex*) idlOS::bsearch( & sFindIndex,
                                                 aDataPlan->childrenIndex,
                                                 aDataPlan->selectedChildrenCount,
                                                 ID_SIZEOF(qmnChildrenIndex),
                                                 compareChildrenIndex );

    if ( sFound == NULL )
    {
        // 다음과 같은 경우 child는 없을 수 있다.
        // select * from t1 partition (p1) where i1=1;

        // 데이터 없음을 Setting
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        sChildPlan     = (qmncSCAN*) sFound->childPlan;
        sChildDataPlan = (qmndSCAN*)( aTemplate->tmplate.data + sChildPlan->plan.offset );

        //----------------------------
        // read row
        //----------------------------

        if ( ( *sChildDataPlan->flag & QMND_SCAN_CURSOR_MASK )
             == QMND_SCAN_CURSOR_CLOSED )
        {
            IDE_TEST( qmnSCAN::openCursorForPartition( aTemplate, sChildPlan, sChildDataPlan )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // grid로 row를 읽는다.
        IDE_TEST( qmnSCAN::readRowFromGRID( aTemplate, (qmnPlan*)sChildPlan, sRowGRID, aFlag )
                  != IDE_SUCCESS );

        // Index Row가 있다면 Table Row도 반드시 존재해야 한다.
        IDE_TEST_RAISE( ( *aFlag & QMC_ROW_DATA_MASK )
                        == QMC_ROW_DATA_NONE,
                        ERR_ABORT_RECORD_NOT_FOUND );

        // PROJ-2002 Column Security
        // child의 doIt시에도 plan.myTuple을 참조할 수 있다.
        aDataPlan->plan.myTuple->columns = sChildDataPlan->plan.myTuple->columns;
        aDataPlan->plan.myTuple->partitionTupleID = sChildPlan->tupleRowID;

        // PROJ-1502 PARTITIONED DISK TABLE
        aDataPlan->plan.myTuple->tableHandle = (void *)sChildPlan->table;

        /* PROJ-2464 hybrid partitioned table 지원 */
        aDataPlan->plan.myTuple->rowOffset  = sChildDataPlan->plan.myTuple->rowOffset;
        aDataPlan->plan.myTuple->rowMaximum = sChildDataPlan->plan.myTuple->rowMaximum;

        aDataPlan->plan.myTuple->row = sChildDataPlan->plan.myTuple->row;
        aDataPlan->plan.myTuple->rid = sChildDataPlan->plan.myTuple->rid;

        // 데이터 있음을 Setting
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ABORT_RECORD_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnPCRD::readRowWithIndexRow",
                                  "record not found" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
