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
 * $Id: qmnScan.cpp 91512 2021-08-21 07:50:50Z emlee $
 *
 * Description :
 *     SCAN Plan Node
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smiMisc.h>
#include <qcuProperty.h>
#include <qtc.h>
#include <qmoKeyRange.h>
#include <qmoUtil.h>
#include <qmx.h>
#include <qmnScan.h>
#include <qmo.h>
#include <qdbCommon.h>
#include <qcmPartition.h>
#include <qcuTemporaryObj.h>
#include <qcsModule.h>

extern mtfModule mtfOr;
extern mtfModule mtfEqual;
extern mtfModule mtfEqualAny;
extern mtfModule mtfList;
extern mtdModule mtdBigint;

IDE_RC
qmnSCAN::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SCAN 노드의 초기화
 *
 * Implementation :
 *    - 최초 초기화가 되지 않은 경우 최초 초기화 수행
 *    - Constant Filter의 수행 결과 검사
 *    - Constant Filter의 결과에 따른 수행 함수 결정
 *
 ***********************************************************************/

#define IDE_FN "qmnSCAN::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    idBool sJudge;

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnSCAN::doItDefault;
    sDataPlan->plan.mTID = QMN_PLAN_INIT_THREAD_ID;

    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( (*sDataPlan->flag & QMND_SCAN_INIT_DONE_MASK)
         == QMND_SCAN_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------
    // Constant Filter를 수행
    //------------------------------------------------

    /* TASK-7307 DML Data Consistency in Shard */
    if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) == ID_TRUE ) &&
         ( sCodePlan->tableRef->tableInfo->mIsUsable == ID_FALSE ) )
    {
        sJudge = ID_FALSE;
    }
    else if ( sCodePlan->method.constantFilter != NULL )
    {
        IDE_TEST( qtc::judge( & sJudge,
                              sCodePlan->method.constantFilter,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJudge = ID_TRUE;
    }

    //------------------------------------------------
    // Constant Filter에 따른 수행 함수 결정
    //------------------------------------------------

    if ( sJudge == ID_TRUE )
    {
        //---------------------------------
        // Constant Filter를 만족하는 경우
        //---------------------------------

        if ( ( sCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
             == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_FALSE )
        {
            // 수행 함수 결정
            sDataPlan->doIt = qmnSCAN::doItFirst;
        }
        else
        {
            sDataPlan->doIt = qmnSCAN::doItFirstFixedTable;
        }

        // Update, Delete를 위한 Flag 설정
        *sDataPlan->flag &= ~QMND_SCAN_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_SCAN_ALL_FALSE_FALSE;
    }
    else
    {
        //-------------------------------------------
        // Constant Filter를 만족하지 않는 경우
        // - 항상 조건을 만족하지 않으므로
        //   어떠한 결과도 리턴하지 않는 함수를 결정한다.
        //-------------------------------------------

        // 수행 함수 결정
        sDataPlan->doIt = qmnSCAN::doItAllFalse;

        // Update, Delete를 위한 Flag 설정
        *sDataPlan->flag &= ~QMND_SCAN_ALL_FALSE_MASK;
        *sDataPlan->flag |= QMND_SCAN_ALL_FALSE_TRUE;
    }

    //------------------------------------------------
    // 가변 Data Flag 의 초기화
    //------------------------------------------------

    // Subquery내부에 포함되어 있을 경우 초기화하여
    // In Subquery Key Range를 다시 생성할 수 있도록 한다.
    *sDataPlan->flag &= ~QMND_SCAN_INSUBQ_RANGE_BUILD_MASK;
    *sDataPlan->flag |= QMND_SCAN_INSUBQ_RANGE_BUILD_SUCCESS;

    // BUG-43721 IN-SUBQUERY KEY RANGE를 초기화가 안되어서 결과가 틀립니다.
    if ( ( sCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK )
         == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE )
    {
        resetExecInfo4Subquery( aTemplate, (qmnPlan*)sCodePlan );
    }
    else
    {
        // nothing to do.
    }

    if ( ( sCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
         == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_FALSE )
    {
        //------------------------------------------------
        // PR-24281
        // select for update의 경우 init시에 cursor를 미리
        // 열어 record lock을 획득
        //------------------------------------------------

        if ( ( sDataPlan->lockMode == SMI_LOCK_REPEATABLE ) &&
             ( sJudge == ID_TRUE ) )
        {
            /* BUG-41110 */
            IDE_TEST(makeRidRange(aTemplate, sCodePlan, sDataPlan)
                     != IDE_SUCCESS);

            // KeyRange, KeyFilter, Filter 구성
            IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                             sCodePlan,
                                             sDataPlan ) != IDE_SUCCESS );

            // Cursor를 연다
            if ( ( *sDataPlan->flag & QMND_SCAN_CURSOR_MASK )
                 == QMND_SCAN_CURSOR_OPEN )
            {
                // 이미 열려 있는 경우
                IDE_TEST( restartCursor( sCodePlan, sDataPlan )
                          != IDE_SUCCESS );
            }
            else
            {
                // 처음 여는 경우
                IDE_TEST( openCursor( aTemplate, sCodePlan, sDataPlan )
                          != IDE_SUCCESS );
            }

            // BUG-28533 SELECT FOR UPDATE의 경우 cursor를 미리 열어두었으므로
            // doItFirst를 처음 수행 시 cursor를 restart 할 필요 없음
            *sDataPlan->flag &= ~QMND_SCAN_RESTART_CURSOR_MASK;
            *sDataPlan->flag |= QMND_SCAN_RESTART_CURSOR_FALSE;
        }
        else
        {
            *sDataPlan->flag &= ~QMND_SCAN_RESTART_CURSOR_MASK;
            *sDataPlan->flag |= QMND_SCAN_RESTART_CURSOR_TRUE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-1071
    // addAccessCount 병목제거
    sDataPlan->mOrgModifyCnt      = sDataPlan->plan.myTuple->modify;
    sDataPlan->mOrgSubQFilterDepCnt = sDataPlan->subQFilterDepCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSCAN::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SCAN의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

    qmndSCAN * sDataPlan = (qmndSCAN*)(aTemplate->tmplate.data + aPlan->offset);
    UInt       sAddAccessCnt;

    IDE_TEST(sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS);

    if ((*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        sAddAccessCnt = ((sDataPlan->plan.myTuple->modify -
                          sDataPlan->mOrgModifyCnt) -
                         (sDataPlan->subQFilterDepCnt -
                          sDataPlan->mOrgSubQFilterDepCnt));

        // add access count
        (void)qmnSCAN::addAccessCount( (qmncSCAN*)aPlan,
                                       aTemplate,
                                       sAddAccessCnt);


        sDataPlan->mAccessCnt4Parallel += sAddAccessCnt;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnSCAN::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SCAN 에 해당하는 Null Row를 획득한다.
 *
 * Implementation :
 *    Disk Table인 경우
 *        - Null Row가 없다면, Null Row공간 할당 및 획득
 *        - Null Row가 있다면, 복사
 *    Memory Temp Table인 경우
 *        - Null Row획득
 *
 *    Modify 증가
 *
 ***********************************************************************/

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    // 초기화가 수행되지 않은 경우 초기화를 수행
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_SCAN_INIT_DONE_MASK)
         == QMND_SCAN_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_DISK )
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

            IDU_FIT_POINT( "qmnSCAN::padNull::cralloc::nullRow",
                            idERR_ABORT_InsufficientMemory );

            // Null Row를 위한 공간 할당
            IDE_TEST( aTemplate->stmt->qmxMem->cralloc(
                    sDataPlan->plan.myTuple->rowOffset,
                    (void**) & sDataPlan->nullRow ) != IDE_SUCCESS);

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
        if ( ( ( sCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_MASK ) ==
               QMNC_SCAN_REMOTE_TABLE_TRUE ) ||
             ( ( sCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_STORE_MASK ) ==
               QMNC_SCAN_REMOTE_TABLE_STORE_TRUE ) )
        {
            IDE_TEST( sDataPlan->cursor->getTableNullRow( (void **)&(sDataPlan->plan.myTuple->row),
                                                          &(sDataPlan->plan.myTuple->rid) )
                      != IDE_SUCCESS );
        }
        else
        {
            //-----------------------------------
            // Memory Table인 경우
            //-----------------------------------
            
            // NULL ROW를 가져온다.
            IDE_TEST( smiGetTableNullRow( sCodePlan->table,
                                          (void **) & sDataPlan->plan.myTuple->row,
                                          & sDataPlan->plan.myTuple->rid )
                      != IDE_SUCCESS );
        }
    }

    // Null Padding도 record가 변한 것임
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * -----------------------------------------------------------------------------
 * PROJ-2402 Parallel Table Scan
 *
 * doItFirst 에서 하는 동작중 전반부 (cursor open) 를 수행한다.
 * 이 부분까지는 serial 로 수행되어야 하고,
 * 이후 read row 부분은 parallel 하게 수행한다.
 * -----------------------------------------------------------------------------
 */
IDE_RC qmnSCAN::readyIt( qcTemplate * aTemplate,
                         qmnPlan    * aPlan,
                         UInt         aTID )
{
    qmncSCAN* sCodePlan = (qmncSCAN*)aPlan;
    qmndSCAN* sDataPlan = (qmndSCAN*)(aTemplate->tmplate.data + aPlan->offset);

    UInt       sModifyCnt;

    // ----------------
    // TID 설정
    // ----------------
    sDataPlan->plan.mTID = aTID;

    // 기존 ACCESS count
    sModifyCnt = sDataPlan->plan.myTuple->modify;

    // ----------------
    // Tuple 위치의 결정
    // ----------------
    sDataPlan->plan.myTuple = &aTemplate->tmplate.rows[sCodePlan->tupleRowID];

    // ACCESS count 원복
    sDataPlan->plan.myTuple->modify = sModifyCnt;

    // --------------------------------
    // PROJ-2444
    // parallel aggr 일때는 SCAN 의 row 를 PRLQ 에서 변경해 주지 않는다.
    // 따라서 새로운 row 버퍼를 할당 받아야 한다.
    // --------------------------------
    if ( sDataPlan->plan.mTID != QMN_PLAN_INIT_THREAD_ID )
    {

        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   &aTemplate->tmplate,
                                   sCodePlan->tupleRowID ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ((*sDataPlan->flag & QMND_SCAN_ALL_FALSE_MASK) ==
        QMND_SCAN_ALL_FALSE_FALSE)
    {
        // 비정상 종료 검사
        IDE_TEST(iduCheckSessionEvent(aTemplate->stmt->mStatistics)
                 != IDE_SUCCESS);

        // cursor가 이미 open되어 있고 doItFirst가 처음 호출되므로 restart 필요 없음
        if ((*sDataPlan->flag & QMND_SCAN_RESTART_CURSOR_MASK) ==
            QMND_SCAN_RESTART_CURSOR_FALSE)
        {
            // Nothing to do.
        }
        else
        {
            IDE_TEST(makeRidRange(aTemplate, sCodePlan, sDataPlan)
                     != IDE_SUCCESS);

            // KeyRange, KeyFilter, Filter 구성
            IDE_TEST(makeKeyRangeAndFilter(aTemplate, sCodePlan, sDataPlan)
                     != IDE_SUCCESS);

            // Cursor를 연다
            if ((*sDataPlan->flag & QMND_SCAN_CURSOR_MASK) ==
                QMND_SCAN_CURSOR_OPEN)
            {
                // 이미 열려 있는 경우
                IDE_TEST(restartCursor(sCodePlan, sDataPlan)
                         != IDE_SUCCESS);
            }
            else
            {
                // 처음 여는 경우
                IDE_TEST(openCursor(aTemplate, sCodePlan, sDataPlan)
                         != IDE_SUCCESS);
            }
        }

        // 다음 번 doItFirst 수행 시에는 cursor를 restart
        *sDataPlan->flag &= ~QMND_SCAN_RESTART_CURSOR_MASK;
        *sDataPlan->flag |= QMND_SCAN_RESTART_CURSOR_TRUE;

        sDataPlan->doIt = qmnSCAN::doItNext;
    }
    else
    {
        sDataPlan->doIt = qmnSCAN::doItAllFalse;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qmnSCAN::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, sCodePlan );

    if ( ( ( sCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_MASK ) ==
           QMNC_SCAN_REMOTE_TABLE_TRUE ) ||
        ( ( sCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_STORE_MASK ) ==
          QMNC_SCAN_REMOTE_TABLE_STORE_TRUE ) )
    {
        IDE_TEST( printRemotePlan( aPlan, aDepth, aString )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( printLocalPlan( aTemplate, aPlan, aDepth, aString, aMode )
                  != IDE_SUCCESS );
    }

    //----------------------------
    // Predicate 정보의 상세 출력
    //----------------------------

    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        IDE_TEST( printPredicateInfo( aTemplate,
                                      sCodePlan,
                                      aDepth,
                                      aString,
                                      aMode )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------
    // Subquery 정보의 출력
    // Subquery는 다음과 같은 predicate에만 존재할 수 있다.
    //     1. Variable Key Range
    //     2. Variable Key Filter
    //     3. Constant Filter
    //     4. Subquery Filter
    //----------------------------

    // Variable Key Range의 Subquery 정보 출력
    if ( sMethod->varKeyRange != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sMethod->varKeyRange,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    // Variable Key Filter의 Subquery 정보 출력
    if ( sMethod->varKeyFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sMethod->varKeyFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    // Constant Filter의 Subquery 정보 출력
    if ( sMethod->constantFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sMethod->constantFilter,
                                          aDepth,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }

    // Subquery Filter의 Subquery 정보 출력
    if ( sMethod->subqueryFilter != NULL )
    {
        IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                          sMethod->subqueryFilter,
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
    else
    {
        // Nothing to do.
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qmnSCAN::printRemotePlan( qmnPlan      * aPlan,
                                 ULong          aDepth,
                                 iduVarString * aString )
{
    UInt i = 0;
    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString, " " );
    }

    if ( ( sCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_MASK ) ==
         QMNC_SCAN_REMOTE_TABLE_TRUE )
    {
        iduVarStringAppend( aString, "SCAN ( REMOTE_TABLE ( " );
    }
    else
    {
        iduVarStringAppend( aString, "SCAN ( REMOTE_TABLE_STORE ( " );
    }

    iduVarStringAppend( aString, sCodePlan->databaseLinkName );
    iduVarStringAppend( aString, ", " );

    iduVarStringAppend( aString, sCodePlan->remoteQuery );
    iduVarStringAppend( aString, " )" );

    iduVarStringAppendFormat( aString, ", SELF_ID: %"ID_INT32_FMT" )",
                              (SInt)sCodePlan->tupleRowID );

    iduVarStringAppend( aString, "\n" );

    return IDE_SUCCESS;
}

IDE_RC
qmnSCAN::printLocalPlan( qcTemplate   * aTemplate,
                         qmnPlan      * aPlan,
                         ULong          aDepth,
                         iduVarString * aString,
                         qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    SCAN 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, sCodePlan );

    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    qcmIndex * sIndex;
    qcmIndex * sOrgIndex;

    //----------------------------
    // Display 위치 결정
    //----------------------------

    qmn::printSpaceDepth(aString, aDepth);

    if( ( sCodePlan->flag & QMNC_SCAN_FOR_PARTITION_MASK )
        == QMNC_SCAN_FOR_PARTITION_TRUE )
    {
        //----------------------------
        // Table Partition Name 출력
        //----------------------------

        iduVarStringAppend( aString,
                            "SCAN ( PARTITION: " );

        /* BUG-44520 미사용 Disk Partition의 SCAN Node를 출력하다가,
         *           Partition Name 부분에서 비정상 종료할 수 있습니다.
         *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
         *  SCAN Node에서 Partition Name을 보관하도록 수정합니다.
         */
        iduVarStringAppend( aString,
                            sCodePlan->partitionName );
    }
    else
    {
        //----------------------------
        // Table Owner Name 출력
        //----------------------------

        iduVarStringAppend( aString,
                            "SCAN ( TABLE: " );

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
            iduVarStringAppend( aString,
                                " " );

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
    }

    //----------------------------
    // Access Method 출력
    //----------------------------
    sIndex = sMethod->index;

    if ( sIndex != NULL )
    {
        // Index가 사용된 경우
        iduVarStringAppend( aString,
                            ", INDEX: " );

        // PROJ-1502 PARTITIONED DISK TABLE
        // local index partition의 이름을
        // partitioned index의 이름으로 출력하기 위함.
        if ( ( sCodePlan->flag & QMNC_SCAN_FOR_PARTITION_MASK )
             == QMNC_SCAN_FOR_PARTITION_TRUE )
        {
            /* BUG-44633 미사용 Disk Partition의 SCAN Node를 출력하다가,
             *           Index Name 부분에서 비정상 종료할 수 있습니다.
             *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
             *  SCAN Node에서 Index ID를 보관하도록 수정합니다.
             */
            IDE_TEST( qcmPartition::getPartIdxFromIdxId(
                          sCodePlan->partitionIndexId,
                          sCodePlan->tableRef,
                          &sOrgIndex )
                      != IDE_SUCCESS );

            iduVarStringAppend( aString,
                                sOrgIndex->userName );
            iduVarStringAppend( aString,
                                "." );
            iduVarStringAppend( aString,
                                sOrgIndex->name );
        }
        else
        {
            iduVarStringAppend( aString,
                                sIndex->userName );
            iduVarStringAppend( aString,
                                "." );
            iduVarStringAppend( aString,
                                sIndex->name );
        }

        // PROJ-2242 print full scan or range scan
        if ( (*sDataPlan->flag & QMND_SCAN_INIT_DONE_MASK)
             == QMND_SCAN_INIT_DONE_TRUE )
        {
            // IN SUBQUERY KEYRANGE가 있는 경우:
            // keyrange만들다 실패하면 fetch종료
            // 언제나 range scan 을 한다.
            if ( ( sCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK )
                 == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE )
            {
                (void) iduVarStringAppend( aString, ", RANGE SCAN" );
            }
            else
            {
                if ( sDataPlan->keyRange == smiGetDefaultKeyRange() )
                {
                    (void) iduVarStringAppend( aString, ", FULL SCAN" );
                }
                else
                {
                    (void) iduVarStringAppend( aString, ", RANGE SCAN" );
                }
            }
        }
        else
        {
            if ( ( sCodePlan->method.fixKeyRange == NULL ) &&
                 ( sCodePlan->method.varKeyRange == NULL ) )
            {
                iduVarStringAppend( aString, ", FULL SCAN" );
            }
            else
            {
                iduVarStringAppend( aString, ", RANGE SCAN" );
            }
        }

        // PROJ-2242 print index asc, desc
        if ( ( sCodePlan->flag & QMNC_SCAN_TRAVERSE_MASK )
             == QMNC_SCAN_TRAVERSE_FORWARD )
        {
            // nothing todo
            // asc 의 경우에는 출력하지 않는다.
        }
        else
        {
            iduVarStringAppend( aString, " DESC" );
        }
    }
    else
    {
        if ( ( ( sCodePlan->flag & QMNC_SCAN_FORCE_RID_SCAN_MASK )
               == QMNC_SCAN_FORCE_RID_SCAN_TRUE ) ||
             ( sMethod->ridRange != NULL ) )
        {
            // PROJ-1789 PROWID
            iduVarStringAppend(aString, ", RID SCAN");
        }
        else
        {
            // Full Scan인 경우
            iduVarStringAppend(aString, ", FULL SCAN");
        }
    }

    //----------------------------
    // 수행 횟수 출력
    //----------------------------

    IDE_TEST( printAccessInfo( sCodePlan,
                               sDataPlan,
                               aString,
                               aMode ) != IDE_SUCCESS );

    //----------------------------
    // PROJ-1071
    // Thread ID
    //----------------------------
    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( ( (*sDataPlan->flag & QMND_SCAN_INIT_DONE_MASK)
               == QMND_SCAN_INIT_DONE_TRUE ) &&
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

    //----------------------------
    // Plan ID 출력
    //----------------------------
    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        qmn::printSpaceDepth(aString, aDepth);

        // sCodePlan 의 값을 출력하기때문에 qmn::printMTRinfo을 사용하지 못한다.
        iduVarStringAppendFormat( aString,
                                  "[ SELF NODE INFO, "
                                  "SELF: %"ID_INT32_FMT" ]\n",
                                  (SInt)sCodePlan->tupleRowID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC
qmnSCAN::doItDefault( qcTemplate * /* aTemplate */,
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
qmnSCAN::doItAllFalse( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Filter 조건을 만족하는 Record가 하나도 없는 경우 사용
 *
 *    Constant Filter 검사후에 결정되는 함수로 절대 만족하는
 *    Record가 존재하지 않는다.
 *
 * Implementation :
 *    항상 record 없음을 리턴한다.
 *
 ***********************************************************************/

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    /* TASK-7307 DML Data Consistency in Shard */
    // 적합성 검사
    IDE_DASSERT( (sCodePlan->method.constantFilter != NULL) ||
                 ((QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) == ID_TRUE ) &&
                  (sCodePlan->tableRef->tableInfo->mIsUsable == ID_FALSE)) )
    IDE_DASSERT( ( *sDataPlan->flag & QMND_SCAN_ALL_FALSE_MASK )
                 == QMND_SCAN_ALL_FALSE_TRUE );

    // 데이터 없음을 Setting
    *aFlag &= ~QMC_ROW_DATA_MASK;
    *aFlag |= QMC_ROW_DATA_NONE;

    return IDE_SUCCESS;
}


IDE_RC qmnSCAN::doItFirst( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SCAN의 최초 수행 함수
 *    Cursor를 열고 최초 Record를 읽는다.
 *
 * Implementation :
 *    - Table 에 IS Lock을 건다.
 *    - Session Event Check (비정상 종료 Detect)
 *    - Key Range, Key Filter, Filter 구성
 *    - Cursor Open
 *    - Record 읽기
 *
 ***********************************************************************/

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST(readyIt(aTemplate, aPlan, QMN_PLAN_INIT_THREAD_ID) != IDE_SUCCESS);

    // table이거나 temporary table인 경우
    if ( ( *sDataPlan->flag & QMND_SCAN_CURSOR_MASK )
         == QMND_SCAN_CURSOR_OPEN )
    {
        // Record를 획득한다.
        IDE_TEST( readRow( aTemplate, sCodePlan, sDataPlan, aFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        // 만족하는 Record 없음
        *aFlag = QMC_ROW_DATA_NONE;
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sDataPlan->doIt = qmnSCAN::doItNext;
    }
    else
    {
        sDataPlan->doIt = qmnSCAN::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnSCAN::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SCAN의 다음 수행 함수
 *    다음 Record를 읽는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    if ( ( *sDataPlan->flag & QMND_SCAN_CURSOR_MASK )
         == QMND_SCAN_CURSOR_OPEN )
    {
        // 다음 Record를 읽는다.
        IDE_TEST( readRow( aTemplate, sCodePlan, sDataPlan, aFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        // 만족하는 Record 없음
        *aFlag = QMC_ROW_DATA_NONE;
    }
    
    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnSCAN::doItFirst;
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
qmnSCAN::storeCursor( qcTemplate * aTemplate,
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
    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    // 적합성 검사
    IDE_DASSERT( (*sDataPlan->flag & QMND_SCAN_CURSOR_MASK)
                 == QMND_SCAN_CURSOR_OPEN );

    IDE_TEST_RAISE( ( sCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
                    == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE, ERR_FIXED_TABLE );

    // Cursor 정보를 저장함.
    IDE_TEST( sDataPlan->cursor->getCurPos( & sDataPlan->cursorInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIXED_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSCAN::storeCursor",
                                  "Fixed Table" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSCAN::restoreCursor( qcTemplate * aTemplate,
                        qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    이미 저장된 Cursor를 이용하여 Cursor 위치를 복원시킴
 *
 * Implementation :
 *    Cursor의 복원 뿐 아니라 record 내용도 복원한다.
 *
 ***********************************************************************/
    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    void  * sOrgRow;
    void  * sSearchRow;

    // 적합성 검사
    IDE_DASSERT( (*sDataPlan->flag & QMND_SCAN_CURSOR_MASK)
                 == QMND_SCAN_CURSOR_OPEN );

    IDE_TEST_RAISE( ( sCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
                    == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE, ERR_FIXED_TABLE );

    //-----------------------------
    // Cursor의 복원
    //-----------------------------
    
    IDE_TEST( sDataPlan->cursor->setCurPos( & sDataPlan->cursorInfo )
              != IDE_SUCCESS );

    //-----------------------------
    // Record의 복원
    //-----------------------------

    // To Fix PR-8110
    // 저장된 Cursor를 복원하면, 저장 위치의 이전으로 이동한다.
    // 따라서, 단순히 Cursor를 Read하면 된다.
    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;

    IDE_TEST(
        sDataPlan->cursor->readRow( (const void**) & sSearchRow,
                                    &sDataPlan->plan.myTuple->rid,
                                    SMI_FIND_NEXT )
        != IDE_SUCCESS );

    sDataPlan->plan.myTuple->row =
        (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // 반드시 저장된 Cursor에는 Row가 존재하여야 한다.
    IDE_ASSERT( sSearchRow != NULL );

    // 커서의 복원 후의 수행 함수
    sDataPlan->doIt = qmnSCAN::doItNext;

    // Modify값 증가, record가 변경된 경우임
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FIXED_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSCAN::restoreCursor",
                                  "Fixed Table" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSCAN::makeKeyRangeAndFilter( qcTemplate * aTemplate,
                                qmncSCAN   * aCodePlan,
                                qmndSCAN   * aDataPlan )
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
    qmncScanMethod    * sMethod = getScanMethod( aTemplate, aCodePlan );

    //-------------------------------------
    // 적합성 검사
    //-------------------------------------

    if ( ( aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK )
         == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE )
    {
        // IN Subquery Key Range가 있을 경우 다음 조건을 만족해야 함
        IDE_DASSERT( sMethod->fixKeyRange == NULL );
        IDE_DASSERT( sMethod->varKeyRange != NULL );
        IDE_DASSERT( sMethod->fixKeyFilter == NULL );
        IDE_DASSERT( sMethod->varKeyFilter == NULL );
    }

    //-------------------------------------
    // Predicate 정보의 구성
    //-------------------------------------

    sPredicateInfo.index = sMethod->index;
    sPredicateInfo.tupleRowID = aCodePlan->tupleRowID;

    // Fixed Key Range 정보의 구성
    sPredicateInfo.fixKeyRangeArea = aDataPlan->fixKeyRangeArea;
    sPredicateInfo.fixKeyRange = aDataPlan->fixKeyRange;
    sPredicateInfo.fixKeyRangeOrg = sMethod->fixKeyRange;
    sPredicateInfo.fixKeyRangeSize = aDataPlan->fixKeyRangeSize;

    // Variable Key Range 정보의 구성
    sPredicateInfo.varKeyRangeArea = aDataPlan->varKeyRangeArea;
    sPredicateInfo.varKeyRange = aDataPlan->varKeyRange;
    sPredicateInfo.varKeyRangeOrg = sMethod->varKeyRange;
    sPredicateInfo.varKeyRange4FilterOrg = sMethod->varKeyRange4Filter;
    sPredicateInfo.varKeyRangeSize = aDataPlan->varKeyRangeSize;

    // Fixed Key Filter 정보의 구성
    sPredicateInfo.fixKeyFilterArea = aDataPlan->fixKeyFilterArea;
    sPredicateInfo.fixKeyFilter = aDataPlan->fixKeyFilter;
    sPredicateInfo.fixKeyFilterOrg = sMethod->fixKeyFilter;
    sPredicateInfo.fixKeyFilterSize = aDataPlan->fixKeyFilterSize;

    // Variable Key Filter 정보의 구성
    sPredicateInfo.varKeyFilterArea = aDataPlan->varKeyFilterArea;
    sPredicateInfo.varKeyFilter = aDataPlan->varKeyFilter;
    sPredicateInfo.varKeyFilterOrg = sMethod->varKeyFilter;
    sPredicateInfo.varKeyFilter4FilterOrg = sMethod->varKeyFilter4Filter;
    sPredicateInfo.varKeyFilterSize = aDataPlan->varKeyFilterSize;

    // Not Null Key Range 정보의 구성
    sPredicateInfo.notNullKeyRange = aDataPlan->notNullKeyRange;

    // Filter 정보의 구성
    sPredicateInfo.filter = sMethod->filter;

    sPredicateInfo.filterCallBack = & aDataPlan->callBack;
    sPredicateInfo.callBackDataAnd = & aDataPlan->callBackDataAnd;
    sPredicateInfo.callBackData = aDataPlan->callBackData;

    /* PROJ-2632 */
    sPredicateInfo.mSerialFilterInfo  = aCodePlan->mSerialFilterInfo;
    sPredicateInfo.mSerialExecuteData = aDataPlan->mSerialExecuteData;

    //-------------------------------------
    // Key Range, Key Filter, Filter의 생성
    //-------------------------------------

    IDE_TEST( qmn::makeKeyRangeAndFilter( aTemplate,
                                          & sPredicateInfo )
              != IDE_SUCCESS );

    /* PROJ-2632 */
    if ( sPredicateInfo.mSerialExecuteData != NULL )
    {
        *aDataPlan->flag &= ~QMND_SCAN_SERIAL_EXECUTE_MASK;
        *aDataPlan->flag |= QMND_SCAN_SERIAL_EXECUTE_TRUE;
    }
    else
    {
        *aDataPlan->flag &= ~QMND_SCAN_SERIAL_EXECUTE_MASK;
        *aDataPlan->flag |= QMND_SCAN_SERIAL_EXECUTE_FALSE;
    }

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
        *aDataPlan->flag &= ~QMND_SCAN_INSUBQ_RANGE_BUILD_MASK;
        *aDataPlan->flag |= QMND_SCAN_INSUBQ_RANGE_BUILD_FAILURE;
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
qmnSCAN::firstInit( qcTemplate * aTemplate,
                    qmncSCAN   * aCodePlan,
                    qmndSCAN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    SCAN node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *    - Data 영역의 주요 멤버에 대한 초기화를 수행
 *
 ***********************************************************************/

    qmnCursorInfo  * sCursorInfo;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    //--------------------------------
    // 적합성 검사
    //--------------------------------

    // Key Range는 Fixed KeyRange와 Variable KeyRange가 혼용될 수 없다.
    IDE_DASSERT( (sMethod->fixKeyRange == NULL) ||
                 (sMethod->varKeyRange == NULL) );

    // Key Filter는 Fixed KeyFilter와 Variable KeyFilter가 혼용될 수 없다.
    IDE_DASSERT( (sMethod->fixKeyFilter == NULL) ||
                 (sMethod->varKeyFilter == NULL) );

    //---------------------------------
    // SCAN 고유 정보의 초기화
    //---------------------------------

    // Tuple 위치의 결정
    aDataPlan->plan.myTuple =
        & aTemplate->tmplate.rows[aCodePlan->tupleRowID];

    // PROJ-1382, jhseong, FixedTable and PerformanceView
    // FIX BUG-12167
    if ( ( aCodePlan->flag & QMNC_SCAN_TABLE_FV_MASK )
         == QMNC_SCAN_TABLE_FV_FALSE )
    {
        if ( aDataPlan->plan.myTuple->cursorInfo == NULL )
        {
            // Table에 IS Lock을 건다.
            IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aTemplate->stmt))->getTrans(),
                                                aCodePlan->table,
                                                aCodePlan->tableSCN,
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
                                                aCodePlan->table,
                                                aCodePlan->tableSCN,
                                                SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                SMI_TABLE_LOCK_IX,
                                                ID_ULONG_MAX,
                                                ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                     != IDE_SUCCESS);
        }
    }
    else
    {
        // do nothing
    }

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

    // Cursor Property의 설정
    // Session Event 정보를 설정하기 위하여 복사해야 한다.

    idlOS::memcpy( & aDataPlan->cursorProperty,
                   & aCodePlan->cursorProperty,
                   ID_SIZEOF( smiCursorProperties ) );
    aDataPlan->cursorProperty.mStatistics =
        aTemplate->stmt->mStatistics;

    // BUG-10146 limit 절에 host variable 허용
    // aCodePlan의 limit 정보를 가지고 cursorProperties를 세팅한다.
    if( aCodePlan->limit != NULL )
    {
        IDE_TEST( qmsLimitI::getStartValue(
                      aTemplate,
                      aCodePlan->limit,
                      &aDataPlan->cursorProperty.mFirstReadRecordPos )
                  != IDE_SUCCESS );

        // limit의 start는 1부터 시작하지만,
        // recordPosition은 0부터 시작한다.
        aDataPlan->cursorProperty.mFirstReadRecordPos--;

        IDE_TEST( qmsLimitI::getCountValue(
                      aTemplate,
                      aCodePlan->limit,
                      &aDataPlan->cursorProperty.mReadRecordCount )
                  != IDE_SUCCESS );

    }

    // PROJ-1071
    IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF(smiTableCursor),
                                              (void**) & aDataPlan->cursor )
              != IDE_SUCCESS);

    // fix BUG-9052
    aDataPlan->subQFilterDepCnt = 0;

    //---------------------------------
    // Predicate 종류의 초기화
    //---------------------------------

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

    // PROJ-1789 PROWID
    IDE_TEST(allocRidRange(aTemplate, aCodePlan, aDataPlan) != IDE_SUCCESS);

    aDataPlan->keyRange = NULL;
    aDataPlan->keyFilter = NULL;

    //---------------------------------
    // Disk Table 관련 정보의 초기화
    //---------------------------------

    // [Disk Table인 경우]
    //   Record를 읽기 위한 공간 마련,
    //   Variable Column의 value pointer지정
    //   등의 작업을 다음 함수를 통하여 처리한다.
    // [Memory Table의 경우]
    //   Variable Column의 value pointer지정
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aCodePlan->tupleRowID ) != IDE_SUCCESS );

    // Disk Table인 경우
    //   Null Row를 위한 공간으로 빈번한 disk I/O를 방지하기
    //   위하여 해당 영역에 복사하여 처리한다.
    //   최초 호출 시 memory를 할당받아 SM로부터 null row를 얻어 온다.
    aDataPlan->nullRow = NULL;

    //---------------------------------
    // Trigger를 위한 공간을 마련
    //---------------------------------

    aDataPlan->isNeedAllFetchColumn = ID_FALSE;

    //---------------------------------
    // cursor 정보 설정
    //---------------------------------

    if ( aDataPlan->plan.myTuple->cursorInfo != NULL )
    {
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

        if ( sCursorInfo->isRowMovementUpdate == ID_TRUE )
        {
            aDataPlan->isNeedAllFetchColumn = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // DML이 아닌 경우 (select, select for update, dequeue)

        aDataPlan->updateColumnList = NULL;
        aDataPlan->cursorType       = SMI_SELECT_CURSOR;
        aDataPlan->inplaceUpdate    = ID_FALSE;
        aDataPlan->lockMode         = aCodePlan->lockMode;  // 항상 SMI_LOCK_READ는 아니다.
    }

    /* PROJ-2402 */
    aDataPlan->mAccessCnt4Parallel = 0;

    /* BUG-42639 Monitoring query */
    if ( ( aCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
         == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE )
    {
        aDataPlan->fixedTable.recRecord   = NULL;
        aDataPlan->fixedTable.traversePtr = NULL;
        SMI_FIXED_TABLE_PROPERTIES_INIT( &aDataPlan->fixedTableProperty );
    
        IDE_TEST_RAISE( ( aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK )
                          == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE, UNEXPECTED_ERROR );
    }
    else
    {
        /* Nothing to do */
    }


    /* PROJ-2632 */
    if ( aCodePlan->mSerialFilterInfo != NULL )
    {
        QTC_SERIAL_EXEUCTE_DATA_INITIALIZE( aDataPlan->mSerialExecuteData,
                                            aTemplate->tmplate.data,
                                            aCodePlan->mSerialFilterSize,
                                            aCodePlan->mSerialFilterCount,
                                            aCodePlan->mSerialFilterOffset );
    }
    else
    {
        aDataPlan->mSerialExecuteData = NULL;
    }

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_SCAN_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_SCAN_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSCAN::firstInit",
                                  "The fixed table has insubquery key range" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSCAN::allocFixKeyRange( qcTemplate * aTemplate,
                           qmncSCAN   * aCodePlan,
                           qmndSCAN   * aDataPlan )
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    if ( sMethod->fixKeyRange != NULL )
    {
        IDE_DASSERT( sMethod->index != NULL );

        // Fixed Key Range의 크기 계산
        IDE_TEST( qmoKeyRange::estimateKeyRange( aTemplate,
                                                 sMethod->fixKeyRange,
                                                 & aDataPlan->fixKeyRangeSize )
                  != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->fixKeyRangeSize > 0 );
        // Fixed Key Range를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;

        IDU_FIT_POINT( "qmnSCAN::allocFixKeyRange::cralloc::fixKeyRangeArea",
                        idERR_ABORT_InsufficientMemory );

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
qmnSCAN::allocFixKeyFilter( qcTemplate * aTemplate,
                            qmncSCAN   * aCodePlan,
                            qmndSCAN   * aDataPlan )
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    if ( ( sMethod->fixKeyFilter != NULL ) &&
         ( sMethod->fixKeyRange != NULL ) )
    {
        IDE_DASSERT( sMethod->index != NULL );

        // Fixed Key Filter의 크기 계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           sMethod->fixKeyFilter,
                                           & aDataPlan->fixKeyFilterSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->fixKeyFilterSize > 0 );

        // Fixed Key Filter를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;

        IDU_FIT_POINT( "qmnSCAN::allocFixKeyFilter::cralloc::fixKeyFilterArea",
                        idERR_ABORT_InsufficientMemory );

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
qmnSCAN::allocVarKeyRange( qcTemplate * aTemplate,
                           qmncSCAN   * aCodePlan,
                           qmndSCAN   * aDataPlan )
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    if ( sMethod->varKeyRange != NULL )
    {
        IDE_DASSERT( sMethod->index != NULL );

        // Variable Key Range의 크기 계산
        IDE_TEST( qmoKeyRange::estimateKeyRange( aTemplate,
                                                 sMethod->varKeyRange,
                                                 & aDataPlan->varKeyRangeSize )
                  != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->varKeyRangeSize > 0 );

        // Variable Key Range를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;

        IDU_FIT_POINT( "qmnSCAN::allocVarKeyRange::cralloc::varKeyRangeArea",
                        idERR_ABORT_InsufficientMemory );

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
qmnSCAN::allocVarKeyFilter( qcTemplate * aTemplate,
                            qmncSCAN   * aCodePlan,
                            qmndSCAN   * aDataPlan )
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    if ( ( sMethod->varKeyFilter != NULL ) &&
         ( (sMethod->varKeyRange != NULL) ||
           (sMethod->fixKeyRange != NULL) ) ) // BUG-20679
    {
        IDE_DASSERT( sMethod->index != NULL );

        // Variable Key Filter의 크기 계산
        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTemplate,
                                           sMethod->varKeyFilter,
                                           & aDataPlan->varKeyFilterSize )
            != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->varKeyFilterSize > 0 );

        // Variable Key Filter를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;
        IDU_LIMITPOINT("qmnSCAN::allocVarKeyFilter::malloc");
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
qmnSCAN::allocNotNullKeyRange( qcTemplate * aTemplate,
                               qmncSCAN   * aCodePlan,
                               qmndSCAN   * aDataPlan )
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    if ( ( (aCodePlan->flag & QMNC_SCAN_NOTNULL_RANGE_MASK) ==
           QMNC_SCAN_NOTNULL_RANGE_TRUE ) &&
         ( sMethod->fixKeyRange == NULL ) &&
         ( sMethod->varKeyRange == NULL ) )
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

        IDU_FIT_POINT( "qmnSCAN::allocNotNullKeyRange::cralloc::notNullKeyRange",
                        idERR_ABORT_InsufficientMemory );

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
qmnSCAN::openCursor( qcTemplate * aTemplate,
                     qmncSCAN   * aCodePlan,
                     qmndSCAN   * aDataPlan )
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

    const void            * sTableHandle = NULL;
    const void            * sIndexHandle = NULL;
    smSCN                   sTableSCN;
    UInt                    sTraverse;
    UInt                    sPrevious;
    UInt                    sCursorFlag;
    UInt                    sInplaceUpdate;
    void                  * sOrgRow;
    qmncScanMethod        * sMethod = getScanMethod( aTemplate, aCodePlan );
    idBool                  sIsDequeue = ID_FALSE;
    smiFetchColumnList    * sFetchColumnList = NULL;
    smiRange              * sRange; // KeyRange or RIDRange
    qmnCursorInfo         * sCursorInfo;
    idBool                  sIsMutexLock = ID_FALSE;
    smSCN                   sBaseTableSCN;
    qcStatement           * sStmt   = aTemplate->stmt;

    if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
          == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
         ((*aDataPlan->flag & QMND_SCAN_INSUBQ_RANGE_BUILD_MASK)
          == QMND_SCAN_INSUBQ_RANGE_BUILD_FAILURE ) )
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

        //---------------------------
        // Index Handle의 획득
        //---------------------------

        if (sMethod->index != NULL)
        {
            sIndexHandle = sMethod->index->indexHandle;
        }
        else
        {
            sIndexHandle = NULL;
        }

        // PROJ-1407 Temporary Table
        if ( qcuTemporaryObj::isTemporaryTable(
                 aCodePlan->tableRef->tableInfo ) == ID_TRUE )
        {
            qcuTemporaryObj::getTempTableHandle( sStmt,
                                                 aCodePlan->tableRef->tableInfo,
                                                 &sTableHandle,
                                                 &sBaseTableSCN );
            
            IDE_TEST_CONT( sTableHandle == NULL, NORMAL_EXIT_EMPTY );

            IDE_TEST_RAISE( !SM_SCN_IS_EQ( &(aCodePlan->tableRef->tableSCN), &sBaseTableSCN ),
                            ERR_TEMPORARY_TABLE_EXIST );

            // Session Temp Table이 존재하는 경우
            sTableSCN = smiGetRowSCN( sTableHandle );

            if( sIndexHandle != NULL )
            {
                sIndexHandle = qcuTemporaryObj::getTempIndexHandle(
                    sStmt,
                    aCodePlan->tableRef->tableInfo,
                    sMethod->index->indexId );
                // 반드시 존재하여야 한다.
                IDE_ASSERT( sIndexHandle != NULL );
            }
        }
        else
        {
            sTableHandle = aCodePlan->table;
            sTableSCN    = aCodePlan->tableSCN;
        }

        //-------------------------------------------------------
        //  Cursor를 연다.
        //-------------------------------------------------------

        // Cursor의 초기화
        aDataPlan->cursor->initialize();

        // PROJ-1618
        aDataPlan->cursor->setDumpObject( aCodePlan->dumpObject );

        if ( ( (aCodePlan->flag & QMNC_SCAN_TABLE_QUEUE_MASK)
             == QMNC_SCAN_TABLE_QUEUE_TRUE ) && 
             ( smiIsAllowDeleteQueue(aCodePlan->table)  
             == ID_FALSE ) && 
             ( ( (aCodePlan->flag & QMNC_SCAN_MOVE_AND_DELETE_MASK) /*BUG-49127*/
             == QMNC_SCAN_MOVE_AND_DELETE_FALSE ) ) )
        {
            //BUG-48230: DEQUEUE 성능 개선
            //BUG-49063: delete off 상태인 QUEUE 만 BUG-48230 적용으로 수행
            sIsDequeue = ID_TRUE;
        }
        else
        {
            sIsDequeue = ID_FALSE;
        }

        // PROJ-1705
        // 레코드패치시 복사되어야 할 컬럼정보생성
        if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
             == QMN_PLAN_STORAGE_DISK )
        {
            IDE_TEST( qdbCommon::makeFetchColumnList4TupleID(
                          aTemplate,
                          aCodePlan->tupleRowID,
                          aDataPlan->isNeedAllFetchColumn,
                          ( sIndexHandle != NULL ) ? sMethod->index: NULL,
                          ID_TRUE,
                          & sFetchColumnList ) != IDE_SUCCESS );

            aDataPlan->cursorProperty.mFetchColumnList = sFetchColumnList;

            // select for update와 repeatable read 처리를 위해
            // sm에 qp에서 할당한 메모리영역을 내려준다.
            // sm에서 select for update와 repeatable read처리시 메모리 잠시 사용.
            aDataPlan->cursorProperty.mLockRowBuffer = (UChar*)aDataPlan->plan.myTuple->row;
            aDataPlan->cursorProperty.mLockRowBufferSize =
                        aTemplate->tmplate.rows[aCodePlan->tupleRowID].rowOffset;
        }
        else
        {
            // Nothing To Do
        }

        /*
         * SMI_CURSOR_PROP_INIT 을 사용하지 않고
         * mIndexTypeID 만 따로 setting 하는 이유는
         * 위에는 mFetchColumnList, mLockRowBuffer 를 변경하기 때문이다.
         * SMI_CURSOR_PROP_INIT 을 사용하면 다시 다 초기화 되므로
         */
        if (sMethod->ridRange != NULL)
        {
            aDataPlan->cursorProperty.mIndexTypeID =
                SMI_BUILTIN_GRID_INDEXTYPE_ID;
            sRange = aDataPlan->ridRange;
        }
        else
        {
            if ( sIndexHandle == NULL )
            {
                aDataPlan->cursorProperty.mIndexTypeID =
                    SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID;
            }
            else
            {
                aDataPlan->cursorProperty.mIndexTypeID =
                    (UChar)sMethod->index->indexTypeId;
            }

            sRange = aDataPlan->keyRange;
        }

        /* PROJ-1832 New database link */
        if ( ( ( aCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_MASK ) ==
                QMNC_SCAN_REMOTE_TABLE_TRUE ) ||
             ( ( aCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_STORE_MASK ) ==
               QMNC_SCAN_REMOTE_TABLE_STORE_TRUE ) )
        {
            aDataPlan->cursorProperty.mRemoteTableParam.mQcStatement = sStmt;
            aDataPlan->cursorProperty.mRemoteTableParam.mDkiSession =
                QCG_GET_DATABASE_LINK_SESSION( sStmt );
        }
        else
        {
            /* do nothing */
        }

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
        IDE_TEST( sStmt->mCursorMutex.lock(NULL) != IDE_SUCCESS );
        sIsMutexLock = ID_TRUE;

        IDE_TEST( aDataPlan->cursor->open( QC_SMI_STMT( sStmt ),
                                           sTableHandle,
                                           sIndexHandle,
                                           sTableSCN,
                                           aDataPlan->updateColumnList,
                                           sRange,
                                           aDataPlan->keyFilter,
                                           &aDataPlan->callBack,
                                           sCursorFlag,
                                           aDataPlan->cursorType,
                                           &aDataPlan->cursorProperty,
                                           sIsDequeue ) 
                  != IDE_SUCCESS );
        
        // Cursor를 등록
        IDE_TEST( aTemplate->cursorMgr->addOpenedCursor(
                      sStmt->qmxMem,
                      aCodePlan->tupleRowID,
                      aDataPlan->cursor )
                  != IDE_SUCCESS );

        sIsMutexLock = ID_FALSE;
        IDE_TEST( sStmt->mCursorMutex.unlock() != IDE_SUCCESS );

        // Cursor가 열렸음을 표기
        *aDataPlan->flag &= ~QMND_SCAN_CURSOR_MASK;
        *aDataPlan->flag |= QMND_SCAN_CURSOR_OPEN;

        //-------------------------------------------------------
        //  Cursor를 최초 위치로 이동
        //-------------------------------------------------------

        // Disk Table의 경우
        // Key Range 검색으로 인해 해당 row의 공간을 상실할 수 있다.
        // 이를 위해 저장 공간의 위치를 저장하고 이를 복원한다.
        sOrgRow = aDataPlan->plan.myTuple->row;
        
        IDE_TEST( aDataPlan->cursor->beforeFirst() != IDE_SUCCESS );
        
        aDataPlan->plan.myTuple->row = sOrgRow;

        // for empty session temporary table
        IDE_EXCEPTION_CONT( NORMAL_EXIT_EMPTY );

        //---------------------------------
        // cursor 정보 설정
        //---------------------------------

        if ( aDataPlan->plan.myTuple->cursorInfo != NULL )
        {
            // DML에서도 사용하는 scan인 경우
            // cursor 설정을 변경하고 cursor를 전달한다.

            sCursorInfo = (qmnCursorInfo*) aDataPlan->plan.myTuple->cursorInfo;

            sCursorInfo->cursor = aDataPlan->cursor;

            // selected index
            sCursorInfo->selectedIndex = sMethod->index;

            /* PROJ-2359 Table/Partition Access Option */
            sCursorInfo->accessOption = aCodePlan->accessOption;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TEMPORARY_TABLE_EXIST )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMN_INVALID_TEMPORARY_TABLE ));
    }
    IDE_EXCEPTION_END;

    if ( sIsMutexLock == ID_TRUE )
    {
        (void)sStmt->mCursorMutex.unlock();
        sIsMutexLock = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC qmnSCAN::restartCursor( qmncSCAN   * aCodePlan,
                               qmndSCAN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 Restart한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void    * sOrgRow;
    smiRange* sRange;

    if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
          == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
         ((*aDataPlan->flag & QMND_SCAN_INSUBQ_RANGE_BUILD_MASK)
          == QMND_SCAN_INSUBQ_RANGE_BUILD_FAILURE ) )
    {
        // Cursor를 열지 않는다.
        // 더 이상 IN SUBQUERY Key Range를 생성할 수 없는 경우로
        // record를 fetch하지 않아야 한다.
        // Nothing To Do
    }
    else
    {

        /* BUG-41490 */
        if (aDataPlan->ridRange != NULL)
        {
            sRange = aDataPlan->ridRange;
        }
        else
        {
            sRange = aDataPlan->keyRange;
        }

        // Disk Table의 경우
        // Key Range 검색으로 인해 해당 row의 공간을 상실할 수 있다.
        // 이를 위해 저장 공간의 위치를 저장하고 이를 복원한다.
        sOrgRow = aDataPlan->plan.myTuple->row;

        IDE_TEST( aDataPlan->cursor->restart( sRange,
                                              aDataPlan->keyFilter,
                                              &aDataPlan->callBack )
                  != IDE_SUCCESS);

        IDE_TEST( aDataPlan->cursor->beforeFirst() != IDE_SUCCESS );

        aDataPlan->plan.myTuple->row = sOrgRow;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSCAN::readRow(qcTemplate * aTemplate,
                        qmncSCAN   * aCodePlan,
                        qmndSCAN   * aDataPlan,
                        qmcRowFlag * aFlag)
{
/***********************************************************************
 *
 * Description :
 *    Cursor로부터 조건에 맞는 Record를 읽는다.
 *
 * Implementation :
 *    Cursor를 이용하여 KeyRange, KeyFilter, Filter를 만족하는
 *    Record를 읽는다.
 *    이후, subquery filter를 적용하여 이를 만족하면 record를 리턴한다.
 *
 *    [ Record 위치의 복원 문제 ]
 *        - Disk Table의 경우
 *             SM의 Filter 검사, Data 없음 등으로 인해
 *             저장 공간을 잃어 버릴 수 있다.
 *             - SM의 Filter 검사 : plan.myTuple->row 사라짐
 *             - Data 없음        : sSearchRow 사라짐
 *        -  Memory Table을 사용하는 경우에도
 *           동일한 logic 적용에 의한 문제가 없도록 한다.
 *
 ***********************************************************************/

    idBool sJudge;
    void * sOrgRow;
    void * sSearchRow;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    //----------------------------------------------
    // 1. Cursor를 이용하여 Record를 읽음
    // 2. Subquery Filter 적용
    // 3. Subquery Filter 를 만족하면 결과 리턴
    //----------------------------------------------

    sJudge = ID_FALSE;

    while ( sJudge == ID_FALSE )
    {
        //----------------------------------------
        // Cursor를 이용하여 Record를 얻음
        //----------------------------------------

        if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
              == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
             ((*aDataPlan->flag & QMND_SCAN_INSUBQ_RANGE_BUILD_MASK)
              == QMND_SCAN_INSUBQ_RANGE_BUILD_FAILURE ) )
        {
            // Subquery 결과의 부재로
            // IN SUBQUERY KEYRANGE를 위한 RANGE 생성을 실패한 경우
            sSearchRow = NULL;
        }
        else
        {
            sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
            IDE_TEST(
                aDataPlan->cursor->readRow( (const void**) & sSearchRow,
                                            &aDataPlan->plan.myTuple->rid,
                                            SMI_FIND_NEXT )
                != IDE_SUCCESS );
            aDataPlan->plan.myTuple->row =
                (sSearchRow == NULL) ? sOrgRow : sSearchRow;

            // Proj 1360 Queue
            // dequeue문에서는 해당 row를 삭제해야 한다.
            if ( (sSearchRow != NULL) &&
                 ((aCodePlan->flag &  QMNC_SCAN_TABLE_QUEUE_MASK)
                  == QMNC_SCAN_TABLE_QUEUE_TRUE ))
            {
                IDE_TEST(aDataPlan->cursor->deleteRow() != IDE_SUCCESS);
            }
        }

        //----------------------------------------
        // IN SUBQUERY KEY RANGE시 재시도
        //----------------------------------------

        if ( ((aCodePlan->flag & QMNC_SCAN_INSUBQ_KEYRANGE_MASK)
              == QMNC_SCAN_INSUBQ_KEYRANGE_TRUE ) &&
             (sSearchRow == NULL) )
        {
            // IN SUBQUERY KEYRANGE인 경우 다시 시도한다.
            IDE_TEST( reRead4InSubRange( aTemplate,
                                         aCodePlan,
                                         aDataPlan,
                                         & sSearchRow ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }

        //----------------------------------------
        // SUBQUERY FILTER 처리
        //----------------------------------------

        if ( sSearchRow == NULL )
        {
            sJudge = ID_FALSE;
            break;
        }
        else
        {
            // modify값 보정
            if ( sMethod->filter == NULL )
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
            if ( sMethod->subqueryFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      sMethod->subqueryFilter,
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

IDE_RC qmnSCAN::readRowFromGRID( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 scGRID       aRowGRID,
                                 qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Cursor로부터 rid에 해당하는 Record를 읽는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    //----------------------------------------------
    // 적합성 검사
    //----------------------------------------------

    IDE_DASSERT( ( sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK )
                 == QMN_PLAN_STORAGE_DISK );

    IDE_DASSERT( ( sCodePlan->flag & QMNC_SCAN_FORCE_RID_SCAN_MASK )
                 == QMNC_SCAN_FORCE_RID_SCAN_TRUE );

    IDE_DASSERT( ( *sDataPlan->flag & QMND_SCAN_CURSOR_MASK )
                 == QMND_SCAN_CURSOR_OPEN );

    //----------------------------------------------
    // Cursor를 이용하여 Record를 읽음
    //----------------------------------------------

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;

    IDE_TEST( sDataPlan->cursor->readRowFromGRID( (const void**) & sSearchRow,
                                                  aRowGRID )
              != IDE_SUCCESS );

    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;
    sDataPlan->plan.myTuple->rid = aRowGRID;

    if ( sSearchRow != NULL )
    {
        // modify값 증가.
        sDataPlan->plan.myTuple->modify++;

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
qmnSCAN::reRead4InSubRange( qcTemplate * aTemplate,
                            qmncSCAN   * aCodePlan,
                            qmndSCAN   * aDataPlan,
                            void      ** aRow )
{
/***********************************************************************
 *
 * Description :
 *     IN SUBQUERY KEYRANGE가 있을 경우 Record Read를 다시 시도한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void  * sSearchRow = NULL;
    void  * sOrgRow;

    if ( (*aDataPlan->flag & QMND_SCAN_INSUBQ_RANGE_BUILD_MASK)
         == QMND_SCAN_INSUBQ_RANGE_BUILD_SUCCESS )
    {
        //---------------------------------------------------------
        // [IN SUBQUERY KEY RANGE]
        // 검색된 Record는 없으나, Subquery의 결과가 아직 존재하는 경우
        //     - Key Range를 다시 생성한다.
        //     - Record를 Fetch한다.
        //     - 위의 과정을 Fetch한 Record가 있거나,
        //       Key Range 생성을 실패할 때까지 반복한다.
        //---------------------------------------------------------

        while ( (sSearchRow == NULL) &&
                ((*aDataPlan->flag & QMND_SCAN_INSUBQ_RANGE_BUILD_MASK)
                 == QMND_SCAN_INSUBQ_RANGE_BUILD_SUCCESS ) )
        {
            /* BUG-41110 */
            IDE_TEST(makeRidRange(aTemplate, aCodePlan, aDataPlan)
                     != IDE_SUCCESS);

            // Key Range를 재생성한다.
            IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                             aCodePlan,
                                             aDataPlan ) != IDE_SUCCESS );

            // Cursor를 연다.
            // 이미 Open되어 있으므로, Restart한다.
            IDE_TEST( restartCursor( aCodePlan,
                                     aDataPlan ) != IDE_SUCCESS );


            if ( (*aDataPlan->flag & QMND_SCAN_INSUBQ_RANGE_BUILD_MASK)
                 == QMND_SCAN_INSUBQ_RANGE_BUILD_SUCCESS )
            {
                // Key Range 생성을 성공한 경우 Record를 읽는다.
                sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

                IDE_TEST(
                    aDataPlan->cursor->readRow( (const void**) & sSearchRow,
                                                &aDataPlan->plan.myTuple->rid,
                                                SMI_FIND_NEXT )
                    != IDE_SUCCESS );

                aDataPlan->plan.myTuple->row =
                    (sSearchRow == NULL) ? sOrgRow : sSearchRow;
            }
            else
            {
                sSearchRow = NULL;
            }
        }
    }
    else
    {
        // sSearchRow = NULL;
    }

    *aRow = sSearchRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSCAN::printAccessInfo( qmncSCAN     * aCodePlan,
                          qmndSCAN     * aDataPlan,
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

    ULong  sPageCount;

    IDE_DASSERT( aCodePlan != NULL );
    IDE_DASSERT( aDataPlan != NULL );
    IDE_DASSERT( aString   != NULL );

    if ( aMode == QMN_DISPLAY_ALL )
    {
        //----------------------------
        // explain plan = on; 인 경우
        //----------------------------

        // 수행 횟수 출력
        if ( (*aDataPlan->flag & QMND_SCAN_INIT_DONE_MASK)
             == QMND_SCAN_INIT_DONE_TRUE )
        {
            if (aCodePlan->cursorProperty.mParallelReadProperties.mThreadCnt == 1)
            {
                // fix BUG-9052
                iduVarStringAppendFormat( aString,
                                          ", ACCESS: %"ID_UINT32_FMT"",
                                          (aDataPlan->plan.myTuple->modify -
                                          aDataPlan->subQFilterDepCnt) );
            }
            else
            {
                if (aDataPlan->mAccessCnt4Parallel == 0)
                {
                    aDataPlan->mAccessCnt4Parallel = 
                        ((aDataPlan->plan.myTuple->modify -
                          aDataPlan->mOrgModifyCnt) -
                         (aDataPlan->subQFilterDepCnt -
                          aDataPlan->mOrgSubQFilterDepCnt));
                }
                else
                {
                    /* nothing to do */
                }

                iduVarStringAppendFormat( aString,
                                          ", ACCESS: %"ID_UINT32_FMT"",
                                          aDataPlan->mAccessCnt4Parallel );
            }
        }
        else
        {
            iduVarStringAppend( aString,
                                ", ACCESS: 0" );
        }

        // Disk Page 개수 출력
        if ( ( aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
            {
                if ( ( ( *aDataPlan->flag & QMND_SCAN_INIT_DONE_MASK )
                                         == QMND_SCAN_INIT_DONE_TRUE ) ||
                     ( ( aCodePlan->flag & QMNC_SCAN_FOR_PARTITION_MASK )
                                        == QMNC_SCAN_FOR_PARTITION_FALSE ) )
                {
                    // Disk Table에 대한 접근인 경우 출력
                    // SM으로부터 Disk Page Count를 획득
                    IDE_TEST( smiGetTableBlockCount( aCodePlan->table,
                                                     & sPageCount )
                              != IDE_SUCCESS );

                    iduVarStringAppendFormat( aString,
                                              ", DISK_PAGE_COUNT: %"ID_UINT64_FMT"",
                                              sPageCount );
                }
                else
                {
                    /* BUG-44510 미사용 Disk Partition의 SCAN Node를 출력하다가,
                     *           Page Count 부분에서 비정상 종료합니다.
                     *  Lock을 잡지 않고 smiGetTableBlockCount()를 호출하면, 비정상 종료할 수 있습니다.
                     *  미사용 Disk Partition의 SCAN Node에서 Page Count를 출력하지 않도록 수정합니다.
                     */
                    iduVarStringAppendFormat( aString,
                                              ", DISK_PAGE_COUNT: ??" );
                }
            }
            else
            {
                // BUG-29209
                // DISK_PAGE_COUNT 정보 보여주지 않음
                iduVarStringAppendFormat( aString,
                                          ", DISK_PAGE_COUNT: BLOCKED" );
            }
        }
        else
        {
            // Memory Table에 대한 접근인 경우 출력하지 않음
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnSCAN::printPredicateInfo( qcTemplate   * aTemplate,
                             qmncSCAN     * aCodePlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *     Predicate의 상세 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

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
                               aString,
                               aMode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSCAN::printKeyRangeInfo( qcTemplate   * aTemplate,
                            qmncSCAN     * aCodePlan,
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    // Fixed Key Range 출력
    if (sMethod->fixKeyRange4Print != NULL)
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
                                          sMethod->fixKeyRange4Print)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Variable Key Range 출력
    if (sMethod->varKeyRange != NULL)
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
                                          sMethod->varKeyRange)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // BUG-41591
    if ( sMethod->ridRange != NULL )
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString, " " );
        }

        iduVarStringAppend( aString, " [ RID FILTER ]\n" );

        IDE_TEST( qmoUtil::printPredInPlan( aTemplate,
                                            aString,
                                            aDepth + 1,
                                            sMethod->ridRange )
                  != IDE_SUCCESS);

        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString, " " );
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
qmnSCAN::printKeyFilterInfo( qcTemplate   * aTemplate,
                             qmncSCAN     * aCodePlan,
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    // Fixed Key Filter 출력
    if (sMethod->fixKeyFilter4Print != NULL)
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
                                          sMethod->fixKeyFilter4Print)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Variable Key Filter 출력
    if (sMethod->varKeyFilter != NULL)
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
                                          sMethod->varKeyFilter )
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
qmnSCAN::printFilterInfo( qcTemplate   * aTemplate,
                          qmncSCAN     * aCodePlan,
                          ULong          aDepth,
                          iduVarString * aString,
                          qmnDisplay     aMode )
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
    qmncScanMethod * sMethod   = getScanMethod( aTemplate, aCodePlan );
    qmndSCAN       * sDataPlan;

    // Constant Filter 출력
    if (sMethod->constantFilter != NULL)
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
                                          sMethod->constantFilter )
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Normal Filter 출력
    if (sMethod->filter != NULL)
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

        /* BUG-48370 remote table fatal */
        if ( ( ( aCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_MASK ) ==
               QMNC_SCAN_REMOTE_TABLE_FALSE ) &&
             ( ( aCodePlan->flag & QMNC_SCAN_REMOTE_TABLE_STORE_MASK ) ==
               QMNC_SCAN_REMOTE_TABLE_STORE_FALSE ) &&
             ( aMode == QMN_DISPLAY_ALL ) )
        {
            sDataPlan = (qmndSCAN*)(aTemplate->tmplate.data + aCodePlan->plan.offset); /* PROJ-2632 */
            /* PROJ-2632 */
            if ( ( ( *sDataPlan->flag & QMND_SCAN_INIT_DONE_MASK ) == QMND_SCAN_INIT_DONE_TRUE ) &&
                 ( ( *sDataPlan->flag & QMND_SCAN_SERIAL_EXECUTE_MASK ) == QMND_SCAN_SERIAL_EXECUTE_TRUE ) )
            {
                if ( QCG_GET_SESSION_TRCLOG_DETAIL_INFORMATION( aTemplate->stmt ) == 0 )
                {
                    iduVarStringAppend( aString,
                                        " [ FILTER SERIAL EXECUTE ]\n" );
                }
                else
                {
                    iduVarStringAppendFormat( aString,
                                              " [ FILTER SERIAL EXECUTE, SIZE: %"ID_UINT32_FMT" ]\n",
                                              QTC_GET_SERIAL_EXECUTE_DATA_SIZE( aCodePlan->mSerialFilterSize ) );
                }
            }
            else
            {
                iduVarStringAppend( aString,
                                    " [ FILTER ]\n" );
            }
        }
        else
        {
                iduVarStringAppend( aString,
                                    " [ FILTER ]\n" );
        }

        IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                          aString,
                                          aDepth + 1,
                                          sMethod->filter)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // Subquery Filter 출력
    if (sMethod->subqueryFilter != NULL)
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
                                          sMethod->subqueryFilter )
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

qmncScanMethod *
qmnSCAN::getScanMethod( qcTemplate * aTemplate,
                        qmncSCAN   * aCodePlan )
{
    qmncScanMethod * sDefaultMethod = &aCodePlan->method;

    // sdf가 달려 있고, data 영역이 초기화되어 있으면
    // data 영역의 selected method를 얻어온다.
    if( ( aTemplate->planFlag[aCodePlan->planID] &
          QMND_SCAN_SELECTED_METHOD_SET_MASK )
        == QMND_SCAN_SELECTED_METHOD_SET_TRUE )
    {
        IDE_DASSERT( aCodePlan->sdf != NULL );

        return qmo::getSelectedMethod( aTemplate,
                                       aCodePlan->sdf,
                                       sDefaultMethod );
    }
    else
    {
        return sDefaultMethod;
    }
}

IDE_RC
qmnSCAN::notifyOfSelectedMethodSet( qcTemplate * aTemplate,
                                    qmncSCAN   * aCodePlan )
{
    UInt *sFlag = &aTemplate->planFlag[aCodePlan->planID];

    *sFlag |= QMND_SCAN_SELECTED_METHOD_SET_TRUE;

    return IDE_SUCCESS;
}

IDE_RC
qmnSCAN::openCursorForPartition( qcTemplate * aTemplate,
                                 qmncSCAN   * aCodePlan,
                                 qmndSCAN   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-1502 PARTITIONED DISK TABLE
 *
 * Implementation :
 *    - Session Event Check (비정상 종료 Detect)
 *    - Key Range, Key Filter, Filter 구성
 *    - Cursor Open
 *
 ***********************************************************************/

    // 비정상 종료 검사
    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    /* BUG-41110 */
    IDE_TEST(makeRidRange(aTemplate, aCodePlan, aDataPlan)
             != IDE_SUCCESS);

    // KeyRange, KeyFilter, Filter 구성
    IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                     aCodePlan,
                                     aDataPlan ) != IDE_SUCCESS );

    // Cursor를 연다
    if ( ( *aDataPlan->flag & QMND_SCAN_CURSOR_MASK )
         != QMND_SCAN_CURSOR_OPEN )
    {
        // 처음 여는 경우
        IDE_TEST( openCursor( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );

        // doItFirst시 cursor를 restart하지 않는다.
        *aDataPlan->flag &= ~QMND_SCAN_RESTART_CURSOR_MASK;
        *aDataPlan->flag |= QMND_SCAN_RESTART_CURSOR_FALSE;
    }
    else
    {
        /* BUG-39399 remove search key preserved table
         * 이미 열려 있는 경우
         * join update row movement 수행시 중복으로 열려고 하는 경우가
         * 있다. update 수행 시 중복 update 체크 하여 에러 처리하기 때문에
         * ide_dassert는 제거 한다.*/
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
qmnSCAN::addAccessCount( qmncSCAN   * aPlan,
                         qcTemplate * aTemplate,
                         ULong        aCount )
{
    qmncSCAN     * sCodePlan  = (qmncSCAN*) aPlan;
    qcmTableInfo * sTableInfo = NULL;

    sTableInfo = sCodePlan->tableRef->tableInfo;

    if( ( sTableInfo != NULL ) &&   /* PROJ-1832 New database link */
        ( sTableInfo->tableType    != QCM_FIXED_TABLE ) &&
        ( sTableInfo->tableType    != QCM_PERFORMANCE_VIEW ) )
    {
        if( (aPlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                                   == QMN_PLAN_STORAGE_MEMORY )
        {
            // startup 시에 aTemplate->stmt->mStatistics은 NULL이다.
            if( aTemplate->stmt->mStatistics != NULL )
            {
                IDV_SQL_ADD( aTemplate->stmt->mStatistics,
                             mMemoryTableAccessCount,
                             aCount );

                IDV_SESS_ADD( aTemplate->stmt->mStatistics->mSess,
                              IDV_STAT_INDEX_MEMORY_TABLE_ACCESS_COUNT,
                              aCount );
            }
        }
    }
}

IDE_RC qmnSCAN::allocRidRange(qcTemplate* aTemplate,
                              qmncSCAN  * aCodePlan,
                              qmndSCAN  * aDataPlan)
{
    UInt            sRangeCnt;

    mtcNode*        sNode;
    mtcNode*        sNode2;

    iduMemory*      sMemory;
    qmncScanMethod* sMethod;

    sMethod = getScanMethod(aTemplate, aCodePlan);
    sMemory = QC_QMX_MEM(aTemplate->stmt);

    if (sMethod->ridRange != NULL)
    {
        IDE_ASSERT(sMethod->ridRange->node.module == &mtfOr);
        sNode = sMethod->ridRange->node.arguments;
        sRangeCnt = 0;

        while (sNode != NULL)
        {
            if (sNode->module == &mtfEqual)
            {
                sRangeCnt++;
            }
            else if (sNode->module == &mtfEqualAny)
            {
                IDE_ASSERT(sNode->arguments->next->module == &mtfList);

                sNode2 = sNode->arguments->next->arguments;
                while (sNode2 != NULL)
                {
                    sRangeCnt++;
                    sNode2 = sNode2->next;
                }
            }
            else
            {
                IDE_ASSERT(0);
            }
            sNode = sNode->next;
        }

        IDU_LIMITPOINT("qmnSCAN::allocRidRange::malloc");
        IDE_TEST(sMemory->cralloc(ID_SIZEOF(smiRange) * sRangeCnt,
                                  (void**)&aDataPlan->ridRange)
                 != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->ridRange = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSCAN::makeRidRange(qcTemplate* aTemplate,
                             qmncSCAN  * aCodePlan,
                             qmndSCAN  * aDataPlan)
{
    UInt            sRangeCnt;
    UInt            i;
    UInt            j;
    UInt            sPrevIdx;

    IDE_RC          sRc;

    mtcNode*        sNode;
    mtcNode*        sNode2;

    qtcNode*        sValueNode;
    qmncScanMethod* sMethod;
    smiRange*       sRange;

    sMethod = getScanMethod(aTemplate, aCodePlan);
    sRange  = aDataPlan->ridRange;

    if (sMethod->ridRange != NULL)
    {
        /*
         * ridRange는 다음과 같은 형식으로 들어옴
         *
         * 예1) SELECT .. FROM .. WHERE _PROWID = 1 OR _PROWID = 2
         *
         *   [OR]
         *    |
         *   [=]----------[=]
         *    |            |
         *   [RID]--[1]   [RID]--[2]
         *
         * 예2) SELECT .. FROM .. WHERE _PROWID IN (1, 2)
         *
         *    [OR]
         *     |
         *   [=ANY]
         *     |
         *   [RID]--[LIST]
         *            |
         *           [1]---[2]
         *
         */

        /*
         * ridRange->node = 'OR'
         * ridRange->node.argements = '=' or '=ANY'
         */
        sNode = sMethod->ridRange->node.arguments;
        sRangeCnt = 0;

        while (sNode != NULL)
        {
            /*
             * smiRange data 만드는 방법
             *
             * 1) calculate 하고
             * 2) converted node 로 변경
             * 3) mtc::value 로 값 가져오기
             *
             * converted 된 node 가 반드시 bigint 이어야 한다.
             * SM 이 인식하는 format 이 scGRID(=mtdBigint) 이기 때문이다.
             * conversion 이 가능하지만 결과가 bigint 가 아닌경우는
             * rid scan 이 불가능하고 이미 filter 로 분류되었다.
             */

            if (sNode->module == &mtfEqual)
            {
                // BUG-41215 _prowid predicate fails to create ridRange
                if ( sNode->arguments->module == &gQtcRidModule )
                {
                    sValueNode  = (qtcNode*)(sNode->arguments->next);
                }
                else
                {
                    sValueNode  = (qtcNode*)(sNode->arguments);
                }

                sRc = QTC_TMPL_TUPLE(aTemplate, sValueNode)->
                    execute->calculate(&sValueNode->node,
                                       aTemplate->tmplate.stack,
                                       aTemplate->tmplate.stackRemain,
                                       NULL,
                                       &aTemplate->tmplate);
                IDE_TEST(sRc != IDE_SUCCESS);

                sValueNode = (qtcNode*)mtf::convertedNode( (mtcNode*)sValueNode, &aTemplate->tmplate );
                IDE_DASSERT( QTC_TMPL_COLUMN(aTemplate, sValueNode)->module == &mtdBigint );

                sRange[sRangeCnt].minimum.callback = mtk::rangeCallBack4Rid;
                sRange[sRangeCnt].maximum.callback = mtk::rangeCallBack4Rid;

                sRange[sRangeCnt].minimum.data =
                    (void*)mtc::value( QTC_TMPL_COLUMN(aTemplate, sValueNode),
                                       QTC_TMPL_TUPLE(aTemplate, sValueNode)->row,
                                       MTD_OFFSET_USE );

                sRange[sRangeCnt].maximum.data = sRange[sRangeCnt].minimum.data;
                sRangeCnt++;
            }
            else if (sNode->module == &mtfEqualAny)
            {
                sNode2 = sNode->arguments->next->arguments;
                while (sNode2 != NULL)
                {
                    sRc = QTC_TMPL_TUPLE(aTemplate, (qtcNode*)sNode2)->
                        execute->calculate(sNode2,
                                           aTemplate->tmplate.stack,
                                           aTemplate->tmplate.stackRemain,
                                           NULL,
                                           &aTemplate->tmplate);
                    IDE_TEST(sRc != IDE_SUCCESS);

                    sValueNode = (qtcNode*)mtf::convertedNode( sNode2, &aTemplate->tmplate );
                    IDE_DASSERT( QTC_TMPL_COLUMN(aTemplate, sValueNode)->module == &mtdBigint );

                    sRange[sRangeCnt].minimum.callback = mtk::rangeCallBack4Rid;
                    sRange[sRangeCnt].maximum.callback = mtk::rangeCallBack4Rid;

                    sRange[sRangeCnt].minimum.data =
                        (void*)mtc::value( QTC_TMPL_COLUMN(aTemplate, sValueNode),
                                           QTC_TMPL_TUPLE(aTemplate, sValueNode)->row,
                                           MTD_OFFSET_USE );

                    sRange[sRangeCnt].maximum.data = sRange[sRangeCnt].minimum.data;
                    sRangeCnt++;

                    sNode2 = sNode2->next;
                }
            }
            else
            {
                IDE_DASSERT(0);
            }

            sNode = sNode->next;
        }

        sPrevIdx = 0;
        sRange[sPrevIdx].next = NULL;
        sRange[sPrevIdx].prev = NULL;

        for (i = 1; i < sRangeCnt; i++)
        {
            /*
             * BUG-41211 중복된 range 제거
             */
            for (j = 0; j <= sPrevIdx; j++)
            {
                if (idlOS::memcmp(sRange[i].minimum.data,
                                  sRange[j].minimum.data,
                                  ID_SIZEOF(mtdBigintType)) == 0)
                {
                    break;
                }
            }

            if (j == sPrevIdx + 1)
            {
                sRange[sPrevIdx].next = &sRange[i];
                sRange[i].prev        = &sRange[sPrevIdx];
                sPrevIdx = i;
            }
            else
            {
                /* skip this range */
            }
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

#define QMN_SCAN_SUBQUERY_PREDICATE_MAX (4)

void qmnSCAN::resetExecInfo4Subquery(qcTemplate *aTemplate, qmnPlan *aPlan)
{
/***********************************************************************
 *
 * Description :
 *
 *    BUG-31378
 *    template 에 있는 execInfo[] 배열 중
 *    subquery node 에 해당하는 원소를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt             i;

    /*   
     * qmncSCAN 에서 method 를 얻기 위해서 곧장 qmncSCAN::method 에 접근해서는 안된다.
     * qmnSCAN::getScanMethod() 를 이용해야 한다.
     */
    qmncSCAN        *sCodePlan = (qmncSCAN *)aPlan;
    qmncScanMethod  *sMethod = qmnSCAN::getScanMethod(aTemplate, sCodePlan);

    qtcNode  *sOutterNode;
    qtcNode  *sInnerNode;
    qtcNode  *sSubqueryWrapperNode;

    /*   
     * qmnSCAN::printLocalPlan() 내의 주석에 의하면
     * Subquery는 다음과 같은 predicate에만 존재할 수 있다 :
     *     1. Variable Key Range
     *     2. Variable Key Filter
     *     3. Constant Filter
     *     4. Subquery Filter
     */
    qtcNode  *sNode[QMN_SCAN_SUBQUERY_PREDICATE_MAX] = {NULL,};

    sNode[0] = sMethod->varKeyRange;
    sNode[1] = sMethod->varKeyFilter;
    sNode[2] = sMethod->constantFilter;
    sNode[3] = sMethod->subqueryFilter;
    /*
     * method 아래에 달리는 qtcNode 들의 구조는 아래와 같다.
     * () 안은 qtcNode 의 module name
     *
     * varKeyRange --- qtcNode                      : sNode[i]
     *                  (OR)
     *                    |
     *                 qtcNode                      : sOutterNode
     *                  (AND)
     *                    |
     *                 qtcNode                      : sInnerNode
     *              (=, <, >, ...)
     *                    |
     *                 qtcNode ------------- qtcNode  <--- (1)
     *           (COLUMN, VALUE, ...)   (SUBQUERY_WRAPPER)
     *                                          |
     *                                       qtcNode
     *                                      (SUBQUERY)
     *                                          |
     *                                       qtcNode
     *                                       (COLUMN)
     *
     * 이 함수에서 찾고자 하는 노드는 SUBQUERY_WRAPPER 노드(1번)이다.
     */
    for (i = 0; i < QMN_SCAN_SUBQUERY_PREDICATE_MAX; i++)
    {
        if (sNode[i] == NULL)
        {
            continue;
        }
        else
        {
            // nothing to do
        }
        /*
         * 아래의 중첩 for loop 는 qmoKeyRange::calculateSubqueryInRangeNode() 함수에서
         * 그대로 가져온 것임.
         */
        for (sOutterNode = (qtcNode *)sNode[i]->node.arguments;
             sOutterNode != NULL;
             sOutterNode = (qtcNode *)sOutterNode->node.next)
        {
            for (sInnerNode = (qtcNode *)sOutterNode->node.arguments;
                 sInnerNode != NULL;
                 sInnerNode = (qtcNode *)sInnerNode->node.next)
            {
                if ((sInnerNode->lflag & QTC_NODE_SUBQUERY_RANGE_MASK)
                    == QTC_NODE_SUBQUERY_RANGE_TRUE)
                {
                    if (sNode[i]->indexArgument == 0)
                    {
                        sSubqueryWrapperNode = (qtcNode*)sInnerNode->node.arguments->next;
                    }
                    else
                    {
                        sSubqueryWrapperNode = (qtcNode*)sInnerNode->node.arguments;
                    }

                    if (sSubqueryWrapperNode->node.module == &qtc::subqueryWrapperModule)
                    {
                        /*
                         * BINGGO!!
                         */
                        aTemplate->tmplate.execInfo[sSubqueryWrapperNode->node.info] =
                                                        QTC_WRAPPER_NODE_EXECUTE_FALSE;
                    }
                    else
                    {
                    }
                }
                else
                {
                }
            }
        }
    }
}

IDE_RC qmnSCAN::doItFirstFixedTable( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan,
                                     qmcRowFlag * aFlag )
{
    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);
    UInt                sModifyCnt;
    smiTrans          * sSmiTrans;
    mtkRangeCallBack  * sData;
    smiRange          * sRange;

    sDataPlan->plan.mTID = QMN_PLAN_INIT_THREAD_ID;

    // 기존 ACCESS count
    sModifyCnt = sDataPlan->plan.myTuple->modify;

    // ----------------
    // Tuple 위치의 결정
    // ----------------
    sDataPlan->plan.myTuple = &aTemplate->tmplate.rows[sCodePlan->tupleRowID];

    // ACCESS count 원복
    sDataPlan->plan.myTuple->modify = sModifyCnt;

    // 비정상 종료 검사
    IDE_TEST(iduCheckSessionEvent(aTemplate->stmt->mStatistics)
             != IDE_SUCCESS);

    IDE_TEST(makeRidRange(aTemplate, sCodePlan, sDataPlan)
             != IDE_SUCCESS);

    // KeyRange, KeyFilter, Filter 구성
    IDE_TEST(makeKeyRangeAndFilter(aTemplate, sCodePlan, sDataPlan)
             != IDE_SUCCESS);

    sSmiTrans = QC_SMI_STMT( aTemplate->stmt )->getTrans();
    if ( ( *sDataPlan->flag & QMND_SCAN_CURSOR_MASK )
         == QMND_SCAN_CURSOR_CLOSED )
    {
        sDataPlan->fixedTable.memory.initialize( QC_QMX_MEM( aTemplate->stmt ) );

        sDataPlan->fixedTableProperty.mFirstReadRecordPos = sDataPlan->cursorProperty.mFirstReadRecordPos;
        sDataPlan->fixedTableProperty.mReadRecordCount = sDataPlan->cursorProperty.mReadRecordCount;
        sDataPlan->fixedTableProperty.mStatistics = aTemplate->stmt->mStatistics;
        sDataPlan->fixedTableProperty.mFilter = &sDataPlan->callBack;
        /* BUG-43006 FixedTable Indexing Filter */
        sDataPlan->fixedTableProperty.mKeyRange = sDataPlan->keyRange;

        if ( sDataPlan->keyRange != smiGetDefaultKeyRange() )
        {
            sData = (mtkRangeCallBack *)sDataPlan->keyRange->minimum.data;
            sDataPlan->fixedTableProperty.mMinColumn = &sData->columnDesc.column;

            sData = (mtkRangeCallBack *)sDataPlan->keyRange->maximum.data;
            sDataPlan->fixedTableProperty.mMaxColumn = &sData->columnDesc.column;

            for ( sRange = sDataPlan->fixedTableProperty.mKeyRange->next;
                  sRange != NULL;
                  sRange = sRange->next )
            {
                sData = (mtkRangeCallBack *)sRange->minimum.data;
                sData->columnDesc.column.offset = 0;
                sData = (mtkRangeCallBack *)sRange->maximum.data;
                sData->columnDesc.column.offset = 0;
            }
        }
        else
        {
            /* Nothing to do */
        }
        if ( sSmiTrans != NULL )
        {
            sDataPlan->fixedTableProperty.mTrans = sSmiTrans->mTrans;
        }
        else
        {
            /* Nothing to do */
        }

        sDataPlan->fixedTable.memory.setContext( &sDataPlan->fixedTableProperty );

        IDE_TEST( smiFixedTable::build( aTemplate->stmt->mStatistics,
                                        (void *)sCodePlan->table,
                                        sCodePlan->dumpObject,
                                        &sDataPlan->fixedTable.memory,
                                        &sDataPlan->fixedTable.recRecord,
                                        &sDataPlan->fixedTable.traversePtr )
                  != IDE_SUCCESS );

        *sDataPlan->flag &= ~QMND_SCAN_CURSOR_MASK;
        *sDataPlan->flag |= QMND_SCAN_CURSOR_OPEN;
    }
    else
    {
        sDataPlan->fixedTable.memory.restartInit();

        sDataPlan->fixedTableProperty.mFirstReadRecordPos = sDataPlan->cursorProperty.mFirstReadRecordPos;
        sDataPlan->fixedTableProperty.mReadRecordCount = sDataPlan->cursorProperty.mReadRecordCount;
        sDataPlan->fixedTableProperty.mStatistics = aTemplate->stmt->mStatistics;
        sDataPlan->fixedTableProperty.mFilter = &sDataPlan->callBack;

        /* BUG-43006 FixedTable Indexing Filter */
        sDataPlan->fixedTableProperty.mKeyRange = sDataPlan->keyRange;
        if ( sDataPlan->keyRange != smiGetDefaultKeyRange() )
        {
            sData = (mtkRangeCallBack *)sDataPlan->keyRange->minimum.data;
            sDataPlan->fixedTableProperty.mMinColumn = &sData->columnDesc.column;

            sData = (mtkRangeCallBack *)sDataPlan->keyRange->maximum.data;
            sDataPlan->fixedTableProperty.mMaxColumn = &sData->columnDesc.column;

            for ( sRange = sDataPlan->fixedTableProperty.mKeyRange->next;
                  sRange != NULL;
                  sRange = sRange->next )
            {
                sData = (mtkRangeCallBack *)sRange->minimum.data;
                sData->columnDesc.column.offset = 0;
                sData = (mtkRangeCallBack *)sRange->maximum.data;
                sData->columnDesc.column.offset = 0;
            }
        }
        else
        {
            /* Nothing to do */
        }
        if ( sSmiTrans != NULL )
        {
            sDataPlan->fixedTableProperty.mTrans = sSmiTrans->mTrans;
        }
        else
        {
            /* Nothing to do */
        }

        sDataPlan->fixedTable.memory.setContext( &sDataPlan->fixedTableProperty );

        IDE_TEST( smiFixedTable::build( aTemplate->stmt->mStatistics,
                                        (void *)sCodePlan->table,
                                        sCodePlan->dumpObject,
                                        &sDataPlan->fixedTable.memory,
                                        &sDataPlan->fixedTable.recRecord,
                                        &sDataPlan->fixedTable.traversePtr )
                  != IDE_SUCCESS );
    }

    // Record를 획득한다.
    IDE_TEST( readRowFixedTable( aTemplate, sCodePlan, sDataPlan, aFlag )
              != IDE_SUCCESS );

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sDataPlan->doIt = qmnSCAN::doItNextFixedTable;
    }
    else
    {
        sDataPlan->doIt = qmnSCAN::doItFirstFixedTable;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSCAN::doItNextFixedTable( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag )
{
    qmncSCAN * sCodePlan = (qmncSCAN*) aPlan;
    qmndSCAN * sDataPlan =
        (qmndSCAN*) (aTemplate->tmplate.data + aPlan->offset);

    // Record를 획득한다.
    IDE_TEST( readRowFixedTable( aTemplate, sCodePlan, sDataPlan, aFlag )
              != IDE_SUCCESS );

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        sDataPlan->doIt = qmnSCAN::doItNextFixedTable;
    }
    else
    {
        sDataPlan->doIt = qmnSCAN::doItFirstFixedTable;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSCAN::readRowFixedTable( qcTemplate * aTemplate,
                                   qmncSCAN   * aCodePlan,
                                   qmndSCAN   * aDataPlan,
                                   qmcRowFlag * aFlag )
{
    idBool           sJudge;
    void           * sOrgRow;
    void           * sSearchRow;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );
    smiFixedTableRecord * sCurRec;

    sJudge = ID_FALSE;

    while ( sJudge == ID_FALSE )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

        IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
                  != IDE_SUCCESS );

        if ( aDataPlan->fixedTable.traversePtr == NULL )
        {
            sSearchRow = NULL;
        }
        else
        {
            sSearchRow = smiFixedTable::getRecordPtr( aDataPlan->fixedTable.traversePtr );
            sCurRec = ( smiFixedTableRecord * )aDataPlan->fixedTable.traversePtr;

            if ( sCurRec != NULL )
            {
                aDataPlan->fixedTable.traversePtr = ( UChar *)sCurRec->mNext;
            }
            else
            {
                aDataPlan->fixedTable.traversePtr = NULL;
            }
        }

        if ( sSearchRow == NULL )
        {
            aDataPlan->plan.myTuple->row = sOrgRow;
        }
        else
        {
            aDataPlan->plan.myTuple->row = sSearchRow;
        }

        if ( sSearchRow == NULL )
        {
            sJudge = ID_FALSE;
            break;
        }
        else
        {
            // modify값 보정
            if ( sMethod->filter == NULL )
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
            if ( sMethod->subqueryFilter != NULL )
            {
                IDE_TEST( qtc::judge( & sJudge,
                                      sMethod->subqueryFilter,
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

