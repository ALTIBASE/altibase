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
 * $Id: qmnViewScan.cpp 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *     VSCN(View SCaN) Node
 *
 *     관계형 모델에서 Materialized View에 대한
 *     Selection을 수행하는 Node이다.
 *
 *     하위 VMTR 노드의 저장 매체에 따라 다른 동작을 하게 되며,
 *     Memory Temp Table일 경우 Memory Sort Temp Table 객체의
 *        interface를 직접 사용하여 접근하며,
 *     Disk Temp Table일 경우 table handle과 index handle을 얻어
 *        별도의 Cursor를 통해 Sequetial Access한다.
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
#include <qmnViewScan.h>

IDE_RC
qmnVSCN::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    VSCN 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnVSCN::doItDefault;
    
    // first initialization
    if ( (*sDataPlan->flag & QMND_VSCN_INIT_DONE_MASK)
         == QMND_VSCN_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // PROJ-2415 Grouping Sets Clause
        // VMTR의 Dependency 처리추가 에 따른 변경
        IDE_TEST( initForChild( aTemplate, sCodePlan, sDataPlan ) != IDE_SUCCESS );
    }
         
    if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sDataPlan->doIt = qmnVSCN::doItFirstMem;
    }
    else
    {
        sDataPlan->doIt = qmnVSCN::doItFirstDisk;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVSCN::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    VSCN의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );

#undef IDE_FN
}


