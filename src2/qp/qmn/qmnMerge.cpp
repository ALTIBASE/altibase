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
 * $Id: qmnMerge.cpp 55241 2012-08-27 09:13:19Z linkedlist $
 *
 * Description :
 *     MRGE(MeRGE) Node
 *
 *     관계형 모델에서 merge를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qmnMerge.h>
#include <qmnProject.h>
#include <qmnInsert.h>
#include <qmnUpdate.h>
#include <qmnDelete.h>

IDE_RC
qmnMRGE::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MRGE 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMRGE::init"));

    qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnMRGE::doItDefault;
    
    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( ( *sDataPlan->flag & QMND_MRGE_INIT_DONE_MASK )
         == QMND_MRGE_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
        
        //---------------------------------
        // 초기화 완료를 표기
        //---------------------------------
        
        *sDataPlan->flag &= ~QMND_MRGE_INIT_DONE_MASK;
        *sDataPlan->flag |= QMND_MRGE_INIT_DONE_TRUE;
    }
    else
    {
        // Nothing to do.
    }
        
    //------------------------------------------------
    // Child Plan의 초기화
    //------------------------------------------------

    // select source의 init
    IDE_TEST( initSource( aTemplate, aPlan ) != IDE_SUCCESS );
    
    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnMRGE::doItFirst;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MRGE 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );

#undef IDE_FN
}

