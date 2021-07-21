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
 * $Id$
 *
 * Description :
 *     SDIN(SharD INsert) Node
 *
 *     관계형 모델에서 insert를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <cm.h>
#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qmnShardInsert.h>
#include <qdbCommon.h>
#include <qmx.h>
#include <qmxShard.h>

IDE_RC qmnSDIN::init( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SDIN 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnSDIN::doItDefault;

    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( ( *sDataPlan->flag & QMND_SDIN_INIT_DONE_MASK )
         == QMND_SDIN_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );

        //---------------------------------
        // 초기화 완료를 표기
        //---------------------------------

        *sDataPlan->flag &= ~QMND_SDIN_INIT_DONE_MASK;
        *sDataPlan->flag |= QMND_SDIN_INIT_DONE_TRUE;
    }
    else
    {
        //-----------------------------------
        // init lob info
        //-----------------------------------

        if ( sDataPlan->lobInfo != NULL )
        {
            (void) qmxShard::initLobInfo( sDataPlan->lobInfo );
        }
        else
        {
            // Nothing to do.
        }
    }

    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    if ( aPlan->left != NULL )
    {
        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    if ( sCodePlan->isInsertSelect == ID_TRUE )
    {
        sDataPlan->doIt = qmnSDIN::doItFirst;
    }
    else
    {
        sDataPlan->doIt = qmnSDIN::doItFirstMultiRows;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::doIt( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SDIN 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );
}

IDE_RC qmnSDIN::padNull( qcTemplate * aTemplate,
                         qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SDIN 노드는 별도의 null row를 가지지 않으며,
 *    Child에 대하여 padNull()을 호출한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    // qmndSDIN * sDataPlan = 
    //     (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_SDIN_INIT_DONE_MASK)
         == QMND_SDIN_INIT_DONE_FALSE )
    {
        // 초기화되지 않은 경우 초기화 수행
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // Child Plan에 대하여 Null Padding수행
    if ( aPlan->left != NULL )
    {
        IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::printPlan( qcTemplate   * aTemplate,
                           qmnPlan      * aPlan,
                           ULong          aDepth,
                           iduVarString * aString,
                           qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    SDIN 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiClientInfo * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;
    qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    qmmValueNode * sValue;
    qmmMultiRows * sMultiRows;

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    //------------------------------------------------------
    // SDIN Target 정보의 출력
    //------------------------------------------------------

    // SDIN 정보의 출력
    qmn::printSpaceDepth( aString, aDepth );
    iduVarStringAppendFormat( aString,
                              "SHARD-INSERT ( TABLE: " );

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
    // New line 출력
    //----------------------------
    iduVarStringAppend( aString, " )\n" );

    //------------------------------------------------------
    // BUG-38343 VALUES 내부의 Subquery 정보 출력
    //------------------------------------------------------

    for ( sMultiRows = sCodePlan->rows;
          sMultiRows != NULL;
          sMultiRows = sMultiRows->next )
    {
        for ( sValue = sMultiRows->values;
              sValue != NULL;
              sValue = sValue->next)
        {
            IDE_TEST( qmn::printSubqueryPlan( aTemplate,
                                              sValue->value,
                                              aDepth,
                                              aString,
                                              aMode ) != IDE_SUCCESS );
        }
    }

    //----------------------------
    // 수행 정보의 상세 출력
    //----------------------------

    if ( ( ( QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1 ) ||
           ( SDU_SHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE == 1 ) ) &&
         ( sClientInfo != NULL ) )
    {
        //---------------------------------------------
        // shard execution
        //---------------------------------------------

        IDE_TEST( qmnSDEX::printDataInfo( aTemplate,
                                          sClientInfo,
                                          sDataPlan->mDataInfo,
                                          aDepth + 1,
                                          aString,
                                          aMode,
                                          sDataPlan->flag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    //------------------------------------------------------
    // Child Plan 정보의 출력
    //------------------------------------------------------

    if ( aPlan->left != NULL )
    {
        IDE_TEST( aPlan->left->printPlan( aTemplate,
                                          aPlan->left,
                                          aDepth + 1,
                                          aString,
                                          aMode ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::firstInit( qcTemplate * aTemplate,
                           qmncSDIN   * aCodePlan,
                           qmndSDIN   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    SDIN node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *    - Data 영역의 주요 멤버에 대한 초기화를 수행
 *
 ***********************************************************************/

    sdiClientInfo  * sClientInfo = NULL;
    sdiBindParam   * sBindParams = NULL;
    sdiDataNode      sDataNodeArg;
    smiValue       * sInsertedRow;
    sdiSVPStep       sSVPStep = SDI_SVP_STEP_DO_NOT_NEED_SAVEPOINT;

    //---------------------------------
    // 기본 설정
    //---------------------------------

    IDE_TEST_RAISE( aTemplate->shardExecData.execInfo == NULL,
                    ERR_NO_SHARD_INFO );

    aDataPlan->mDataInfo = ((sdiDataNodes*)aTemplate->shardExecData.execInfo)
        + aCodePlan->shardDataIndex;

    aDataPlan->rows = aCodePlan->rows;

    /*
     * PROJ-2728 Sharding LOB
     */
    if ( aCodePlan->tableRef->tableInfo->lobColumnCount > 0 )
    {
        // PROJ-1362
        IDE_TEST( qmxShard::initializeLobInfo(
                    aTemplate->stmt,
                    &(aDataPlan->lobInfo),
                    (UShort)aCodePlan->tableRef->tableInfo->lobColumnCount )
                  != IDE_SUCCESS );

        /* BUG-30351
         * insert into select에서 각 Row Insert 후 해당 Lob Cursor를 바로 해제했으면 합니다.
         */
        if ( aCodePlan->isInsertSelect == ID_TRUE )
        {
            qmx::setImmediateCloseLobInfo( aDataPlan->lobInfo, ID_TRUE );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        aDataPlan->lobInfo = NULL;
    }

    //------------------------------------------
    // INSERT를 위한 Default ROW 구성
    //------------------------------------------

    if ( aCodePlan->isInsertSelect == ID_TRUE )
    {
        sInsertedRow = aTemplate->insOrUptRow[aCodePlan->valueIdx];

        if ( aDataPlan->rows != NULL )
        {
            // set DEFAULT value
            IDE_TEST( qmx::makeSmiValueWithValue( aCodePlan->columnsForValues,
                                                  aDataPlan->rows->values,
                                                  aTemplate,
                                                  aCodePlan->tableRef->tableInfo,
                                                  sInsertedRow,
                                                  aDataPlan->lobInfo )
                      != IDE_SUCCESS);
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    //-------------------------------
    // 수행노드 초기화
    //-------------------------------

    // shard linker 검사 & 초기화
    IDE_TEST( sdi::checkShardLinker( aTemplate->stmt ) != IDE_SUCCESS );

    //-------------------------------
    // shard 수행을 위한 준비
    //-------------------------------
    
    if ( QCG_GET_SESSION_IS_AUTOCOMMIT( aTemplate->stmt ) == ID_TRUE )
    {
        sSVPStep = SDI_SVP_STEP_DO_NOT_NEED_SAVEPOINT;
    }
    else
    {
        sSVPStep = SDI_SVP_STEP_NEED_SAVEPOINT;
    }

    sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;

    if ( aDataPlan->mDataInfo->mInitialized == ID_FALSE )
    {
        idlOS::memset( &sDataNodeArg, 0x00, ID_SIZEOF(sdiDataNode) );

        sDataNodeArg.mBindParamCount = aCodePlan->shardParamCount;
        sDataNodeArg.mBindParams = (sdiBindParam*)
            ( aTemplate->shardExecData.data + aCodePlan->bindParam );

        /* PROJ-2728 Sharding LOB */
        sDataNodeArg.mOutBindParams = (sdiOutBindParam*)
            ( aTemplate->shardExecData.data + aCodePlan->outBindParam );
        idlOS::memset( sDataNodeArg.mOutBindParams, 0x00,
                ID_SIZEOF(sdiOutBindParam) * aCodePlan->shardParamCount );

        // 초기화
        IDE_TEST( setParamInfo( aTemplate,
                                aCodePlan,
                                sDataNodeArg.mBindParams )
                  != IDE_SUCCESS );

        sDataNodeArg.mRemoteStmt = NULL;

        sDataNodeArg.mSVPStep= sSVPStep;

        IDE_TEST( sdi::initShardDataInfo( aTemplate,
                                          aCodePlan->shardAnalysis,
                                          sClientInfo,
                                          aDataPlan->mDataInfo,
                                          & sDataNodeArg )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aCodePlan->shardParamCount > 0 )
        {
            IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                          ID_SIZEOF(sdiBindParam) * aCodePlan->shardParamCount,
                          (void**) & sBindParams )
                      != IDE_SUCCESS );

            IDE_TEST( setParamInfo( aTemplate,
                                    aCodePlan,
                                    sBindParams )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( sdi::reuseShardDataInfo( aTemplate,
                                           sClientInfo,
                                           aDataPlan->mDataInfo,
                                           sBindParams,
                                           aCodePlan->shardParamCount,
                                           sSVPStep )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_SHARD_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnSDIN::firstInit",
                                  "Shard Info is not found" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::setParamInfo( qcTemplate   * aTemplate,
                              qmncSDIN     * aCodePlan,
                              sdiBindParam * aBindParams )
{
    qcmTableInfo   * sTableForInsert;
    qcmColumn      * sColumn;
    mtcTuple       * sCanonizedTuple;
    mtcColumn      * sMtcColumn;
    UInt             i;
    UInt             j;

    sTableForInsert = aCodePlan->tableRef->tableInfo;
    sCanonizedTuple = &(aTemplate->tmplate.rows[aCodePlan->canonizedTuple]);

    for ( i = 0, j = 0; i < sTableForInsert->columnCount; i++ )
    {
        sColumn = &(sTableForInsert->columns[i]);

        if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_FALSE )
        {
            sMtcColumn = &(sCanonizedTuple->columns[i]);

            aBindParams[j].mId        = j + 1;
            aBindParams[j].mInoutType = CMP_DB_PARAM_INPUT;
            aBindParams[j].mType      = sColumn->basicInfo->module->id;
            aBindParams[j].mData      =
                (UChar*)sCanonizedTuple->row + sMtcColumn->column.offset;
            aBindParams[j].mDataSize  = sMtcColumn->column.size;
            aBindParams[j].mPrecision = sMtcColumn->precision;
            aBindParams[j].mScale     = sMtcColumn->scale;

            /* BUG-46623 padding 변수를 0으로 초기화 해야 한다. */
            aBindParams[j].padding    = 0;

            j++;
            IDE_DASSERT( j <= aCodePlan->shardParamCount );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC qmnSDIN::doItDefault( qcTemplate * /* aTemplate */,
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

IDE_RC qmnSDIN::doItFirst( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SDIN의 최초 수행 함수
 *
 * Implementation :
 *    - Table에 IX Lock을 건다.
 *    - Session Event Check (비정상 종료 Detect)
 *    - Cursor Open
 *    - insert one record
 *
 ***********************************************************************/

    //qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    //-----------------------------------
    // Child Plan을 수행함
    //-----------------------------------

    // doIt left child
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
              != IDE_SUCCESS );

    //-----------------------------------
    // Insert를 수행함
    //-----------------------------------

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        // insert one record
        IDE_TEST( insertOneRow( aTemplate, aPlan ) != IDE_SUCCESS );

        sDataPlan->doIt = qmnSDIN::doItNext;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDataPlan->lobInfo != NULL )
    {
        (void)qmx::finalizeLobInfo( aTemplate->stmt, sDataPlan->lobInfo );
    }

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::doItNext( qcTemplate * aTemplate,
                          qmnPlan    * aPlan,
                          qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SDIN의 다음 수행 함수
 *    다음 Record를 삭제한다.
 *
 * Implementation :
 *    - insert one record
 *
 ***********************************************************************/

    //qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    //-----------------------------------
    // Child Plan을 수행함
    //-----------------------------------

    // doIt left child
    IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
              != IDE_SUCCESS );

    //-----------------------------------
    // Insert를 수행함
    //-----------------------------------

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        // insert one record
        IDE_TEST( insertOneRow( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnSDIN::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDataPlan->lobInfo != NULL )
    {
        (void)qmx::finalizeLobInfo( aTemplate->stmt, sDataPlan->lobInfo );
    }

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::doItFirstMultiRows( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag )
{
    //qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    //-----------------------------------
    // Insert를 수행함
    //-----------------------------------

    // insert one record
    IDE_TEST( insertOnce( aTemplate, aPlan ) != IDE_SUCCESS );

    if ( sDataPlan->rows->next != NULL )
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;

        sDataPlan->doIt = qmnSDIN::doItNextMultiRows;
    }
    else
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDataPlan->lobInfo != NULL )
    {
        (void)qmx::finalizeLobInfo( aTemplate->stmt, sDataPlan->lobInfo );
    }

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::doItNextMultiRows( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   qmcRowFlag * aFlag )
{
    //qmncSDIN * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST_RAISE( sDataPlan->rows->next == NULL, ERR_UNEXPECTED );

    //-----------------------------------
    // 다름 Row를 선택
    //-----------------------------------
    sDataPlan->rows = sDataPlan->rows->next;

    //-----------------------------------
    // Insert를 수행함
    //-----------------------------------

    // insert one record
    IDE_TEST( insertOnce( aTemplate, aPlan ) != IDE_SUCCESS );

    if ( sDataPlan->rows->next == NULL )
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_NONE;

        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnSDIN::doItFirstMultiRows;
    }
    else
    {
        *aFlag &= ~QMC_ROW_DATA_MASK;
        *aFlag |= QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnInsert::doItNextMultiRows",
                                  "Invalid Next rows" ));
    }
    IDE_EXCEPTION_END;

    if ( sDataPlan->lobInfo != NULL )
    {
        (void)qmx::finalizeLobInfo( aTemplate->stmt, sDataPlan->lobInfo );
    }

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::insertOneRow( qcTemplate * aTemplate,
                              qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SDIN 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    - insert one record 수행
 *
 ***********************************************************************/

    qmncSDIN         * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN         * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    iduMemoryStatus    sQmxMemStatus;
    qcmTableInfo     * sTableForInsert;
    mtcTuple         * sCanonizedTuple;
    smiValue         * sInsertedRow;

    sdiClientInfo    * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;
    vSLong             sNumRows = 0;

    //---------------------------------
    // 기본 설정
    //---------------------------------

    sCanonizedTuple = &(aTemplate->tmplate.rows[sCodePlan->canonizedTuple]);
    sTableForInsert = sCodePlan->tableRef->tableInfo;
    sInsertedRow = aTemplate->insOrUptRow[sCodePlan->valueIdx];

    // Memory 재사용을 위하여 현재 위치 기록
    IDE_TEST( aTemplate->stmt->qmxMem->getStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    //-----------------------------------
    // set next sequence
    //-----------------------------------

    // Sequence Value 획득
    if ( sCodePlan->nextValSeqs != NULL )
    {
        IDE_TEST( qmx::readSequenceNextVals(
                      aTemplate->stmt,
                      sCodePlan->nextValSeqs )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // make insert value
    //-----------------------------------

    // stack의 값을 이용
    IDE_TEST( qmx::makeSmiValueWithResult( sCodePlan->columns,
                                           aTemplate,
                                           sTableForInsert,
                                           sInsertedRow,
                                           sDataPlan->lobInfo )
              != IDE_SUCCESS );

    //-----------------------------------
    // check null
    //-----------------------------------

    IDE_TEST( qmx::checkNotNullColumnForInsert(
                  sTableForInsert->columns,
                  sInsertedRow,
                  NULL,
                  ID_TRUE )
              != IDE_SUCCESS );

    //------------------------------------------
    // Shard Insert를 위한 Default ROW 구성
    //------------------------------------------

    if ( ( sCodePlan->columnsForValues != NULL ) &&
         ( sDataPlan->rows != NULL ) &&
         ( sCodePlan->nextValSeqs != NULL ) )
    {
        sInsertedRow = aTemplate->insOrUptRow[sCodePlan->valueIdx];

        // set DEFAULT value
        IDE_TEST( qmx::makeSmiValueWithValue( sCodePlan->columnsForValues,
                                              sDataPlan->rows->values,
                                              aTemplate,
                                              sTableForInsert,
                                              sInsertedRow,
                                              sDataPlan->lobInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    //-----------------------------------
    // canonized tuple로 값을 복사
    //-----------------------------------

    IDE_TEST( copySmiValueToTuple( aTemplate,
                                   sTableForInsert,
                                   sInsertedRow,
                                   sCanonizedTuple,
                                   (sdiBindParam *)(aTemplate->shardExecData.data +
                                       sCodePlan->bindParam),
                                   (sdiOutBindParam *)(aTemplate->shardExecData.data +
                                       sCodePlan->outBindParam),
                                   sDataPlan->lobInfo )
              != IDE_SUCCESS );

    //-------------------------------
    // 수행노드 결정
    //-------------------------------

    IDE_TEST( sdi::decideShardDataInfo(
                  aTemplate,
                  &(aTemplate->tmplate.rows[sCodePlan->canonizedTuple]),
                  sCodePlan->shardAnalysis,
                  sClientInfo,
                  sDataPlan->mDataInfo,
                  &(sCodePlan->shardQuery) )
              != IDE_SUCCESS );

    /* PROJ-2733-DistTxInfo 분산정보를 설정하자. */ 
    sdi::calculateGCTxInfo( aTemplate,
                            sDataPlan->mDataInfo,
                            aTemplate->shardExecData.globalPSM,
                            sCodePlan->shardDataIndex );

    //-------------------------------
    // 수행
    //-------------------------------

    IDE_TEST( sdi::executeInsert( aTemplate->stmt,
                                  sClientInfo,
                                  sDataPlan->mDataInfo,
                                  sDataPlan->lobInfo,
                                  & sNumRows )
              != IDE_SUCCESS );

    // result row count
    aTemplate->numRows += sNumRows;

    //-----------------------------------
    // clear lob info
    //-----------------------------------
    if ( sDataPlan->lobInfo != NULL )
    {
        (void) qmxShard::initLobInfo( sDataPlan->lobInfo );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // 완료
    //-----------------------------------

    // Memory 재사용을 위한 Memory 이동
    IDE_TEST( aTemplate->stmt->qmxMem->setStatus( &sQmxMemStatus )
              != IDE_SUCCESS);

    if ( ( *sDataPlan->flag & QMND_SDIN_INSERT_MASK )
         == QMND_SDIN_INSERT_FALSE )
    {
        *sDataPlan->flag &= ~QMND_SDIN_INSERT_MASK;
        *sDataPlan->flag |= QMND_SDIN_INSERT_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::insertOnce( qcTemplate * aTemplate,
                            qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    insertOnce는 insertOneRow와 cursor를 open하는 시점이 다르다.
 *    insertOnce에서는 makeSmiValue이후에 cursor를 open한다.
 *    다음 쿼리에서 t1 insert cursor를 먼저 열면 subquery가 수행될 수 없다.
 *
 *    ex)
 *    insert into t1 values ( select max(i1) from t1 );
 *
 * Implementation :
 *    - cursor open
 *    - insert one record 수행
 *    - trigger each row 수행
 *
 ***********************************************************************/

    qmncSDIN       * sCodePlan = (qmncSDIN*) aPlan;
    qmndSDIN       * sDataPlan =
        (qmndSDIN*) (aTemplate->tmplate.data + aPlan->offset);

    qcmTableInfo   * sTableForInsert;
    mtcTuple       * sCanonizedTuple;
    smiValue       * sInsertedRow;

    sdiClientInfo  * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;
    vSLong           sNumRows = 0;

    sCanonizedTuple = &(aTemplate->tmplate.rows[sCodePlan->canonizedTuple]);
    sTableForInsert = sCodePlan->tableRef->tableInfo;
    sInsertedRow = aTemplate->insOrUptRow[sCodePlan->valueIdx];

    //-----------------------------------
    // set next sequence
    //-----------------------------------

    // Sequence Value 획득
    if ( sCodePlan->nextValSeqs != NULL )
    {
        IDE_TEST( qmx::readSequenceNextVals(
                      aTemplate->stmt,
                      sCodePlan->nextValSeqs )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // make insert value
    //-----------------------------------

    // values의 값을 이용
    IDE_TEST( qmx::makeSmiValueWithValue( aTemplate,
                                          sTableForInsert,
                                          sCodePlan->canonizedTuple,
                                          sDataPlan->rows->values,
                                          sCodePlan->queueMsgIDSeq,
                                          sInsertedRow,
                                          sDataPlan->lobInfo )
              != IDE_SUCCESS );

    //-----------------------------------
    // canonized tuple로 값을 복사
    //-----------------------------------

    IDE_TEST( copySmiValueToTuple( aTemplate,
                                   sTableForInsert,
                                   sInsertedRow,
                                   sCanonizedTuple,
                                   (sdiBindParam *)(aTemplate->shardExecData.data +
                                       sCodePlan->bindParam),
                                   (sdiOutBindParam *)(aTemplate->shardExecData.data +
                                       sCodePlan->outBindParam),
                                   sDataPlan->lobInfo )
              != IDE_SUCCESS );

    //-----------------------------------
    // check null
    //-----------------------------------

    IDE_TEST( qmx::checkNotNullColumnForInsert(
                  sTableForInsert->columns,
                  sInsertedRow,
                  NULL,
                  ID_TRUE )
              != IDE_SUCCESS );

    //-------------------------------
    // 수행노드 결정
    //-------------------------------

    IDE_TEST( sdi::decideShardDataInfo(
                  aTemplate,
                  &(aTemplate->tmplate.rows[sCodePlan->canonizedTuple]),
                  sCodePlan->shardAnalysis,
                  sClientInfo,
                  sDataPlan->mDataInfo,
                  &(sCodePlan->shardQuery) )
              != IDE_SUCCESS );

    /* PROJ-2733-DistTxInfo 분산정보를 설정하자. */ 
    sdi::calculateGCTxInfo( aTemplate,
                            sDataPlan->mDataInfo,
                            aTemplate->shardExecData.globalPSM,
                            sCodePlan->shardDataIndex );

    //-------------------------------
    // 수행
    //-------------------------------

    IDE_TEST( sdi::executeInsert( aTemplate->stmt,
                                  sClientInfo,
                                  sDataPlan->mDataInfo,
                                  sDataPlan->lobInfo,
                                  & sNumRows )
              != IDE_SUCCESS );

    // result row count
    aTemplate->numRows += sNumRows;

    //-----------------------------------
    // clear lob info
    //-----------------------------------
    if ( sDataPlan->lobInfo != NULL )
    {
        (void) qmxShard::initLobInfo( sDataPlan->lobInfo );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // 완료
    //-----------------------------------

    if ( ( *sDataPlan->flag & QMND_SDIN_INSERT_MASK )
         == QMND_SDIN_INSERT_FALSE )
    {
        *sDataPlan->flag &= ~QMND_SDIN_INSERT_MASK;
        *sDataPlan->flag |= QMND_SDIN_INSERT_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSDIN::copySmiValueToTuple( qcTemplate      * aTemplate,
                                     qcmTableInfo    * aTableInfo,
                                     smiValue        * aInsertedRow,
                                     mtcTuple        * aTuple,
                                     sdiBindParam    * aBindParams,
                                     sdiOutBindParam * aOutBindParams,
                                     qmxLobInfo      * aLobInfo )
{
    qcmColumn  * sColumn          = NULL;
    SInt         sColumnOrder;
    mtcColumn  * sCanonizedColumn = NULL;
    mtcColumn  * sStoringColumn   = NULL;
    void       * sCanonizedValue  = NULL;
    void       * sValue           = NULL;
    UInt         sActualSize;
    UInt         sClientCount;
    SInt         sBindId;

    sClientCount = aTemplate->stmt->session->mQPSpecific.mClientInfo->mCount;

    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aTuple != NULL );

    for ( sColumn = aTableInfo->columns, sBindId = 0;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
             == QCM_COLUMN_HIDDEN_COLUMN_FALSE )
        {
            IDE_DASSERT( sColumn->basicInfo != NULL );
            sColumnOrder = sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

            sCanonizedColumn = &(aTuple->columns[sColumnOrder]);
            sStoringColumn = sColumn->basicInfo;

            IDE_DASSERT( sCanonizedColumn != NULL );
            sCanonizedValue = (void*)
                ((UChar*)aTuple->row + sCanonizedColumn->column.offset);

            IDE_DASSERT( sColumn->basicInfo->column.id == sCanonizedColumn->column.id );

            if ( (sColumn->basicInfo->column.flag & SMI_COLUMN_TYPE_MASK)
                  != SMI_COLUMN_TYPE_LOB )
            {
                IDE_TEST( qdbCommon::storingValue2MtdValue(
                              sStoringColumn,
                              (void*) aInsertedRow[sColumnOrder].value,
                              & sValue )
                          != IDE_SUCCESS );

                // BUG-45751
                if ( sValue == NULL )
                {
                    sValue = sCanonizedColumn->module->staticNull;
                }
                else
                {
                    // Nothing to do.
                }

                if ( sCanonizedValue != sValue )
                {
                    sActualSize = sCanonizedColumn->module->actualSize(
                        sCanonizedColumn,
                        sValue );

                    idlOS::memcpy( sCanonizedValue,
                                   sValue,
                                   sActualSize );
                }
                else
                {
                    // Nothing to do.
                }

                aOutBindParams[sBindId].mIndicator  = 0;
                aOutBindParams[sBindId].mShadowData = NULL;
            }
            else // LOB
            {
                /*
                 * PROJ-2728 Sharding LOB 
                 *
                 * 1. qmx::makeSmiValueWithResult/WithValue를 통해서
                 *    lobInfo에 src. locator가 저장되었다면 (qmx::addLobInfoForCopy)
                 *    locator OUT 바인딩후, getLob -> putLob (qmxShard::copyAndOutBindLobInfo)
                 * 2. 그렇지 않다면 CHAR 또는 BINARY 바인딩
                 */
                // 1. locator
                if ( existInLobInfoAndAdjustBindId( aLobInfo,
                                                    sColumnOrder,
                                                    sBindId )
                     == ID_TRUE )
                {
                    sActualSize = ID_SIZEOF(smLobLocator);

                    aBindParams[sBindId].mInoutType = CMP_DB_PARAM_OUTPUT;
                    aBindParams[sBindId].mType = sColumn->basicInfo->module->id + 1;
                    aBindParams[sBindId].mDataSize  = sActualSize;

                    if ( aOutBindParams[sBindId].mShadowData == NULL )
                    {
                        IDE_TEST( QC_QME_MEM( aTemplate->stmt)->alloc(
                                  sActualSize * sClientCount,
                                  (void**) &(aOutBindParams[sBindId].mShadowData) )
                              != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                // 2. non-locator
                else
                {
                    /* constant value like 'INSERT INTO T1 SELECT 'ABC' FROM T1' */

                    // clob을 SQL_C_CHAR로, blob을 SQL_C_BINARY로 바인딩.
                    //        sdl::bindParam에서 바인딩 타입 지정.
                    //   autocommit on이어도 가능.
                    if ( aInsertedRow[sColumnOrder].length > 0 )
                    {
                        sActualSize = aInsertedRow[sColumnOrder].length;
                        idlOS::memcpy( sCanonizedValue,
                                       aInsertedRow[sColumnOrder].value,
                                       aInsertedRow[sColumnOrder].length );
                    }
                    else
                    {
                        sActualSize = 0;
                    }
                    aBindParams[sBindId].mInoutType = CMP_DB_PARAM_INPUT;
                    aBindParams[sBindId].mPrecision = MTD_CHAR_PRECISION_MAXIMUM;
                    aBindParams[sBindId].mDataSize  = MTD_CHAR_PRECISION_MAXIMUM;

                    aOutBindParams[sBindId].mIndicator  = sActualSize;
                    aOutBindParams[sBindId].mShadowData = NULL;
#if 0
                    // locator를 사용하는 방법.
                    //        locator OUT 으로 바인딩한 후 putLob.
                    //   autocommit off일 때만 가능.
                    aBindParams[sBindId].mInoutType = CMP_DB_PARAM_OUTPUT;
                    aBindParams[sBindId].mType = sColumn->basicInfo->module->id + 1;
                    aBindParams[sBindId].mDataSize  = ID_SIZEOF(smLobLocator);

                    // dest.
                    if ( aOutBindParams[sBindId].mShadowData == NULL )
                    {
                        IDE_TEST( QC_QME_MEM( aTemplate->stmt)->alloc(
                                  ID_SIZEOF(smLobLocator) * sClientCount,
                                  (void**) &(aOutBindParams[sBindId].mShadowData) )
                              != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                    (void) qmxShard::addLobInfoForPutLob(
                                      aLobInfo,
                                      sBindId );

                    // src.
                    sLobValue = (mtdLobType *) sCanonizedValue;
                    if ( aInsertedRow[sColumnOrder].value == NULL )
                    {
                        sLobValue->length = MTD_LOB_NULL_LENGTH;
                    }
                    else
                    {
                        sLobValue->length = aInsertedRow[sColumnOrder].length;
                    }
                    if ( aInsertedRow[sColumnOrder].length > 0 )
                    {
                        sLobValue->length = aInsertedRow[sColumnOrder].length;
                        idlOS::memcpy( sLobValue->value,
                                       aInsertedRow[sColumnOrder].value,
                                       aInsertedRow[sColumnOrder].length );
                    }
                    else
                    {
                        // Nothing to do.
                    }
#endif
                }
            }

            sBindId++;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmnSDIN::shardStmtPartialRollbackUsingSavepoint( qcTemplate  * aTemplate,
                                                      qmnPlan     * aPlan )
{
    qmndSDIN        * sDataPlan = (qmndSDIN*)(aTemplate->tmplate.data + aPlan->offset);
    sdiClientInfo   * sClientInfo = aTemplate->stmt->session->mQPSpecific.mClientInfo;

    if ( ( sDataPlan->mDataInfo != NULL ) &&
         ( sDataPlan->mDataInfo->mInitialized == ID_TRUE ) )
    {
        sdi::shardStmtPartialRollbackUsingSavepoint( aTemplate->stmt,
                                                     sClientInfo, 
                                                     sDataPlan->mDataInfo );
    }
}

/*
 * PROJ-2728 Sharding LOB
 *   hidden column이 있는 경우 ColumnOrder와 BindId가 다르므로 조정해야 함
 */
idBool qmnSDIN::existInLobInfoAndAdjustBindId(
                                     qmxLobInfo   * aLobInfo,
                                     SInt           aColumnOrder,
                                     SInt           aBindId )
{
    UInt   i;
    idBool sExist = ID_FALSE;

    if ( aLobInfo != NULL )
    {
        // for copy
        for ( i = 0;
              i < aLobInfo->count;
              i++ )
        {
            if ( aLobInfo->dstBindId[i] == aColumnOrder )
            {
                sExist = ID_TRUE;

                if ( aColumnOrder != aBindId )
                {
                    aLobInfo->dstBindId[i] = aBindId;
                }
                break;
            }
        }
    }

    return sExist;
}
