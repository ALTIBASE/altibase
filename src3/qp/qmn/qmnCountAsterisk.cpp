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
 * $Id: qmnCountAsterisk.cpp 91019 2021-06-18 01:25:54Z donovan.seo $
 *
 * Description :
 *     CoUNT (*) Plan Node
 *
 *     관계형 모델에서 COUNT(*)만을 처리하기 위한 특수한 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qcuProperty.h>
#include <qcuTemporaryObj.h>
#include <qmoUtil.h>
#include <qmnCountAsterisk.h>
#include <qmo.h>
#include <qdbCommon.h>

extern mtfModule mtfOr;
extern mtfModule mtfEqual;
extern mtfModule mtfEqualAny;
extern mtfModule mtfList;
extern mtdModule mtdBigint;

IDE_RC
qmnCUNT::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    CUNT 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncCUNT *sCodePlan = (qmncCUNT*) aPlan;
    qmndCUNT *sDataPlan =
        (qmndCUNT*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    idBool sDependency;

    sDataPlan->doIt = qmnCUNT::doItDefault;

    if ( (*sDataPlan->flag & QMND_CUNT_INIT_DONE_MASK)
         == QMND_CUNT_INIT_DONE_FALSE )
    {
        // first init.
        IDE_TEST(firstInit(aTemplate, sCodePlan, sDataPlan)
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( checkDependency( sDataPlan,
                               & sDependency ) != IDE_SUCCESS );

    if ( sDependency == ID_TRUE )
    {
        // Count(*) 값을 새로 획득한다.
        IDE_TEST( getCountValue( aTemplate, sCodePlan, sDataPlan )
                  != IDE_SUCCESS );

        sDataPlan->depValue = sDataPlan->depTuple->modify;
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------------
    // 수행 함수의 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnCUNT::doItFirst;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    CUNT 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

    // qmncCUNT *sCodePlan = (qmncCUNT*) aPlan;
    qmndCUNT *sDataPlan =
        (qmndCUNT*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );
}

IDE_RC
qmnCUNT::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    COUNT(*)에 NULL Padding을 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncCUNT *sCodePlan = (qmncCUNT*) aPlan;
    qmndCUNT *sDataPlan =
        (qmndCUNT*) (aTemplate->tmplate.data + aPlan->offset);

    mtcColumn * sColumn;

    // 초기화가 수행되지 않은 경우 초기화를 수행
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_CUNT_INIT_DONE_MASK)
         == QMND_CUNT_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // COUNT(*) 영역에 Null 값을 설정한다.
    sColumn = & aTemplate->tmplate.rows[sCodePlan->countNode->node.table]
        .columns[sCodePlan->countNode->node.column];

    // To Fix PR-8005
    sColumn->module->
        null( sColumn,
              (void*) ( (SChar*) aTemplate->tmplate
                        .rows[sCodePlan->countNode->node.table].row
                        + sColumn->column.offset ) );

    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnCUNT::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    CUNT 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncCUNT *sCodePlan = (qmncCUNT*) aPlan;
    qmndCUNT *sDataPlan =
        (qmndCUNT*) (aTemplate->tmplate.data + aPlan->offset);
    qmncScanMethod * sMethod = getScanMethod( aTemplate, sCodePlan );

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    UInt  i;
    qcmIndex * sIndex;

    //----------------------------
    // Display 위치 결정
    //----------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // Table Owner Name 출력
    //----------------------------

    iduVarStringAppend( aString,
                        "COUNT ( TABLE: " );

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
         sCodePlan->aliasName.size != QC_POS_EMPTY_SIZE &&
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
    // Access Method 출력
    //----------------------------

    sIndex = sMethod->index;

    if ( sIndex != NULL )
    {
        // Index가 사용된 경우
        iduVarStringAppend( aString,
                            ", INDEX: " );

        iduVarStringAppend( aString,
                            sIndex->userName );
        iduVarStringAppend( aString,
                            "." );
        iduVarStringAppend( aString,
                            sIndex->name );
    }
    else
    {
        // BUG-41700
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
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    //----------------------------
    // Plan ID 출력
    //----------------------------

    if( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        for ( i = 0; i < aDepth; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }

        // sCodePlan 의 값을 출력하기때문에 qmn::printMTRinfo을 사용하지 못한다.
        iduVarStringAppendFormat( aString,
                                  "[ SELF NODE INFO, "
                                  "SELF: %"ID_INT32_FMT", "
                                  "REF1: %"ID_INT32_FMT" ]\n",
                                  (SInt)sCodePlan->tupleRowID,
                                  (SInt)sCodePlan->depTupleRowID );
    }
    else
    {
        /* Nothing to do */
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

    // Subquery가 존재하지 않는다.

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::doItDefault( qcTemplate * /* aTemplate */,
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
qmnCUNT::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    COUNT(*)을 리턴
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncCUNT *sCodePlan = (qmncCUNT*) aPlan;
    qmndCUNT *sDataPlan =
        (qmndCUNT*) (aTemplate->tmplate.data + aPlan->offset);

    mtcColumn * sColumn;
    void      * sColumnPos;

    sColumn = & aTemplate->tmplate.rows[sCodePlan->countNode->node.table]
        .columns[sCodePlan->countNode->node.column];

    IDE_DASSERT( sColumn->column.size == ID_SIZEOF(SLong) );

    sColumnPos = (void*)
        ( (SChar*) aTemplate->tmplate
          .rows[sCodePlan->countNode->node.table].row
          + sColumn->column.offset );

    idlOS::memcpy( sColumnPos,
                   & sDataPlan->countValue,
                   ID_SIZEOF(SLong) );

    *aFlag = QMC_ROW_DATA_EXIST;

    sDataPlan->doIt = qmnCUNT::doItLast;

    return IDE_SUCCESS;
}

IDE_RC
qmnCUNT::doItLast( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    결과 없음을 리턴
 *
 * Implementation :
 *
 ***********************************************************************/

    // qmncCUNT *sCodePlan = (qmncCUNT*) aPlan;
    qmndCUNT *sDataPlan =
        (qmndCUNT*) (aTemplate->tmplate.data + aPlan->offset);

    *aFlag = QMC_ROW_DATA_NONE;

    sDataPlan->doIt = qmnCUNT::doItFirst;

    return IDE_SUCCESS;
}

IDE_RC
qmnCUNT::firstInit( qcTemplate * aTemplate,
                    qmncCUNT   * aCodePlan,
                    qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    CUNT node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    //--------------------------------
    // 적합성 검사
    //--------------------------------

    IDE_DASSERT( aCodePlan->plan.left == NULL );
    IDE_DASSERT( aCodePlan->countNode != NULL );

    IDE_DASSERT( sMethod->fixKeyRange == NULL ||
                 sMethod->varKeyRange == NULL );

    IDE_DASSERT( sMethod->fixKeyFilter == NULL ||
                 sMethod->varKeyFilter == NULL );

    //---------------------------------
    // CUNT 고유 정보의 초기화
    //---------------------------------

    // Tuple 위치의 결정
    aDataPlan->plan.myTuple =
        & aTemplate->tmplate.rows[aCodePlan->tupleRowID];

    // Dependent Tuple 위치의 결정
    aDataPlan->depTuple =
        & aTemplate->tmplate.rows[aCodePlan->depTupleRowID];

    // Dependency Value 초기화
    aDataPlan->depValue = 0xFFFFFFFF;

    idlOS::memcpy( & aDataPlan->cursorProperty,
                   & aCodePlan->cursorProperty,
                   ID_SIZEOF( smiCursorProperties ) );
    aDataPlan->cursorProperty.mStatistics = aTemplate->stmt->mStatistics;

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

    // PROJ-1789 PROWID
    IDE_TEST( allocRidRange( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    aDataPlan->keyRange = NULL;
    aDataPlan->keyFilter = NULL;

    //---------------------------------
    // Disk Table 관련 정보의 초기화
    //---------------------------------

    // Disk Table의 경우 Memory 공간을 할당한다.
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aCodePlan->tupleRowID )
              != IDE_SUCCESS );

    //---------------------------------
    // 접근 방법 결정
    //---------------------------------

    // BUG-37266
    // repeatable read 모드에서 select count(*) 쿼리시 서버 사망합니다.
    // smiStatistics::getTableStatNumRow 함수에서
    // 현재 table 헤더에 기록된 레코드 건수 + 현재 트렌젝션의 레코드 건수를 리턴해주고 있습니다.
    // 따라서 isolation level을 고려하지 않아도 됩니다.
    if ( ( (aCodePlan->flag & QMNC_CUNT_METHOD_MASK) ==
           QMNC_CUNT_METHOD_HANDLE )  )
    {
        *aDataPlan->flag &= ~QMND_CUNT_METHOD_MASK;
        *aDataPlan->flag |= QMND_CUNT_METHOD_HANDLE;
    }
    else
    {
        *aDataPlan->flag &= ~QMND_CUNT_METHOD_MASK;
        *aDataPlan->flag |= QMND_CUNT_METHOD_CURSOR;
    }

    /* BUG-42639 Monitoring query */
    if ( ( aCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
         == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE )
    {
        aDataPlan->fixedTable.recRecord   = NULL;
        aDataPlan->fixedTable.traversePtr = NULL;
        SMI_FIXED_TABLE_PROPERTIES_INIT( &aDataPlan->fixedTableProperty );
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_CUNT_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_CUNT_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::allocFixKeyRange( qcTemplate * aTemplate,
                           qmncCUNT   * aCodePlan,
                           qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-1436
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

        IDU_FIT_POINT( "qmnCUNT::allocFixKeyRange::cralloc::fixKeyRangeArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->cralloc( aDataPlan->fixKeyRangeSize,
                                    (void**) & aDataPlan->fixKeyRangeArea )
                  != IDE_SUCCESS);

        aDataPlan->fixKeyRange = NULL;
    }
    else
    {
        aDataPlan->fixKeyRangeSize = 0;
        aDataPlan->fixKeyRange = NULL;
        aDataPlan->fixKeyRangeArea = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::allocFixKeyFilter( qcTemplate * aTemplate,
                            qmncCUNT   * aCodePlan,
                            qmndCUNT   * aDataPlan )
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
        IDE_TEST( qmoKeyRange::estimateKeyRange( aTemplate,
                                                 sMethod->fixKeyFilter,
                                                 & aDataPlan->fixKeyFilterSize )
                  != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->fixKeyFilterSize > 0 );

        // Fixed Key Filter를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;

        IDU_FIT_POINT( "qmnCUNT::allocFixKeyFilter::cralloc::fixKeyFilterArea", 
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->cralloc( aDataPlan->fixKeyFilterSize,
                                    (void**) & aDataPlan->fixKeyFilterArea )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->fixKeyFilterSize = 0;
        aDataPlan->fixKeyFilter = NULL;
        aDataPlan->fixKeyFilterArea = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::allocVarKeyRange( qcTemplate * aTemplate,
                           qmncCUNT   * aCodePlan,
                           qmndCUNT   * aDataPlan )
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

        IDU_FIT_POINT( "qmnCUNT::allocVarKeyRange::alloc::varKeyRangeArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->cralloc( aDataPlan->varKeyRangeSize,
                                    (void**) & aDataPlan->varKeyRangeArea )
                  != IDE_SUCCESS);

        aDataPlan->varKeyRange = NULL;
    }
    else
    {
        aDataPlan->varKeyRangeSize = 0;
        aDataPlan->varKeyRange = NULL;
        aDataPlan->varKeyRangeArea = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::allocVarKeyFilter( qcTemplate * aTemplate,
                            qmncCUNT   * aCodePlan,
                            qmndCUNT   * aDataPlan )
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
        IDE_TEST( qmoKeyRange::estimateKeyRange( aTemplate,
                                                 sMethod->varKeyFilter,
                                                 & aDataPlan->varKeyFilterSize )
                  != IDE_SUCCESS );

        IDE_DASSERT( aDataPlan->varKeyFilterSize > 0 );

        // Variable Key Filter를 위한 공간 할당
        sMemory = aTemplate->stmt->qmxMem;

        IDU_FIT_POINT( "qmnCUNT::allocVarKeyFilter::cralloc::varKeyFilterArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->cralloc( aDataPlan->varKeyFilterSize,
                                    (void**) & aDataPlan->varKeyFilterArea )
                  != IDE_SUCCESS);
    }
    else
    {
        aDataPlan->varKeyFilterSize = 0;
        aDataPlan->varKeyFilter = NULL;
        aDataPlan->varKeyFilterArea = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::checkDependency( qmndCUNT   * aDataPlan,
                          idBool     * aDependent )
{
/***********************************************************************
 *
 * Description :
 *     Dependent Tuple의 변경 여부를 검사
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( aDataPlan->depValue != aDataPlan->depTuple->modify )
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
qmnCUNT::getCountValue( qcTemplate * aTemplate,
                        qmncCUNT   * aCodePlan,
                        qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    COUNT(*)의 값을 획득한다.
 *
 * Implementation :
 *    다음과 같이 그 조건에 따라 값을 얻는 방법이 달라진다.
 *    - Constant Filter 조건이 FALSE인 경우 : 0
 *    - CURSOR를 사용하는 경우 : Cursor를 사용
 *    - HANDLE을 사용하는 경우 : Handle을 사용
 *
 ***********************************************************************/

    idBool sJudge;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    //------------------------------------------------
    // Constant Filter를 수행
    //------------------------------------------------

    /* TASK-7307 DML Data Consistency in Shard */
    if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) == ID_TRUE ) &&
         ( aCodePlan->tableRef->tableInfo->mIsUsable == ID_FALSE ) )
    {
        sJudge = ID_FALSE;
    }
    else if ( sMethod->constantFilter != NULL )
    {
        IDE_TEST( qtc::judge( & sJudge,
                              sMethod->constantFilter,
                              aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJudge = ID_TRUE;
    }

    if ( sJudge == ID_TRUE )
    {
        /* BUG-42639 Monitoring query */
        if ( ( aCodePlan->flag & QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK )
             == QMNC_SCAN_FAST_SELECT_FIXED_TABLE_FALSE )
        {
            if ( (*aDataPlan->flag & QMND_CUNT_METHOD_MASK)
                 == QMND_CUNT_METHOD_CURSOR )
            {
                IDE_TEST( getCountByCursor( aTemplate, aCodePlan, aDataPlan )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( getCountByHandle( aTemplate, aCodePlan, aDataPlan )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( getCountByFixedTable( aTemplate, aCodePlan, aDataPlan )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // 조건을 항상 만족할 수 없다.
        // 따라서, COUNT(*)값은 0 이 된다.
        aDataPlan->countValue = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::getCountByHandle( qcTemplate * aTemplate,
                           qmncCUNT   * aCodePlan,
                           qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Handle을 이용한 COUNT(*)값 획득
 * Implementation :
 *
 ***********************************************************************/

    const void      * sTableHandle = NULL;
    qcmTableInfo    * sTableInfo;
    qmsPartitionRef * sPartitionRef;
    SLong             sCountValue = 0;
    smSCN             sBaseTableSCN;

    // BUG-17483 파티션 테이블 count(*) 지원
    // 파티션 테이블은 QMND_CUNT_METHOD_HANDLE 방식만 지원한다.
    if( aCodePlan->tableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        // 플랜이 캐쉬가 된 상태에서 일부 파티션이 drop 되었을때 rebuild 를 하기 위해서이다.
        // Partition에 IS Lock을 건다.
        IDE_TEST( qcmPartition::validateAndLockPartitions( aTemplate->stmt,
                                                           aCodePlan->tableRef,
                                                           SMI_TABLE_LOCK_IS )
                  != IDE_SUCCESS );

        aDataPlan->countValue = 0;
        sPartitionRef         = aCodePlan->tableRef->partitionRef;

        while( sPartitionRef != NULL )
        {
            IDE_TEST( smiStatistics::getTableStatNumRow(
                          (void*)sPartitionRef->partitionHandle,
                          ID_TRUE, /*CurrentValue*/
                          (QC_SMI_STMT(aTemplate->stmt))->getTrans(),
                          & sCountValue )
                      != IDE_SUCCESS );
            
            aDataPlan->countValue += sCountValue;

            sPartitionRef = sPartitionRef->next;
        }
        aDataPlan->plan.myTuple->modify++;
    }
    else
    {
        sTableInfo = aCodePlan->tableRef->tableInfo;

        /* TASK-7307 DML Data Consistency in Shard */
        if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) == ID_TRUE ) &&
             ( sTableInfo->mIsUsable == ID_FALSE ) )
        {
            aDataPlan->countValue = 0;
        }
        // PROJ-1407 Temporary Table
        else if ( qcuTemporaryObj::isTemporaryTable( sTableInfo ) == ID_TRUE )
        {
            qcuTemporaryObj::getTempTableHandle( aTemplate->stmt,
                                                 sTableInfo,
                                                 &sTableHandle,
                                                 &sBaseTableSCN );

            if ( sTableHandle == NULL )
            {
                /* session temp table이 아직 생성되지 않은경우.*/
                aDataPlan->countValue = 0;
            }
            else
            {
                IDE_TEST_RAISE( !SM_SCN_IS_EQ( &(aCodePlan->tableRef->tableSCN), &sBaseTableSCN ),
                                ERR_TEMPORARY_TABLE_EXIST );

                IDE_TEST( smiStatistics::getTableStatNumRow(
                              (void*)sTableHandle,
                              ID_TRUE, /*CurrentValue*/
                              (QC_SMI_STMT(aTemplate->stmt))->getTrans(),
                              & aDataPlan->countValue )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            sTableHandle = aCodePlan->tableRef->tableInfo->tableHandle;

            IDE_TEST( smiStatistics::getTableStatNumRow(
                          (void*)sTableHandle,
                          ID_TRUE, /*CurrentValue*/
                          (QC_SMI_STMT(aTemplate->stmt))->getTrans(),
                          & aDataPlan->countValue )
                      != IDE_SUCCESS );
        }

        aDataPlan->plan.myTuple->modify++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TEMPORARY_TABLE_EXIST )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QMN_INVALID_TEMPORARY_TABLE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnCUNT::getCountByCursor( qcTemplate * aTemplate,
                           qmncCUNT   * aCodePlan,
                           qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 이용하여 COUNT(*)값을 계산
 *
 * Implementation :
 *
 ***********************************************************************/

    void           * sOrgRow;
    void           * sSearchRow;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // PROJ-1382, jhseong, FixedTable and PerformanceView
    // BUG-12167
    if( ( aCodePlan->flag & QMNC_CUNT_TABLE_FV_MASK )
        == QMNC_CUNT_TABLE_FV_FALSE )
    {
        IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT(aTemplate->stmt))->getTrans(),
                                            aCodePlan->tableRef->tableHandle,
                                            aCodePlan->tableSCN,
                                            SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                            SMI_TABLE_LOCK_IS,
                                            ID_ULONG_MAX,
                                            ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                 != IDE_SUCCESS);
    }
    else
    {
        // do nothing
    }

    // BUG-41591
    IDE_TEST( makeRidRange( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                     aCodePlan,
                                     aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( openCursor( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    // Cursor를 이용하여 COUNT(*)의 값을 구함
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

    if ( ( *aDataPlan->flag & QMND_SCAN_CURSOR_MASK )
         == QMND_SCAN_CURSOR_OPEN )
    {
        IDE_TEST( aDataPlan->cursor.countRow( (const void**) & sSearchRow,
                                              & aDataPlan->countValue )
                  != IDE_SUCCESS );
    }
    else
    {
        /* session temp table이 아직 생성되지 않은경우.*/
        aDataPlan->countValue = 0;
    }

    aDataPlan->plan.myTuple->row = sOrgRow;

    if ( sMethod->filter == NULL )
    {
        /* BUG-44537
           count 플랜 노드의 access 정보가 올바르게 나오지 않습니다. */
        aDataPlan->plan.myTuple->modify += aDataPlan->countValue;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnCUNT::getCountByFixedTable( qcTemplate * aTemplate,
                                      qmncCUNT   * aCodePlan,
                                      qmndCUNT   * aDataPlan )
{
    qmncScanMethod      * sMethod = getScanMethod( aTemplate, aCodePlan );
    smiFixedTableRecord * sCurRec;
    smiTrans            * sSmiTrans;
    mtkRangeCallBack    * sData;
    smiRange            * sRange;

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // BUG-41591
    IDE_TEST( makeRidRange( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( makeKeyRangeAndFilter( aTemplate,
                                     aCodePlan,
                                     aDataPlan ) != IDE_SUCCESS );

    sSmiTrans = QC_SMI_STMT( aTemplate->stmt )->getTrans();
    if ( ( *aDataPlan->flag & QMND_CUNT_CURSOR_MASK )
         == QMND_CUNT_CURSOR_CLOSED )
    {
        aDataPlan->fixedTable.memory.initialize( QC_QMX_MEM( aTemplate->stmt ) );

        aDataPlan->fixedTableProperty.mFirstReadRecordPos = aDataPlan->cursorProperty.mFirstReadRecordPos;
        aDataPlan->fixedTableProperty.mReadRecordCount = aDataPlan->cursorProperty.mReadRecordCount;
        aDataPlan->fixedTableProperty.mStatistics = aTemplate->stmt->mStatistics;
        aDataPlan->fixedTableProperty.mFilter = &aDataPlan->callBack;

        /* BUG-43006 FixedTable Indexing Filter*/
        aDataPlan->fixedTableProperty.mKeyRange = aDataPlan->keyRange;
        if ( aDataPlan->keyRange != smiGetDefaultKeyRange() )
        {
            sData = (mtkRangeCallBack *)aDataPlan->keyRange->minimum.data;
            aDataPlan->fixedTableProperty.mMinColumn = &sData->columnDesc.column;

            sData = (mtkRangeCallBack *)aDataPlan->keyRange->maximum.data;
            aDataPlan->fixedTableProperty.mMaxColumn = &sData->columnDesc.column;

            for ( sRange = aDataPlan->fixedTableProperty.mKeyRange->next;
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
            aDataPlan->fixedTableProperty.mTrans = sSmiTrans->mTrans;
        }
        else
        {
            /* Nothing to do */
        }

        aDataPlan->fixedTable.memory.setContext( &aDataPlan->fixedTableProperty );

        IDE_TEST( smiFixedTable::build( aTemplate->stmt->mStatistics,
                                        (void *)aCodePlan->tableRef->tableHandle,
                                        aCodePlan->dumpObject,
                                        &aDataPlan->fixedTable.memory,
                                        &aDataPlan->fixedTable.recRecord,
                                        &aDataPlan->fixedTable.traversePtr )
                  != IDE_SUCCESS );

        *aDataPlan->flag &= ~QMND_CUNT_CURSOR_MASK;
        *aDataPlan->flag |= QMND_CUNT_CURSOR_OPENED;
    }
    else
    {
        aDataPlan->fixedTable.memory.restartInit();

        aDataPlan->fixedTableProperty.mFirstReadRecordPos = aDataPlan->cursorProperty.mFirstReadRecordPos;
        aDataPlan->fixedTableProperty.mReadRecordCount = aDataPlan->cursorProperty.mReadRecordCount;
        aDataPlan->fixedTableProperty.mStatistics = aTemplate->stmt->mStatistics;
        aDataPlan->fixedTableProperty.mFilter = &aDataPlan->callBack;

        /* BUG-43006 FixedTable Indexing Filter*/
        aDataPlan->fixedTableProperty.mKeyRange = aDataPlan->keyRange;
        if ( aDataPlan->keyRange != smiGetDefaultKeyRange() )
        {
            sData = (mtkRangeCallBack *)aDataPlan->keyRange->minimum.data;
            aDataPlan->fixedTableProperty.mMinColumn = &sData->columnDesc.column;

            sData = (mtkRangeCallBack *)aDataPlan->keyRange->maximum.data;
            aDataPlan->fixedTableProperty.mMaxColumn = &sData->columnDesc.column;

            for ( sRange = aDataPlan->fixedTableProperty.mKeyRange->next;
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
            aDataPlan->fixedTableProperty.mTrans = sSmiTrans->mTrans;
        }
        else
        {
            /* Nothing to do */
        }

        aDataPlan->fixedTable.memory.setContext( &aDataPlan->fixedTableProperty );

        IDE_TEST( smiFixedTable::build( aTemplate->stmt->mStatistics,
                                        (void *)aCodePlan->tableRef->tableHandle,
                                        aCodePlan->dumpObject,
                                        &aDataPlan->fixedTable.memory,
                                        &aDataPlan->fixedTable.recRecord,
                                        &aDataPlan->fixedTable.traversePtr )
                  != IDE_SUCCESS );
    }

    aDataPlan->countValue = 0;

    while ( aDataPlan->fixedTable.traversePtr != NULL )
    {
        sCurRec = ( smiFixedTableRecord * )aDataPlan->fixedTable.traversePtr;

        if ( sCurRec != NULL )
        {
            aDataPlan->fixedTable.traversePtr = ( UChar *)sCurRec->mNext;
        }
        else
        {
            aDataPlan->fixedTable.traversePtr = NULL;
        }

        aDataPlan->countValue++;
    }

    if ( sMethod->filter == NULL )
    {
        /* BUG-44537
           count 플랜 노드의 access 정보가 올바르게 나오지 않습니다. */
        aDataPlan->plan.myTuple->modify += aDataPlan->countValue;
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
qmnCUNT::makeKeyRangeAndFilter( qcTemplate * aTemplate,
                                qmncCUNT   * aCodePlan,
                                qmndCUNT   * aDataPlan )
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
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

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
    sPredicateInfo.notNullKeyRange = NULL;

    // Filter 정보의 구성
    sPredicateInfo.filter = sMethod->filter;

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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnCUNT::openCursor( qcTemplate * aTemplate,
                     qmncCUNT   * aCodePlan,
                     qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Cursor를 연다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnCUNT::openCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                 sCursorFlag;
    const void         * sTableHandle = NULL;
    const void         * sIndexHandle;
    smSCN                sTableSCN;
    void               * sOrgRow;
    qmncScanMethod     * sMethod = getScanMethod( aTemplate, aCodePlan );
    smiFetchColumnList * sFetchColumnList;
    idBool               sIsMutexLock = ID_FALSE;
    smSCN                sBaseTableSCN;
    qcStatement        * sStmt  = aTemplate->stmt;
    smiRange           * sRange = NULL;

    //----------------------------------
    // Cursor를 위한 정보 설정
    //----------------------------------

    /* BUG-49077 */
    sCursorFlag = aCodePlan->lockMode | SMI_TRAVERSE_FORWARD | SMI_PREVIOUS_DISABLE |
                  SMI_TRANS_ISOLATION_IGNORE;

    if (sMethod->index != NULL)
    {
        sIndexHandle = sMethod->index->indexHandle;
    }
    else
    {
        sIndexHandle = NULL;
    }

    //----------------------------------
    // Cursor를 연다
    //----------------------------------

    if ( (*aDataPlan->flag & QMND_CUNT_CURSOR_MASK)
         == QMND_CUNT_CURSOR_CLOSED )
    {
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

            if ( sIndexHandle != NULL )
            {
                sIndexHandle = qcuTemporaryObj::getTempIndexHandle(
                    sStmt,
                    aCodePlan->tableRef->tableInfo,
                    sMethod->index->indexId );
                // 반드시 존재하여야 한다.
                IDE_ASSERT( sIndexHandle != NULL );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            sTableHandle = aCodePlan->tableRef->tableInfo->tableHandle;
            sTableSCN    = aCodePlan->tableSCN;
        }

        // PROJ-1705
        // 레코드패치시 복사되어야 할 컬럼정보생성
        if ( ( aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK )
             == QMN_PLAN_STORAGE_DISK )
        {
            IDE_TEST( qdbCommon::makeFetchColumnList4TupleID(
                          aTemplate,
                          aCodePlan->tupleRowID,
                          ID_FALSE, // aIsNeedAllFetchColumn
                          ( sIndexHandle != NULL ) ? sMethod->index: NULL,
                          ID_TRUE,
                          & sFetchColumnList ) != IDE_SUCCESS );

            aDataPlan->cursorProperty.mFetchColumnList = sFetchColumnList;
        }
        else
        {
            // Nothing To Do
        }

        // BUG-41591
        /*
         * SMI_CURSOR_PROP_INIT 을 사용하지 않고
         * mIndexTypeID 만 따로 setting 하는 이유는
         * 위에는 mFetchColumnList, mLockRowBuffer 를 변경하기 때문이다.
         * SMI_CURSOR_PROP_INIT 을 사용하면 다시 다 초기화 되므로
         */
        if ( sMethod->ridRange != NULL )
        {
            aDataPlan->cursorProperty.mIndexTypeID = SMI_BUILTIN_GRID_INDEXTYPE_ID;
            sRange = aDataPlan->ridRange;
        }
        else
        {
            if ( sIndexHandle == NULL )
            {
                aDataPlan->cursorProperty.mIndexTypeID = SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID;
            }
            else
            {
                aDataPlan->cursorProperty.mIndexTypeID = (UChar)sMethod->index->indexTypeId;
            }
 
            sRange = aDataPlan->keyRange;
        }

        aDataPlan->cursor.initialize();
        // BUG-16651
        aDataPlan->cursor.setDumpObject( aCodePlan->dumpObject );

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

        IDE_TEST(
            aDataPlan->cursor.open( QC_SMI_STMT(sStmt),
                                    sTableHandle,
                                    sIndexHandle,
                                    sTableSCN,
                                    NULL,
                                    sRange,
                                    aDataPlan->keyFilter,
                                    & aDataPlan->callBack,
                                    sCursorFlag,
                                    SMI_SELECT_CURSOR,
                                    & aDataPlan->cursorProperty )
            != IDE_SUCCESS);
        // Cursor를 등록
        IDE_TEST( aTemplate->cursorMgr->addOpenedCursor( sStmt->qmxMem,
                                                         aCodePlan->tupleRowID,
                                                         & aDataPlan->cursor )
                  != IDE_SUCCESS );

        sIsMutexLock = ID_FALSE;
        IDE_TEST( sStmt->mCursorMutex.unlock() != IDE_SUCCESS );

        // Cursor가 열렸음을 표기
        *aDataPlan->flag &= ~QMND_CUNT_CURSOR_MASK;
        *aDataPlan->flag |= QMND_CUNT_CURSOR_OPENED;
    }
    else
    {
        // BUG-41591
        if ( aDataPlan->ridRange != NULL )
        {
            sRange = aDataPlan->ridRange;
        }
        else
        {
            sRange = aDataPlan->keyRange;
        }

        IDE_TEST( aDataPlan->cursor.restart( sRange,
                                             aDataPlan->keyFilter,
                                             & aDataPlan->callBack )
                  != IDE_SUCCESS);
    }

    //----------------------------------
    // 최초 위치로 이동
    //----------------------------------

    sOrgRow = aDataPlan->plan.myTuple->row;
    
    IDE_TEST( aDataPlan->cursor.beforeFirst() != IDE_SUCCESS);
    
    aDataPlan->plan.myTuple->row = sOrgRow;

    // for empty session temporary table
    IDE_EXCEPTION_CONT( NORMAL_EXIT_EMPTY );

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

#undef IDE_FN
}

IDE_RC
qmnCUNT::printAccessInfo( qmncCUNT     * aCodePlan,
                          qmndCUNT     * aDataPlan,
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

#define IDE_FN "qmnCUNT::printAccessInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

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
        if ( (*aDataPlan->flag & QMND_CUNT_INIT_DONE_MASK)
             == QMND_CUNT_INIT_DONE_TRUE )
        {
            iduVarStringAppendFormat( aString,
                                      ", ACCESS: %"ID_UINT32_FMT"",
                                      aDataPlan->plan.myTuple->modify );
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
                // Disk Table에 대한 접근인 경우 출력
                // SM으로부터 Disk Page Count를 획득
                IDE_TEST( smiGetTableBlockCount( aCodePlan->tableRef->tableInfo->tableHandle,
                                                 & sPageCount )
                          != IDE_SUCCESS );

                iduVarStringAppendFormat( aString,
                                          ", DISK_PAGE_COUNT: %"ID_UINT64_FMT"",
                                          sPageCount );
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

#undef IDE_FN
}


IDE_RC
qmnCUNT::printPredicateInfo( qcTemplate   * aTemplate,
                             qmncCUNT     * aCodePlan,
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

#define IDE_FN "qmnCUNT::printPredicateInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

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

#undef IDE_FN
}

IDE_RC
qmnCUNT::printKeyRangeInfo( qcTemplate   * aTemplate,
                            qmncCUNT     * aCodePlan,
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

#define IDE_FN "qmnCUNT::printKeyRangeInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

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
        iduVarStringAppend( aString,
                            " [ VARIABLE KEY ]\n" );
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

#undef IDE_FN
}

IDE_RC
qmnCUNT::printKeyFilterInfo( qcTemplate   * aTemplate,
                             qmncCUNT     * aCodePlan,
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

#define IDE_FN "qmnCUNT::printKeyFilterInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

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

#undef IDE_FN
}


IDE_RC
qmnCUNT::printFilterInfo( qcTemplate   * aTemplate,
                          qmncCUNT     * aCodePlan,
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

#define IDE_FN "qmnCUNT::printFilterInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    qmncScanMethod * sMethod = getScanMethod( aTemplate, aCodePlan );

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
        iduVarStringAppend( aString,
                            " [ FILTER ]\n" );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

qmncScanMethod *
qmnCUNT::getScanMethod( qcTemplate * aTemplate,
                        qmncCUNT   * aCodePlan )
{
    qmncScanMethod * sDefaultMethod = &aCodePlan->method;

    if( ( aTemplate->planFlag[aCodePlan->planID] &
          QMND_CUNT_SELECTED_METHOD_SET_MASK )
        == QMND_CUNT_SELECTED_METHOD_SET_TRUE )
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
qmnCUNT::notifyOfSelectedMethodSet( qcTemplate * aTemplate,
                                    qmncCUNT   * aCodePlan )
{
    UInt *sFlag = &aTemplate->planFlag[aCodePlan->planID];

    *sFlag |= QMND_CUNT_SELECTED_METHOD_SET_TRUE;

    return IDE_SUCCESS;
}

IDE_RC qmnCUNT::allocRidRange( qcTemplate * aTemplate,
                               qmncCUNT   * aCodePlan,
                               qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description : BUG-41591
 *               qmnSCAN 에 적용된 ridRange 수행방식을 qmnCUNT 에 적용한다.
 *               firstInit 에서 호출되어 ridRange 를 할당한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcNode        * sNode   = NULL;
    mtcNode        * sNode2  = NULL;
    iduMemory      * sMemory = NULL;
    qmncScanMethod * sMethod = NULL;
    UInt sRangeCnt;

    sMethod = getScanMethod( aTemplate, aCodePlan );
    sMemory = QC_QMX_MEM( aTemplate->stmt );

    if ( sMethod->ridRange != NULL )
    {
        IDE_DASSERT( sMethod->ridRange->node.module == &mtfOr );
        sNode = sMethod->ridRange->node.arguments;
        sRangeCnt = 0;

        while( sNode != NULL )
        {
            if ( sNode->module == &mtfEqual )
            {
                sRangeCnt++;
            }
            else if ( sNode->module == &mtfEqualAny )
            {
                IDE_DASSERT( sNode->arguments->next->module == &mtfList );

                sNode2 = sNode->arguments->next->arguments;
                while( sNode2 != NULL )
                {
                    sRangeCnt++;
                    sNode2 = sNode2->next;
                }
            }
            else
            {
                IDE_DASSERT( 0 );
            }
            sNode = sNode->next;
        }

        IDU_FIT_POINT( "qmnCUNT::allocRidRange::cralloc::ridRangeArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->cralloc( ID_SIZEOF(smiRange) * sRangeCnt,
                                    (void**)&aDataPlan->ridRange )
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

IDE_RC qmnCUNT::makeRidRange( qcTemplate * aTemplate,
                              qmncCUNT   * aCodePlan,
                              qmndCUNT   * aDataPlan )
{
/***********************************************************************
 *
 * Description : BUG-41591
 *               qmnSCAN 에 적용된 ridRange 수행방식을 qmnCUNT 에 적용한다.
 *               getCountByCursor 에서 호출되어 ridRange 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcNode        * sNode   = NULL;
    mtcNode        * sNode2  = NULL;
    qtcNode        * sValueNode = NULL;
    qmncScanMethod * sMethod    = NULL;
    smiRange       * sRange  = NULL;

    UInt   sRangeCnt;
    UInt   i;
    UInt   j;
    UInt   sPrevIdx;
    IDE_RC sRc;

    sMethod = getScanMethod( aTemplate, aCodePlan );
    sRange  = aDataPlan->ridRange;

    if ( sMethod->ridRange != NULL )
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

        while ( sNode != NULL )
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

            if ( sNode->module == &mtfEqual )
            {
                // BUG-41215 _prowid predicate fails to create ridRange
                if ( sNode->arguments->module == &gQtcRidModule )
                {
                    sValueNode = (qtcNode*)(sNode->arguments->next);
                }
                else
                {
                    sValueNode = (qtcNode*)(sNode->arguments);
                }

                sRc = QTC_TMPL_TUPLE(aTemplate, sValueNode)->
                    execute->calculate( &sValueNode->node,
                                        aTemplate->tmplate.stack,
                                        aTemplate->tmplate.stackRemain,
                                        NULL,
                                        &aTemplate->tmplate );
                IDE_TEST( sRc != IDE_SUCCESS );

                sValueNode = (qtcNode*)mtf::convertedNode( (mtcNode*)sValueNode, &aTemplate->tmplate);
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
            else if ( sNode->module == &mtfEqualAny )
            {
                sNode2 = sNode->arguments->next->arguments;
                while ( sNode2 != NULL )
                {
                    sRc = QTC_TMPL_TUPLE(aTemplate, (qtcNode*)sNode2)->execute->calculate(
                            sNode2,
                            aTemplate->tmplate.stack,
                            aTemplate->tmplate.stackRemain,
                            NULL,
                            &aTemplate->tmplate );
                    IDE_TEST( sRc != IDE_SUCCESS );

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
                IDE_DASSERT( 0 );
            }

            sNode = sNode->next;
        }

        sPrevIdx = 0;
        sRange[sPrevIdx].next = NULL;
        sRange[sPrevIdx].prev = NULL;

        for ( i = 1; i < sRangeCnt; i++ )
        {
            /*
             * BUG-41211 중복된 range 제거
             */
            for ( j = 0; j <= sPrevIdx; j++ )
            {
                if ( idlOS::memcmp( sRange[i].minimum.data,
                                    sRange[j].minimum.data,
                                    ID_SIZEOF(mtdBigintType) ) == 0 )
                {
                    break;
                }
            }

            if ( j == sPrevIdx + 1 )
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
