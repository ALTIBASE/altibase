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
 * $Id: qmnMove.cpp 55241 2012-08-27 09:13:19Z linkedlist $
 *
 * Description :
 *     MOVE Node
 *
 *     관계형 모델에서 move를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qmnMove.h>
#include <qdnTrigger.h>
#include <qdnForeignKey.h>
#include <qdbCommon.h>
#include <qmsDefaultExpr.h>
#include <qmx.h>
#include <qcuTemporaryObj.h>
#include <qdtCommon.h>

IDE_RC
qmnMOVE::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::init"));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    sDataPlan->doIt = qmnMOVE::doItDefault;
    
    //------------------------------------------------
    // 최초 초기화 수행 여부 판단
    //------------------------------------------------

    if ( ( *sDataPlan->flag & QMND_MOVE_INIT_DONE_MASK )
         == QMND_MOVE_INIT_DONE_FALSE )
    {
        // 최초 초기화 수행
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
        
        //------------------------------------------------
        // Child Plan의 초기화
        //------------------------------------------------
        
        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);

        //---------------------------------
        // trigger row를 생성
        //---------------------------------

        // child의 offset을 이용하므로 firstInit이 끝나야 offset을 이용할 수 있다.
        IDE_TEST( allocTriggerRow(aTemplate, sCodePlan, sDataPlan)
                  != IDE_SUCCESS );
        
        //---------------------------------
        // index table cursor를 생성
        //---------------------------------
        
        IDE_TEST( allocIndexTableCursor(aTemplate, sCodePlan, sDataPlan)
                  != IDE_SUCCESS );
    
        //---------------------------------
        // 초기화 완료를 표기
        //---------------------------------
        
        *sDataPlan->flag &= ~QMND_MOVE_INIT_DONE_MASK;
        *sDataPlan->flag |= QMND_MOVE_INIT_DONE_TRUE;
    }
    else
    {
        //------------------------------------------------
        // Child Plan의 초기화
        //------------------------------------------------
        
        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);
        
        //-----------------------------------
        // init lob info
        //-----------------------------------
        
        if ( sDataPlan->lobInfo != NULL )
        {
            (void) qmx::initLobInfo( sDataPlan->lobInfo );
        }
        else
        {
            // Nothing to do.
        }
    }
        
    //------------------------------------------------
    // 가변 Data 의 초기화
    //------------------------------------------------

    // Limit 시작 개수의 초기화
    sDataPlan->limitCurrent = 1;

    //------------------------------------------------
    // 수행 함수 결정
    //------------------------------------------------

    sDataPlan->doIt = qmnMOVE::doItFirst;
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    return sDataPlan->doIt( aTemplate, aPlan, aFlag );

#undef IDE_FN
}