IDE_RC
qmnVSCN::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 매체에 따라 null row를 setting한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);
    mtcColumn * sColumn;
    UInt        i;

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_VSCN_INIT_DONE_MASK)
         == QMND_VSCN_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        //----------------------------------
        // Memory Temp Table인 경우
        //----------------------------------

        sDataPlan->plan.myTuple->row = sDataPlan->nullRow;

        // PROJ-2362 memory temp 저장 효율성 개선
        sColumn = sDataPlan->plan.myTuple->columns;
        for ( i = 0; i < sDataPlan->plan.myTuple->columnCount; i++, sColumn++ )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( sColumn->column.flag ) == ID_TRUE )
            {
                sColumn->module->null( sColumn,
                                       sColumn->column.value );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        //----------------------------------
        // Disk Temp Table인 경우
        //----------------------------------

        idlOS::memcpy( sDataPlan->plan.myTuple->row,
                       sDataPlan->nullRow,
                       sDataPlan->nullRowSize );
        idlOS::memcpy( & sDataPlan->plan.myTuple->rid,
                       & sDataPlan->nullRID,
                       ID_SIZEOF(scGRID) );
    }

    // Null Padding도 record가 변한 것임
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVSCN::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *   VSCN 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar      sNameBuffer[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    ULong  i;

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //-------------------------------
    // View의 이름 및 alias name 출력
    //-------------------------------

    iduVarStringAppend( aString,
                        "VIEW-SCAN ( " );

    if ( ( sCodePlan->viewName.name != NULL ) &&
         ( sCodePlan->viewName.size != QC_POS_EMPTY_SIZE ) )
    {
        iduVarStringAppend( aString,
                            "VIEW: " );

        if ( ( sCodePlan->viewOwnerName.name != NULL ) &&
             ( sCodePlan->viewOwnerName.size > 0 ) )
        {
            iduVarStringAppend( aString,
                                sCodePlan->viewOwnerName.name );
            iduVarStringAppend( aString,
                                "." );
        }
        else
        {
            // Nothing to do.
        }

        if ( sCodePlan->viewName.size <= QC_MAX_OBJECT_NAME_LEN )
        {
            idlOS::memcpy( sNameBuffer,
                           sCodePlan->viewName.name,
                           sCodePlan->viewName.size );
            sNameBuffer[sCodePlan->viewName.size] = '\0';

            iduVarStringAppend( aString,
                                sNameBuffer );
        }
        else
        {
            // Nothing to do.
        }

        if ( sCodePlan->aliasName.name != NULL &&
                        sCodePlan->aliasName.size != QC_POS_EMPTY_SIZE &&
                        sCodePlan->aliasName.name != sCodePlan->viewName.name )
        {
            // View 이름 정보와 Alias 이름 정보가 다를 경우
            // (alias name)
            iduVarStringAppend( aString,
                                " " );

            if ( sCodePlan->aliasName.size <= QC_MAX_OBJECT_NAME_LEN )
            {
                idlOS::memcpy( sNameBuffer,
                               sCodePlan->aliasName.name,
                               sCodePlan->aliasName.size );
                sNameBuffer[sCodePlan->aliasName.size] = '\0';

                iduVarStringAppend( aString,
                                    sNameBuffer );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Alias 이름 정보가 없거나 View 이름 정보와 동일한 경우
            // Nothing To Do
        }

        iduVarStringAppend( aString,
                            ", " );
    }
    else
    {
        // Nothing to do.
    }

    //-------------------------------
    // Access 정보의 출력
    //-------------------------------

    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( (*sDataPlan->flag & QMND_VSCN_INIT_DONE_MASK)
             == QMND_VSCN_INIT_DONE_TRUE )
        {
            iduVarStringAppendFormat( aString,
                                      "ACCESS: %"ID_UINT32_FMT,
                                      sDataPlan->plan.myTuple->modify );
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "ACCESS: 0" );
        }
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "ACCESS: ??" );
    }

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

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

    //-------------------------------
    // Child Plan의 출력
    //-------------------------------

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
qmnVSCN::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnVSCN::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVSCN::doItFirstMem( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Memory Temp Table 사용 시 최초 수행 함수
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::doItFirstMem"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    if ( ( *sDataPlan->flag & QMND_VSCN_INIT_DONE_MASK )
         == QMND_VSCN_INIT_DONE_FALSE )
    {
        IDE_TEST( qmnVSCN::init( aTemplate, aPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    // 최초 Record 위치로부터 획득
    sDataPlan->recordPos = 0;

    sDataPlan->doIt = qmnVSCN::doItNextMem;

    return qmnVSCN::doItNextMem( aTemplate, aPlan, aFlag );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnVSCN::doItNextMem( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Memory Temp Table 사용 시 다음 수행 함수
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::doItNextMem"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);

    if ( sDataPlan->recordPos < sDataPlan->recordCnt )
    {
        IDE_TEST( qmcMemSort::getElement( sDataPlan->memSortMgr,
                                          sDataPlan->recordPos,
                                          & sDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
        IDE_DASSERT( sDataPlan->plan.myTuple->row != NULL );

        IDE_TEST( setTupleSet( aTemplate,
                               sCodePlan,
                               sDataPlan )
                  != IDE_SUCCESS );
        
        sDataPlan->recordPos++;
        sDataPlan->plan.myTuple->modify++;

        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_NONE;
        sDataPlan->doIt = qmnVSCN::doItFirstMem;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVSCN::doItFirstDisk( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Disk Temp Table 사용 시 최초 수행 함수
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::doItFirstDisk"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    IDE_TEST( openCursor( sDataPlan )
              != IDE_SUCCESS );

    sDataPlan->doIt = qmnVSCN::doItNextDisk;

    return qmnVSCN::doItNextDisk( aTemplate, aPlan, aFlag );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnVSCN::doItNextDisk( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Disk Temp Table 사용 시 다음 수행 함수
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::doItNextDisk"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    // 저장 공간 보존을 위한 Pointer 기록
    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;

    // Cursor를 이용하여 Record를 얻음
    IDE_TEST( smiSortTempTable::fetch( sDataPlan->tempCursor,
                                       (UChar**) & sSearchRow,
                                       & sDataPlan->plan.myTuple->rid )
              != IDE_SUCCESS );

    if ( sSearchRow != NULL )
    {
        //------------------------------
        // 데이터 존재하는 경우
        //------------------------------

        sDataPlan->plan.myTuple->row = sSearchRow;
        sDataPlan->plan.myTuple->modify++;

        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;
    }
    else
    {
        //------------------------------
        // 데이터가 없는 경우
        //------------------------------

        sDataPlan->plan.myTuple->row = sOrgRow;

        *aFlag = QMC_ROW_DATA_NONE;

        sDataPlan->doIt = qmnVSCN::doItFirstDisk;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnVSCN::firstInit( qcTemplate * aTemplate,
                    qmncVSCN   * aCodePlan,
                    qmndVSCN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    VMTR Child를 수행한 후,
 *    VSCN node의 Data 영역의 멤버에 대한 초기화를 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->plan.left->type == QMN_VMTR );

    //---------------------------------
    // Child 수행
    //---------------------------------

    IDE_TEST( execChild( aTemplate, aCodePlan )
              != IDE_SUCCESS );

    //---------------------------------
    // Data Member의 초기화
    //---------------------------------

    // Tuple 의 초기화
    // Tuple을 구성하는 column의 offset을 조정하고,
    // Row를 위한 size계산 및 공간을 할당받는다.
    aDataPlan->plan.myTuple =
        & aTemplate->tmplate.rows[aCodePlan->tupleRowID];

    IDE_TEST( refineOffset( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aCodePlan->tupleRowID )
              != IDE_SUCCESS );

    // Row Size 획득
    IDE_TEST( qmnVMTR::getNullRowSize( aTemplate,
                                       aCodePlan->plan.left,
                                       & aDataPlan->nullRowSize )
              != IDE_SUCCESS );
    
    // Null Row 초기화
    IDE_TEST( getNullRow( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // Temp Table 전용 정보의 초기화
    //---------------------------------

    IDE_TEST( qmnVMTR::getCursorInfo( aTemplate,
                                      aCodePlan->plan.left,
                                      & aDataPlan->tableHandle,
                                      & aDataPlan->indexHandle,
                                      & aDataPlan->memSortMgr,
                                      & aDataPlan->memSortRecord )
              != IDE_SUCCESS );

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        // 적합성 검사
        IDE_DASSERT( aDataPlan->tableHandle == NULL );
        IDE_DASSERT( aDataPlan->indexHandle == NULL );
        IDE_DASSERT( aDataPlan->memSortMgr != NULL );

        IDE_TEST( qmcMemSort::getNumOfElements( aDataPlan->memSortMgr,
                                                & aDataPlan->recordCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // 적합성 검사
        IDE_DASSERT( aDataPlan->tableHandle != NULL );
        IDE_DASSERT( aDataPlan->memSortMgr == NULL );

        // Temp Cursor 초기화
        aDataPlan->tempCursor = NULL;

        // PROJ-1597 Temp record size 제약 제거
        aDataPlan->plan.myTuple->tableHandle = aDataPlan->tableHandle;
    }

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_VSCN_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_VSCN_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnVSCN::initForChild( qcTemplate * aTemplate,
                              qmncVSCN   * aCodePlan,
                              qmndVSCN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2415 Grouping Sets Clause를 통해
 *    VMTR이 Depedency에 따라 재수행 됨으로, 기존 firstInit에서 한번만 수행하던
 *    VMTR Child를 이후 init에서도 수행하도록 변경.
 *    
 *    VMTR Child를 수행한 후,
 *    VSCN node의 Data 영역의 멤버에 대한 재설정을 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/
    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->plan.left->type == QMN_VMTR );
    
    //---------------------------------
    // Child 수행
    //---------------------------------

    IDE_TEST( execChild( aTemplate, aCodePlan )
              != IDE_SUCCESS );

    //---------------------------------
    // Temp Table 전용 정보의 초기화
    //---------------------------------

    IDE_TEST( qmnVMTR::getCursorInfo( aTemplate,
                                      aCodePlan->plan.left,
                                      & aDataPlan->tableHandle,
                                      & aDataPlan->indexHandle,
                                      & aDataPlan->memSortMgr,
                                      & aDataPlan->memSortRecord )
              != IDE_SUCCESS );

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        // 적합성 검사
        IDE_DASSERT( aDataPlan->tableHandle == NULL );
        IDE_DASSERT( aDataPlan->indexHandle == NULL );
        IDE_DASSERT( aDataPlan->memSortMgr != NULL );

        IDE_TEST( qmcMemSort::getNumOfElements( aDataPlan->memSortMgr,
                                                & aDataPlan->recordCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // 적합성 검사
        IDE_DASSERT( aDataPlan->tableHandle != NULL );
        IDE_DASSERT( aDataPlan->memSortMgr == NULL );

        // Temp Cursor 초기화
        aDataPlan->tempCursor = NULL;

        // PROJ-1597 Temp record size 제약 제거
        aDataPlan->plan.myTuple->tableHandle = aDataPlan->tableHandle;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnVSCN::refineOffset( qcTemplate * aTemplate,
                       qmncVSCN   * aCodePlan,
                       qmndVSCN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Tuple을 구성하는 Column의 offset 재조정
 *
 * Implementation :
 *    qmc::refineOffsets()을 사용하지 않음.
 *    VSCN 노드는 qmdMtrNode가 없으며, VMTR의 모든 내용을 복사하기만
 *    한다.  따라서, 내부적으로 offset을 refine한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::refineOffset"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcTuple  * sVMTRTuple;

    IDE_DASSERT( aDataPlan->plan.myTuple != NULL );
    IDE_DASSERT( aDataPlan->plan.myTuple->columnCount > 0 );

    IDE_TEST( qmnVMTR::getTuple( aTemplate,
                                 aCodePlan->plan.left,
                                 & sVMTRTuple )
              != IDE_SUCCESS );

    // PROJ-2362 memory temp 저장 효율성 개선
    // VMTR의 columns를 복사한다.
    IDE_DASSERT( aDataPlan->plan.myTuple->columnCount == sVMTRTuple->columnCount );
    
    idlOS::memcpy( (void*)aDataPlan->plan.myTuple->columns,
                   (void*)sVMTRTuple->columns,
                   ID_SIZEOF(mtcColumn) * sVMTRTuple->columnCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC
qmnVSCN::execChild( qcTemplate * aTemplate,
                    qmncVSCN   * aCodePlan )
{
/***********************************************************************
 *
 * Description :
 *    Child Plan을 VMTR을 초기화한다.
 *    이 때, VMTR은 모든 Data를 저장하게 된다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::execChild"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // Child VMTR을 수행
    //---------------------------------

    // VMTR은 초기화 자체로 저장이 된다.
    IDE_TEST( aCodePlan->plan.left->init( aTemplate,
                                          aCodePlan->plan.left )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVSCN::getNullRow( qcTemplate * aTemplate,
                     qmncVSCN   * aCodePlan,
                     qmndVSCN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     VMTR로부터 Null Row를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    iduMemory * sMemory;

    // 적합성 검사
    IDE_DASSERT( aDataPlan->nullRowSize > 0 );

    sMemory = aTemplate->stmt->qmxMem;

    if ( ( aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK )
         == QMN_PLAN_STORAGE_DISK )
    {
        // Null Row를 위한 공간 할당
        IDU_FIT_POINT( "qmnVSCN::getNullRow::cralloc::nullRow",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( sMemory->cralloc( aDataPlan->nullRowSize,
                                    & aDataPlan->nullRow )
                  != IDE_SUCCESS);

        // Null Row 획득
        IDE_TEST( qmnVMTR::getNullRowDisk( aTemplate,
                                           aCodePlan->plan.left,
                                           aDataPlan->nullRow,
                                           & aDataPlan->nullRID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Null Row 획득
        IDE_TEST( qmnVMTR::getNullRowMemory( aTemplate,
                                             aCodePlan->plan.left,
                                             &aDataPlan->nullRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnVSCN::openCursor( qmndVSCN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Disk Temp Table 사용 시 호출되며,
 *     해당 정보를 이용하여 Cursor를 연다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::openCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if( aDataPlan->tempCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor를 최초 여는 경우
        //-----------------------------------------
        IDE_TEST( smiSortTempTable::openCursor( 
                aDataPlan->tableHandle,
                SMI_TCFLAG_FORWARD | 
                SMI_TCFLAG_ORDEREDSCAN |
                SMI_TCFLAG_IGNOREHIT,
                NULL,                         // Update Column정보
                smiGetDefaultKeyRange(),      // Key Range
                smiGetDefaultKeyRange(),      // Key Filter
                smiGetDefaultFilter(),        // Filter
                &aDataPlan->tempCursor )
            != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // 2. Cursor가 열려 있는 경우
        //-----------------------------------------
        IDE_TEST( smiSortTempTable::restartCursor( 
                aDataPlan->tempCursor,
                SMI_TCFLAG_FORWARD | 
                SMI_TCFLAG_ORDEREDSCAN |
                SMI_TCFLAG_IGNOREHIT,
                smiGetDefaultKeyRange(),      // Key Range
                smiGetDefaultKeyRange(),      // Key Filter
                smiGetDefaultFilter() )         // Filter
            != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnVSCN::setTupleSet( qcTemplate * aTemplate,
                      qmncVSCN   * aCodePlan,
                      qmndVSCN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     VSCN tuple 복원
 *
 * Implementation :
 *     VMTR tuple을 복원한뒤 VSCN을 복원(copy)한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnVSCN::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;
    mtcTuple   * sVMTRTuple;
    mtcColumn  * sMyColumn;
    mtcColumn  * sVMTRColumn;
    UInt         i;
    
    // PROJ-2362 memory temp 저장 효율성 개선
    // VSCN tuple 복원
    if ( aDataPlan->memSortRecord != NULL )
    {
        for ( sNode = aDataPlan->memSortRecord;
              sNode != NULL;
              sNode = sNode->next )
        {
            IDE_TEST( sNode->func.setTuple( aTemplate,
                                            sNode,
                                            aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }

        IDE_TEST( qmnVMTR::getTuple( aTemplate,
                                     aCodePlan->plan.left,
                                     & sVMTRTuple )
                  != IDE_SUCCESS );
        
        sMyColumn = aDataPlan->plan.myTuple->columns;
        sVMTRColumn = sVMTRTuple->columns;
        for ( i = 0; i < sVMTRTuple->columnCount; i++, sMyColumn++, sVMTRColumn++ )
        {
            if ( SMI_COLUMN_TYPE_IS_TEMP( sMyColumn->column.flag ) == ID_TRUE )
            {
                IDE_DASSERT( sVMTRColumn->module->actualSize(
                                 sVMTRColumn,
                                 sVMTRColumn->column.value )
                             <= sVMTRColumn->column.size );
                
                idlOS::memcpy( (SChar*)sMyColumn->column.value,
                               (SChar*)sVMTRColumn->column.value,
                               sVMTRColumn->module->actualSize(
                                   sVMTRColumn,
                                   sVMTRColumn->column.value ) );
            }
            else
            {
                // Nothing to do.
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

#undef IDE_FN
}

IDE_RC qmnVSCN::touchDependency( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *    dependency를 변경시킨다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // qmncVSCN * sCodePlan = (qmncVSCN*) aPlan;
    qmndVSCN * sDataPlan =
        (qmndVSCN*) (aTemplate->tmplate.data + aPlan->offset);
    
    sDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;
}