IDE_RC 
qmnMRGE::padNull( qcTemplate * /* aTemplate */,
                  qmnPlan    * /* aPlan */ )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMRGE::padNull"));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    MRGE 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMRGE::printPlan"));

    qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan  * sPlan;
    ULong      i;
    
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    //------------------------------------------------------
    // 시작 정보의 출력
    //------------------------------------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //------------------------------------------------------
    // MRGE Target 정보의 출력
    //------------------------------------------------------

    // MRGE 정보의 출력
    if ( sCodePlan->tableRef->tableType == QCM_VIEW )
    {
        iduVarStringAppendFormat( aString,
                                  "MERGE ( VIEW: " );
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "MERGE ( TABLE: " );
    }

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

    //---------------------------------
    // select source child
    //---------------------------------
    
    sPlan = aPlan->children[QMO_MERGE_SELECT_SOURCE_IDX].childPlan;
    
    IDE_TEST( sPlan->printPlan( aTemplate,
                                sPlan,
                                aDepth + 1,
                                aString,
                                aMode ) != IDE_SUCCESS );

    //----------------------------
    // match condition, matched, not-mached 정보의 상세 출력
    //----------------------------
    
    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        // plan node로 구성되지만 구현이 이럴뿐 개념상
        // MERGE 노드의 predicate에 불과하다. 라고 주장한다.
        
        //---------------------------------
        // select target child
        //---------------------------------
    
        for ( i = 0; i < aDepth + 1; i++ )
        {
            iduVarStringAppend( aString,
                                " " );
        }
        iduVarStringAppendFormat( aString,
                                  "[ MATCH CONDITION ]\n" );
        
        sPlan = aPlan->children[QMO_MERGE_SELECT_TARGET_IDX].childPlan;
    
        IDE_TEST( sPlan->printPlan( aTemplate,
                                    sPlan,
                                    aDepth + 1,
                                    aString,
                                    aMode ) != IDE_SUCCESS );
    
        //---------------------------------
        // update child
        //---------------------------------

        if ( ( sCodePlan->updateStatement != NULL ) ||
             ( sCodePlan->deleteStatement != NULL ) )
        {
            for ( i = 0; i < aDepth + 1; i++ )
            {
                iduVarStringAppend( aString,
                                    " " );
            }
            iduVarStringAppendFormat( aString,
                                      "[ MATCHED ]\n" );
        }
        else
        {
            // Nothing to do.
        }
        
        sPlan = aPlan->children[QMO_MERGE_UPDATE_IDX].childPlan;
    
        if ( sPlan != NULL )
        {
            IDE_TEST( sPlan->printPlan( aTemplate,
                                        sPlan,
                                        aDepth + 1,
                                        aString,
                                        aMode ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        //---------------------------------
        // delete child
        //---------------------------------
    
        sPlan = aPlan->children[QMO_MERGE_DELETE_IDX].childPlan;
        
        if ( sPlan != NULL )
        {
            IDE_TEST( sPlan->printPlan( aTemplate,
                                        sPlan,
                                        aDepth + 1,
                                        aString,
                                        aMode ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        //---------------------------------
        // insert child
        //---------------------------------
        
        if ( sCodePlan->insertStatement != NULL )
        {
            for ( i = 0; i < aDepth + 1; i++ )
            {
                iduVarStringAppend( aString,
                                    " " );
            }
            iduVarStringAppendFormat( aString,
                                      "[ NOT MATCHED ]\n" );
        }
        else
        {
            // Nothing to do.
        }
        
        sPlan = aPlan->children[QMO_MERGE_INSERT_IDX].childPlan;
    
        if ( sPlan != NULL )
        {
            IDE_TEST( sPlan->printPlan( aTemplate,
                                        sPlan,
                                        aDepth + 1,
                                        aString,
                                        aMode ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        //---------------------------------
        // insert empty child
        //---------------------------------
        
        if ( sCodePlan->insertNoRowsStatement != NULL )
        {
            for ( i = 0; i < aDepth + 1; i++ )
            {
                (void) iduVarStringAppend( aString,
                                    " " );
            }
            (void) iduVarStringAppendFormat( aString,
                                      "[ NO ROWS ]\n" );
        }
        else
        {
            // Nothing to do.
        }
        
        sPlan = aPlan->children[QMO_MERGE_INSERT_NOROWS_IDX].childPlan;
    
        if ( sPlan != NULL )
        {
            IDE_TEST( sPlan->printPlan( aTemplate,
                                        sPlan,
                                        aDepth + 1,
                                        aString,
                                        aMode ) != IDE_SUCCESS );
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
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::firstInit( qcTemplate * aTemplate,
                    qmncMRGE   * aCodePlan,
                    qmndMRGE   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    MRGE node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *    - Data 영역의 주요 멤버에 대한 초기화를 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMRGE::firstInit"));

    idBool  sNeedToCloseLocalCursorMgr    = ID_FALSE;
    idBool  sNeedToCloseLocalTempTableMgr = ID_FALSE;

    //---------------------------------
    // 기본 설정
    //---------------------------------

    // select source statement를 변경시키므로 복사해서 사용한다.
    idlOS::memcpy( (void*) & aDataPlan->selectSourceStatement,
                   (void*) aCodePlan->selectSourceStatement,
                   ID_SIZEOF(qcStatement) );
    qmx::setSubStatement( aTemplate->stmt, & aDataPlan->selectSourceStatement );

    // select target statement를 변경시키므로 복사해서 사용한다.
    idlOS::memcpy( (void*) & aDataPlan->selectTargetStatement,
                   (void*) aCodePlan->selectTargetStatement,
                   ID_SIZEOF(qcStatement) );
    qmx::setSubStatement( aTemplate->stmt, & aDataPlan->selectTargetStatement );

    // update statement를 변경시키므로 복사해서 사용한다.
    if ( aCodePlan->updateStatement != NULL )
    {
        idlOS::memcpy( (void*) & aDataPlan->updateStatement,
                       (void*) aCodePlan->updateStatement,
                       ID_SIZEOF(qcStatement) );
        qmx::setSubStatement( aTemplate->stmt, & aDataPlan->updateStatement );
    }
    else
    {
        // Nothing to do.
    }

    // delete statement를 변경시키므로 복사해서 사용한다.
    if ( aCodePlan->deleteStatement != NULL )
    {
        idlOS::memcpy( (void*) & aDataPlan->deleteStatement,
                       (void*) aCodePlan->deleteStatement,
                       ID_SIZEOF(qcStatement) );
        qmx::setSubStatement( aTemplate->stmt, & aDataPlan->deleteStatement );
    }
    else
    {
        // Nothing to do.
    }

    // insert statement를 변경시키므로 복사해서 사용한다.
    if ( aCodePlan->insertStatement != NULL )
    {
        idlOS::memcpy( (void*) & aDataPlan->insertStatement,
                       (void*) aCodePlan->insertStatement,
                       ID_SIZEOF(qcStatement) );
        qmx::setSubStatement( aTemplate->stmt, & aDataPlan->insertStatement );
    }
    else
    {
        // Nothing to do.
    }
    
    // insert empty statement를 변경시키므로 복사해서 사용한다.
    if ( aCodePlan->insertNoRowsStatement != NULL )
    {
        idlOS::memcpy( (void*) & aDataPlan->insertNoRowsStatement,
                       (void*) aCodePlan->insertNoRowsStatement,
                       ID_SIZEOF(qcStatement) );
        qmx::setSubStatement( aTemplate->stmt, & aDataPlan->insertNoRowsStatement );
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-45763 MERGE 구문에서 CLOB Column Update 중 FATAL 발생합니다. */
    IDE_TEST( setBindParam( aTemplate, aDataPlan )
              != IDE_SUCCESS );

    IDE_TEST( aDataPlan->selectSourceCursorMgr.init( aTemplate->stmt->qmxMem )
              != IDE_SUCCESS );
    sNeedToCloseLocalCursorMgr = ID_TRUE;
    
    IDE_TEST( qmcTempTableMgr::init(
                  & (aDataPlan->selectSourceTempTableMgr), aTemplate->stmt->qmxMem )
              != IDE_SUCCESS );
    sNeedToCloseLocalTempTableMgr = ID_TRUE;
    
    aDataPlan->originalCursorMgr    = aTemplate->cursorMgr;
    aDataPlan->originalTempTableMgr = aTemplate->tempTableMgr;

    aDataPlan->originalStatement = aTemplate->stmt;

    // reset merge count
    aDataPlan->mergedCount = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sNeedToCloseLocalCursorMgr == ID_TRUE )
    {
        (void) aDataPlan->selectSourceCursorMgr.closeAllCursor(
            QC_STATISTICS(aTemplate->stmt) );
    }
    else
    {
        /* nothing to do */
    }
    if ( sNeedToCloseLocalTempTableMgr == ID_TRUE )
    {
        (void) qmcTempTableMgr::dropAllTempTable(
            & aDataPlan->selectSourceTempTableMgr );
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnMRGE::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MRGE의 최초 수행 함수
 *
 * Implementation :
 *    - merge one record
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;
    vSLong       sNumRows;

    iduMemoryStatus   sMemoryStatus;
    UInt              sStatus = 0;
    idBool            sInsertFlag = ID_FALSE;
    
    // reset merge count
    sDataPlan->mergedCount = 0;

    //-----------------------------------
    // read source
    //-----------------------------------

    IDE_TEST( readSource( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    IDE_TEST_RAISE( aTemplate->stmt->qmxMem->getStatus( &sMemoryStatus ) != IDE_SUCCESS, ERR_MEM_OP );
    sStatus = 1;

    if ( ( *aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // read target to check for on clause
        //-----------------------------------

        IDE_TEST( matchTarget( aTemplate, aPlan, &sFlag ) != IDE_SUCCESS );

        if ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            //-----------------------------------
            // matched
            //-----------------------------------

            if ( sCodePlan->updateStatement != NULL )
            {
                IDE_TEST( updateTarget( aTemplate, aPlan, &sNumRows ) != IDE_SUCCESS );

                sDataPlan->mergedCount = sNumRows;

                if ( ( sCodePlan->deleteStatement != NULL ) &&
                     ( sNumRows > 0 ) )
                {
                    IDE_TEST( deleteTarget( aTemplate, aPlan ) != IDE_SUCCESS );
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
        }
        else
        {
            //-----------------------------------
            // not matched
            //-----------------------------------
            if ( sCodePlan->insertStatement != NULL )
            {
                if ( sCodePlan->whereForInsert != NULL )
                {
                    IDE_TEST( checkWhereForInsert( aTemplate, aPlan, &sInsertFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    sInsertFlag = ID_TRUE;
                }

                if ( sInsertFlag == ID_TRUE )
                {
                    IDE_TEST( insertTarget( aTemplate, aPlan ) != IDE_SUCCESS );

                    sDataPlan->mergedCount = 1;
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
        }

        sDataPlan->doIt = qmnMRGE::doItNext;
    }
    else
    {
        //-----------------------------------
        // no rows
        //-----------------------------------

        // BUG-37535
        // source table이 no rows인 경우 insert를 수행한다.
        if ( sCodePlan->insertNoRowsStatement != NULL )
        {
            IDE_TEST( insertNoRowsTarget( aTemplate, aPlan ) != IDE_SUCCESS );

            sDataPlan->mergedCount = 1;
        }
        else
        {
            // Nothing to do.
        }
    }

    sStatus = 0;

    /* BUG-46229 */
    aTemplate->stmt = & sDataPlan->selectSourceStatement;

    IDE_TEST_RAISE( aTemplate->stmt->qmxMem->setStatus( &sMemoryStatus ) != IDE_SUCCESS, ERR_MEM_OP );

    //-----------------------------------
    // Restore Original Statement
    //-----------------------------------

    aTemplate->stmt = sDataPlan->originalStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qmnMRGE::doItFirst"
                     " memory error" );
    }
    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 1:
            /* BUG-46229 */
            aTemplate->stmt = & sDataPlan->selectSourceStatement;

            (void) aTemplate->stmt->qmxMem->setStatus( &sMemoryStatus );
            break;
        default:
            break;
    }

    finalize( aTemplate, aPlan );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MRGE의 다음 수행 함수
 *
 * Implementation :
 *    - merge one record
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmcRowFlag   sFlag = QMC_ROW_INITIALIZE;
    vSLong       sNumRows;
    UInt         i;

    iduMemoryStatus   sMemoryStatus;
    UInt              sStatus = 0;
    idBool            sInsertFlag = ID_FALSE;
    
    // reset merge count
    sDataPlan->mergedCount = 0;

    //-----------------------------------
    // read source
    //-----------------------------------

    IDE_TEST( readSource( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    IDE_TEST_RAISE( aTemplate->stmt->qmxMem->getStatus( &sMemoryStatus ) != IDE_SUCCESS, ERR_MEM_OP );
    sStatus = 1;

    //-----------------------------------
    // reset flags at tmplate for plan reuses
    //-----------------------------------
    for ( i = sCodePlan->resetPlanFlagStartIndex;
          i < sCodePlan->resetPlanFlagEndIndex;
          i++ )
    {
        aTemplate->planFlag[i] = QMN_PLAN_PREPARE_DONE_MASK;
    }
    for ( i = sCodePlan->resetExecInfoStartIndex;
          i < sCodePlan->resetExecInfoEndIndex;
          i++ )
    {
        aTemplate->tmplate.execInfo[i] = QTC_WRAPPER_NODE_EXECUTE_FALSE;
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // read target to check for on clause
        //-----------------------------------

        IDE_TEST( matchTarget( aTemplate, aPlan, &sFlag ) != IDE_SUCCESS );

        if ( ( sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            //-----------------------------------
            // matched
            //-----------------------------------

            if ( sCodePlan->updateStatement != NULL )
            {
                IDE_TEST( updateTarget( aTemplate, aPlan, &sNumRows ) != IDE_SUCCESS );

                sDataPlan->mergedCount = sNumRows;

                if ( ( sCodePlan->deleteStatement != NULL ) &&
                     ( sNumRows > 0 ) )
                {
                    IDE_TEST( deleteTarget( aTemplate, aPlan ) != IDE_SUCCESS );
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
        }
        else
        {
            //-----------------------------------
            // not matched
            //-----------------------------------
            if ( sCodePlan->insertStatement != NULL )
            {
                if ( sCodePlan->whereForInsert != NULL )
                {
                    IDE_TEST( checkWhereForInsert( aTemplate, aPlan, &sInsertFlag )
                              != IDE_SUCCESS );
                }
                else
                {
                    sInsertFlag = ID_TRUE;
                }

                if ( sInsertFlag == ID_TRUE )
                {
                    IDE_TEST( insertTarget( aTemplate, aPlan ) != IDE_SUCCESS );

                    sDataPlan->mergedCount = 1;
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
        }
    }
    else
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnMRGE::doItFirst;
    }

    sStatus = 0;

    /* BUG-46229 */
    aTemplate->stmt = & sDataPlan->selectSourceStatement;

    IDE_TEST_RAISE( aTemplate->stmt->qmxMem->setStatus( &sMemoryStatus ) != IDE_SUCCESS, ERR_MEM_OP );

    //-----------------------------------
    // Restore Original Statement
    //-----------------------------------

    aTemplate->stmt = sDataPlan->originalStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qmnMRGE::doItNext"
                     " memory error" );
    }
    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 1:
            /* BUG-46229 */
            aTemplate->stmt = & sDataPlan->selectSourceStatement;

            (void) aTemplate->stmt->qmxMem->setStatus( &sMemoryStatus );
            break;
        default:
            break;
    }

    finalize( aTemplate, aPlan );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::getMergedRowCount( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            vSLong     * aCount )
{
/***********************************************************************
 *
 * Description :
 *    merge count는 update row count와 insert row count의 합니다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::initSource"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    *aCount = sDataPlan->mergedCount;
    
    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnMRGE::initSource( qcTemplate * aTemplate,
                     qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    init source select
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::initSource"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);
    
    qmnPlan  * sPlan;
    
    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    sPlan = aPlan->children[QMO_MERGE_SELECT_SOURCE_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->selectSourceStatement;

    //-----------------------------------
    // Swap Original CursorMgr of tmplate with locals
    //-----------------------------------
    
    setCursorMgr( aTemplate,
                  & sDataPlan->selectSourceCursorMgr,
                  & sDataPlan->selectSourceTempTableMgr );

    //-----------------------------------
    // Execute SELECT SOURCE Plan Tree
    //-----------------------------------
    
    IDE_TEST( qmnPROJ::init( aTemplate, sPlan ) != IDE_SUCCESS );

    //-----------------------------------
    // Restore CursorMgr of tmplate
    //-----------------------------------  
  
    setCursorMgr( aTemplate,
                  sDataPlan->originalCursorMgr,
                  sDataPlan->originalTempTableMgr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-38982
    finalize( aTemplate, aPlan );
    
    setCursorMgr( aTemplate,
                  sDataPlan->originalCursorMgr,
                  sDataPlan->originalTempTableMgr );
   
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::readSource( qcTemplate * aTemplate,
                     qmnPlan    * aPlan,
                     qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    source select fetch one row
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::readSource"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan  * sPlan;
    
    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    sPlan = aPlan->children[QMO_MERGE_SELECT_SOURCE_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->selectSourceStatement;

    //-----------------------------------
    // Swap Original CursorMgr of tmplate with locals
    //-----------------------------------
    
    setCursorMgr( aTemplate,
                  & sDataPlan->selectSourceCursorMgr,
                  & sDataPlan->selectSourceTempTableMgr );

    //-----------------------------------
    // First Fetch from SELECT SOURCE
    //-----------------------------------
    
    IDE_TEST( qmnPROJ::doIt( aTemplate, sPlan, aFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // Restore CursorMgr of tmplate
    //-----------------------------------
    
    setCursorMgr( aTemplate,
                  sDataPlan->originalCursorMgr,
                  sDataPlan->originalTempTableMgr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    setCursorMgr( aTemplate,
                  sDataPlan->originalCursorMgr,
                  sDataPlan->originalTempTableMgr );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::matchTarget( qcTemplate * aTemplate,
                      qmnPlan    * aPlan,
                      qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    target select fetch one row
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::matchTarget"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan  * sPlan;
    UInt       sStatus = 0;

    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    sPlan = aPlan->children[QMO_MERGE_SELECT_TARGET_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->selectTargetStatement;

    //-----------------------------------
    // SELECT TARGET to check for ON Clause
    //-----------------------------------

    IDE_TEST( qmnPROJ::init( aTemplate, sPlan ) != IDE_SUCCESS );
    sStatus = 2;

    // 한 건이라도 존재하는지만 보면 된다.
    IDE_TEST( qmnPROJ::doIt( aTemplate, sPlan, aFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // close all cursor
    //-----------------------------------
    
    sStatus = 1;
    IDE_TEST( aTemplate->cursorMgr->closeAllCursor(
                  QC_STATISTICS(aTemplate->stmt) )
              != IDE_SUCCESS );
    sStatus = 0;
    IDE_TEST( qmcTempTableMgr::dropAllTempTable(
                  aTemplate->tempTableMgr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 2:
            (void) aTemplate->cursorMgr->closeAllCursor(
                      QC_STATISTICS(aTemplate->stmt) );
        case 1:
            (void) qmcTempTableMgr::dropAllTempTable(
                      aTemplate->tempTableMgr );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::updateTarget( qcTemplate * aTemplate,
                       qmnPlan    * aPlan,
                       vSLong     * aNumRows )
{
/***********************************************************************
 *
 * Description :
 *    update target
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::updateTarget"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan          * sPlan;
    qmcRowFlag         sFlag = QMC_ROW_INITIALIZE;
    vSLong             sNumRows = 0;
    UInt               sStatus = 0;

    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    // update plan
    sPlan = aPlan->children[QMO_MERGE_UPDATE_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->updateStatement;

    //-----------------------------------
    // UPDATE를 위한 plan tree 초기화
    //-----------------------------------

    IDE_TEST( qmnUPTE::init( aTemplate, sPlan ) != IDE_SUCCESS );
    sStatus = 3;

    //------------------------------------------
    // UPDATE를 수행
    //------------------------------------------

    do
    {
        // 비정상 종료 검사
        IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
                  != IDE_SUCCESS );
    
        sNumRows++;
        
        IDE_TEST( qmnUPTE::doIt( aTemplate, sPlan, &sFlag ) != IDE_SUCCESS );

    } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );

    sNumRows--;
    
    //-----------------------------------
    // close cursor
    //-----------------------------------

    sStatus = 2;
    IDE_TEST( qmnUPTE::closeCursor( aTemplate, sPlan ) != IDE_SUCCESS );
    
    sStatus = 1;
    IDE_TEST( aTemplate->cursorMgr->closeAllCursor(
                  QC_STATISTICS(aTemplate->stmt) )
              != IDE_SUCCESS );
    sStatus = 0;
    IDE_TEST( qmcTempTableMgr::dropAllTempTable(
                  aTemplate->tempTableMgr )
              != IDE_SUCCESS );

    //------------------------------------------
    // Foreign Key Reference 검사
    //------------------------------------------

    if ( sNumRows > 0 )
    {
        IDE_TEST( qmnUPTE::checkUpdateParentRef( aTemplate, sPlan )
                  != IDE_SUCCESS );

        IDE_TEST( qmnUPTE::checkUpdateChildRef( aTemplate, sPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    *aNumRows = sNumRows;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 3:
            (void) qmnUPTE::closeCursor( aTemplate, sPlan );
        case 2:
            (void) aTemplate->cursorMgr->closeAllCursor(
                      QC_STATISTICS(aTemplate->stmt) );
        case 1:
            (void) qmcTempTableMgr::dropAllTempTable(
                      aTemplate->tempTableMgr );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::deleteTarget( qcTemplate * aTemplate,
                       qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    delete target
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::deleteTarget"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan          * sPlan;
    qmcRowFlag         sFlag = QMC_ROW_INITIALIZE;
    vSLong             sNumRows = 0;
    UInt               sStatus = 0;

    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    // delete plan
    sPlan = aPlan->children[QMO_MERGE_DELETE_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->deleteStatement;

    //-----------------------------------
    // DELETE를 위한 plan tree 초기화
    //-----------------------------------

    IDE_TEST( qmnDETE::init( aTemplate, sPlan ) != IDE_SUCCESS );
    sStatus = 3;

    //------------------------------------------
    // DELETE를 수행
    //------------------------------------------

    do
    {
        // 비정상 종료 검사
        IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
                  != IDE_SUCCESS );
    
        sNumRows++;
        
        IDE_TEST( qmnDETE::doIt( aTemplate, sPlan, &sFlag ) != IDE_SUCCESS );

    } while ( ( sFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST );

    sNumRows--;
    
    //-----------------------------------
    // close cursor
    //-----------------------------------

    sStatus = 2;
    IDE_TEST( qmnDETE::closeCursor( aTemplate, sPlan ) != IDE_SUCCESS );
    
    sStatus = 1;
    IDE_TEST( aTemplate->cursorMgr->closeAllCursor(
                  QC_STATISTICS(aTemplate->stmt) )
              != IDE_SUCCESS );
    sStatus = 0;
    IDE_TEST( qmcTempTableMgr::dropAllTempTable( aTemplate->tempTableMgr )
              != IDE_SUCCESS );

    //------------------------------------------
    // Foreign Key Reference 검사
    //------------------------------------------

    // BUG-28049
    if ( sNumRows > 0 )
    {
        // Child Table이 참조하고 있는 지를 검사
        IDE_TEST( qmnDETE::checkDeleteRef( aTemplate, sPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 3:
            (void) qmnDETE::closeCursor( aTemplate, sPlan );
        case 2:
            (void) aTemplate->cursorMgr->closeAllCursor(
                    QC_STATISTICS(aTemplate->stmt) );
        case 1:
            (void) qmcTempTableMgr::dropAllTempTable( aTemplate->tempTableMgr );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::insertTarget( qcTemplate * aTemplate,
                       qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    insert target
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::insertTarget"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan          * sPlan;
    qmcRowFlag         sFlag = QMC_ROW_INITIALIZE;
    UInt               sStatus = 0;

    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    // insert plan
    sPlan = aPlan->children[QMO_MERGE_INSERT_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->insertStatement;

    //-----------------------------------
    // INSERT를 위한 plan tree 초기화
    //-----------------------------------

    // BUG-45288 atomic insert 는 direct path 가능. normal insert 불가능.
    ((qmndINST*) (aTemplate->tmplate.data + sPlan->offset))->isAppend =
        ((qmncINST*)sPlan)->isAppend;

    IDE_TEST( qmnINST::init( aTemplate, sPlan ) != IDE_SUCCESS );
    sStatus = 3;

    //------------------------------------------
    // INSERT를 수행
    //------------------------------------------

    IDE_TEST( qmnINST::doIt( aTemplate, sPlan, &sFlag ) != IDE_SUCCESS );
    
    //-----------------------------------
    // close cursor
    //-----------------------------------

    sStatus = 2;
    IDE_TEST( qmnINST::closeCursor( aTemplate, sPlan ) != IDE_SUCCESS );
    
    sStatus = 1;
    IDE_TEST( aTemplate->cursorMgr->closeAllCursor(
                  QC_STATISTICS(aTemplate->stmt) )
              != IDE_SUCCESS );
    sStatus = 0;
    IDE_TEST( qmcTempTableMgr::dropAllTempTable(
                  aTemplate->tempTableMgr )
              != IDE_SUCCESS );

    //------------------------------------------
    // Foreign Key Reference 검사
    //------------------------------------------

    // BUG-28049
    IDE_TEST( qmnINST::checkInsertRef( aTemplate, sPlan )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 3:
            (void) qmnINST::closeCursor( aTemplate, sPlan );
        case 2:
            (void) aTemplate->cursorMgr->closeAllCursor(
                      QC_STATISTICS(aTemplate->stmt) );
        case 1:
            (void) qmcTempTableMgr::dropAllTempTable(
                      aTemplate->tempTableMgr );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::checkWhereForInsert( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              idBool     * aFlag )
{
/***********************************************************************
 *
 * Description : 
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::checkWhereForInsert"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);
    
    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    aTemplate->stmt = & sDataPlan->selectSourceStatement;

    //-----------------------------------
    // judge
    //-----------------------------------
    
    IDE_TEST( qtc::judge( aFlag,
                          sCodePlan->whereForInsert,
                          aTemplate )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::insertNoRowsTarget( qcTemplate * aTemplate,
                             qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    insert target
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::insertNoRowsTarget"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnPlan          * sPlan;
    qmcRowFlag         sFlag = QMC_ROW_INITIALIZE;
    UInt               sStatus = 0;

    //-----------------------------------
    // 기본 정보 설정
    //-----------------------------------
    
    // insert plan
    sPlan = aPlan->children[QMO_MERGE_INSERT_NOROWS_IDX].childPlan;

    aTemplate->stmt = & sDataPlan->insertNoRowsStatement;

    //-----------------------------------
    // INSERT를 위한 plan tree 초기화
    //-----------------------------------

    // BUG-45288 atomic insert 는 direct path 가능. normal insert 불가능.
    ((qmndINST*) (aTemplate->tmplate.data + sPlan->offset))->isAppend =
        ((qmncINST*)sPlan)->isAppend;

    IDE_TEST( qmnINST::init( aTemplate, sPlan ) != IDE_SUCCESS );
    sStatus = 3;

    //------------------------------------------
    // INSERT를 수행
    //------------------------------------------

    IDE_TEST( qmnINST::doIt( aTemplate, sPlan, &sFlag ) != IDE_SUCCESS );
    
    //-----------------------------------
    // close cursor
    //-----------------------------------

    sStatus = 2;
    IDE_TEST( qmnINST::closeCursor( aTemplate, sPlan ) != IDE_SUCCESS );
    
    sStatus = 1;
    IDE_TEST( aTemplate->cursorMgr->closeAllCursor(
                  QC_STATISTICS(aTemplate->stmt) )
              != IDE_SUCCESS );
    sStatus = 0;
    IDE_TEST( qmcTempTableMgr::dropAllTempTable(
                  aTemplate->tempTableMgr )
              != IDE_SUCCESS );

    //------------------------------------------
    // Foreign Key Reference 검사
    //------------------------------------------

    // BUG-28049
    IDE_TEST( qmnINST::checkInsertRef( aTemplate, sPlan )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStatus )
    {
        case 3:
            (void) qmnINST::closeCursor( aTemplate, sPlan );
        case 2:
            (void) aTemplate->cursorMgr->closeAllCursor(
                      QC_STATISTICS(aTemplate->stmt) );
        case 1:
            (void) qmcTempTableMgr::dropAllTempTable(
                      aTemplate->tempTableMgr );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMRGE::closeCursor( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description : CLEAN UP
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::closeCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);
    
    if ( ( *sDataPlan->flag & QMND_MRGE_INIT_DONE_MASK )
         == QMND_MRGE_INIT_DONE_TRUE )
    {
        finalize( aTemplate, aPlan );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

#undef IDE_FN
}

void
qmnMRGE::finalize( qcTemplate * aTemplate,
                   qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::finalize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    //qmncMRGE * sCodePlan = (qmncMRGE*) aPlan;
    qmndMRGE * sDataPlan =
        (qmndMRGE*) (aTemplate->tmplate.data + aPlan->offset);
    
    aTemplate->stmt = sDataPlan->originalStatement;

    (void) sDataPlan->selectSourceCursorMgr.closeAllCursor(
        QC_STATISTICS(aTemplate->stmt) );
    
    (void) qmcTempTableMgr::dropAllTempTable(
        & sDataPlan->selectSourceTempTableMgr );
    
#undef IDE_FN
}
    
void
qmnMRGE::setCursorMgr( qcTemplate       * aTemplate,
                       qmcCursor        * aCursorMgr,
                       qmcdTempTableMgr * aTempTableMgr )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMRGE::setCursorMgr"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    IDE_ASSERT( aTemplate != NULL );
    IDE_ASSERT( aCursorMgr != NULL );
    IDE_ASSERT( aTempTableMgr != NULL );

    aTemplate->cursorMgr    = aCursorMgr;
    aTemplate->tempTableMgr = aTempTableMgr;
    
#undef IDE_FN
}

IDE_RC qmnMRGE::setBindParam( qcTemplate * aTemplate,
                              qmndMRGE   * aDataPlan )
{
/***********************************************************************
 *
 * Description : BUG-45763 MERGE 구문에서 CLOB Column Update 중 FATAL 발생합니다.
 *
 * Implementation : LOB Column의 Bind 정보는 Merge Statemant인 aStatement->pBindParam 에 저장된다.
 *                  Merge 구문은 Statemant 를 SELECT, ON, UPDATE, INSERT, INSERT 순으로 변경하는데,
 *                  최상위에 있는 Merge 의 pBindParam 을 전달하지 않는다. 따라서 LOB Bind 정보를
 *                  검색할 때에, FATAL 이 발생하므로, pBindParam 정보를 아래로 전달해야 한다.
 *
 ***********************************************************************/

    if ( & aDataPlan->updateStatement != NULL )
    {
        aDataPlan->updateStatement.pBindParam      = aTemplate->stmt->pBindParam;
        aDataPlan->updateStatement.pBindParamCount = aTemplate->stmt->pBindParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    if ( & aDataPlan->insertStatement != NULL )
    {
        aDataPlan->insertStatement.pBindParam      = aTemplate->stmt->pBindParam;
        aDataPlan->insertStatement.pBindParamCount = aTemplate->stmt->pBindParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    if ( & aDataPlan->insertNoRowsStatement != NULL )
    {
        aDataPlan->insertNoRowsStatement.pBindParam      = aTemplate->stmt->pBindParam;
        aDataPlan->insertNoRowsStatement.pBindParamCount = aTemplate->stmt->pBindParamCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    return IDE_FAILURE;
}