IDE_RC 
qmnMOVE::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 노드는 별도의 null row를 가지지 않으며,
 *    Child에 대하여 padNull()을 호출한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::padNull"));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    // qmndMOVE * sDataPlan = 
    //     (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_MOVE_INIT_DONE_MASK)
         == QMND_MOVE_INIT_DONE_FALSE )
    {
        // 초기화되지 않은 경우 초기화 수행
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // Child Plan에 대하여 Null Padding수행
    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::printPlan"));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

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
    // MOVE Target 정보의 출력
    //------------------------------------------------------

    // MOVE 정보의 출력
    if ( sCodePlan->targetTableRef->tableType == QCM_VIEW )
    {
        iduVarStringAppendFormat( aString,
                                  "MOVE ( VIEW: " );
    }
    else
    {
        iduVarStringAppendFormat( aString,
                                  "MOVE ( TABLE: " );
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
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    //------------------------------------------------------
    // Child Plan 정보의 출력
    //------------------------------------------------------

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
qmnMOVE::firstInit( qcTemplate * aTemplate,
                    qmncMOVE   * aCodePlan,
                    qmndMOVE   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    MOVE node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *    - Data 영역의 주요 멤버에 대한 초기화를 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::firstInit"));

    ULong    sCount;
    UShort   sTableID;

    //--------------------------------
    // 적합성 검사
    //--------------------------------

    //--------------------------------
    // DETE 고유 정보의 초기화
    //--------------------------------

    // Tuple Set정보의 초기화
    aDataPlan->deleteTuple = & aTemplate->tmplate.rows[aCodePlan->tableRef->table];
    aDataPlan->deleteCursor = NULL;
    aDataPlan->deleteTupleID = ID_USHORT_MAX;
    
    /* PROJ-2464 hybrid partitioned table 지원 */
    aDataPlan->deletePartInfo = NULL;

    // index table cursor 초기화
    aDataPlan->indexDeleteCursor = NULL;

    // where column list 초기화
    smiInitDMLRetryInfo( &(aDataPlan->retryInfo) );

    /* PROJ-2359 Table/Partition Access Option */
    aDataPlan->accessOption = QCM_ACCESS_OPTION_READ_WRITE;

    //--------------------------------
    // cursorInfo 생성
    //--------------------------------
    
    IDE_TEST( allocCursorInfo( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );
    
    //---------------------------------
    // insert cursor manager 초기화
    //---------------------------------
    
    IDE_TEST( aDataPlan->insertCursorMgr.initialize(
                  aTemplate->stmt->qmxMem,
                  aCodePlan->targetTableRef,
                  ID_TRUE,
                  ID_FALSE )
              != IDE_SUCCESS );
    
    //---------------------------------
    // lob info 초기화
    //---------------------------------

    if ( aCodePlan->targetTableRef->tableInfo->lobColumnCount > 0 )
    {
        // PROJ-1362
        IDE_TEST( qmx::initializeLobInfo(
                      aTemplate->stmt,
                      & aDataPlan->lobInfo,
                      (UShort)aCodePlan->targetTableRef->tableInfo->lobColumnCount )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->lobInfo = NULL;
    }

    //--------------------------------
    // Limitation 관련 정보의 초기화
    //--------------------------------
    
    if( aCodePlan->limit != NULL )
    {
        IDE_TEST( qmsLimitI::getStartValue(
                      aTemplate,
                      aCodePlan->limit,
                      &aDataPlan->limitStart )
                  != IDE_SUCCESS );

        IDE_TEST( qmsLimitI::getCountValue(
                      aTemplate,
                      aCodePlan->limit,
                      &sCount )
                  != IDE_SUCCESS );

        if ( aDataPlan->limitStart > 0 )
        {
            aDataPlan->limitEnd = aDataPlan->limitStart + sCount;
        }
        else
        {
            aDataPlan->limitEnd = 0;
        }
    }
    else
    {
        aDataPlan->limitStart = 1;
        aDataPlan->limitEnd   = 0;
    }

    // 적합성 검사
    if ( aDataPlan->limitEnd > 0 )
    {
        IDE_ASSERT( (aCodePlan->flag & QMNC_MOVE_LIMIT_MASK)
                    == QMNC_MOVE_LIMIT_TRUE );
    }

    //------------------------------------------
    // Default Expr의 Row Buffer 구성
    //------------------------------------------

    if ( aCodePlan->defaultExprColumns != NULL )
    {
        sTableID = aCodePlan->defaultExprTableRef->table;
        
        IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                                   &(aTemplate->tmplate),
                                   sTableID )
                  != IDE_SUCCESS );
        
        if ( (aTemplate->tmplate.rows[sTableID].lflag & MTC_TUPLE_STORAGE_MASK)
             == MTC_TUPLE_STORAGE_MEMORY )
        {
            IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                          aTemplate->tmplate.rows[sTableID].rowOffset,
                          &(aTemplate->tmplate.rows[sTableID].row) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Disk Table의 경우, qmc::setRowSize()에서 이미 할당 */
        }

        aDataPlan->defaultExprRowBuffer = aTemplate->tmplate.rows[sTableID].row;
    }
    else
    {
        aDataPlan->defaultExprRowBuffer = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::allocCursorInfo( qcTemplate * aTemplate,
                          qmncMOVE   * aCodePlan,
                          qmndMOVE   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::allocCursorInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::allocCursorInfo"));

    qmnCursorInfo     * sCursorInfo;
    qmsPartitionRef   * sPartitionRef;
    UInt                sPartitionCount;
    UInt                i = 0;

    //--------------------------------
    // cursorInfo 생성
    //--------------------------------
    
    IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF(qmnCursorInfo),
                                              (void**)& sCursorInfo )
              != IDE_SUCCESS );
        
    // cursorInfo 초기화
    sCursorInfo->cursor              = NULL;
    sCursorInfo->selectedIndex       = NULL;
    sCursorInfo->selectedIndexTuple  = NULL;
    sCursorInfo->accessOption        = QCM_ACCESS_OPTION_READ_WRITE; /* PROJ-2359 Table/Partition Access Option */
    sCursorInfo->updateColumnList    = NULL;
    sCursorInfo->cursorType          = SMI_DELETE_CURSOR;
    sCursorInfo->isRowMovementUpdate = ID_FALSE;
    sCursorInfo->inplaceUpdate       = ID_FALSE;
    sCursorInfo->lockMode            = SMI_LOCK_WRITE;

    /* PROJ-2464 hybrid partitioned table 지원 */
    sCursorInfo->stmtRetryColLst     = aCodePlan->whereColumnList;
    sCursorInfo->rowRetryColLst      = NULL;

    // cursorInfo 설정
    aDataPlan->deleteTuple->cursorInfo = sCursorInfo;
    
    //--------------------------------
    // partition cursorInfo 생성
    //--------------------------------
    
    if ( aCodePlan->tableRef->partitionRef != NULL )
    {
        sPartitionCount = 0;
        for ( sPartitionRef = aCodePlan->tableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef = sPartitionRef->next )
        {
            sPartitionCount++;
        }
        
        // cursorInfo 생성
        IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                      sPartitionCount * ID_SIZEOF(qmnCursorInfo),
                      (void**)& sCursorInfo )
                  != IDE_SUCCESS );

        for ( sPartitionRef = aCodePlan->tableRef->partitionRef, i = 0;
              sPartitionRef != NULL;
              sPartitionRef = sPartitionRef->next, sCursorInfo++, i++ )
        {
            // cursorInfo 초기화
            sCursorInfo->cursor              = NULL;
            sCursorInfo->selectedIndex       = NULL;
            sCursorInfo->selectedIndexTuple  = NULL;
            /* PROJ-2359 Table/Partition Access Option */
            sCursorInfo->accessOption        = QCM_ACCESS_OPTION_READ_WRITE;
            sCursorInfo->updateColumnList    = NULL;
            sCursorInfo->cursorType          = SMI_DELETE_CURSOR;
            sCursorInfo->isRowMovementUpdate = ID_FALSE;
            sCursorInfo->inplaceUpdate       = ID_FALSE;
            sCursorInfo->lockMode            = SMI_LOCK_WRITE;

            /* PROJ-2464 hybrid partitioned table 지원 */
            sCursorInfo->stmtRetryColLst     = aCodePlan->wherePartColumnList[i];
            sCursorInfo->rowRetryColLst      = NULL;

            // cursorInfo 설정
            aTemplate->tmplate.rows[sPartitionRef->table].cursorInfo = sCursorInfo;
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
qmnMOVE::allocTriggerRow( qcTemplate * aTemplate,
                          qmncMOVE   * aCodePlan,
                          qmndMOVE   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::allocTriggerRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::allocTriggerRow"));

    UInt sMaxRowOffset = 0;

    //---------------------------------
    // Insert Trigger를 위한 공간을 마련
    //---------------------------------

    if ( aCodePlan->targetTableRef->tableInfo->triggerCount > 0 )
    {
        aDataPlan->needTriggerRowForInsert = ID_FALSE;
        aDataPlan->existTriggerForInsert = ID_TRUE;
    }
    else
    {
        aDataPlan->needTriggerRowForInsert = ID_FALSE;
        aDataPlan->existTriggerForInsert = ID_FALSE;
    }
    
    //---------------------------------
    // Delete Trigger를 위한 공간을 마련
    //---------------------------------
    
    if ( aCodePlan->tableRef->tableInfo->triggerCount > 0 )
    {
        sMaxRowOffset = qmx::getMaxRowOffset( &(aTemplate->tmplate),
                                              aCodePlan->tableRef );

        if ( sMaxRowOffset > 0 )
        {
            // Old Row Referencing을 위한 공간 할당
            IDE_TEST( aTemplate->stmt->qmxMem->alloc(
                    sMaxRowOffset,
                    (void**) & aDataPlan->oldRow )
                != IDE_SUCCESS);
        }
        else
        {
            aDataPlan->oldRow = NULL;
        }

        aDataPlan->columnsForRow = aCodePlan->tableRef->tableInfo->columns;

        aDataPlan->needTriggerRowForDelete = ID_FALSE;
        aDataPlan->existTriggerForDelete = ID_TRUE;
    }
    else
    {
        aDataPlan->oldRow = NULL;

        aDataPlan->needTriggerRowForDelete = ID_FALSE;
        aDataPlan->existTriggerForDelete = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::allocIndexTableCursor( qcTemplate * aTemplate,
                                qmncMOVE   * aCodePlan,
                                qmndMOVE   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::allocIndexTableCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmnMOVE::allocIndexTableCursor"));

    //---------------------------------
    // target index table 처리를 위한 정보
    //---------------------------------

    if ( aCodePlan->targetTableRef->indexTableRef != NULL )
    {
        IDE_TEST( qmsIndexTable::initializeIndexTableCursors4Insert(
                      aTemplate->stmt,
                      aCodePlan->targetTableRef->indexTableRef,
                      aCodePlan->targetTableRef->indexTableCount,
                      & (aDataPlan->insertIndexTableCursorInfo) )
                  != IDE_SUCCESS );
        
        *aDataPlan->flag &= ~QMND_MOVE_INSERT_INDEX_CURSOR_MASK;
        *aDataPlan->flag |= QMND_MOVE_INSERT_INDEX_CURSOR_INITED;
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------------
    // source index table 처리를 위한 정보
    //---------------------------------
    
    if ( aCodePlan->tableRef->indexTableRef != NULL )
    {
        IDE_TEST( qmsIndexTable::initializeIndexTableCursors(
                      aTemplate->stmt,
                      aCodePlan->tableRef->indexTableRef,
                      aCodePlan->tableRef->indexTableCount,
                      aCodePlan->tableRef->selectedIndexTable,
                      & (aDataPlan->deleteIndexTableCursorInfo) )
                  != IDE_SUCCESS );
        
        *aDataPlan->flag &= ~QMND_MOVE_DELETE_INDEX_CURSOR_MASK;
        *aDataPlan->flag |= QMND_MOVE_DELETE_INDEX_CURSOR_INITED;
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
qmnMOVE::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnMOVE::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MOVE의 최초 수행 함수
 *
 * Implementation :
 *    - Table에 IX Lock을 건다.
 *    - Session Event Check (비정상 종료 Detect)
 *    - Cursor Open
 *    - move one record
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    /* PROJ-2359 Table/Partition Access Option */
    idBool     sIsTableCursorChanged;

    //-----------------------------------
    // Child Plan을 수행함
    //-----------------------------------

    // To fix PR-3921
    if ( sDataPlan->limitCurrent == sDataPlan->limitEnd )
    {
        // 주어진 Limit 조건에 다다른 경우
        *aFlag = QMC_ROW_DATA_NONE;
    }
    /* TASK-7307 DML Data Consistency in Shard */
    else if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) == ID_TRUE ) &&
              ( sCodePlan->tableRef->tableInfo->mIsUsable == ID_FALSE ) )
    {
        // 무시한다
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        // doIt left child
        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                  != IDE_SUCCESS );
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        // Limit Start 처리
        for ( ;
              sDataPlan->limitCurrent < sDataPlan->limitStart;
              sDataPlan->limitCurrent++ )
        {
            // Limitation 범위에 들지 않는다.
            // 따라서 Delete없이 Child를 수행하기만 한다.
            IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                      != IDE_SUCCESS );

            if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_NONE )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sDataPlan->limitStart <= sDataPlan->limitEnd )
        {
            if ( ( sDataPlan->limitCurrent >= sDataPlan->limitStart ) &&
                 ( sDataPlan->limitCurrent < sDataPlan->limitEnd ) )
            {
                // Limit값 증가
                sDataPlan->limitCurrent++;
            }
            else
            {
                // Limitation 범위를 벗어난 경우
                *aFlag = QMC_ROW_DATA_NONE;
            }
        }
        else
        {
            // Nothing To Do
        }

        if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            // check trigger
            IDE_TEST( checkTrigger( aTemplate, aPlan ) != IDE_SUCCESS );

            // open cursor
            IDE_TEST( openInsertCursor( aTemplate, aPlan ) != IDE_SUCCESS );
                
            /* PROJ-2359 Table/Partition Access Option */
            IDE_TEST( qmx::checkAccessOption( sCodePlan->targetTableRef->tableInfo,
                                              ID_TRUE, /* aIsInsertion */
                                              QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) )
                      != IDE_SUCCESS );

            // insert one record
            IDE_TEST( insertOneRow( aTemplate, aPlan ) != IDE_SUCCESS );
                
            // get cursor
            IDE_TEST( getCursor( aTemplate, aPlan, &sIsTableCursorChanged ) != IDE_SUCCESS );
            
            /* PROJ-2359 Table/Partition Access Option */
            IDE_TEST( qmx::checkAccessOption( sCodePlan->tableRef->tableInfo,
                                              ID_FALSE /* aIsInsertion */ )
                      != IDE_SUCCESS );

            if ( sCodePlan->tableRef->partitionRef != NULL )
            {
                IDE_TEST( qmx::checkAccessOptionForExistentRecord(
                                    sDataPlan->accessOption,
                                    sDataPlan->deleteTuple->tableHandle )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            // delete one record
            IDE_TEST( deleteOneRow( aTemplate, aPlan ) != IDE_SUCCESS );

            sDataPlan->doIt = qmnMOVE::doItNext;
        }
        else
        {
            // Nothing To Do
        }
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

#undef IDE_FN
}

IDE_RC
qmnMOVE::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    MOVE의 다음 수행 함수
 *    다음 Record를 삭제한다.
 *
 * Implementation :
 *    - move one record
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    /* PROJ-2359 Table/Partition Access Option */
    idBool     sIsTableCursorChanged;

    //-----------------------------------
    // Child Plan을 수행함
    //-----------------------------------

    // To fix PR-3921
    if ( sDataPlan->limitCurrent == sDataPlan->limitEnd )
    {
        // 주어진 Limit 조건에 다다른 경우
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        // doIt left child
        IDE_TEST( aPlan->left->doIt( aTemplate, aPlan->left, aFlag )
                  != IDE_SUCCESS );
    }

    if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
    {
        if ( sDataPlan->limitStart <= sDataPlan->limitEnd )
        {
            if ( ( sDataPlan->limitCurrent >= sDataPlan->limitStart ) &&
                 ( sDataPlan->limitCurrent < sDataPlan->limitEnd ) )
            {
                // Limit값 증가
                sDataPlan->limitCurrent++;
            }
            else
            {
                // Limitation 범위를 벗어난 경우
                *aFlag = QMC_ROW_DATA_NONE;
            }
        }
        else
        {
            // Nothing To Do
        }

        if ( ( *aFlag & QMC_ROW_DATA_MASK ) == QMC_ROW_DATA_EXIST )
        {
            // insert one record
            IDE_TEST( insertOneRow( aTemplate, aPlan ) != IDE_SUCCESS );
            
            if ( sCodePlan->tableRef->partitionRef != NULL )
            {
                // get cursor
                IDE_TEST( getCursor( aTemplate, aPlan, &sIsTableCursorChanged ) != IDE_SUCCESS );

                /* PROJ-2359 Table/Partition Access Option */
                if ( sIsTableCursorChanged == ID_TRUE )
                {
                    IDE_TEST( qmx::checkAccessOptionForExistentRecord(
                                        sDataPlan->accessOption,
                                        sDataPlan->deleteTuple->tableHandle )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                // Nothing to do.
            }
                
            // delete one record
            IDE_TEST( deleteOneRow( aTemplate, aPlan ) != IDE_SUCCESS );
        }
        else
        {
            // record가 없는 경우
            // 다음 수행을 위해 최초 수행 함수로 설정함.
            sDataPlan->doIt = qmnMOVE::doItFirst;
        }
    }
    else
    {
        // record가 없는 경우
        // 다음 수행을 위해 최초 수행 함수로 설정함.
        sDataPlan->doIt = qmnMOVE::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDataPlan->lobInfo != NULL )
    {
        (void)qmx::finalizeLobInfo( aTemplate->stmt, sDataPlan->lobInfo );
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::checkTrigger( qcTemplate * aTemplate,
                       qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::checkTrigger"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);
    
    idBool     sNeedTriggerRowForInsert;
    idBool     sNeedTriggerRowForDelete;

    //---------------------------------
    // Check Insert Trigger
    //---------------------------------
    
    if ( sDataPlan->existTriggerForInsert == ID_TRUE )
    {
        // Trigger를 위한 Referencing Row가 필요한지를 검사
        IDE_TEST( qdnTrigger::needTriggerRow(
                      aTemplate->stmt,
                      sCodePlan->targetTableRef->tableInfo,
                      QCM_TRIGGER_BEFORE,
                      QCM_TRIGGER_EVENT_INSERT,
                      NULL,
                      & sNeedTriggerRowForInsert )
                  != IDE_SUCCESS );
        
        if ( sNeedTriggerRowForInsert == ID_FALSE )
        {
            IDE_TEST( qdnTrigger::needTriggerRow(
                          aTemplate->stmt,
                          sCodePlan->targetTableRef->tableInfo,
                          QCM_TRIGGER_AFTER,
                          QCM_TRIGGER_EVENT_INSERT,
                          NULL,
                          & sNeedTriggerRowForInsert )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        sDataPlan->needTriggerRowForInsert = sNeedTriggerRowForInsert;
    }
    else
    {
        // Nothing to do.
    }
    
    //---------------------------------
    // Check Delete Trigger
    //---------------------------------
    
    if ( sDataPlan->existTriggerForDelete == ID_TRUE )
    {
        // Trigger를 위한 Referencing Row가 필요한지를 검사
        IDE_TEST( qdnTrigger::needTriggerRow(
                      aTemplate->stmt,
                      sCodePlan->tableRef->tableInfo,
                      QCM_TRIGGER_BEFORE,
                      QCM_TRIGGER_EVENT_DELETE,
                      NULL,
                      & sNeedTriggerRowForDelete )
                  != IDE_SUCCESS );
        
        if ( sNeedTriggerRowForDelete == ID_FALSE )
        {
            IDE_TEST( qdnTrigger::needTriggerRow(
                          aTemplate->stmt,
                          sCodePlan->tableRef->tableInfo,
                          QCM_TRIGGER_AFTER,
                          QCM_TRIGGER_EVENT_DELETE,
                          NULL,
                          & sNeedTriggerRowForDelete )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        sDataPlan->needTriggerRowForDelete = sNeedTriggerRowForDelete;
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
qmnMOVE::getCursor( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    idBool     * aIsTableCursorChanged ) /* PROJ-2359 Table/Partition Access Option */
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *     하위 scan이 open한 cursor를 얻는다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::getCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    qmnCursorInfo * sCursorInfo = NULL;

    /* PROJ-2359 Table/Partition Access Option */
    *aIsTableCursorChanged = ID_FALSE;

    if ( sCodePlan->tableRef->partitionRef == NULL )
    {
        if ( sDataPlan->deleteTupleID != sCodePlan->tableRef->table )
        {
            sDataPlan->deleteTupleID = sCodePlan->tableRef->table;
            
            // cursor를 얻는다.
            sCursorInfo = (qmnCursorInfo*)
                aTemplate->tmplate.rows[sDataPlan->deleteTupleID].cursorInfo;
            
            IDE_TEST_RAISE( sCursorInfo == NULL, ERR_NOT_FOUND );

            sDataPlan->deleteCursor = sCursorInfo->cursor;

            /* PROJ-2464 hybrid partitioned table 지원 */
            sDataPlan->deletePartInfo = sCodePlan->tableRef->tableInfo;

            sDataPlan->retryInfo.mIsWithoutRetry  = sCodePlan->withoutRetry;
            sDataPlan->retryInfo.mStmtRetryColLst = sCursorInfo->stmtRetryColLst;

            /* PROJ-2359 Table/Partition Access Option */
            sDataPlan->accessOption = sCursorInfo->accessOption;
            *aIsTableCursorChanged  = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        if ( sDataPlan->deleteTupleID != sDataPlan->deleteTuple->partitionTupleID )
        {
            sDataPlan->deleteTupleID = sDataPlan->deleteTuple->partitionTupleID;
            
            // partition의 cursor를 얻는다.
            sCursorInfo = (qmnCursorInfo*)
                aTemplate->tmplate.rows[sDataPlan->deleteTupleID].cursorInfo;
            
            IDE_TEST_RAISE( sCursorInfo == NULL, ERR_NOT_FOUND );
            
            sDataPlan->deleteCursor = sCursorInfo->cursor;

            /* PROJ-2464 hybrid partitioned table 지원 */
            IDE_TEST( smiGetTableTempInfo( sDataPlan->deleteTuple->tableHandle,
                                           (void **)&(sDataPlan->deletePartInfo) )
                      != IDE_SUCCESS );

            sDataPlan->retryInfo.mIsWithoutRetry  = sCodePlan->withoutRetry;
            sDataPlan->retryInfo.mStmtRetryColLst = sCursorInfo->stmtRetryColLst;

            /* PROJ-2359 Table/Partition Access Option */
            sDataPlan->accessOption = sCursorInfo->accessOption;
            *aIsTableCursorChanged  = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
        
        // index table cursor를 얻는다.
        if ( sDataPlan->indexDeleteCursor == NULL )
        {
            sCursorInfo = (qmnCursorInfo*)
                aTemplate->tmplate.rows[sCodePlan->tableRef->table].cursorInfo;
            
            IDE_TEST_RAISE( sCursorInfo == NULL, ERR_NOT_FOUND );
            
            sDataPlan->indexDeleteCursor = sCursorInfo->cursor;
        }
        else
        {
            // Nothing to do.
        }
    }
     
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnMOVE::getCursor",
                                  "cursor not found" ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::openInsertCursor( qcTemplate * aTemplate,
                           qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *     insert cursor를 open한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::openInsertCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    smiCursorProperties   sCursorProperty;
    qcmTableInfo        * sDiskInfo        = NULL;
    UShort                sTupleID         = 0;
    idBool                sNeedAllFetchColumn;
    smiFetchColumnList  * sFetchColumnList = NULL;
    
    if ( ( *sDataPlan->flag & QMND_MOVE_CURSOR_MASK )
         == QMND_MOVE_CURSOR_CLOSED )
    {
        // MOVE 를 위한 Cursor 구성
        SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sCursorProperty, aTemplate->stmt->mStatistics );

        if ( sCodePlan->targetTableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            if ( sCodePlan->targetTableRef->partitionSummary->diskPartitionRef != NULL )
            {
                sDiskInfo = sCodePlan->targetTableRef->partitionSummary->diskPartitionRef->partitionInfo;
                sTupleID = sCodePlan->targetTableRef->partitionSummary->diskPartitionRef->table;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            if ( QCM_TABLE_TYPE_IS_DISK( sCodePlan->targetTableRef->tableInfo->tableFlag ) == ID_TRUE )
            {
                sDiskInfo = sCodePlan->targetTableRef->tableInfo;
                sTupleID = sCodePlan->targetTableRef->table;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sDiskInfo != NULL )
        {
            sNeedAllFetchColumn = sDataPlan->needTriggerRowForInsert;
            
            // PROJ-1705
            // 포린키 체크를 위해 읽어야 할 패치컬럼리스트 생성
            IDE_TEST( qdbCommon::makeFetchColumnList4TupleID(
                          aTemplate,
                          sTupleID,
                          sNeedAllFetchColumn,  // aIsNeedAllFetchColumn
                          NULL,                 // index
                          ID_TRUE,              // allocSmiColumnListEx
                          & sFetchColumnList )
                      != IDE_SUCCESS );
        
            /* PROJ-1107 Check Constraint 지원 */
            if ( (sDataPlan->needTriggerRowForInsert == ID_FALSE) &&
                 (sCodePlan->checkConstrList != NULL) )
            {
                IDE_TEST( qdbCommon::addCheckConstrListToFetchColumnList(
                              aTemplate->stmt->qmxMem,
                              sCodePlan->checkConstrList,
                              sDiskInfo->columns,
                              & sFetchColumnList )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        
            sCursorProperty.mFetchColumnList = sFetchColumnList;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( sDataPlan->insertCursorMgr.openCursor(
                      aTemplate->stmt,
                      SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD | SMI_PREVIOUS_DISABLE,
                      & sCursorProperty )
                  != IDE_SUCCESS );
        
        *sDataPlan->flag &= ~QMND_MOVE_CURSOR_MASK;
        *sDataPlan->flag |= QMND_MOVE_CURSOR_OPEN;
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
qmnMOVE::closeCursor( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::closeCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    if ( ( *sDataPlan->flag & QMND_MOVE_CURSOR_MASK )
         == QMND_MOVE_CURSOR_OPEN )
    {
        *sDataPlan->flag &= ~QMND_MOVE_CURSOR_MASK;
        *sDataPlan->flag |= QMND_MOVE_CURSOR_CLOSED;
        
        IDE_TEST( sDataPlan->insertCursorMgr.closeCursor()
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( ( *sDataPlan->flag & QMND_MOVE_INSERT_INDEX_CURSOR_MASK )
         == QMND_MOVE_INSERT_INDEX_CURSOR_INITED )
    {
        IDE_TEST( qmsIndexTable::closeIndexTableCursors(
                      & (sDataPlan->insertIndexTableCursorInfo) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( ( *sDataPlan->flag & QMND_MOVE_DELETE_INDEX_CURSOR_MASK )
         == QMND_MOVE_DELETE_INDEX_CURSOR_INITED )
    {
        IDE_TEST( qmsIndexTable::closeIndexTableCursors(
                      & (sDataPlan->deleteIndexTableCursorInfo) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( *sDataPlan->flag & QMND_MOVE_INSERT_INDEX_CURSOR_MASK )
         == QMND_MOVE_INSERT_INDEX_CURSOR_INITED )
    {
        qmsIndexTable::finalizeIndexTableCursors(
            & (sDataPlan->insertIndexTableCursorInfo) );
    }

    if ( ( *sDataPlan->flag & QMND_MOVE_DELETE_INDEX_CURSOR_MASK )
         == QMND_MOVE_DELETE_INDEX_CURSOR_INITED )
    {
        qmsIndexTable::finalizeIndexTableCursors(
            & (sDataPlan->deleteIndexTableCursorInfo) );
    }
        
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::insertOneRow( qcTemplate * aTemplate,
                       qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    - move one record 수행
 *    - trigger each row 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::insertOneRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);

    iduMemoryStatus     sQmxMemStatus;
    qcmTableInfo      * sTableForInsert;
    smiValue          * sInsertedRow;
    smiValue          * sInsertedRowForPartition = NULL;
    smiValue            sValuesForPartition[QC_MAX_COLUMN_COUNT];
    smiTableCursor    * sCursor;
    void              * sRow;
    scGRID              sRowGRID;
    qcmTableInfo      * sSelectedPartitionInfo = NULL;
    UInt                sInsertedRowValueCount = 0;

    sTableForInsert = sCodePlan->targetTableRef->tableInfo;
    sInsertedRow = aTemplate->insOrUptRow[sCodePlan->valueIdx];
    sInsertedRowValueCount = aTemplate->insOrUptRowValueCount[sCodePlan->valueIdx];
    
    // Memory 재사용을 위하여 현재 위치 기록
    IDE_TEST_RAISE( aTemplate->stmt->qmxMem->getStatus( &sQmxMemStatus ) != IDE_SUCCESS, ERR_MEM_OP );
    
    //-----------------------------------
    // clear lob info
    //-----------------------------------

    if ( sDataPlan->lobInfo != NULL )
    {
        (void) qmx::clearLobInfo( sDataPlan->lobInfo );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // set next sequence
    //-----------------------------------
    
    // fix BUG-11917 1Row씩 insert할 때마다 sequence next value를
    // 얻어와야 함.
    // Sequence Value 획득
    if ( sCodePlan->nextValSeqs != NULL )
    {
        IDE_TEST( qmx::readSequenceNextVals( aTemplate->stmt,
                                             sCodePlan->nextValSeqs )
                  != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do
    }

    //-----------------------------------
    // make insert value
    //-----------------------------------
    
    IDE_TEST( qmx::makeSmiValueWithValue( sCodePlan->columns,
                                          sCodePlan->values,
                                          aTemplate,
                                          sTableForInsert,
                                          sInsertedRow,
                                          sDataPlan->lobInfo )
                != IDE_SUCCESS );
    
    //-----------------------------------
    // insert before trigger
    //-----------------------------------
    
    if ( sDataPlan->existTriggerForInsert == ID_TRUE )
    {
        // PROJ-1359 Trigger
        // ROW GRANULARITY TRIGGER의 수행
        IDE_TEST( qdnTrigger::fireTrigger(
                      aTemplate->stmt,
                      aTemplate->stmt->qmxMem,
                      sCodePlan->targetTableRef->tableInfo,
                      QCM_TRIGGER_ACTION_EACH_ROW,
                      QCM_TRIGGER_BEFORE,
                      QCM_TRIGGER_EVENT_INSERT,
                      NULL,                      // UPDATE Column
                      NULL,                      /* Table Cursor */
                      SC_NULL_GRID,              /* Row GRID */
                      NULL,                      // OLD ROW
                      NULL,                      // OLD ROW Column
                      sInsertedRow,              // NEW ROW(value list)
                      sTableForInsert->columns ) // NEW ROW Column
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // check null
    //-----------------------------------
    
    IDE_TEST( qmx::checkNotNullColumnForInsert(
                  sTableForInsert->columns,
                  sInsertedRow,
                  sDataPlan->lobInfo,
                  ID_TRUE )
              != IDE_SUCCESS );

    //-----------------------------------
    // Default Expr
    //-----------------------------------

    if ( sCodePlan->defaultExprColumns != NULL )
    {
        IDE_TEST( qmsDefaultExpr::setRowBufferFromSmiValueArray(
                      &(aTemplate->tmplate),
                      sCodePlan->defaultExprTableRef,
                      sTableForInsert->columns,
                      sDataPlan->defaultExprRowBuffer,
                      sInsertedRow,
                      QCM_TABLE_TYPE_IS_DISK( sTableForInsert->tableFlag ) )
                  != IDE_SUCCESS );
        
        IDE_TEST( qmsDefaultExpr::calculateDefaultExpression(
                      aTemplate,
                      sCodePlan->defaultExprTableRef,
                      NULL,
                      sCodePlan->defaultExprColumns,
                      sDataPlan->defaultExprRowBuffer,
                      sInsertedRow,
                      sTableForInsert->columns )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //-----------------------------------
    // insert one row
    //-----------------------------------
    
    IDE_TEST( sDataPlan->insertCursorMgr.partitionFilteringWithRow(
                  sInsertedRow,
                  sDataPlan->lobInfo,
                  &sSelectedPartitionInfo )
              != IDE_SUCCESS );
    
    /* PROJ-2359 Table/Partition Access Option */
    if ( sSelectedPartitionInfo != NULL )
    {
        IDE_TEST( qmx::checkAccessOption( sSelectedPartitionInfo,
                                          ID_TRUE, /* aIsInsertion */
                                          QCG_CHECK_SHARD_DML_CONSISTENCY( aTemplate->stmt ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2264 Dictionary table
    if( sCodePlan->compressedTuple != UINT_MAX )
    {
        IDE_TEST( qmx::makeSmiValueForCompress( aTemplate,
                                                sCodePlan->compressedTuple,
                                                sInsertedRow )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sDataPlan->insertCursorMgr.getCursor( &sCursor )
              != IDE_SUCCESS );

    if ( sSelectedPartitionInfo != NULL )
    {
        if ( QCM_TABLE_TYPE_IS_DISK( sTableForInsert->tableFlag ) !=
             QCM_TABLE_TYPE_IS_DISK( sSelectedPartitionInfo->tableFlag ) )
        {
            /* PROJ-2464 hybrid partitioned table 지원
             * Partitioned Table을 기준으로 만든 smiValue Array를 Table Partition에 맞게 변환한다.
             */
            IDE_TEST( qmx::makeSmiValueWithSmiValue( sTableForInsert,
                                                     sSelectedPartitionInfo,
                                                     sTableForInsert->columns,
                                                     sInsertedRowValueCount,
                                                     sInsertedRow,
                                                     sValuesForPartition )
                      != IDE_SUCCESS );

            sInsertedRowForPartition = sValuesForPartition;
        }
        else
        {
            sInsertedRowForPartition = sInsertedRow;
        }
    }
    else
    {
        sInsertedRowForPartition = sInsertedRow;
    }

    IDE_TEST( sCursor->insertRow( sInsertedRowForPartition,
                                  & sRow,
                                  & sRowGRID )
              != IDE_SUCCESS);
    
    // insert index table
    IDE_TEST( insertIndexTableCursor( aTemplate,
                                      sDataPlan,
                                      sInsertedRow,
                                      sRowGRID )
              != IDE_SUCCESS );
    
    //------------------------------------------
    // insert를 수행후 Lob 컬럼을 처리
    //------------------------------------------
    
    IDE_TEST( qmx::copyAndOutBindLobInfo( aTemplate->stmt,
                                          sDataPlan->lobInfo,
                                          sCursor,
                                          sRow,
                                          sRowGRID )
              != IDE_SUCCESS );
    
    //-----------------------------------
    // insert after trigger
    //-----------------------------------
    
    if ( ( sDataPlan->existTriggerForInsert == ID_TRUE ) ||
         ( sCodePlan->checkConstrList != NULL ) )
    {
        IDE_TEST( qmx::fireTriggerInsertRowGranularity(
                      aTemplate->stmt,
                      sCodePlan->targetTableRef,
                      & sDataPlan->insertCursorMgr,
                      sRowGRID,
                      NULL,
                      sCodePlan->checkConstrList,
                      aTemplate->numRows,
                      sDataPlan->needTriggerRowForInsert )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( ( *sDataPlan->flag & QMND_MOVE_INSERT_MASK )
         == QMND_MOVE_INSERT_FALSE )
    {
        *sDataPlan->flag &= ~QMND_MOVE_INSERT_MASK;
        *sDataPlan->flag |= QMND_MOVE_INSERT_TRUE;
    }
    else
    {
        // BUG-36914 insert value가 subquery인 경우 memory를 해제하면 UMR이 발생한다.
        // BUG-33567과 유사하게 첫번째 makeSmiValueWithValue의 qmxMem은 유지한다.
        
        // Memory 재사용을 위한 Memory 이동
        IDE_TEST_RAISE( aTemplate->stmt->qmxMem->setStatus( &sQmxMemStatus ) != IDE_SUCCESS, ERR_MEM_OP );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qmnMOVE::insertOneRow"
                     " memory error" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::deleteOneRow( qcTemplate * aTemplate,
                       qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    - delete one record 수행
 *    - trigger each row 수행
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::deleteOneRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE * sCodePlan = (qmncMOVE*) aPlan;
    qmndMOVE * sDataPlan =
        (qmndMOVE*) (aTemplate->tmplate.data + aPlan->offset);
    smiValue            sWhereSmiValues[QC_MAX_COLUMN_COUNT];
    idBool              sIsDiskTableOrPartition;

    //-----------------------------------
    // copy old row
    //-----------------------------------
    
    if ( sDataPlan->needTriggerRowForDelete == ID_TRUE )
    {
        // OLD ROW REFERENCING을 위한 저장
        idlOS::memcpy( sDataPlan->oldRow,
                       sDataPlan->deleteTuple->row,
                       sDataPlan->deleteTuple->rowOffset );
    }
    else
    {
        // Nothing to do.
    }
    
    //-----------------------------------
    // PROJ-2334 PMT
    // set delete trigger memory variable column info
    //-----------------------------------
    if ( ( sDataPlan->needTriggerRowForDelete == ID_TRUE ) &&
         ( sCodePlan->tableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) )
    {
        sDataPlan->columnsForRow = sDataPlan->deletePartInfo->columns;
    }
    else
    {
        // Nothing To Do
    }
    
    //-----------------------------------
    // delete before trigger
    //-----------------------------------
    
    if ( sDataPlan->existTriggerForDelete == ID_TRUE )
    {
        // PROJ-1359 Trigger
        // ROW GRANULARITY TRIGGER의 수행
        IDE_TEST( qdnTrigger::fireTrigger(
                      aTemplate->stmt,
                      aTemplate->stmt->qmxMem,
                      sCodePlan->tableRef->tableInfo,
                      QCM_TRIGGER_ACTION_EACH_ROW,
                      QCM_TRIGGER_BEFORE,
                      QCM_TRIGGER_EVENT_DELETE,
                      NULL,                        // UPDATE Column
                      sDataPlan->deleteCursor,     /* Table Cursor */
                      sDataPlan->deleteTuple->rid, /* Row GRID */
                      sDataPlan->oldRow,           // OLD ROW
                      sDataPlan->columnsForRow,    // OLD ROW Column
                      NULL,                        // NEW ROW
                      NULL )                       // NEW ROW Column
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( sDataPlan->retryInfo.mIsWithoutRetry == ID_TRUE )
    {
        if ( sCodePlan->tableRef->tableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            sIsDiskTableOrPartition = QCM_TABLE_TYPE_IS_DISK( sDataPlan->deletePartInfo->tableFlag );
        }
        else
        {
            sIsDiskTableOrPartition = QCM_TABLE_TYPE_IS_DISK( sCodePlan->tableRef->tableInfo->tableFlag );
        }

        if ( sIsDiskTableOrPartition == ID_TRUE )
        {
            IDE_TEST( qmx::setChkSmiValueList( sDataPlan->deleteTuple->row,
                                               sDataPlan->retryInfo.mStmtRetryColLst,
                                               sWhereSmiValues,
                                               & (sDataPlan->retryInfo.mStmtRetryValLst) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // Nothing to do.
    }

    //-----------------------------------
    // delete one row
    //-----------------------------------
    
    IDE_TEST( sDataPlan->deleteCursor->deleteRow( &(sDataPlan->retryInfo) )
              != IDE_SUCCESS );

    // delete index table
    IDE_TEST( deleteIndexTableCursor( aTemplate,
                                      sCodePlan,
                                      sDataPlan )
              != IDE_SUCCESS );
    
    //-----------------------------------
    // delete after trigger
    //-----------------------------------
    
    if ( sDataPlan->existTriggerForDelete == ID_TRUE )
    {
        // PROJ-1359 Trigger
        // ROW GRANULARITY TRIGGER의 수행
        IDE_TEST( qdnTrigger::fireTrigger(
                      aTemplate->stmt,
                      aTemplate->stmt->qmxMem,
                      sCodePlan->tableRef->tableInfo,
                      QCM_TRIGGER_ACTION_EACH_ROW,
                      QCM_TRIGGER_AFTER,
                      QCM_TRIGGER_EVENT_DELETE,
                      NULL,                        // UPDATE Column
                      sDataPlan->deleteCursor,     /* Table Cursor */
                      sDataPlan->deleteTuple->rid, /* Row GRID */
                      sDataPlan->oldRow,           // OLD ROW
                      sDataPlan->columnsForRow,    // OLD ROW Column
                      NULL,                        // NEW ROW
                      NULL )                       // NEW ROW Column
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    if ( ( *sDataPlan->flag & QMND_MOVE_REMOVE_MASK )
         == QMND_MOVE_REMOVE_FALSE )
    {
        *sDataPlan->flag &= ~QMND_MOVE_REMOVE_MASK;
        *sDataPlan->flag |= QMND_MOVE_REMOVE_TRUE;
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
qmnMOVE::checkInsertRef( qcTemplate * aTemplate,
                         qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::checkInsertRef"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE            * sCodePlan;
    qmndMOVE            * sDataPlan;
    qmcInsertPartCursor * sCursorIter;
    void                * sRow = NULL;
    UInt                  i;
    
    sCodePlan = (qmncMOVE*) aPlan;
    sDataPlan = (qmndMOVE*) ( aTemplate->tmplate.data + aPlan->offset );
    
    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aTemplate != NULL );
    
    //------------------------------------------
    // parent constraint 검사
    //------------------------------------------

    if ( sCodePlan->parentConstraints != NULL )
    {
        for ( i = 0; i < sDataPlan->insertCursorMgr.mCursorIndexCount; i++ )
        {
            sCursorIter = sDataPlan->insertCursorMgr.mCursorIndex[i];
            
            if( sCursorIter->partitionRef == NULL )
            {
                IDE_TEST( checkInsertChildRefOnScan(
                              aTemplate,
                              sCodePlan,
                              sCodePlan->targetTableRef->tableInfo,
                              sCodePlan->targetTableRef->table,
                              sCursorIter,
                              & sRow )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( checkInsertChildRefOnScan(
                              aTemplate,
                              sCodePlan,
                              sCursorIter->partitionRef->partitionInfo,
                              sCursorIter->partitionRef->table,  // table tuple 사용
                              sCursorIter,
                              & sRow )
                          != IDE_SUCCESS );
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

IDE_RC qmnMOVE::checkInsertChildRefOnScan( qcTemplate           * aTemplate,
                                           qmncMOVE             * aCodePlan,
                                           qcmTableInfo         * aTableInfo,
                                           UShort                 aTable,
                                           qmcInsertPartCursor  * aCursorIter,
                                           void                ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    INSERT 구문 수행 시 Parent Table에 대한 Referencing 제약 조건을 검사
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::checkInsertChildRefOnScan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                sTableType;
    UInt                sRowSize = 0;
    void              * sRow;
    scGRID              sRid;
    iduMemoryStatus     sQmxMemStatus;
    mtcTuple          * sTuple;

    //----------------------------
    // Record 공간 확보
    //----------------------------

    sTuple = &(aTemplate->tmplate.rows[aTable]);
    sTableType = aTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;
    sRow = *aRow;

    if ( ( sTableType == SMI_TABLE_DISK ) &&
         ( sRow == NULL ) )
    {
        // Disk Table인 경우
        // Record Read를 위한 공간을 할당한다.
        IDE_TEST( qdbCommon::getDiskRowSize( aTableInfo,
                                             & sRowSize )
                  != IDE_SUCCESS );

        IDE_TEST( aTemplate->stmt->qmxMem->cralloc( sRowSize,
                                                    (void **) & sRow )
                  != IDE_SUCCESS);

        *aRow = sRow;
    }
    else
    {
        // Memory Table인 경우
        // Nothing to do.
    }

    //------------------------------------------
    // INSERT된 로우 검색을 위해,
    // 갱신연산이 수행된 첫번째 row 이전 위치로 cursor 위치 설정
    //------------------------------------------

    IDE_TEST( aCursorIter->cursor.beforeFirstModified( SMI_FIND_MODIFIED_NEW )
              != IDE_SUCCESS );
    
    IDE_TEST( aCursorIter->cursor.readNewRow( (const void **) & sRow,
                                              & sRid )
              != IDE_SUCCESS);

    //----------------------------
    // 반복 검사
    //----------------------------

    while ( sRow != NULL )
    {
        //------------------------------
        // 제약 조건 검사
        //------------------------------

        // Memory 재사용을 위하여 현재 위치 기록
        IDE_TEST_RAISE( aTemplate->stmt->qmxMem->getStatus( &sQmxMemStatus ) != IDE_SUCCESS, ERR_MEM_OP );

        IDE_TEST( qdnForeignKey::checkParentRef( aTemplate->stmt,
                                                 NULL,
                                                 aCodePlan->parentConstraints,
                                                 sTuple,
                                                 sRow,
                                                 0 )
                  != IDE_SUCCESS);

        // Memory 재사용을 위한 Memory 이동
        IDE_TEST_RAISE( aTemplate->stmt->qmxMem->setStatus( &sQmxMemStatus ) != IDE_SUCCESS, ERR_MEM_OP );
        
        IDE_TEST( aCursorIter->cursor.readNewRow( (const void **) & sRow,
                                                  & sRid )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qmnMOVE::checkInsertChildRefOnScan"
                     " memory error" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::checkDeleteRef( qcTemplate * aTemplate,
                         qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::checkDeleteRef"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncMOVE        * sCodePlan;
    qmndMOVE        * sDataPlan;
    qmsPartitionRef * sPartitionRef;
    smiStatement      sSmiStmt;
    smiStatement    * sSmiStmtOrg;
    UInt              sStage = 0;
    
    sCodePlan = (qmncMOVE*) aPlan;
    sDataPlan = (qmndMOVE*) ( aTemplate->tmplate.data + aPlan->offset );
    
    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aTemplate != NULL );
    
    //------------------------------------------
    // child constraint 검사
    //------------------------------------------

    if ( sCodePlan->childConstraints != NULL )
    {
        // BUG-17940 parent key를 갱신하고 child key를 찾을때
        // parent row에 lock을 잡은 이후 view를 보기위해
        // 새로운 smiStmt를 이용한다.
        // Delete cascade 옵션에 대비해서 normal로 한다.
        // child table의 타입을 현재 알 수 없기 때문에 ALL CURSOR로 한다.
        qcg::getSmiStmt( aTemplate->stmt, & sSmiStmtOrg );

        IDE_TEST( sSmiStmt.begin( aTemplate->stmt->mStatistics,
                                  QC_SMI_STMT( aTemplate->stmt ),
                                  SMI_STATEMENT_NORMAL |
                                  SMI_STATEMENT_SELF_TRUE |
                                  SMI_STATEMENT_ALL_CURSOR )
                  != IDE_SUCCESS );
        qcg::setSmiStmt( aTemplate->stmt, & sSmiStmt );

        sStage = 1;

        if ( sDataPlan->deleteCursor != NULL )
        {
            if ( sCodePlan->tableRef->partitionRef == NULL )
            {
                IDE_TEST( checkDeleteChildRefOnScan( aTemplate,
                                                     sCodePlan,
                                                     sCodePlan->tableRef->tableInfo,
                                                     sDataPlan->deleteTuple )
                          != IDE_SUCCESS );
            }
            else
            {
                for ( sPartitionRef = sCodePlan->tableRef->partitionRef;
                      sPartitionRef != NULL;
                      sPartitionRef = sPartitionRef->next )
                {
                    IDE_TEST( checkDeleteChildRefOnScan(
                                  aTemplate,
                                  sCodePlan,
                                  sPartitionRef->partitionInfo,
                                  & aTemplate->tmplate.rows[sPartitionRef->table] )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            // Nothing to do.
        }

        sStage = 0;

        qcg::setSmiStmt( aTemplate->stmt, sSmiStmtOrg );

        IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStage == 1 )
    {
        qcg::setSmiStmt( aTemplate->stmt, sSmiStmtOrg );

        if (sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS)
        {
            IDE_CALLBACK_FATAL("Check Child Key On Delete smiStmt.end() failed");
        }
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::checkDeleteChildRefOnScan( qcTemplate     * aTemplate,
                                    qmncMOVE       * aCodePlan,
                                    qcmTableInfo   * aTableInfo,
                                    mtcTuple       * aDeleteTuple )
{
/***********************************************************************
 *
 * Description :
 *    DELETE 구문 수행 시 Child Table에 대한 Referencing 제약 조건을 검사
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::checkDeleteChildRefOnScan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduMemoryStatus   sQmxMemStatus;
    void            * sOrgRow;
    void            * sSearchRow;
    smiTableCursor  * sDeleteCursor;
    qmnCursorInfo   * sCursorInfo;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aCodePlan->childConstraints != NULL );

    //------------------------------------------
    // DELETE된 로우 검색을 위해,
    // 갱신연산이 수행된 첫번째 row 이전 위치로 cursor 위치 설정
    //------------------------------------------

    sCursorInfo = (qmnCursorInfo*) aDeleteTuple->cursorInfo;
    
    IDE_TEST_RAISE( sCursorInfo == NULL, ERR_NOT_FOUND );
    
    sDeleteCursor = sCursorInfo->cursor;

    // PROJ-1624 non-partitioned index
    // index table scan으로 open되지 않은 partition이 존재한다.
    if ( sDeleteCursor != NULL )
    {
        IDE_TEST( sDeleteCursor->beforeFirstModified( SMI_FIND_MODIFIED_OLD )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Referencing 검사를 위해 삭제된 Row들을 검색
        //------------------------------------------

        sOrgRow = sSearchRow = aDeleteTuple->row;
        IDE_TEST(
            sDeleteCursor->readOldRow( (const void**) & sSearchRow,
                                       & aDeleteTuple->rid )
            != IDE_SUCCESS );

        aDeleteTuple->row = ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        while( sSearchRow != NULL )
        {
            // Memory 재사용을 위하여 현재 위치 기록
            IDE_TEST_RAISE( aTemplate->stmt->qmxMem->getStatus(&sQmxMemStatus) != IDE_SUCCESS, ERR_MEM_OP );

            //------------------------------------------
            // Child Table에 대한 Referencing 검사
            //------------------------------------------
        
            IDE_TEST( qdnForeignKey::checkChildRefOnDelete(
                          aTemplate->stmt,
                          aCodePlan->childConstraints,
                          aTableInfo->tableID,
                          aDeleteTuple,
                          aDeleteTuple->row,
                          ID_TRUE )
                      != IDE_SUCCESS );
        
            // Memory 재사용을 위한 Memory 이동
            IDE_TEST_RAISE( aTemplate->stmt->qmxMem->setStatus(&sQmxMemStatus) != IDE_SUCCESS, ERR_MEM_OP );

            sOrgRow = sSearchRow = aDeleteTuple->row;
            
            IDE_TEST(
                sDeleteCursor->readOldRow( (const void**) & sSearchRow,
                                           & aDeleteTuple->rid )
                != IDE_SUCCESS );
            
            aDeleteTuple->row =
                (sSearchRow == NULL) ? sOrgRow : sSearchRow;
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEM_OP )
    {
        ideLog::log( IDE_ERR_0,
                     "Unexpected errors may have occurred:"
                     " qmnMOVE::checkDeleteChildRefOnScan"
                     " memory error" );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmnMOVE::checkDeleteChildRefOnScan",
                                  "cursor not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnMOVE::insertIndexTableCursor( qcTemplate     * aTemplate,
                                 qmndMOVE       * aDataPlan,
                                 smiValue       * aInsertValue,
                                 scGRID           aRowGRID )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 구문 수행 시 index table에 대한 insert 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::insertIndexTableCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    smOID  sPartOID;
    
    // insert index table
    if ( ( *aDataPlan->flag & QMND_MOVE_INSERT_INDEX_CURSOR_MASK )
         == QMND_MOVE_INSERT_INDEX_CURSOR_INITED )
    {
        IDE_TEST( aDataPlan->insertCursorMgr.getSelectedPartitionOID(
                      & sPartOID )
                  != IDE_SUCCESS );
        
        IDE_TEST( qmsIndexTable::insertIndexTableCursors(
                      aTemplate->stmt,
                      & (aDataPlan->insertIndexTableCursorInfo),
                      aInsertValue,
                      sPartOID,
                      aRowGRID )
                  != IDE_SUCCESS );
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
qmnMOVE::deleteIndexTableCursor( qcTemplate     * aTemplate,
                                 qmncMOVE       * aCodePlan,
                                 qmndMOVE       * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    MOVE 구문 수행 시 index table에 대한 delete 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnMOVE::deleteIndexTableCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // update index table
    if ( ( *aDataPlan->flag & QMND_MOVE_DELETE_INDEX_CURSOR_MASK )
         == QMND_MOVE_DELETE_INDEX_CURSOR_INITED )
    {
        // selected index table
        if ( aCodePlan->tableRef->selectedIndexTable != NULL )
        {
            IDE_TEST( aDataPlan->indexDeleteCursor->deleteRow()
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // 다른 index table도 update
        IDE_TEST( qmsIndexTable::deleteIndexTableCursors(
                      aTemplate->stmt,
                      & (aDataPlan->deleteIndexTableCursorInfo),
                      aDataPlan->deleteTuple->rid )
                  != IDE_SUCCESS );
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
