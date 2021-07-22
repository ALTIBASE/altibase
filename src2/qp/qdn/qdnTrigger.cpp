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
 * $Id: qdnTrigger.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *
 *     [PROJ-1359] Trigger
 *
 *     Trigger 처리를 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <mtuProperty.h>
#include <qcg.h>
#include <qdnTrigger.h>
#include <qcmCache.h>
#include <qcmUser.h>
#include <qcmTrigger.h>
#include <qdbCommon.h>
#include <qdpPrivilege.h>
#include <qcpManager.h>
#include <qcmProc.h>
#include <qmv.h>
#include <qsv.h>
#include <qsx.h>
#include <qsxRelatedProc.h>
#include <qsvProcVar.h>
#include <qcsModule.h>
#include <qdpRole.h>

extern mtdModule    mtdBoolean;

IDE_RC
qdnTrigger::parseCreate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    다음과 같은 Post Parsing 작업을 수행함.
 *    - Syntax Validation
 *    - 미지원 기능의 검사.
 *    - CREATE TRIGGER를 위한 부가 Parsing 작업을 수행함.
 *    - Action Body의 Post Parsing
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::parseCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnCreateTriggerParseTree * sParseTree;
    qdnTriggerRef             * sRefer;
    qdnTriggerEventTypeList   * sEventTypeList;
    qdnTriggerEventTypeList   * sEventTypeHeader;

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    //---------------------------------------------------
    // Syntax Validation
    //---------------------------------------------------

    // Action Granularity가 FOR EACH ROW가 아닌 경우에는
    // RERENCING 구문을 사용할 수 없다.

    if ( sParseTree->actionCond.actionGranularity !=
         QCM_TRIGGER_ACTION_EACH_ROW )
    {
        IDE_TEST_RAISE( sParseTree->triggerReference != NULL,
                        err_invalid_referencing );
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------------------
    // parseTree->triggerEvent->eventTypelist에 insert or
    // delete or update.. 의 정보를 통합.
    //---------------------------------------------------

    sEventTypeHeader = sParseTree->triggerEvent.eventTypeList;

    for (sEventTypeList = sParseTree->triggerEvent.eventTypeList->next;
         sEventTypeList != NULL;
         sEventTypeList = sEventTypeList->next)
    {
        sEventTypeHeader->eventType |= sEventTypeList->eventType;
        if (sEventTypeList->updateColumns != NULL)
        {
            sEventTypeHeader->updateColumns = sEventTypeList->updateColumns;
        }

        else
        {
            // do nothing
        }
    }

    //---------------------------------------------------
    // 미지원 기능의 검사
    //---------------------------------------------------

    // TABLE referencing 구문은 지원하지 않음.
    for ( sRefer = sParseTree->triggerReference;
          sRefer != NULL;
          sRefer = sRefer->next )
    {
        switch ( sRefer->refType )
        {
            case QCM_TRIGGER_REF_OLD_ROW:
            case QCM_TRIGGER_REF_NEW_ROW:
                // Valid Referencing Type
                break;
            default:
                IDE_RAISE( err_unsupported_trigger_referencing );
                break;
        }
    }

    //---------------------------------------------------
    // Granularity가  FOR EACH ROW인 경우 Validation을 위한
    // 부가 정보를 구축해 주어야 한다.
    //---------------------------------------------------

    switch ( sParseTree->actionCond.actionGranularity )
    {
        case QCM_TRIGGER_ACTION_EACH_ROW:
            IDE_TEST( addGranularityInfo( sParseTree )
                      != IDE_SUCCESS );
            IDE_TEST( addActionBodyInfo( aStatement, sParseTree )
                      != IDE_SUCCESS );
            break;
        case QCM_TRIGGER_ACTION_EACH_STMT:
            sParseTree->actionBody.paraDecls = NULL;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    aStatement->spvEnv->createProc              = &sParseTree->actionBody;
    aStatement->spvEnv->createProc->stmtText    = aStatement->myPlan->stmtText;
    aStatement->spvEnv->createProc->stmtTextLen = aStatement->myPlan->stmtTextLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_referencing);
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_INVALID_REFERENCING_GRANULARITY ));
    }
    IDE_EXCEPTION(err_unsupported_trigger_referencing);
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_UNSUPPORTED_EVENT_TIMING ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::validateCreate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Validation을 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::validateCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnCreateTriggerParseTree * sParseTree;
    volatile UInt               sSessionUserID;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qdnTrigger::validateCreate::__FT__" );

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    // 현재 session userID 저장
    sSessionUserID = QCG_GET_SESSION_USER_ID( aStatement );
    IDU_FIT_POINT_FATAL( "qdnTrigger::validateCreate::__FT__::SessionUserID" );

    // Action Body의 Validtion을 위하여 해당 정보를 설정해 주어야 함.
    aStatement->spvEnv->createProc = & sParseTree->actionBody;

    // BUG-24570
    // Trigger User에 대한 정보 설정
    IDE_TEST( setTriggerUser( aStatement, sParseTree ) != IDE_SUCCESS );

    //----------------------------------------
    // CREATE TRIGGER 구문의 Validation
    //----------------------------------------

    // BUG-24408
    // trigger의 소유자로 validation한다.
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->triggerUserID );

    // Trigger의 모든 구문에서 Host 변수는 존재할 수 없다.
    IDE_TEST( qsv::checkHostVariable( aStatement ) != IDE_SUCCESS);

    // Trigger 생성에 대한 권한 검사
    IDE_TEST( valPrivilege( aStatement, sParseTree ) != IDE_SUCCESS );

    // Trigger Name에 대한 Validation
    IDE_TEST( valTriggerName( aStatement, sParseTree ) != IDE_SUCCESS );

    // Trigger Table에 대한 Validation
    IDE_TEST( valTableName( aStatement, sParseTree ) != IDE_SUCCESS );

    // Trigger Event와 Referncing 에 대한 Validation
    IDE_TEST( valEventReference( aStatement, sParseTree ) != IDE_SUCCESS );

    // Action Condition에 대한 Validation
    IDE_TEST( valActionCondition( aStatement, sParseTree ) != IDE_SUCCESS );

    // Action Body 에 대한 Validation
    IDE_TEST( valActionBody( aStatement, sParseTree ) != IDE_SUCCESS );

    //PROJ-1888 INSTEAD OF TRIGGER 제약사항 검사
    IDE_TEST( valInsteadOfTrigger( sParseTree ) != IDE_SUCCESS );

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::validateReplace( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Validation을 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::validateReplace"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnCreateTriggerParseTree * sParseTree;
    volatile UInt               sSessionUserID;

    UInt                        sTableID;
    idBool                      sExist;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qdnTrigger::validateReplace::__FT__" );

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    // 현재 session userID 저장
    sSessionUserID = QCG_GET_SESSION_USER_ID( aStatement );

    IDU_FIT_POINT_FATAL( "qdnTrigger::validateReplace::__FT__::SessionUserID" );

    // Action Body의 Validtion을 위하여 해당 정보를 설정해 주어야 함.
    aStatement->spvEnv->createProc = & sParseTree->actionBody;

    // BUG-24570
    // Trigger User에 대한 정보 설정
    IDE_TEST( setTriggerUser( aStatement, sParseTree ) != IDE_SUCCESS );

    //----------------------------------------
    // CREATE TRIGGER 구문의 Validation
    //----------------------------------------

    // BUG-24408
    // trigger의 소유자로 validation한다.
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->triggerUserID );

    // Trigger의 모든 구문에서 Host 변수는 존재할 수 없다.
    IDE_TEST( qsv::checkHostVariable( aStatement ) != IDE_SUCCESS);

    // Trigger 생성에 대한 권한 검사
    IDE_TEST( valPrivilege( aStatement, sParseTree ) != IDE_SUCCESS );

    IDE_TEST(qcmTrigger::getTriggerOID( aStatement,
                                        sParseTree->triggerUserID,
                                        sParseTree->triggerNamePos,
                                        & (sParseTree->triggerOID),
                                        & sTableID,
                                        & sExist )
             != IDE_SUCCESS );
    if (sExist == ID_TRUE)
    {
        // To Fix BUG-20948
        // 원본 테이블의 TABLEID를 저장한다.
        sParseTree->orgTableID = sTableID;

        // replace trigger
        sParseTree->common.execute = qdnTrigger::executeReplace;

        // Trigger SCN 설정
        IDE_TEST(getTriggerSCN( sParseTree->triggerOID,
                                &(sParseTree->triggerSCN) )
                 != IDE_SUCCESS);
    }
    else
    {
        // create trigger
        sParseTree->triggerOID = 0;
        sParseTree->common.execute = qdnTrigger::executeCreate;
    }

    // Trigger Name의 설정
    QC_STR_COPY( sParseTree->triggerName, sParseTree->triggerNamePos );

    // Trigger Table에 대한 Validation
    IDE_TEST( valTableName( aStatement, sParseTree ) != IDE_SUCCESS );

    //To Fix BUG-20948
    if ( sExist == ID_TRUE )
    {
        if ( sParseTree->tableID != sParseTree->orgTableID )
        {
            //Trigger의 원본 Table이 변경되었을 경우의 처리
            IDE_TEST( valOrgTableName ( aStatement, sParseTree ) != IDE_SUCCESS );
        }
        else
        {
            //Nothing To Do.
        }
    }
    else
    {
        //Nothing To Do.
    }

    // Trigger Event와 Referncing 에 대한 Validation
    IDE_TEST( valEventReference( aStatement, sParseTree ) != IDE_SUCCESS );

    // Action Condition에 대한 Validation
    IDE_TEST( valActionCondition( aStatement, sParseTree ) != IDE_SUCCESS );

    // Action Body 에 대한 Validation
    IDE_TEST( valActionBody( aStatement, sParseTree ) != IDE_SUCCESS );

    //PROJ-1888 INSTEAD OF TRIGGER 제약사항 검사
    IDE_TEST( valInsteadOfTrigger( sParseTree ) != IDE_SUCCESS );

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::validateRecompile( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    Recompile을 위한 Validation을 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::validateRecompile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnCreateTriggerParseTree * sParseTree;
    UInt                        sSessionUserID;

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    // 현재 session userID 저장
    sSessionUserID = QCG_GET_SESSION_USER_ID( aStatement );

    // Action Body의 Validtion을 위하여 해당 정보를 설정해 주어야 함.
    aStatement->spvEnv->createProc = & sParseTree->actionBody;

    // BUG-24570
    // Trigger User에 대한 정보 설정
    IDE_TEST( setTriggerUser( aStatement, sParseTree ) != IDE_SUCCESS );

    //----------------------------------------
    // CREATE TRIGGER 구문의 Validation
    //----------------------------------------

    // BUG-24408
    // trigger의 소유자로 validation한다.
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->triggerUserID );

    // Trigger의 모든 구문에서 Host 변수는 존재할 수 없다.
    IDE_TEST( qsv::checkHostVariable( aStatement ) != IDE_SUCCESS);

    // Trigger 생성에 대한 권한 검사
    IDE_TEST( valPrivilege( aStatement, sParseTree ) != IDE_SUCCESS );

    // Trigger Name에 대한 Validation
    // Recompile 과정이므로 반드시 존재해야 함.
    IDE_TEST( reValTriggerName( aStatement, sParseTree ) != IDE_SUCCESS );

    // Trigger Table에 대한 Validation
    IDE_TEST( valTableName( aStatement, sParseTree ) != IDE_SUCCESS );

    // Trigger Event와 Referncing 에 대한 Validation
    IDE_TEST( valEventReference( aStatement, sParseTree ) != IDE_SUCCESS );

    // Action Condition에 대한 Validation
    IDE_TEST( valActionCondition( aStatement, sParseTree ) != IDE_SUCCESS );

    // Action Body 에 대한 Validation
    IDE_TEST( valActionBody( aStatement, sParseTree ) != IDE_SUCCESS );

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::optimizeCreate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Optimization을 수행함.
 *
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::optimizeCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnCreateTriggerParseTree * sParseTree;
    volatile UInt               sSessionUserID;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qdnTrigger::optimizeCreate::__FT__" );

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    // 현재 session userID 저장
    sSessionUserID = QCG_GET_SESSION_USER_ID( aStatement );

    IDU_FIT_POINT_FATAL( "qdnTrigger::optimizeCreate::__FT__::SessionUserID" );

    //---------------------------------------------------
    // Action Body의 Optimization
    //---------------------------------------------------

    // BUG-24408
    // trigger의 소유자로 optimize한다.
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->triggerUserID );

    IDE_TEST( sParseTree->actionBody.block->common.
              optimize( aStatement,
                        (qsProcStmts *) (sParseTree->actionBody.block) )
              != IDE_SUCCESS);

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    // session userID를 원복
    QCG_SET_SESSION_USER_ID( aStatement, sSessionUserID );

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::executeCreate( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Execution을 수행함.
 *
 *    이전의 PVO 과정에서 구축된 정보는 Session에 종속적인 정보이다.
 *    따라서, Session에 종속적이지 않은 정보를 구축하기 위해서는
 *    영구적인 Memory 공간을 할당 받아 새로이 PVO를 수행하여야 한다.
 *
 *    이와 같은 Recompile 작업을 위한 최소 정보만을 구축하며,
 *    최초 DML발생시 Trigger Object Cache 정보를 이용하여 Recompile하게 된다.
 *
 *    - Trigger Object 생성(Trigger String을 영구히 괸리)
 *    - Trigger Object와 Trigger Object Cache 정보를 연결
 *      (CREATE TRIGGER 시점과 Server 구동 시점에 생성된다)
 *    - 관련 Meta Table에 정보 등록
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::executeCreate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                        sStage = 0;
    void                      * sTriggerHandle = NULL;
    qdnTriggerCache           * sCache = NULL;
    qdnCreateTriggerParseTree * sParseTree;
    smOID                       sTriggerOID;

    qcmPartitionInfoList      * sAllPartInfoList = NULL;
    qcmPartitionInfoList      * sOldPartInfoList = NULL;
    qcmPartitionInfoList      * sNewPartInfoList = NULL;
    qcmTableInfo              * sOldTableInfo    = NULL;
    qcmTableInfo              * sNewTableInfo    = NULL;
    void                      * sNewTableHandle  = NULL;
    smSCN                       sNewSCN          = SM_SCN_INIT;

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    sOldTableInfo = sParseTree->tableInfo;

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      QC_QMX_MEM( aStatement ),
                                                      sOldTableInfo->tableID,
                                                      & sAllPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sAllPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()) )
                  != IDE_SUCCESS );

        // 예외 처리를 위하여, Lock을 잡은 후에 Partition List를 설정한다.
        sOldPartInfoList = sAllPartInfoList;
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------------
    // Trigger Object의 생성하고
    // CREATE TRIGGER 를 위한 구문 자체를 저장한다.
    //---------------------------------------

    IDE_TEST( createTriggerObject( aStatement, &sTriggerHandle )
              != IDE_SUCCESS );

    //---------------------------------------
    // Trigger Object에  Trigger Object Cache 정보 구축
    //---------------------------------------

    sTriggerOID = smiGetTableId( sTriggerHandle ) ;

    // Trigger Handle에
    // Trigger Object Cache를 위한 필요한 공간을 설정한다.
    IDE_TEST( allocTriggerCache( sTriggerHandle,
                                 sTriggerOID,
                                 & sCache )
              != IDE_SUCCESS );

    sStage = 1;

    //---------------------------------------
    // Trigger를 위한 Meta Table에 정보 삽입
    //---------------------------------------

    IDE_TEST( qcmTrigger::addMetaInfo( aStatement, sTriggerHandle )
              != IDE_SUCCESS );

    //---------------------------------------
    // Table 의 Meta Cache 정보를 재구축함.
    //---------------------------------------

    IDE_TEST( qcm::touchTable(QC_SMI_STMT( aStatement ),
                              sParseTree->tableID,
                              SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS);

    IDE_TEST(qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                          sParseTree->tableID,
                                          sParseTree->tableOID )
             != IDE_SUCCESS);

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sOldTableInfo->tableID,
                                     & sNewTableInfo,
                                     & sNewSCN,
                                     & sNewTableHandle )
              != IDE_SUCCESS );

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                        sOldPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                      sNewTableInfo,
                                                                      sOldPartInfoList,
                                                                      & sNewPartInfoList )
                  != IDE_SUCCESS );

        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }
    else
    {
        /* Nothing to do */
    }

    (void)qcm::destroyQcmTableInfo(sParseTree->tableInfo);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void) freeTriggerCache( sCache );

        case 0:
            break;
    }

    (void)qcm::destroyQcmTableInfo( sNewTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sNewPartInfoList );

    qcmPartition::restoreTempInfo( sOldTableInfo,
                                   sOldPartInfoList,
                                   NULL );

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdnTrigger::executeReplace( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Execution을 수행함.
 *
 *    이전의 PVO 과정에서 구축된 정보는 Session에 종속적인 정보이다.
 *    따라서, Session에 종속적이지 않은 정보를 구축하기 위해서는
 *    영구적인 Memory 공간을 할당 받아 새로이 PVO를 수행하여야 한다.
 *
 *    이와 같은 Recompile 작업을 위한 최소 정보만을 구축하며,
 *    최초 DML발생시 Trigger Object Cache 정보를 이용하여 Recompile하게 된다.
 *
 *    - Trigger Object 생성(Trigger String을 영구히 괸리)
 *    - Trigger Object와 Trigger Object Cache 정보를 연결
 *      (CREATE TRIGGER 시점과 Server 구동 시점에 생성된다)
 *    - 관련 Meta Table에 정보 등록
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::executeReplace"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                        sStage = 0;
    void                      * sNewTriggerHandle = NULL;
    void                      * sOldTriggerHandle;
    qdnTriggerCache           * sOldTriggerCache;
    qdnTriggerCache           * sNewTriggerCache = NULL;
    qdnCreateTriggerParseTree * sParseTree;
    smOID                       sTriggerOID;

    qcmPartitionInfoList      * sAllPartInfoList    = NULL;
    qcmPartitionInfoList      * sOldPartInfoList    = NULL;
    qcmPartitionInfoList      * sNewPartInfoList    = NULL;
    qcmTableInfo              * sOldTableInfo       = NULL;
    qcmTableInfo              * sNewTableInfo       = NULL;
    qcmPartitionInfoList      * sOrgOldPartInfoList = NULL;
    qcmPartitionInfoList      * sOrgNewPartInfoList = NULL;
    qcmTableInfo              * sOrgOldTableInfo    = NULL;
    qcmTableInfo              * sOrgNewTableInfo    = NULL;
    void                      * sNewTableHandle     = NULL;
    smSCN                       sNewSCN             = SM_SCN_INIT;

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    //---------------------------------------
    // TASK-2176
    // Table에 대한 Lock을 확득한다.
    //---------------------------------------

    IDE_TEST( qcm::validateAndLockTable(aStatement,
                                        sParseTree->tableHandle,
                                        sParseTree->tableSCN,
                                        SMI_TABLE_LOCK_X)
              != IDE_SUCCESS );

    sOldTableInfo = sParseTree->tableInfo;

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      QC_QMX_MEM( aStatement ),
                                                      sOldTableInfo->tableID,
                                                      & sAllPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sAllPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()) )
                  != IDE_SUCCESS );

        // 예외 처리를 위하여, Lock을 잡은 후에 Partition List를 설정한다.
        sOldPartInfoList = sAllPartInfoList;
    }
    else
    {
        /* Nothing to do */
    }

    //------------------------------------------------------
    // BUG-20948
    // Comment : Replace할 Trigger의 TABLE ID가 같은지 비교한다.
    //           TABLE ID가 다를 경우, Trigger의 Meta Data뿐만이나라
    //           기존 TABLE에 저장된 Trigger 정보 또한 삭제해야 한다.
    //------------------------------------------------------
    if ( sParseTree->orgTableID != sParseTree->tableID )
    {
        //Validate & Lock
        IDE_TEST( qcm::validateAndLockTable(aStatement,
                                            sParseTree->orgTableHandle,
                                            sParseTree->orgTableSCN,
                                            SMI_TABLE_LOCK_X)
                  != IDE_SUCCESS);

        sOrgOldTableInfo = sParseTree->orgTableInfo;

        if ( sOrgOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                          QC_SMI_STMT( aStatement ),
                                                          QC_QMX_MEM( aStatement ),
                                                          sOrgOldTableInfo->tableID,
                                                          & sAllPartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                      sAllPartInfoList,
                                                                      SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                      SMI_TABLE_LOCK_X,
                                                                      smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()) )
                      != IDE_SUCCESS );

            // 예외 처리를 위하여, Lock을 잡은 후에 Partition List를 설정한다.
            sOrgOldPartInfoList = sAllPartInfoList;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        //Nothing To Do.
    }

    //---------------------------------------
    // Trigger Object의 생성하고
    // CREATE TRIGGER 를 위한 구문 자체를 저장한다.
    //---------------------------------------

    IDE_TEST( createTriggerObject( aStatement, &sNewTriggerHandle )
              != IDE_SUCCESS );


    //---------------------------------------
    // Trigger Object에  Trigger Object Cache 정보 구축
    //---------------------------------------

    sTriggerOID = smiGetTableId( sNewTriggerHandle );

    // Trigger Handle에
    // Trigger Object Cache를 위한 필요한 공간을 설정한다.
    IDE_TEST( allocTriggerCache( sNewTriggerHandle,
                                 sTriggerOID,
                                 & sNewTriggerCache )
              != IDE_SUCCESS );

    sStage = 1;

    //---------------------------------------
    // 트리거 SCN 검사 fix BUG-20479
    //---------------------------------------

    sOldTriggerHandle = (void *)smiGetTable( sParseTree->triggerOID );
    IDE_TEST_RAISE(smiValidateObjects(
                       sOldTriggerHandle,
                       sParseTree->triggerSCN)
                   != IDE_SUCCESS, ERR_TRIGGER_MODIFIED);

    //---------------------------------------
    // 이전 트리거 정보 획득 : old trigger 캐쉬 정보 획득
    //---------------------------------------
    IDE_TEST( smiObject::getObjectTempInfo( sOldTriggerHandle,
                                            (void**)&sOldTriggerCache )
              != IDE_SUCCESS );

    //---------------------------------------
    // 이전 트리거 메타에서 삭제.
    //---------------------------------------

    IDE_TEST( qcmTrigger::removeMetaInfo( aStatement, sParseTree->triggerOID )
              != IDE_SUCCESS );

    //---------------------------------------
    // NewTrigger를 위한 Meta Table에 정보 삽입
    //---------------------------------------

    IDE_TEST( qcmTrigger::addMetaInfo( aStatement, sNewTriggerHandle )
              != IDE_SUCCESS );

    //---------------------------------------
    // Table 의 Meta Cache 정보를 재구축함.
    //---------------------------------------

    IDE_TEST( qcm::touchTable(QC_SMI_STMT( aStatement ),
                              sParseTree->tableID,
                              SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS);

    IDE_TEST(qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                          sParseTree->tableID,
                                          sParseTree->tableOID )
             != IDE_SUCCESS);

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sOldTableInfo->tableID,
                                     & sNewTableInfo,
                                     & sNewSCN,
                                     & sNewTableHandle )
              != IDE_SUCCESS );

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                        sOldPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                      sNewTableInfo,
                                                                      sOldPartInfoList,
                                                                      & sNewPartInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    //To Fix BUG-20948
    if ( sParseTree->orgTableID != sParseTree->tableID )
    {
        IDE_TEST( qcm::touchTable(QC_SMI_STMT( aStatement ),
                                  sParseTree->orgTableID,
                                  SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS);

        IDE_TEST(qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                              sParseTree->orgTableID,
                                              sParseTree->orgTableOID )
                 != IDE_SUCCESS);

        IDE_TEST( qcm::getTableInfoByID( aStatement,
                                         sOrgOldTableInfo->tableID,
                                         & sOrgNewTableInfo,
                                         & sNewSCN,
                                         & sNewTableHandle )
                  != IDE_SUCCESS );

        if ( sOrgOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                            sOrgOldPartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                          sOrgNewTableInfo,
                                                                          sOrgOldPartInfoList,
                                                                          & sOrgNewPartInfoList )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    //---------------------------------------
    // 이전 트리거 object 삭제
    //---------------------------------------

    IDE_TEST( dropTriggerObject( aStatement, sOldTriggerHandle )
              != IDE_SUCCESS );

    IDE_TEST(freeTriggerCache(sOldTriggerCache) != IDE_SUCCESS);

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }
    else
    {
        /* Nothing to do */
    }

    (void)qcm::destroyQcmTableInfo(sParseTree->tableInfo);

    //To Fix BUG-20948
    if ( sParseTree->orgTableID != sParseTree->tableID )
    {
        if ( sOrgOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            (void)qcmPartition::destroyQcmPartitionInfoList( sOrgOldPartInfoList );
        }
        else
        {
            /* Nothing to do */
        }

        //Free Old Table Infomation
        (void)qcm::destroyQcmTableInfo( sParseTree->orgTableInfo );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRIGGER_MODIFIED );
    {
        IDE_SET(ideSetErrorCode( qpERR_REBUILD_TRIGGER_INVALID ) );
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void) freeTriggerCache( sNewTriggerCache );

        case 0:
            break;
    }

    (void)qcm::destroyQcmTableInfo( sNewTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sNewPartInfoList );

    (void)qcm::destroyQcmTableInfo( sOrgNewTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sOrgNewPartInfoList );

    qcmPartition::restoreTempInfo( sOldTableInfo,
                                   sOldPartInfoList,
                                   NULL );

    qcmPartition::restoreTempInfo( sOrgOldTableInfo,
                                   sOrgOldPartInfoList,
                                   NULL );

    return IDE_FAILURE;

#undef IDE_FN
}



IDE_RC
qdnTrigger::parseAlter( qcStatement * /* aStatement */ )
{
/***********************************************************************
 *
 * Description :
 *    ALTER TRIGGER를 위한 부가 Parsing 작업을 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::parseAlter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // Nothing To Do

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qdnTrigger::validateAlter( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER TRIGGER를 위한 Validation.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::validateAlter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcuSqlSourceInfo  sqlInfo;

    idBool     sExist;

    qdnAlterTriggerParseTree * sParseTree;

    sParseTree = (qdnAlterTriggerParseTree *) aStatement->myPlan->parseTree;

    //----------------------------------------
    // Trigger의 User 정보 획득
    //----------------------------------------

    // Trigger User 정보의 획득
    if ( QC_IS_NULL_NAME( sParseTree->triggerUserPos ) == ID_TRUE)
    {
        // User ID 획득
        sParseTree->triggerUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        // User ID 획득
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      sParseTree->triggerUserPos,
                                      & sParseTree->triggerUserID )
                  != IDE_SUCCESS);
    }

    //----------------------------------------
    // Trigger의 존재 여부 검사
    //----------------------------------------

    // 존재하는 Trigger인지 검사
    IDE_TEST( qcmTrigger::getTriggerOID( aStatement,
                                         sParseTree->triggerUserID,
                                         sParseTree->triggerNamePos,
                                         & sParseTree->triggerOID,
                                         & sParseTree->tableID,
                                         & sExist )
              != IDE_SUCCESS );

    if ( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->triggerUserPos,
                               & sParseTree->triggerNamePos );

        IDE_RAISE( err_non_exist_trigger_name );
    }
    else
    {
        // 해당 Trigger가 존재함.
    }

    //----------------------------------------
    // Alter Trigger 권한 검사
    //----------------------------------------

    IDE_TEST( qdpRole::checkDDLAlterTriggerPriv( aStatement,
                                                 sParseTree->triggerUserID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_non_exist_trigger_name);
    {
        (void) sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_TRIGGER_NOT_EXIST,
                            sqlInfo.getErrMessage() ));
        (void) sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::optimizeAlter( qcStatement * /* aStatement */ )
{
/***********************************************************************
 *
 * Description :
 *    ALTER TRIGGER를 위한 Optimization.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::optimizeAlter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // Nothing To Do

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qdnTrigger::executeAlter( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER TRIGGER의 수행.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::executeAlter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt              i;
    smSCN             sSCN;

    qcmTableInfo    * sTableInfo;
    qcmTriggerInfo  * sTriggerInfo;

    SChar           * sBuffer;
    vSLong             sRowCnt;
    void            * sTableHandle;

    qdnAlterTriggerParseTree * sParseTree;

    //----------------------------------------
    // 기본 정보의 획득
    //----------------------------------------

    sParseTree = (qdnAlterTriggerParseTree *) aStatement->myPlan->parseTree;

    IDE_TEST(qcm::getTableInfoByID( aStatement,
                                    sParseTree->tableID,
                                    & sTableInfo,
                                    & sSCN,
                                    & sTableHandle )
             != IDE_SUCCESS);

    IDE_TEST(qcm::validateAndLockTable(aStatement,
                                       sTableHandle,
                                       sSCN,
                                       SMI_TABLE_LOCK_X)
             != IDE_SUCCESS);

    for ( i = 0; i < sTableInfo->triggerCount; i++ )
    {
        if ( sTableInfo->triggerInfo[i].triggerOID ==
             sParseTree->triggerOID )
        {
            break;
        }
    }

    // 반드시 해당 Trigger 정보가 존재해야 함.
    IDE_DASSERT( i < sTableInfo->triggerCount );

    sTriggerInfo = & sTableInfo->triggerInfo[i];

    //----------------------------------------
    // ALTER TRIGGER의 수행
    //----------------------------------------

    switch ( sParseTree->option )
    {
        //--------------------------------------------
        // ENABLE, DISABLE :
        //    Table Lock을 잡고 수행하기 때문에
        //    별도로 Meta Cache를 재구성하지 않고 값만 변경시킨다.
        //--------------------------------------------
        case QDN_TRIGGER_ALTER_ENABLE:

            IDU_LIMITPOINT("qdnTrigger::executeAlter::malloc1");
            IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                                 (void**) & sBuffer )
                      != IDE_SUCCESS );

            //---------------------------------------
            // SYS_TRIGGERS_ 에서 정보 변경
            // UPDATE SYS_TRIGGERS_ SET IS_ENABLE = QCM_TRIGGER_ENABLE
            //        WHERE TRIGGER_OID = triggerOID;
            //---------------------------------------

            idlOS::snprintf(
                sBuffer,
                QD_MAX_SQL_LENGTH,
                "UPDATE SYS_TRIGGERS_ SET "
                "  IS_ENABLE = "QCM_SQL_INT32_FMT             // 0. IS_ENABLE
                ", LAST_DDL_TIME = SYSDATE "
                "WHERE TRIGGER_OID = "QCM_SQL_BIGINT_FMT,     // 1. TRIGGER_OID
                QCM_TRIGGER_ENABLE,                           // 0.
                QCM_OID_TO_BIGINT( sParseTree->triggerOID ) );// 1.

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sBuffer,
                                         & sRowCnt )
                      != IDE_SUCCESS );

            IDE_DASSERT( sRowCnt == 1 );

            //----------------------------------
            // Meta Cache에 ENABLE 설정
            //----------------------------------

            sTriggerInfo->enable = QCM_TRIGGER_ENABLE;

            break;

        case QDN_TRIGGER_ALTER_DISABLE:

            IDU_LIMITPOINT("qdnTrigger::executeAlter::malloc2");
            IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                                 (void**) & sBuffer )
                      != IDE_SUCCESS );

            //---------------------------------------
            // SYS_TRIGGERS_ 에서 정보 변경
            // UPDATE SYS_TRIGGERS_ SET IS_ENABLE = QCM_TRIGGER_DISABLE
            //        WHERE TRIGGER_OID = triggerOID;
            //---------------------------------------

            idlOS::snprintf(
                sBuffer, QD_MAX_SQL_LENGTH,
                "UPDATE SYS_TRIGGERS_ SET "
                "  IS_ENABLE = "QCM_SQL_INT32_FMT              // 0. IS_ENABLE
                ", LAST_DDL_TIME = SYSDATE "
                "WHERE TRIGGER_OID = "QCM_SQL_BIGINT_FMT,     // 1. TRIGGER_OID
                QCM_TRIGGER_DISABLE,                          // 0.
                QCM_OID_TO_BIGINT( sParseTree->triggerOID ) );// 1.

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sBuffer,
                                         & sRowCnt )
                      != IDE_SUCCESS );

            IDE_DASSERT( sRowCnt == 1 );

            //----------------------------------
            // Meta Cache에 DISABLE 설정
            //----------------------------------

            sTriggerInfo->enable = QCM_TRIGGER_DISABLE;

            break;

        case QDN_TRIGGER_ALTER_COMPILE:

            //----------------------------------
            // Recompile을 수행한다.
            //----------------------------------

            IDE_TEST( recompileTrigger( aStatement, sTriggerInfo )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::parseDrop( qcStatement * /* aStatement */ )
{
/***********************************************************************
 *
 * Description :
 *    DROP TRIGGER를 위한 부가 Parsing 작업을 수행함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::parseDrop"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // Nothing To Do

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qdnTrigger::validateDrop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    DROP TRIGGER를 위한 Validation.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::validateDrop"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcuSqlSourceInfo  sqlInfo;

    idBool                sExist;
    
    qdnDropTriggerParseTree * sParseTree;

    sParseTree = (qdnDropTriggerParseTree *) aStatement->myPlan->parseTree;
    
    //----------------------------------------
    // Trigger의 User 정보 획득
    //----------------------------------------

    // Trigger User 정보의 획득
    if ( QC_IS_NULL_NAME( sParseTree->triggerUserPos ) == ID_TRUE)
    {
        // User ID 획득
        sParseTree->triggerUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        // User ID 획득
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      sParseTree->triggerUserPos,
                                      & sParseTree->triggerUserID )
                  != IDE_SUCCESS);
    }

    //----------------------------------------
    // Trigger의 존재 여부 검사
    //----------------------------------------

    // 존재하는 Trigger인지 검사
    IDE_TEST( qcmTrigger::getTriggerOID( aStatement,
                                         sParseTree->triggerUserID,
                                         sParseTree->triggerNamePos,
                                         & sParseTree->triggerOID,
                                         & sParseTree->tableID,
                                         & sExist )
              != IDE_SUCCESS );

    if ( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->triggerUserPos,
                               & sParseTree->triggerNamePos );

        IDE_RAISE( err_non_exist_trigger_name );
    }
    else
    {
        // 해당 Trigger가 존재함.
    }

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sParseTree->tableID,
                                     & sParseTree->tableInfo,
                                     & sParseTree->tableSCN,
                                     & sParseTree->tableHandle)
              != IDE_SUCCESS);

    IDE_TEST(qcm::lockTableForDDLValidation(aStatement,
                                            sParseTree->tableHandle,
                                            sParseTree->tableSCN)
             != IDE_SUCCESS);

    //----------------------------------------
    // Trigger 삭제 권한 검사
    //----------------------------------------

    IDE_TEST( qdpRole::checkDDLDropTriggerPriv( aStatement,
                                                sParseTree->triggerUserID )
              != IDE_SUCCESS );

    //----------------------------------------
    // Replication 검사
    //----------------------------------------
    // PROJ-1567
    // IDE_TEST_RAISE( sParseTree->tableInfo->replicationCount > 0,
    //                 ERR_DDL_WITH_REPLICATED_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_non_exist_trigger_name);
    {
        (void) sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_TRIGGER_NOT_EXIST,
                            sqlInfo.getErrMessage() ));
        (void) sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::optimizeDrop( qcStatement * /* aStatement */ )
{
/***********************************************************************
 *
 * Description :
 *    DROP TRIGGER를 위한 Optimization.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::optimizeDrop"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // Nothing To Do

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qdnTrigger::executeDrop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    DROP TRIGGER의 수행.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::executeDrop"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void            * sTriggerHandle;
    qdnTriggerCache * sTriggerCache;
    qcmTableInfo    * sOldTableInfo = NULL;
    qcmTableInfo    * sNewTableInfo = NULL;
    smOID             sTableOID;
    void            * sTableHandle;
    smSCN             sSCN;

    qdnDropTriggerParseTree * sParseTree;

    qcmPartitionInfoList    * sAllPartInfoList = NULL;
    qcmPartitionInfoList    * sOldPartInfoList = NULL;
    qcmPartitionInfoList    * sNewPartInfoList = NULL;

    sParseTree = (qdnDropTriggerParseTree *) aStatement->myPlan->parseTree;

    //---------------------------------------
    // To Fix PR-11552
    // 관련된 PSM를 Invalid 함을 설정해 주어야 함.
    // To Fix PR-12102
    // 관련 TableInfo에 먼저 XLatch를 잡아야 함
    //
    // DROP TRIGGER(DDL) ---->Table->TRIGGER
    // DML ------------------>
    // 위와 같이 참조되므로 DROP TRIGGER에서 바로 TRIGGER에 접근하여
    // 삭제하면 안됨
    //---------------------------------------

    //---------------------------------------
    // TASK-2176 Table에 대한 Lock을 획득한다.
    //---------------------------------------

    IDE_TEST( qcm::validateAndLockTable(aStatement,
                                        sParseTree->tableHandle,
                                        sParseTree->tableSCN,
                                        SMI_TABLE_LOCK_X)
              != IDE_SUCCESS);

    sOldTableInfo = sParseTree->tableInfo;
    sTableOID     = smiGetTableId(sOldTableInfo->tableHandle);

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      QC_QMX_MEM( aStatement ),
                                                      sOldTableInfo->tableID,
                                                      & sAllPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sAllPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()) )
                  != IDE_SUCCESS );

        // 예외 처리를 위하여, Lock을 잡은 후에 Partition List를 설정한다.
        sOldPartInfoList = sAllPartInfoList;
    }
    else
    {
        /* Nothing to do */
    }

    //---------------------------------------
    // Trigger Object 삭제
    //---------------------------------------

    sTriggerHandle = (void *)smiGetTable( sParseTree->triggerOID );

    IDE_TEST( dropTriggerObject( aStatement, sTriggerHandle ) != IDE_SUCCESS );

    //---------------------------------------
    // Trigger를 위한 Meta Table의 정보 삭제
    //---------------------------------------

    IDE_TEST( qcmTrigger::removeMetaInfo( aStatement, sParseTree->triggerOID )
              != IDE_SUCCESS );

    //---------------------------------------
    // Table 의 Meta Cache 정보를 재구축함.
    //---------------------------------------

    IDE_TEST( qcm::touchTable( QC_SMI_STMT( aStatement ),
                               sParseTree->tableID,
                               SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS);

    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sParseTree->tableID,
                                           sTableOID )
              != IDE_SUCCESS);

    IDE_TEST( qcm::getTableInfoByID(aStatement,
                                    sParseTree->tableID,
                                    &sNewTableInfo,
                                    &sSCN,
                                    &sTableHandle)
              != IDE_SUCCESS);

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                        sOldPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                      sNewTableInfo,
                                                                      sOldPartInfoList,
                                                                      & sNewPartInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // To fix BUG-12102(BUG-12034와 같은 내용임)
    // SM이 더이상 실패할 여지가 없을 때 QP에서 유지하고
    // 있는 메모리들을 해제해야 한다.
    IDE_TEST( smiObject::getObjectTempInfo( sTriggerHandle,
                                            (void**)&sTriggerCache )
              != IDE_SUCCESS );;

    IDE_TEST( freeTriggerCache( sTriggerCache ) != IDE_SUCCESS );

    if ( sOldTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }
    else
    {
        /* Nothing to do */
    }

    (void)qcm::destroyQcmTableInfo(sOldTableInfo);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void)qcm::destroyQcmTableInfo( sNewTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sNewPartInfoList );

    qcmPartition::restoreTempInfo( sOldTableInfo,
                                   sOldPartInfoList,
                                   NULL );

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdnTrigger::needTriggerRow( qcStatement         * aStatement,
                            qcmTableInfo        * aTableInfo,
                            qcmTriggerEventTime   aEventTime,
                            UInt                  aEventType,
                            smiColumnList       * aUptColumn,
                            idBool              * aIsNeed )
{
/***********************************************************************
 *
 * Description :
 *    관련된 Trigger들 중 Trigger를 수행하기 위하여
 *    OLD ROW, NEW ROW에 대한 값을 참조하는 지를 검사하고,
 *    이를 구축해야 하는 지를 판단함.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::needTriggerRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;

    idBool sIsNeedTriggerRow = ID_FALSE;
    idBool sIsNeedAction     = ID_FALSE;
    idBool sIsRebuild        = ID_FALSE;

    qcmTriggerInfo * sTriggerInfo;

    //-------------------------------------------
    // 동일한 Trigger Event중 Row Granularity가 존재하는 지를 검사
    //-------------------------------------------

    sTriggerInfo = aTableInfo->triggerInfo;

    for ( i = 0; i < aTableInfo->triggerCount; i++ )
    {
        // PROJ-2219 Row-level before update trigger
        // sIsRebuild의 값을 checkCondition()에서 얻어오지만
        // 이 함수에서는 사용하지 않는다.
        IDE_TEST( checkCondition( aStatement,
                                  & sTriggerInfo[i],
                                  QCM_TRIGGER_ACTION_EACH_ROW,
                                  aEventTime,
                                  aEventType,
                                  aUptColumn,
                                  & sIsNeedAction,
                                  & sIsRebuild )
                  != IDE_SUCCESS ) ;

        if ( ( sIsNeedAction == ID_TRUE ) &&
             ( sTriggerInfo[i].refRowCnt > 0 ) )

        {
            sIsNeedTriggerRow = ID_TRUE;
            break;
        }
        else
        {
            // continue
        }
    }

    *aIsNeed = sIsNeedTriggerRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC
qdnTrigger::fireTrigger( qcStatement         * aStatement,
                         iduMemory           * aNewValueMem,
                         qcmTableInfo        * aTableInfo,
                         qcmTriggerGranularity aGranularity,
                         qcmTriggerEventTime   aEventTime,
                         UInt                  aEventType,
                         smiColumnList       * aUptColumn,
                         smiTableCursor      * aTableCursor, /* for LOB */
                         scGRID                aGRID,
                         void                * aOldRow,
                         qcmColumn           * aOldRowColumns,
                         void                * aNewRow,
                         qcmColumn           * aNewRowColumns )
{
/***********************************************************************
 *
 * Description :
 *    해당 입력 조건에 부합하는 Trigger를 획득하고,
 *    Trigger 조건을 만족할 경우 해당 Trigger Action을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::fireTrigger"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    idBool sNeedAction = ID_FALSE;
    idBool sIsRebuild  = ID_FALSE;
    qmmUptParseTree * sUptParseTree;
    qmmMultiTables  * sTmp;

    // BUG-46074 Multiple trigger event
    UInt            sOrgTriggerEventType  = aStatement->spxEnv->mTriggerEventType;
    qcmColumn     * sOrgTriggerUptColList = aStatement->spxEnv->mTriggerUptColList;

    if ( (aEventType == QCM_TRIGGER_EVENT_UPDATE) &&
         (aStatement->myPlan->parseTree->stmtKind == QCI_STMT_UPDATE) )
    {
        sUptParseTree = ((qmmUptParseTree*)aStatement->myPlan->parseTree);

        if ( sUptParseTree->mTableList == NULL )
        {
            aStatement->spxEnv->mTriggerUptColList = sUptParseTree->updateColumns;
        }
        else
        {
            aStatement->spxEnv->mTriggerUptColList = NULL;
            for ( sTmp = sUptParseTree->mTableList; sTmp != NULL; sTmp = sTmp->mNext )
            {
                if ( sTmp->mTableRef->tableInfo == aTableInfo )
                {
                    aStatement->spxEnv->mTriggerUptColList = sTmp->mColumns;
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
        aStatement->spxEnv->mTriggerUptColList = NULL;
    }

    aStatement->spxEnv->mTriggerEventType = aEventType;

    //-------------------------------------------
    // Trigger 조건 검사
    //-------------------------------------------

    for ( i = 0;  i < aTableInfo->triggerCount; i++ )
    {
        sNeedAction = ID_FALSE;

        // PROJ-2219 Row-level before update trigger
        // sIsRebuild의 값을 checkCondition()에서 얻어오지만
        // 이 함수에서는 사용하지 않는다.
        IDE_TEST( checkCondition( aStatement,
                                  & aTableInfo->triggerInfo[i],
                                  aGranularity,
                                  aEventTime,
                                  aEventType,
                                  aUptColumn,
                                  & sNeedAction,
                                  & sIsRebuild )
                  != IDE_SUCCESS );

        if ( sNeedAction == ID_TRUE )
        {
            // Trigger 조건이 부합하는 경우로 Trigger를 수행한다.
            IDE_TEST( fireTriggerAction( aStatement,
                                         aNewValueMem,
                                         aTableInfo,
                                         & aTableInfo->triggerInfo[i],
                                         aTableCursor,
                                         aGRID,
                                         aOldRow,
                                         aOldRowColumns,
                                         aNewRow,
                                         aNewRowColumns )
                      != IDE_SUCCESS );
        }
        else
        {
            // Trigger 수행 조건이 부합되지 않는 경우임.
            // Nothing To Do
        }
    }

    // BUG-46074 Multiple trigger event
    aStatement->spxEnv->mTriggerEventType  = sOrgTriggerEventType;
    aStatement->spxEnv->mTriggerUptColList = sOrgTriggerUptColList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-46074 Multiple trigger event
    aStatement->spxEnv->mTriggerEventType  = sOrgTriggerEventType;
    aStatement->spxEnv->mTriggerUptColList = sOrgTriggerUptColList;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::loadAllTrigger( smiStatement * aSmiStmt )
{
/***********************************************************************
 *
 * Description :
 *    Server 구동 시 동작하며, 모든 Trigger Object에 대하여
 *    Trigger Object Cache를 구성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::loadAllTrigger"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtcColumn           *sTriggerOIDCol;
    mtcColumn           *sUserIDCol;

    smiTableCursor        sCursor;
    smiCursorProperties   sCursorProperty;
    idBool                sIsCursorOpen = ID_FALSE;

    smOID                 sTriggerOID;
    void                * sTriggerHandle;
    qdnTriggerCache     * sTriggerCache = NULL;

    scGRID                sRID;
    void                * sRow;

    //-------------------------------------------
    // 적합성 검사
    //-------------------------------------------

    IDE_DASSERT( aSmiStmt != NULL );

    //-------------------------------------------
    // 모든 Trigger Record를 읽는다.
    //-------------------------------------------

    // Cursor의 초기화
    sCursor.initialize();

    // Cursor Property 초기화
    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

    // INDEX (USER_ID, TRIGGER_NAME)을 이용한 Cursor Open
    IDE_TEST(
        sCursor.open( aSmiStmt,  // smiStatement
                      gQcmTriggers,                  // Table Handle
                      NULL,                          // Index Handle
                      smiGetRowSCN( gQcmTriggers ),  // Table SCN
                      NULL,                          // Update Column정보
                      smiGetDefaultKeyRange(),       // Key Range
                      smiGetDefaultKeyRange(),       // Key Filter
                      smiGetDefaultFilter(),         // Filter
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      & sCursorProperty )            // Cursor Property
        != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    //-------------------------------------------
    // Trigger ID의 획득
    //-------------------------------------------

    // Data를 Read함.
    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( (const void **) & sRow, & sRID, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                  QCM_TRIGGERS_TRIGGER_OID,
                                  (const smiColumn**)&sTriggerOIDCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                  QCM_TRIGGERS_USER_ID,
                                  (const smiColumn**)&sUserIDCol )
              != IDE_SUCCESS );


    while ( sRow != NULL )
    {
        //--------------------------------------------
        // 하나의 Trigger Object 정보를 얻음.
        //--------------------------------------------

        // Trigger OID를 얻음
        // To Fix PR-10648
        // smOID 의 획득 방법이 잘못됨.
        // *aTriggerOID = *(smOID *)
        //    ((UChar*)sRow+sColumn[QCM_TRIGGERS_TRIGGER_OID].column.offset );


        sTriggerOID = (smOID) *(ULong *)
            ( (UChar*)sRow + sTriggerOIDCol->column.offset );

        //--------------------------------------------
        // Trigger Object Cache를 위한 정보 구축
        //--------------------------------------------

        // Trigger Handle을 획득
        sTriggerHandle = (void *)smiGetTable( sTriggerOID );

        // Trigger Handle에 Trigger Cache를 위한 공간 할당
        IDE_TEST( allocTriggerCache( sTriggerHandle,
                                     sTriggerOID,
                                     & sTriggerCache )
                  != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( (const void **) & sRow,
                                   & sRID,
                                   SMI_FIND_NEXT )
                  != IDE_SUCCESS );

    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
    }

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdnTrigger::freeTriggerCaches4DropTable( qcmTableInfo * aTableInfo )
{
    UInt                  i;
    qcmTriggerInfo      * sTriggerInfo;
    qdnTriggerCache     * sTriggerCache;

    IDE_DASSERT( aTableInfo != NULL );

    for ( i = 0, sTriggerInfo = aTableInfo->triggerInfo;
          i < aTableInfo->triggerCount;
          i++ )
    {
        IDE_TEST( smiObject::getObjectTempInfo( sTriggerInfo[i].triggerHandle,
                                                (void**)& sTriggerCache )
                  != IDE_SUCCESS );

        // Trigger Object Cache 삭제
        IDE_TEST( freeTriggerCache( sTriggerCache )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/* To Fix BUG-12034
   만약 여기서 freeTriggerCache까지 하게되면,
   이 함수를 호출하는 qdd::executeDropTable 이
   실패할 경우, 메모리만 해제되고, 메타정보는 그대로 살아있게되는 오류가 발생.
   *
   */
IDE_RC
qdnTrigger::dropTrigger4DropTable( qcStatement  * aStatement,
                                   qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *    해당 Table ID를 포함한 Trigger를 제거한다.
 *       - 관련 Trigger Object를 제거한 후,
 *       - 관련 Meta Table에서 정보를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::dropTrigger4DropTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                  i;
    qcmTriggerInfo *      sTriggerInfo;

    SChar               * sBuffer;
    vSLong                 sRowCnt;

    //-------------------------------------------
    // 적합성 검사
    //-------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableInfo != NULL );

    //-------------------------------------------
    // Trigger Object 및 Object Cache를 삭제
    //-------------------------------------------

    for ( i = 0, sTriggerInfo = aTableInfo->triggerInfo;
          i < aTableInfo->triggerCount;
          i++ )
    {
        // Trigger Object 삭제
        IDE_TEST( dropTriggerObject( aStatement,
                                     sTriggerInfo[i].triggerHandle )
                  != IDE_SUCCESS );
    }

    if ( aTableInfo->triggerCount > 0 )
    {
        //---------------------------------------
        // String을 위한 공간 획득
        //---------------------------------------

        IDU_LIMITPOINT("qdnTrigger::dropTrigger4DropTable::malloc");
        IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                             (void**) & sBuffer )
                  != IDE_SUCCESS );

        //---------------------------------------
        // SYS_TRIGGERS_ 에서 정보 삭제
        // DELETE FROM SYS_TRIGGERS_ WHERE TABLE_ID = aTableID;
        //---------------------------------------

        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_TRIGGERS_ "
                         "WHERE TABLE_ID = "QCM_SQL_INT32_FMT,  // 0. TABLE_ID
                         aTableInfo->tableID );                 // 0.

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        //---------------------------------------
        // SYS_TRIGGER_STRINGS_ 에서 정보 삭제
        // DELETE FROM SYS_TRIGGER_STRINGS_ WHERE TABLE_ID = aTableID;
        //---------------------------------------

        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_TRIGGER_STRINGS_ "
                         "WHERE TABLE_ID = "QCM_SQL_INT32_FMT,  // 0. TABLE_ID
                         aTableInfo->tableID );                 // 0.

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_DASSERT( sRowCnt > 0 );

        //---------------------------------------
        // SYS_TRIGGER_UPDATE_COLUMNS_ 에서 정보 삭제
        //---------------------------------------

        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_TRIGGER_UPDATE_COLUMNS_ "
                         "WHERE TABLE_ID = "QCM_SQL_INT32_FMT,  // 0. TABLE_ID
                         aTableInfo->tableID );                // 0.

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        //---------------------------------------
        // SYS_TRIGGER_DML_TABLES 에 정보 삽입
        //---------------------------------------

        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_TRIGGER_DML_TABLES_ "
                         "WHERE TABLE_ID = "QCM_SQL_INT32_FMT, // 0. TABLE_ID
                         aTableInfo->tableID );                // 0.

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Table에 존재하는 Trigger가 없음
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdnTrigger::addGranularityInfo( qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    Trigger의 Action Granularity가 FOR EACH ROW인 경우
 *    REFERENCING 정보 및 WHEN 정보가 존재할 수 있다.
 *
 *    Ex) CREATE TRIGGER ...
 *        AFTER UPDATE ON t1
 *        REFERENCING OLD ROW old_row, NEW_ROW new_row
 *        FOR EACH ROW WHEN ( old_row.i1 > new_row.i1 )
 *        ...;
 *
 *        위와 같은 구문에서 WHEN 절을 처리하기 위해서는
 *        new_row 정보, old_row 정보등을 쉽게 참조할 수 있어야 한다.
 *
 * Implementation :
 *        // PROJ-1502 PARTITIONED DISK TABLE
 *        이 함수를 호출할 때는 반드시 row granularity가 있어야 하는 상태이다.
 ***********************************************************************/

#define IDE_FN "qdnTrigger::addGranularityInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //----------------------------------------
    // 적합성 검사
    //----------------------------------------

    IDE_DASSERT( aParseTree != NULL );

    IDE_DASSERT( aParseTree->actionCond.actionGranularity
                 == QCM_TRIGGER_ACTION_EACH_ROW );

    if ( ( aParseTree->triggerReference != NULL ) &&
         ( aParseTree->actionCond.whenCondition != NULL ) )
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        // Nothing to do.
    }
    else
    {
        // reference는 없는데 whenCondition이 있는 경우.
        // 에러.
        IDE_TEST_RAISE( aParseTree->actionCond.whenCondition != NULL,
                        err_when_condition );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_when_condition);
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_TRIGGER_WHEN_REFERENCING ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdnTrigger::addActionBodyInfo( qcStatement               * aStatement,
                               qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *     FOR EACH ROW granularity에서 Action Body를 위하여
 *     REFERENCING row 및 Current Row의 정보를 추가해 주어야 한다.
 *     이는 PSM Body인 Action Body에서 REFERENCING 및 Subject Table에 대한
 *     접근 여부를 판단할 수 없기 때문이다.
 *
 *     이를 위해 다음과 같은 개념으로 Action Body의 Variable 정보를 설정한다.
 *     Ex)  CREATE TRIGGER ...
 *          ON t1 REFERENCING OLD old_row, NEW new_row
 *          FOR EACH ROW
 *          AS BEGIN
 *             INSERT INTO log_table VALUES ( old_row.i1, new_row.i2 );
 *          END;
 *
 *          ==>
 *
 *          ...
 *          AS
 *             old_row t1%ROWTYPE;
 *             new_row t1%ROWTYPE;
 *          BEGIN
 *             INSERT INTO log_table VALUES ( old_row.i2, new_row.i3 );
 *          END;
 *
 *          즉, 위와 같이 개념적으로 ROWTYPE의 Variable을 추가하여
 *          BEGIN ... END 까지의 Body에서의 Validation 및 Execution이
 *          가능하게 된다.
 *
 * Implementation :
 *
 *     REFERENCING 구문에 존재하는 row에 대한 ROWTYPE Variable 생성
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::addActionBodyInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnTriggerRef * sRef;
    qsVariables   * sCurVar;
    qtcNode       * sTypeNode[2];

    //----------------------------------------
    // 적합성 검사
    //----------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    IDE_DASSERT( aParseTree->actionCond.actionGranularity
                 == QCM_TRIGGER_ACTION_EACH_ROW );

    //----------------------------------------
    // REFERENCING 구문에 존재하는 row에 대한 ROWTYPE Variable 생성
    //----------------------------------------
    aParseTree->actionBody.paraDecls = NULL;

    for ( sRef = aParseTree->triggerReference;
          sRef != NULL;
          sRef = sRef->next )
    {
        // REFERENCING Row를 위한 ROWTYPE Variable의 생성
        // Ex) ... ON user_name.table_name REFERENCING OLD ROW AS old_row
        //    => old_row  user_name.table_name%ROWTYPE;
        // 으로 생성

        //---------------------------------------------------
        // Type 부분의 정보 생성
        //---------------------------------------------------

        // old_row user_name.[table_name%ROWTYPE] 을 위한 Node 생성
        IDE_TEST( qtc::makeProcVariable( aStatement,
                                         sTypeNode,
                                         & aParseTree->tableNamePos,
                                         NULL,
                                         QTC_PROC_VAR_OP_NONE )
                  != IDE_SUCCESS );

        // old_row [user_name].table_name%ROWTYPE 에서 user_name 정보의 설정
        idlOS::memcpy( & sTypeNode[0]->tableName,
                       & aParseTree->userNamePos,
                       ID_SIZEOF(qcNamePosition) );

        //---------------------------------------------------
        // ROWTYPE Variable 정보의 완성
        //---------------------------------------------------

        IDU_LIMITPOINT("qdnTrigger::addActionBodyInfo::malloc");
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qsVariables),
                                                 (void**) & sCurVar )
                  != IDE_SUCCESS );

        sCurVar->variableType     = QS_ROW_TYPE;
        sCurVar->variableTypeNode = sTypeNode[0];
        sCurVar->defaultValueNode = NULL;

        // To fix BUG-12622
        // BEFORE TRIGGER에서 new row는 변경이 가능하다.
        if( sRef->refType == QCM_TRIGGER_REF_NEW_ROW )
        {
            sCurVar->common.itemType = QS_TRIGGER_NEW_VARIABLE;

            if( aParseTree->triggerEvent.eventTime == QCM_TRIGGER_BEFORE )
            {
                sCurVar->inOutType = QS_INOUT;
            }
            else
            {
                sCurVar->inOutType = QS_IN;
            }
        }
        else
        {
            sCurVar->inOutType = QS_IN;
            sCurVar->common.itemType = QS_TRIGGER_OLD_VARIABLE;
        }

        sCurVar->nocopyType      = QS_COPY;
        sCurVar->typeInfo        = NULL;

        sCurVar->common.table    = sCurVar->variableTypeNode->node.table;
        sCurVar->common.column   = sCurVar->variableTypeNode->node.column;

        // [old_row] user_name.table_name%ROWTYPE
        idlOS::memcpy( & sCurVar->common.name,
                       & sRef->aliasNamePos,
                       ID_SIZEOF( qcNamePosition ) );

        // REFERENCING Row 와 Action Body내의 Variable의 연결
        //sRef->rowVarTableID = sCurVar->common.table;
        //sRef->rowVarColumnID = sCurVar->common.column;

        // PROJ-1075 table, column으로 찾을수없음.
        sRef->rowVar = sCurVar;

        //---------------------------------------------------
        // Variable의 연결
        // PROJ-1502 PARTITIONED DISK TABLE
        // parameter에 연결한다.
        //---------------------------------------------------

        sCurVar->common.next = aParseTree->actionBody.paraDecls;
        aParseTree->actionBody.paraDecls =
            (qsVariableItems*) sCurVar;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::setTriggerUser( qcStatement               * aStatement,
                            qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : BUG-24570
 *    CREATE TRIGGER의 User에 대한 정보 설정
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::setTriggerUser"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar   sUserName[QC_MAX_OBJECT_NAME_LEN + 1];

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //---------------------------------------
    // Trigger User 정보의 획득
    //---------------------------------------

    if ( QC_IS_NULL_NAME( aParseTree->triggerUserPos ) == ID_TRUE)
    {
        // User ID 획득
        if ( aParseTree->isRecompile == ID_FALSE )
        {
            aParseTree->triggerUserID = QCG_GET_SESSION_USER_ID(aStatement);
        }
        else
        {
            // 이미 설정했음

            // Nothing to do.
        }

        // User Name 설정
        IDE_TEST( qcmUser::getUserName( aStatement,
                                        aParseTree->triggerUserID,
                                        sUserName )
                  != IDE_SUCCESS );
        // BUG-25419 [CodeSonar] Format String 사용 Warning
        idlOS::snprintf( aParseTree->triggerUserName, QC_MAX_OBJECT_NAME_LEN + 1, "%s", sUserName );
        aParseTree->triggerUserName[QC_MAX_OBJECT_NAME_LEN] = '\0';

    }
    else
    {
        // User ID 획득
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      aParseTree->triggerUserPos,
                                      & aParseTree->triggerUserID )
                  != IDE_SUCCESS);

        // User Name 설정
        QC_STR_COPY( aParseTree->triggerUserName, aParseTree->triggerUserPos );
    }

    // BUG-42322
    IDE_TEST( qsv::setObjectNameInfo( aStatement,
                                      aParseTree->actionBody.objType,
                                      aParseTree->triggerUserID,
                                      aParseTree->triggerNamePos,
                                      &aParseTree->actionBody.objectNameInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::valPrivilege( qcStatement               * aStatement,
                          qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER의 권한 검사
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valPrivilege"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //---------------------------------------
    // Trigger 생성 권한 검사
    //---------------------------------------

    // BUG-24570
    // create, replace, alter시에만 session user로 trigger 생성 권한 검사
    if ( aParseTree->isRecompile == ID_FALSE )
    {
        // check grant
        IDE_TEST( qdpRole::checkDDLCreateTriggerPriv( aStatement,
                                                      aParseTree->triggerUserID )
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
qdnTrigger::valTriggerName( qcStatement               * aStatement,
                            qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Trigger Name의 유효성 검사.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valTriggerName"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcuSqlSourceInfo     sqlInfo;

    UInt                 sTableID;
    idBool               sExist;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    // 이미 존재하는 Trigger인지 검사
    IDE_TEST( qcmTrigger::getTriggerOID( aStatement,
                                         aParseTree->triggerUserID,
                                         aParseTree->triggerNamePos,
                                         & (aParseTree->triggerOID),
                                         & sTableID,
                                         & sExist )
              != IDE_SUCCESS );

    if ( sExist == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aParseTree->triggerUserPos,
                               & aParseTree->triggerNamePos );
        IDE_RAISE( err_duplicate_trigger_name );
    }
    else
    {
        // 해당 Trigger가 존재하지 않음
    }
    aParseTree->triggerOID = 0;

    // Trigger Name의 설정
    QC_STR_COPY( aParseTree->triggerName, aParseTree->triggerNamePos );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_duplicate_trigger_name)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_TRIGGER_DUPLICATED_NAME,
                                  sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::reValTriggerName( qcStatement               * aStatement,
                              qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    Recompile을 위한 Trigger Name의 유효성 검사.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::reValTriggerName"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt                 sTableID;
    idBool               sExist;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    // 이미 존재하는 Trigger인지 검사
    IDE_TEST( qcmTrigger::getTriggerOID( aStatement,
                                         aParseTree->triggerUserID,
                                         aParseTree->triggerNamePos,
                                         & (aParseTree->triggerOID),
                                         & sTableID,
                                         & sExist )
              != IDE_SUCCESS );

    IDE_DASSERT( sExist == ID_TRUE );

    // Trigger Name의 설정
    QC_STR_COPY( aParseTree->triggerName, aParseTree->triggerNamePos );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdnTrigger::valTableName( qcStatement               * aStatement,
                          qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Table Name의 유효성 검사.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valTableName"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcuSqlSourceInfo     sqlInfo;
    
    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    // BUG-24408
    // triggerUserName을 명시한 경우 on절의 tableUserName도 명시해야 한다.
    if ( ( QC_IS_NULL_NAME( aParseTree->triggerUserPos ) != ID_TRUE ) &&
         ( QC_IS_NULL_NAME( aParseTree->userNamePos ) == ID_TRUE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aParseTree->userNamePos,
                               & aParseTree->tableNamePos );
        IDE_RAISE( err_invalid_table_owner_name );
    }
    else
    {
        // Nothing to do.
    }

    // Table이 존재하여야 한다.
    // Table 정보 획득 및 Table에 대한 IS Lock 획득
    IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                         aParseTree->userNamePos,
                                         aParseTree->tableNamePos,
                                         & aParseTree->tableUserID,
                                         & aParseTree->tableInfo,
                                         & aParseTree->tableHandle,
                                         & aParseTree->tableSCN)
              != IDE_SUCCESS);
    
    IDE_TEST(qcm::lockTableForDDLValidation(aStatement,
                                            aParseTree->tableHandle,
                                            aParseTree->tableSCN)
             != IDE_SUCCESS);
    
    aParseTree->tableID = aParseTree->tableInfo->tableID;
    aParseTree->tableOID = aParseTree->tableInfo->tableOID;

    if (aParseTree->isRecompile != ID_TRUE)
    {
        // To Fix PR-10618
        // Table Owner에 대한 Validation
        IDE_TEST( qdpRole::checkDDLCreateTriggerTablePriv(
                      aStatement,
                      aParseTree->tableInfo->tableOwnerID )
                  != IDE_SUCCESS );
    }
    else
    {
        /*
         * BUG-34422: create trigger privilege should not be checked
         *            when recompling trigger
         */
    }

    // To Fix PR-10708
    // 일반 Table인 경우에만 Trigger를 생성할 수 있음.
    /* PROJ-1888 INSTEAD OF TRIGGER (view 에 트리거를 생성할 수 있음 ) */
    if ( ( aParseTree->tableInfo->tableType != QCM_USER_TABLE )  &&
         ( aParseTree->tableInfo->tableType != QCM_VIEW )        &&
         ( aParseTree->tableInfo->tableType != QCM_MVIEW_TABLE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aParseTree->userNamePos,
                               & aParseTree->tableNamePos );
        IDE_RAISE(err_invalid_object);
    }

    // PROJ-1567
    /* if( aParseTree->isRecompile == ID_FALSE )
    {
        // To fix BUG-14584
        // CREATE or REPLACE인 경우.
        // Replication이 걸려 있는 테이블에 대해서는 DDL을 발생시킬 수 없음
        IDE_TEST_RAISE( aParseTree->tableInfo->replicationCount > 0,
                        ERR_DDL_WITH_REPLICATED_TABLE );
    }
    else
    {
        // recompile인 경우.
        // Nothing to do.
    } */

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_object);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(  qpERR_ABORT_TRIGGER_INVALID_TABLE_NAME,
                              sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(err_invalid_table_owner_name);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_TRIGGER_INVALID_TABLE_OWNER_NAME,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

//To FIX BUG-20948
//--------------------------------------------------------------
// Trigger Replace 시 Target Table이 변경되었을 경우, Original Table
// 정보 획득 및 Lock
//--------------------------------------------------------------
IDE_RC
qdnTrigger::valOrgTableName( qcStatement               * aStatement,
                             qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * BUG-20948
 * Original Table에 대하여 처리한다.
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valTableName"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    // Table 정보 획득 및 Table에 대한 IS Lock 획득

    IDE_TEST( qcm::getTableHandleByID(  QC_SMI_STMT( aStatement ),
                                        aParseTree->orgTableID,
                                        &aParseTree->orgTableHandle,
                                        &aParseTree->orgTableSCN )
              != IDE_SUCCESS);

    IDE_TEST( smiGetTableTempInfo( aParseTree->orgTableHandle,
                                   (void**)&aParseTree->orgTableInfo )
              != IDE_SUCCESS );

    IDE_TEST(qcm::lockTableForDDLValidation(aStatement,
                                            aParseTree->orgTableHandle,
                                            aParseTree->orgTableSCN)
             != IDE_SUCCESS);

    aParseTree->orgTableOID = aParseTree->orgTableInfo->tableOID;

    // To Fix PR-10618
    // Table Owner에 대한 Validation
    IDE_TEST( qdpRole::checkDDLCreateTriggerTablePriv(
                  aStatement,
                  aParseTree->orgTableInfo->tableOwnerID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::valEventReference( qcStatement               * aStatement,
                               qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Trigger Event 의 유효성 검사.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valTriggerEvent"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcmColumn          * sColumn;
    qcmColumn          * sColumnInfo;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //----------------------------------------------
    // Trigger Event 에 대한 Validation
    //----------------------------------------------

    // Post-Parsing 단계에서 이미 검증된 내용.
    // - BEFORE Trigger Event는 지원하지 않음.

    // UPDATE OF i1, i2 ON t1과 같이 Column이 존재하는 경우에 대한 Validation
    for ( sColumn = aParseTree->triggerEvent.eventTypeList->updateColumns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        // 해당 Column이 존재하는 지를 검사
        IDE_TEST(qcmCache::getColumn( aStatement,
                                      aParseTree->tableInfo,
                                      sColumn->namePos,
                                      & sColumnInfo) != IDE_SUCCESS);

        sColumn->basicInfo = sColumnInfo->basicInfo;
    }

    //----------------------------------------------
    // Trigger Referencing 에 대한 Validation
    //----------------------------------------

    // Post-Parsing 단계에서 이미 검증된 내용.
    // - REFERENCING은 FOR EACH ROW Granularity에서만 사용 가능함.
    // - TABLE REFERENCING은 지원하지 않음.

    // DML EVENT와 부합되지 않는 REFERENCING를 검사한다.
    if (aParseTree->triggerEvent.eventTypeList->eventType
        == QCM_TRIGGER_EVENT_NONE)
    {
        IDE_DASSERT(0);
        IDE_RAISE( err_invalid_event_referencing );
    }

    if( aParseTree->actionBody.paraDecls != NULL )
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        // old row, new row는 이제 parameter이므로,
        // parameter의 validation을 통해 생성을 해야 한다.
        IDE_TEST( qsvProcVar::validateParaDef(
                      aStatement,
                      aParseTree->actionBody.paraDecls )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_event_referencing)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_TRIGGER_INVALID_REFERENCING));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::valActionCondition( qcStatement               * aStatement,
                                qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Action Condition에 대한 Validation.
 *
 *    Row Action Granularity 일 경우 생성된 부가 정보에 대하여 Validation한다.
 *    WHEN Condition의 유효성 검사.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valActionCondition"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));


    qcuSqlSourceInfo      sqlInfo;
    mtcColumn           * sColumn;
    qtcNode             * sWhenCondition;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    // fix BUG-32837
    aStatement->spvEnv->allParaDecls = aParseTree->actionBody.paraDecls;

    //----------------------------------------
    // Row Granularity 정보의 Validation
    //----------------------------------------

    if ( ( aParseTree->triggerReference != NULL ) &&
         ( aParseTree->actionCond.whenCondition != NULL ) )
    {
        sWhenCondition = aParseTree->actionCond.whenCondition;

        // when condition의 체크.
        // estimate를 한 다음, 반드시 boolean타입이 리턴되어야 한다.
        // subquery는 존재해서는 안된다.
        IDE_TEST( qtc::estimate( sWhenCondition,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 NULL,
                                 NULL,
                                 NULL )
                  != IDE_SUCCESS );

        sColumn = &(QC_SHARED_TMPLATE(aStatement)->tmplate.
                                        rows[sWhenCondition->node.table].
                                        columns[sWhenCondition->node.column]);

        if ( sColumn->module != &mtdBoolean )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sWhenCondition->position );
            IDE_RAISE(ERR_INSUFFICIENT_ARGUEMNT);
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST_RAISE( qsv::checkNoSubquery( aStatement,
                                              sWhenCondition,
                                              & sqlInfo )
                        != IDE_SUCCESS,
                        err_when_subquery );

        // BUG-38137 Trigger의 when condition에서 PSM을 호출할 수 없다.
        if ( checkNoSpFunctionCall( sWhenCondition )
            != IDE_SUCCESS )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sWhenCondition->position );
            IDE_RAISE( err_invalid_psm_in_trigger );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Statement Granularity trigger임.
        // Nothing to do.
    }

    // fix BUG-32837
    aStatement->spvEnv->allParaDecls = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_when_subquery);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_TRIGGER_WHEN_SUBQUERY,
                                sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INSUFFICIENT_ARGUEMNT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INSUFFICIENT_ARGUEMNT_SQLTEXT,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(err_invalid_psm_in_trigger);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_TRIGGER_PSM,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qdnTrigger::valActionBody( qcStatement               * aStatement,
                           qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    CREATE TRIGGER를 위한 Action Body에 대한 Validation.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::valActionBody"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //----------------------------------------
    // Action Body 구문에 대한 Validation을
    // TRIGGER 를 위한 Mode로 수행한다.
    //----------------------------------------

    // row, column 타입의 변수를 리스트에 추가하기 위해서는
    // aStatement->myPlan->parseTree는 qsProcParseTree가 되어야 한다.
    aStatement->myPlan->parseTree = (qcParseTree *)&aParseTree->actionBody;

    // stmtKind를 변경하는 이유는 테이블에 대한 IS lock을 얻기 위함이다.
    aStatement->myPlan->parseTree->stmtKind = QCI_STMT_MASK_DML;

    //----------------------------------------
    // Action Body의 Post Parsing
    // DML의 경우 post parsing시 validation을 수행한다.
    //----------------------------------------

    IDE_TEST( aParseTree->actionBody.block->common.
              parse( aStatement,
                     (qsProcStmts *) (aParseTree->actionBody.block) )
              != IDE_SUCCESS);

    //----------------------------------------
    // Action Body의 Validation
    //----------------------------------------

    IDE_TEST( aParseTree->actionBody.block->common.
              validate( aStatement,
                        (qsProcStmts *) (aParseTree->actionBody.block),
                        NULL,
                        QS_PURPOSE_TRIGGER )
              != IDE_SUCCESS);

    // validation 이후에는 이전 parseTree로 복구한다.
    aStatement->myPlan->parseTree = (qcParseTree *)aParseTree;

    //----------------------------------------
    // Trigger Cycle Detection
    //----------------------------------------

    IDE_TEST( checkCycle( aStatement, aParseTree ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // validation 이후에는 이전 parseTree로 복구한다.
    aStatement->myPlan->parseTree = (qcParseTree *)aParseTree;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::valInsteadOfTrigger( qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    INSTEAD OF TRIGGER의 CREATE, REPLACE 를 위한 제약사 Validation.
 *
 * Implementation :
 *
 ***********************************************************************/

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aParseTree != NULL );

    // INSTEAD OF 는 반드시 view
    // INSTEAD OF 를 일반 테이블에 걸수 없다.
    IDE_TEST_RAISE(
          ( aParseTree->triggerEvent.eventTime ==
            QCM_TRIGGER_INSTEAD_OF ) &&
          ( aParseTree->tableInfo->tableType !=
            QCM_VIEW ),
        ERR_INSTEAD_OF_NOT_VIEW );

    //VIEW 테이블에 INSTEAD OF외에 다른 트리거를 생성할수 없다.
    IDE_TEST_RAISE(
          ( aParseTree->triggerEvent.eventTime !=
            QCM_TRIGGER_INSTEAD_OF ) &&
          ( aParseTree->tableInfo->tableType ==
            QCM_VIEW ),
        ERR_BEFORE_AFTER_VIEW );

    // not support INSTEAD OF ~ FOR EACH STATEMENT
    IDE_TEST_RAISE(
          ( aParseTree->triggerEvent.eventTime ==
            QCM_TRIGGER_INSTEAD_OF ) &&
          ( aParseTree->actionCond.actionGranularity ==
            QCM_TRIGGER_ACTION_EACH_STMT ),
        ERR_UNSUPPORTED_INSTEAD_OF_EACH_STMT );

    // not support INSTEAD OF ~ WHEN condition
    IDE_TEST_RAISE(
          ( aParseTree->triggerEvent.eventTime ==
            QCM_TRIGGER_INSTEAD_OF ) &&
          ( aParseTree->actionCond.whenCondition !=
            NULL ),
        ERR_UNSUPPORTED_INSTEAD_OF_WHEN_CONDITION );

    // not support INSTEAD OF UPDATE OF
    IDE_TEST_RAISE(
          ( aParseTree->triggerEvent.eventTime ==
            QCM_TRIGGER_INSTEAD_OF ) &&
          ((aParseTree->triggerEvent.eventTypeList->eventType &
            QCM_TRIGGER_EVENT_UPDATE) != 0) &&
          ( aParseTree->triggerEvent.eventTypeList->updateColumns !=
            NULL ),
        ERR_UNSUPPORTED_INSTEAD_OF_UPDATE_OF );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSTEAD_OF_NOT_VIEW );
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_INSTEAD_OF_NOT_VIEW ) );
    }
    IDE_EXCEPTION( ERR_BEFORE_AFTER_VIEW );
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_BEFORE_AFTER_VIEW ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_INSTEAD_OF_EACH_STMT );
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_UNSUPPORTED_INSTEAD_OF_EACH_STMT ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_INSTEAD_OF_WHEN_CONDITION );
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_UNSUPPORTED_INSTEAD_OF_WHEN_COND ) );
    }
    IDE_EXCEPTION(ERR_UNSUPPORTED_INSTEAD_OF_UPDATE_OF);
    {
        IDE_SET( ideSetErrorCode(
                     qpERR_ABORT_TRIGGER_UNSUPPORTED_INSTEAD_OF_COLUMN_LIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qdnTrigger::checkCycle( qcStatement               * aStatement,
                        qdnCreateTriggerParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    Trigger 생성 시 Cycle에 대한 검사를 수행한다.
 *
 *    DML --> [T1] --> [Tr1]--DML--> [T2] --> [TrA] --> [T1]
 *             ^                     [T3]               ^^^^
 *             |                                          |
 *             |                                          |
 *             +------------------------------------------+
 *
 *    위의 그림과 같이 Cycle은 여러 단계에 걸쳐 존재할 수 있다.
 *    단순히 Cache 정보를 이용하여 Cycle을 Detect할 경우,
 *    IPR 등으로 인해 서버가 사망할 수 있으므로 동시성 제어를
 *    명확하게 고려하여야 한다.
 *
 *    Meta Cache에의 연속적 접근은 동시성 제어를 필요로 하게 되므로,
 *    Action Body가 접근하는 Table 정보를 기록하고 있는
 *    SYS_TRIGGER_DML_TABLES_ 를 이용하여 검사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::checkCycle"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsModifiedTable     * sCheckTable;
    qsModifiedTable     * sSameTable;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //---------------------------------------
    // Action Body가 DML로 참조하는 Table중
    // 자신의 Table에 접근하는 Table이 존재하는 지를 검사
    //---------------------------------------

    for ( sCheckTable = aStatement->spvEnv->modifiedTableList;
          sCheckTable != NULL;
          sCheckTable = sCheckTable->next )
    {
        IDE_TEST_RAISE( aParseTree->tableID == sCheckTable->tableID,
                        err_cycle_detected );
    }

    //---------------------------------------
    // Action Body가 DML로 참조되는 Table로부터 발생되는
    // Cycle이 존재하는 지 검사
    // 이미 검사한 Table은 다시 검사하지 않는다.
    //---------------------------------------

    for ( sCheckTable = aStatement->spvEnv->modifiedTableList;
          sCheckTable != NULL;
          sCheckTable = sCheckTable->next )
    {
        for ( sSameTable = aStatement->spvEnv->modifiedTableList;
              (sSameTable != NULL) && (sSameTable != sCheckTable);
              sSameTable = sSameTable->next )
        {
            if ( sSameTable->tableID == sCheckTable->tableID )
            {
                // 동일한 Table이 이미 검사되었음.
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sSameTable == sCheckTable )
        {
            // 아직 검사가 되지 않은 Table의 경우
            // 관련 Table로부터 Cycle이 존재하는 지를 검사한다.
            IDE_TEST( qcmTrigger::checkTriggerCycle ( aStatement,
                                                      sCheckTable->tableID,
                                                      aParseTree->tableID )
                      != IDE_SUCCESS );
        }
        else
        {
            // 이미 검사가 완료된 Table인 경우
            // 중복 검사할 필요가 없다.
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cycle_detected);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_TRIGGER_CYCLE_DETECTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::createTriggerObject( qcStatement     * aStatement,
                                 void           ** aTriggerHandle )
{
/***********************************************************************
 *
 * Description :
 *    Trigger 생성 구문을 저장하는 Object를 생성하고
 *    해당 Handle을 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::createTriggerObject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTriggerHandle != NULL );

    //---------------------------------------
    // Trigger Object의 생성
    //---------------------------------------

    IDE_TEST( smiObject::createObject( QC_SMI_STMT( aStatement ),
                                       aStatement->myPlan->stmtText,
                                       aStatement->myPlan->stmtTextLen,
                                       NULL,
                                       SMI_OBJECT_TRIGGER,
                                       (const void **) aTriggerHandle )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdnTrigger::allocTriggerCache( void             * aTriggerHandle,
                                      smOID              aTriggerOID,
                                      qdnTriggerCache ** aCache )
{
/***********************************************************************
 *
 * Description :
 *
 *    Trigger Object를 위한 Cache 정보를 할당받는다.
 *
 *     [Trigger Handle] -- info ----> CREATE TRIGGER String
 *                      |
 *                      -- tempInfo ----> Trigger Object Cache
 *                                        ^^^^^^^^^^^^^^^^^^^^
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::allocTriggerCache"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnTriggerCache * sTriggerCache;
    SChar             sLatchName[IDU_MUTEX_NAME_LEN];
    UInt              sStage = 0;

    //---------------------------------------
    // Permanant Memory 영역을 할당받는다.
    //---------------------------------------

    IDU_LIMITPOINT("qdnTrigger::allocTriggerCache::malloc1");
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_QSX,
                                ID_SIZEOF(qdnTriggerCache),
                                (void**) & sTriggerCache )
             != IDE_SUCCESS);

    sStage = 1;

    //---------------------------------------
    // Statement Text를 저장하기 위한 공간 할당
    // remote 경우는 원격의 stmt text, len을 얻어 온다.
    //---------------------------------------

    smiObject::getObjectInfoSize( aTriggerHandle,
                                  & sTriggerCache->stmtTextLen );

    IDU_LIMITPOINT("qdnTrigger::allocTriggerCache::malloc2");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QSX,
                                 sTriggerCache->stmtTextLen+1,
                                 (void**) & sTriggerCache->stmtText )
              != IDE_SUCCESS );

    smiObject::getObjectInfo( aTriggerHandle,
                              (void**) & sTriggerCache->stmtText );

    IDE_DASSERT(sTriggerCache->stmtText != NULL);

    // BUG-40483 valgrind warning
    // string null termination
    sTriggerCache->stmtText[sTriggerCache->stmtTextLen] = 0;

    sStage = 2;

    //---------------------------------------
    // Trigger Cache정보를 초기화한다.
    // remote 경우는 smiGetTableId로 triggerOID 얻을 수 없다.
    // 이미 triggerInfo에 원격의 triggerOID가 존재 한다.
    //---------------------------------------

    // bug-29598
    idlOS::snprintf( sLatchName,
                     IDU_MUTEX_NAME_LEN,
                     "TRIGGER_%"ID_vULONG_FMT"_OBJECT_LATCH",
                     aTriggerOID );

    // Latch 정보의 초기화
    IDE_TEST( sTriggerCache->latch.initialize( sLatchName )
              != IDE_SUCCESS );
    sStage = 3;

    // Valid 여부의 초기화
    sTriggerCache->isValid = ID_FALSE;

    //---------------------------------------
    // Trigger Statement의 초기화
    //---------------------------------------

    // 실제 Stack Count는 수행 시점에 Template 복사에 의하여
    // Session 정보로부터 결정된다.
    IDE_TEST( qcg::allocStatement( & sTriggerCache->triggerStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sStage = 4;

    /* BUG-44563
       trigger 생성 후 server stop하면 비정상 종료하는 경우가 발생합니다. */
    IDE_TEST( qcg::freeSession( & sTriggerCache->triggerStatement ) != IDE_SUCCESS );

    //--------------------------------------
    // Handle에 Trigger Object Cache 정보의 연결
    // triggerInfo->handle 원격의 정보이기 때문에 remote인 경우는
    // handle에 연결하지 않고 hash table에서 얻어와서 사용한다.
    //--------------------------------------

    (void)smiObject::setObjectTempInfo( aTriggerHandle, (void*) sTriggerCache );

    *aCache = sTriggerCache;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 4:
            (void) qcg::freeStatement( & sTriggerCache->triggerStatement );
        case 3:
            (void) sTriggerCache->latch.destroy();
        case 2:
            (void) iduMemMgr::free( sTriggerCache->stmtText );
            sTriggerCache->stmtText = NULL;
        case 1:
            (void) iduMemMgr::free( sTriggerCache );
            sTriggerCache = NULL;
        case 0:
            // Nothing To Do
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::dropTriggerObject( qcStatement     * aStatement,
                               void            * aTriggerHandle )
{
/***********************************************************************
 *
 * Description :
 *    Trigger Object를 삭제한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::dropTriggerObject"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTriggerHandle != NULL );

    //---------------------------------------
    // Trigger Object의 삭제
    //---------------------------------------

    IDE_TEST( smiObject::dropObject( QC_SMI_STMT( aStatement ),
                                     aTriggerHandle )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::freeTriggerCache( qdnTriggerCache * aCache )
{
/***********************************************************************
 *
 * Description :
 *    Trigger Handle에 지정된  Trigger Object Cache를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::freeTriggerCache"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------------
    // Trigger Cache의 Statement 공간의 제거
    //---------------------------------------

    if ( qcg::freeStatement( & aCache->triggerStatement ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_QP_1 );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------
    // Trigger Cache의 Text 공간의 제거
    //---------------------------------------

    // To Fix BUG-12034
    // 하나의 qdnTriggerCache에 대해 freeTriggerCache가 두번씩 호출되면 오류!
    IDE_DASSERT( aCache->stmtText != NULL );

    if ( iduMemMgr::free( aCache->stmtText ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_QP_1 );
    }
    else
    {
        // Nothing to do.
    }

    aCache->stmtText = NULL;

    //---------------------------------------
    // Latch 제거
    //---------------------------------------

    if ( aCache->latch.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_QP_1 );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------
    // Trigger Cache의 제거
    //---------------------------------------

    if ( iduMemMgr::free( aCache ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_QP_1 );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

void qdnTrigger::freeTriggerCacheArray( qdnTriggerCache ** aTriggerCaches,
                                        UInt               aTriggerCount )
{
    UInt i = 0;

    if ( aTriggerCaches != NULL )
    {
        for ( i = 0; i < aTriggerCount; i++ )
        {
            if ( aTriggerCaches[i] != NULL )
            {
                (void)freeTriggerCache( aTriggerCaches[i] );
                aTriggerCaches[i] = NULL;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

void qdnTrigger::restoreTempInfo( qdnTriggerCache ** aTriggerCaches,
                                  qcmTableInfo     * aTableInfo )
{
    UInt i = 0;

    if ( ( aTriggerCaches != NULL ) && ( aTableInfo != NULL ) )
    {
        for ( i = 0; i < aTableInfo->triggerCount; i++ )
        {
            if ( aTriggerCaches[i] != NULL )
            {
                (void)smiObject::setObjectTempInfo( aTableInfo->triggerInfo[i].triggerHandle,
                                                    aTriggerCaches[i] );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

IDE_RC
qdnTrigger::checkCondition( qcStatement         * aStatement,
                            qcmTriggerInfo      * aTriggerInfo,
                            qcmTriggerGranularity aGranularity,
                            qcmTriggerEventTime   aEventTime,
                            UInt                  aEventType,
                            smiColumnList       * aUptColumn,
                            idBool              * aNeedAction,
                            idBool              * aIsRecompiled )
{
/***********************************************************************
 *
 * Description :
 *    해당 Trigger가 수행해야 하는 Trigger인지 검사한다.
 *
 * Implementation :
 *
 *    Trigger 조건의 검사
 *       - ENABLE 여부
 *       - 동일한 Granularity 여부(ROW/STMT)
 *       - 동일한 Event Time 여부(BEFORE/AFTER)
 *       - 동일한 Event Type 여부(INSERT, DELETE, UPDATE)
 *           : UPDATE일 경우 OF의 검사도 필요
 *
 *       - WHEN Condition이 있을 경우
 *           : 실제 수행시에 검사한다.
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::checkCondition"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool                       sNeedAction;
    smiColumnList              * sColumn;
    qcStatement                * sTriggerStatement;
    qdnCreateTriggerParseTree  * sTriggerParseTree;
    qdnTriggerCache            * sTriggerCache;
    qdnTriggerEventTypeList    * sEventTypeList;
    qcmColumn                  * sUptColumn;
    UInt                         sStage = 0;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aTriggerInfo  != NULL );
    IDE_DASSERT( aNeedAction   != NULL );
    IDE_DASSERT( aIsRecompiled != NULL );

    //---------------------------------------
    // Event 조건의 검사
    //---------------------------------------

    sNeedAction = ID_FALSE;

    /* BUG-44819 runDMLforDDL, runDMLforInternal 로 동작하는 SQL 의 경우 
     *           trigger 가 동작하면 안됩니다 */
    if ( ( aStatement->session->mMmSession != NULL ) &&
         ( aTriggerInfo->enable == QCM_TRIGGER_ENABLE ) &&
         ( aTriggerInfo->granularity == aGranularity ) &&
         ( aTriggerInfo->eventTime == aEventTime ) &&
         ( (aTriggerInfo->eventType & aEventType) != 0 ) )
    {
        // 모든 Event 조건이 부합하는 경우
        if ( aEventType == QCM_TRIGGER_EVENT_UPDATE )
        {
            //-----------------------------------------
            // UPDATE Event인 경우 OF 구문도 검사해야 함.
            //-----------------------------------------

            // BUG-16543
            // aTriggerInfo->uptColumnID는 정확하지 않으므로 참조하지 않고
            // triggerStatement를 참조한다.
            if ( aTriggerInfo->uptCount == 0 )
            {
                sNeedAction = ID_TRUE;
            }
            else
            {
                sNeedAction = ID_FALSE;

                IDE_TEST( smiObject::getObjectTempInfo( aTriggerInfo->triggerHandle,
                                                        (void**)&sTriggerCache )
                          != IDE_SUCCESS );

                while ( 1 )
                {
                    IDE_TEST( sTriggerCache->latch.lockRead (
                                  NULL, /* idvSQL* */
                                  NULL /* idvWeArgs* */ )
                              != IDE_SUCCESS );
                    sStage = 1;

                    if ( sTriggerCache->isValid != ID_TRUE )
                    {
                        sStage = 0;
                        IDE_TEST( sTriggerCache->latch.unlock() != IDE_SUCCESS );

                        while ( recompileTrigger( aStatement,
                                                  aTriggerInfo )
                                != IDE_SUCCESS )
                        {
                            // rebuild error라면 다시 recompile을 시도한다.
                            IDE_TEST( ideIsRebuild() != IDE_SUCCESS );
                        }

                        // PROJ-2219 Row-level before update trigger
                        // qmnUPTE::firstInit()에서 trigger를 검사하여,
                        // invalid 상태이면 compile 하고, update DML을 rebuild 한다.
                        *aIsRecompiled = ID_TRUE;

                        continue;
                    }
                    else
                    {
                        sTriggerStatement = & sTriggerCache->triggerStatement;
                        sTriggerParseTree = (qdnCreateTriggerParseTree *)
                            sTriggerStatement->myPlan->parseTree;
                        sEventTypeList    =
                            sTriggerParseTree->triggerEvent.eventTypeList;

                        for ( sColumn = aUptColumn;
                              sColumn != NULL;
                              sColumn = sColumn->next )
                        {
                            for ( sUptColumn = sEventTypeList->updateColumns;
                                  sUptColumn != NULL;
                                  sUptColumn = sUptColumn->next )
                            {
                                if ( sUptColumn->basicInfo->column.id ==
                                     sColumn->column->id )
                                {
                                    sNeedAction = ID_TRUE;
                                    break;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }

                            if ( sNeedAction == ID_TRUE )
                            {
                                break;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        } //for ( sColumn = aUptColumn;
                    }

                    sStage = 0;
                    IDE_TEST( sTriggerCache->latch.unlock() != IDE_SUCCESS );

                    break;
                } // while
            } // else
        }
        else
        {
            sNeedAction = ID_TRUE;
        }
    }
    else
    {
        // Event 조건이 부합하지 않는 경우
        sNeedAction = ID_FALSE;
    }

    *aNeedAction = sNeedAction;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void) sTriggerCache->latch.unlock();
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}


/***********************************************************************
 *
 *   [Trigger 수행의 동시성 제어]
 *
 *    비고) t1, t2 : table, r1, r2 : trigger
 *
 *    - Trigger의 수행(r1)과 Subject Table(t1)에 대한 DDL
 *
 *
 *                DDL  |
 *                    (X)
 *                     |
 *                     V
 *     DML --(IX)-->  [t1] --(LS)--> [r1] --(IX)--> [t2]
 *
 *     위의 그림에서 보듯이 Subject Table에 대한 DDL은 X lock과
 *     DML의 IX lock에 의하여 제어된다.
 *
 *    - Trigger의 수행(r1)과 Action Table(t2) 에 대한 DDL
 *
 *      PSM과 동일한 방식으로 처리할 경우 deadlock이 발생할 수 있다.
 *
 *
 *                                              DDL  |
 *                                                  (X)
 *                                                   |
 *                                                   V
 *                                   [  ] <--(LX)-- [  ]
 *     DML --(IX)-->  [t1] --(LS)--> [r1]           [t2]
 *                                   [  ] --(IX)--> [  ]
 *
 *     위의 그림에서 보듯이 DML로부터 시작하는 [r1] --> [t2]로의 IX와
 *     DDL로부터 시작하는 [t2] --> [r1] 으로의 (LX) 는 서로 DDL의 X와
 *     DML의 (LS) 의 release를 기다리게 되어 deadlock에 빠지게 된다.
 *     이러한 문제는 t2의 변경을 r1에 반영하려는 시도로 인해 발생하며,
 *     PSM에서는 이러한 문제를 해결하기 위해 한번 invalid 되면,
 *     명시적으로 recompile하기 전까지는 항상 recompile하게 되는 문제가
 *     존재한다.
 *
 *     이러한 문제를 해결하기 위해서는 다음 그림과 같이
 *     Lock과 Latch의 방향이 서로 다른 경우가 없는 조건하에서 해결하여야 한다.
 *
 *
 *                                              DDL  |
 *                                                  (X)
 *                                                   |
 *                                                   V
 *     DML --(IX)-->  [t1] --(LS)--> [r1] --(IX)--> [t2]
 *
 *     즉, Invalid의 여부는 Trigger를 수행하는 시점에서만 판단하고,
 *     Trigger를 fire하는 Session간의 동시성 제어만 고려하면 된다.
 *
 *                            [DML]
 *                             |
 *                            (LS)
 *                             |
 *                             V
 *           [DML]  --(LS)--> [r1] --------(IX)------> [t2]
 *                             ^             |
 *                             |             |
 *                            (LX)           |
 *                             |             |
 *                             +-------------+
 *
 ***********************************************************************/


IDE_RC
qdnTrigger::fireTriggerAction( qcStatement           * aStatement,
                               iduMemory             * aNewValueMem,
                               qcmTableInfo          * aTableInfo,
                               qcmTriggerInfo        * aTriggerInfo,
                               smiTableCursor        * aTableCursor,
                               scGRID                  aGRID,
                               void                  * aOldRow,
                               qcmColumn             * aOldRowColumns,
                               void                  * aNewRow,
                               qcmColumn             * aNewRowColumns )
{
/***********************************************************************
 *
 * Description :
 *    Trigger Action에 대한 Validation 을 통해
 *    Recompile 및 Trigger Action을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::fireTriggerAction"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnTriggerCache * sCache;
    UInt              sErrorCode;
    UInt              sStage = 0;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTriggerInfo != NULL );

    //---------------------------------------
    // Trigger Cache정보의 획득
    //---------------------------------------

    IDE_TEST( smiObject::getObjectTempInfo( aTriggerInfo->triggerHandle,
                                            (void**)&sCache )
              != IDE_SUCCESS );

    while ( 1 )
    {
        // S Latch 획득
        IDE_TEST( sCache->latch.lockRead (
                      NULL, /* idvSQL * */
                      NULL /* idvWeArgs* */ ) != IDE_SUCCESS );
        sStage = 1;

        if ( sCache->isValid != ID_TRUE )
        {
            //----------------------------------------
            // Valid하지 않다면 Recompile 한다.
            //----------------------------------------

            // Release Latch
            sStage = 0;
            IDE_TEST( sCache->latch.unlock() != IDE_SUCCESS );

            while ( recompileTrigger( aStatement,
                                      aTriggerInfo )
                    != IDE_SUCCESS )
            {
                // rebuild error라면 다시 recompile을 시도한다.
                IDE_TEST( ideIsRebuild() != IDE_SUCCESS );
            }

            continue;
        }
        else
        {
            //----------------------------------------
            // Valid하다면 Trigger Action을 수행한다.
            //----------------------------------------

            if ( runTriggerAction( aStatement,
                                   aNewValueMem,
                                   aTableInfo,
                                   aTriggerInfo,
                                   aTableCursor,
                                   aGRID,
                                   aOldRow,
                                   aOldRowColumns,
                                   aNewRow,
                                   aNewRowColumns )
                 != IDE_SUCCESS )
            {
                sErrorCode = ideGetErrorCode();
                switch( sErrorCode & E_ACTION_MASK )
                {
                    case E_ACTION_RETRY:
                    case E_ACTION_REBUILD:

                        //------------------------------
                        // Rebuild 해야 함.
                        //------------------------------

                        // Release Latch
                        sStage = 0;
                        IDE_TEST( sCache->latch.unlock() != IDE_SUCCESS );

                        // X Latch 획득
                        IDE_TEST( sCache->latch.lockWrite (
                                      NULL, /* idvSQL* */
                                      NULL /* idvWeArgs* */ )
                                  != IDE_SUCCESS );
                        sStage = 1;

                        sCache->isValid = ID_FALSE;

                        sStage = 0;
                        IDE_TEST( sCache->latch.unlock() != IDE_SUCCESS );

                        break;
                    default:

                        //------------------------------
                        // 다른 Error가 존재함.
                        //------------------------------

                        // Release Latch
                        sStage = 0;
                        IDE_TEST( sCache->latch.unlock() != IDE_SUCCESS );

                        break;
                }

                IDE_TEST( 1 );
            }
            else
            {
                // Release Latch
                sStage = 0;
                IDE_TEST( sCache->latch.unlock() != IDE_SUCCESS );

                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void) sCache->latch.unlock();
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::runTriggerAction( qcStatement           * aStatement,
                              iduMemory             * aNewValueMem,
                              qcmTableInfo          * aTableInfo,
                              qcmTriggerInfo        * aTriggerInfo,
                              smiTableCursor        * aTableCursor,
                              scGRID                  aGRID,
                              void                  * aOldRow,
                              qcmColumn             * aOldRowColumns,
                              void                  * aNewRow,
                              qcmColumn             * aNewRowColumns )
{
/***********************************************************************
 *
 * Description :
 *    Trigger Action을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::runTriggerAction"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool                      sJudge;

    qdnTriggerCache           * sCache;
    qcStatement               * sTriggerStatement;
    qdnCreateTriggerParseTree * sTriggerParseTree;

    qcTemplate                * sClonedTriggerTemplate;

    SInt                        sOrgRemain;
    mtcStack                  * sOrgStack;
    UInt                        sStage = 0;

    qtcNode                sDummyNode;
    iduMemoryStatus        sMemStatus;

    qsxProcPlanList      * sOriProcPlanList;
    idBool                 sLatched = ID_FALSE;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTriggerInfo != NULL );

    // procPlanList 백업
    sOriProcPlanList = aStatement->spvEnv->procPlanList;

    //---------------------------------------
    // Trigger Cache정보의 획득
    //---------------------------------------

    IDE_TEST( smiObject::getObjectTempInfo( aTriggerInfo->triggerHandle,
                                            (void**)&sCache )
              != IDE_SUCCESS );;

    sTriggerStatement = & sCache->triggerStatement;
    sTriggerParseTree =
        (qdnCreateTriggerParseTree * ) sTriggerStatement->myPlan->parseTree;

    //---------------------------------------
    // Trigger 의 수행
    // Action Body의 수행을 위하여 EXEC proc1과 유사한 routine을 사용함.
    //---------------------------------------

    // To Fix PR-11368
    // 메모리 해제를 위한 위치 저장
    IDE_TEST( aStatement->qmtMem->getStatus( & sMemStatus )
              != IDE_SUCCESS );

    sStage = 1;

    // 트리거의 0번 template을 복사한다.
    IDU_LIMITPOINT("qdnTrigger::runTriggerAction::malloc");
    IDE_TEST( aStatement->qmtMem->alloc( ID_SIZEOF(qcTemplate),
                                         (void **)&sClonedTriggerTemplate )
              != IDE_SUCCESS );

    IDE_TEST( qcg::cloneAndInitTemplate( aStatement->qmtMem,
                                         QC_SHARED_TMPLATE(sTriggerStatement),
                                         sClonedTriggerTemplate )
              != IDE_SUCCESS );

    // PROJ-2002 Column Security
    // trigger 수행시 statement가 필요하다.
    sClonedTriggerTemplate->stmt = aStatement;

    //---------------------------------------
    // REREFERENCING ROW의 복사
    //---------------------------------------

    IDE_TEST( setReferencingRow( sClonedTriggerTemplate,
                                 aTableInfo,
                                 sTriggerParseTree,
                                 aTableCursor,
                                 aGRID,
                                 aOldRow,
                                 aOldRowColumns,
                                 aNewRow,
                                 aNewRowColumns )
              != IDE_SUCCESS );

    //---------------------------------------
    // WHEN 조건의 검사
    //---------------------------------------

    sJudge = ID_TRUE;

    if ( sTriggerParseTree->actionCond.whenCondition != NULL )
    {
        sOrgRemain = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain;
        sOrgStack = QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack;

        sStage = 2;

        sClonedTriggerTemplate->tmplate.stackRemain =
            QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain;
        sClonedTriggerTemplate->tmplate.stack =
            QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack;

        IDE_TEST( qtc::judge( & sJudge,
                              sTriggerParseTree->actionCond.whenCondition,
                              sClonedTriggerTemplate )
                  != IDE_SUCCESS );

        sStage = 1;

        QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain = sOrgRemain;
        QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack = sOrgStack;
    }
    else
    {
        // Nothing To Do
    }

    //---------------------------------------
    // Trigger Action Body의 수행
    //---------------------------------------

    if ( sJudge == ID_TRUE )
    {
        // WHEN 조건을 만족하는 경우

        // Dummy Node 초기화
        idlOS::memset( & sDummyNode, 0x00, ID_SIZEOF( qtcNode ) );
        sDummyNode.node.arguments = NULL;

        // BUG-20797
        // Trigger에서 사용한 PSM의 s_latch 획득
        if( sLatched == ID_FALSE )
        {
            IDE_TEST_RAISE( qsxRelatedProc::latchObjects( sTriggerStatement->spvEnv->procPlanList )
                            != IDE_SUCCESS, invalid_procedure );
            sLatched = ID_TRUE;

            IDE_TEST_RAISE( qsxRelatedProc::checkObjects( sTriggerStatement->spvEnv->procPlanList )
                            != IDE_SUCCESS, invalid_procedure );
        }
        else
        {
            // Nothing to do.
        }

        // Trigger의 수행시 PSM을 procPlanList에서 검색할 수 있도록 기록
        aStatement->spvEnv->procPlanList = sTriggerStatement->spvEnv->procPlanList;

        // 트리거 실행 시에도 statement에 대한 재빌드가 이루어 진다.
        aStatement->spvEnv->flag &= ~QSV_ENV_TRIGGER_MASK;
        aStatement->spvEnv->flag |= QSV_ENV_TRIGGER_TRUE;

        // Trigger의 수행
        IDE_TEST( qsx::callProcWithNode ( aStatement,
                                          & sTriggerParseTree->actionBody,
                                          & sDummyNode,
                                          NULL,
                                          sClonedTriggerTemplate )
                  != IDE_SUCCESS );

        // Trigger에서 사용한 PSM의 s_latch 해제
        if( sLatched == ID_TRUE )
        {
            sLatched = ID_FALSE;
            IDE_TEST( qsxRelatedProc::unlatchObjects( sTriggerStatement->spvEnv->procPlanList )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nohting To Do
    }

    sStage = 0;

    //---------------------------------------
    // REREFERENCING ROW를 output으로
    //---------------------------------------
    // To fix BUG-12622
    // before trigger에서 new row를 변경한 경우
    // valuelist로 변경된 부분을 복사한다.
    IDE_TEST( makeValueListFromReferencingRow(
                  sClonedTriggerTemplate,
                  aNewValueMem,
                  aTableInfo,
                  sTriggerParseTree,
                  aNewRow,
                  aNewRowColumns )
              != IDE_SUCCESS );

    IDE_TEST( aStatement->qmtMem->setStatus( & sMemStatus )
              != IDE_SUCCESS );

    // procPlanList 원복
    aStatement->spvEnv->procPlanList = sOriProcPlanList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( invalid_procedure );
    {
        IDE_SET(ideSetErrorCode(qpERR_REBUILD_QCI_PROC_INVALID));
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 2:
            QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain = sOrgRemain;
            QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack = sOrgStack;
        case 1:
            (void)aStatement->qmtMem->setStatus( & sMemStatus );
        default:
            break;
    }

    if( sLatched == ID_TRUE )
    {
        (void) qsxRelatedProc::unlatchObjects( sTriggerStatement->spvEnv->procPlanList );
    }
    else
    {
        // Nothing to do.
    }

    // procPlanList 원복
    aStatement->spvEnv->procPlanList = sOriProcPlanList;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::setReferencingRow( qcTemplate                * aClonedTemplate,
                               qcmTableInfo              * aTableInfo,
                               qdnCreateTriggerParseTree * aParseTree,
                               smiTableCursor            * aTableCursor,
                               scGRID                      aGRID,
                               void                      * aOldRow,
                               qcmColumn                 * aOldRowColumns,
                               void                      * aNewRow,
                               qcmColumn                 * aNewRowColumns )
{
/***********************************************************************
 *
 * Description :
 *
 *    WHEN Condition 및 Trigger Action Body를 수행하기 위해
 *    REFERENCING ROW 정보를 Trigger의 Template이 복사된
 *    Cloned Template 에 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::setReferencingRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnTriggerRef             * sRef;
    qsVariables               * sVariable;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aClonedTemplate != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //---------------------------------------
    // REREFERENCING ROW의 복사
    //---------------------------------------

    for ( sRef = aParseTree->triggerReference;
          sRef != NULL;
          sRef = sRef->next )
    {

        //---------------------------------------
        // WHEN Conditition 및 Action Body를 위한 REREFERENCING ROW의 복사
        //---------------------------------------

        for ( sVariable =
                  (qsVariables*) aParseTree->actionBody.paraDecls;
              sVariable != NULL;
              sVariable = (qsVariables*) sVariable->common.next )
        {
            if ( sRef->rowVar == sVariable )
            {
                break;
            }
        }

        // 반드시 해당 Variable 이 존재해야 함.
        IDE_TEST_RAISE( sVariable == NULL, ERR_REF_ROW_NOT_FOUND );

        switch ( sRef->refType )
        {
            case QCM_TRIGGER_REF_OLD_ROW:
                if ( aParseTree->triggerEvent.eventTime == QCM_TRIGGER_INSTEAD_OF )
                {
                    if ( (aOldRow != NULL) && (aOldRowColumns != NULL) )
                    {
                        // trigger rowtype에 table row를 복사
                        // instead of delete OLD_ROW smivalue
                        IDE_TEST( copyRowFromValueList(
                                      aClonedTemplate,
                                      aTableInfo,
                                      sVariable,
                                      aOldRow,
                                      aOldRowColumns )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    if ( (aOldRow != NULL) && (aOldRowColumns != NULL) )
                    {
                        // trigger rowtype에 table row를 복사
                        IDE_TEST( copyRowFromTableRow( aClonedTemplate,
                                                       aTableInfo,
                                                       sVariable,
                                                       aTableCursor,
                                                       aOldRow,
                                                       aGRID,
                                                       aOldRowColumns, 
                                                       QCM_TRIGGER_REF_OLD_ROW /* reftype */ )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                break;

            case QCM_TRIGGER_REF_NEW_ROW:
                if ( (aNewRow != NULL) && (aNewRowColumns != NULL) )
                {
                    // To fix BUG-12622
                    // before trigger는 valuelist로부터
                    // trigger row를 생성한다.
                    if( ( aParseTree->triggerEvent.eventTime ==
                          QCM_TRIGGER_BEFORE ) ||
                        ( aParseTree->triggerEvent.eventTime ==
                        QCM_TRIGGER_INSTEAD_OF ) ) //PROJ-1888 INSTEAD OF TRIGGER
                    {

                        // Insert할 때
                        // aNewRow는 row형태가 아닌 smivalue형태임
                        IDE_TEST( copyRowFromValueList(
                                      aClonedTemplate,
                                      aTableInfo,
                                      sVariable,
                                      aNewRow,
                                      aNewRowColumns )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // PROJ-1075
                        // trigger rowtype에 table row를 복사
                        IDE_TEST( copyRowFromTableRow( aClonedTemplate,
                                                       aTableInfo,
                                                       sVariable,
                                                       aTableCursor,
                                                       aNewRow,
                                                       aGRID,
                                                       aNewRowColumns,
                                                       QCM_TRIGGER_REF_NEW_ROW /* reftype */ )
                                  != IDE_SUCCESS );
                    }
                }
                else
                {
                    // Nothing to do.
                }
                break;

            default:
                IDE_DASSERT( 0 );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REF_ROW_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdnTrigger::setReferencingRow",
                                  "Referencing row not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::makeValueListFromReferencingRow(
    qcTemplate                * aClonedTemplate,
    iduMemory                 * aNewValueMem,
    qcmTableInfo              * aTableInfo,
    qdnCreateTriggerParseTree * aParseTree,
    void                      * aNewRow,
    qcmColumn                 * aNewRowColumns )
{
/***********************************************************************
 *
 * Description : To fix BUG-12622
 *    Before Trigger로 인해 바뀐 new row를 value list에 복사한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qdnTriggerRef             * sRef;
    qsVariables               * sVariable;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aClonedTemplate != NULL );
    IDE_DASSERT( aNewValueMem != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //---------------------------------------
    // REREFERENCING ROW의 복사
    //---------------------------------------

    for ( sRef = aParseTree->triggerReference;
          sRef != NULL;
          sRef = sRef->next )
    {
        for ( sVariable =
                  (qsVariables*) aParseTree->actionBody.paraDecls;
              sVariable != NULL;
              sVariable = (qsVariables*) sVariable->common.next )
        {
            if ( sRef->rowVar == sVariable )
            {
                break;
            }
        }

        IDE_TEST_RAISE( sVariable == NULL, ERR_REF_ROW_NOT_FOUND );

        switch ( sRef->refType )
        {
            case QCM_TRIGGER_REF_OLD_ROW:
                break;

            case QCM_TRIGGER_REF_NEW_ROW:
                // BUG-46074
                // Before delete trigger에서 new row를 reference 할 수 있도록 수정함.
                // 이 경우 param init 과정에서 null row로 초기화 됨.
                if( (aParseTree->triggerEvent.eventTime == QCM_TRIGGER_BEFORE) &&
                    (aClonedTemplate->stmt->spxEnv->mTriggerEventType != QCM_TRIGGER_EVENT_DELETE) )
                {
                    // Insert할 때
                    IDE_TEST( copyValueListFromRow(
                                  aClonedTemplate,
                                  aNewValueMem,
                                  aTableInfo,
                                  sVariable,
                                  aNewRow,    // valuelist
                                  aNewRowColumns )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                break;
            default:
                IDE_DASSERT( 0 );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REF_ROW_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdnTrigger::makeValueListFromReferencingRow",
                                  "Referencing row not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qdnTrigger::checkObjects( qcStatement  * aStatement,
                          idBool       * aInvalidProc )
{
/***********************************************************************
 *
 * Description : BUG-20797
 *    Trigger Statement에 사용된 PSM의 변경을 검사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::checkObjects"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool             sInvalidProc = ID_FALSE;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aInvalidProc != NULL );

    //---------------------------------------
    // Trigger에서 사용한 PSM의 변경을 검사
    //---------------------------------------

    if ( aStatement->spvEnv->procPlanList != NULL )
    {
        if ( qsxRelatedProc::latchObjects(
                 aStatement->spvEnv->procPlanList )
             == IDE_SUCCESS )
        {
            if ( qsxRelatedProc::checkObjects( aStatement->spvEnv->procPlanList )
                 != IDE_SUCCESS )
            {
                sInvalidProc = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            (void) qsxRelatedProc::unlatchObjects( aStatement->spvEnv->procPlanList );
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

    *aInvalidProc = sInvalidProc;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qdnTrigger::recompileTrigger( qcStatement     * aStatement,
                              qcmTriggerInfo  * aTriggerInfo )
{
/***********************************************************************
 *
 * Description :
 *    새로이 PVO 과정을 통하여 Trigger Cache 정보를 구축한다.
 *
 * Implementation :
 *    X Latch를 잡은 후에 validation 검사를 통해
 *    Recompile 여부를 결정한다.
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::recompileTrigger"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SChar                        sErrorMsg[ QC_MAX_OBJECT_NAME_LEN * 2 + 2 ];

    qdnTriggerCache            * sCache;
    qcStatement                * sTriggerStatement;
    smiStatement                 sSmiStmt;
    smiStatement               * sSmiStmtOrg;
    qdnCreateTriggerParseTree  * sTriggerParseTree;
    UInt                         sStage = 0;
    idBool                       sAllocFlag = ID_FALSE;
    idBool                       sInvalidProc;
    qcuSqlSourceInfo             sSqlInfo;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTriggerInfo != NULL );

    //---------------------------------------
    // Trigger Cache정보의 획득
    //---------------------------------------

    IDE_TEST( smiObject::getObjectTempInfo( aTriggerInfo->triggerHandle,
                                            (void**)&sCache )
              != IDE_SUCCESS );

    sTriggerStatement = & sCache->triggerStatement;

    // X Latch 획득
    IDE_TEST( sCache->latch.lockWrite (
                  NULL, /* idvSQL * */
                  NULL /* idvWeArgs* */ )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( checkObjects( aStatement, & sInvalidProc )
              != IDE_SUCCESS );

    // fix BUG-18679
    // ALTER TRIGGER COMPILE 시에는 무조건 recompile한다.
    if( (aStatement->myPlan->parseTree->stmtKind != QCI_STMT_SCHEMA_DDL) &&
        (sCache->isValid == ID_TRUE) &&
        (sInvalidProc == ID_FALSE) )
    {
        IDE_CONT( already_valid );
    }
    else
    {
        // Nothing to do
    }

    //---------------------------------------
    // Trigger Cache Statement의 초기화
    //---------------------------------------

    // Trigger Statement를 위한 공간 초기화
    IDE_TEST( qcg::freeStatement( sTriggerStatement )
              != IDE_SUCCESS );

    // 실제 Stack Count는 수행 시점에 Template 복사에 의하여
    // Session 정보로부터 결정된다.
    IDE_TEST( qcg::allocStatement( sTriggerStatement,
                                   aStatement->session,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    //fix BUG-18158
    sAllocFlag = ID_TRUE;

    // PVO 과정이 발생하게 되므로 새로운 smiStatement로 수행한다.
    // Parent는 내려온 smiStatement를 사용한다(by sjkim)
    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              QC_SMI_STMT( aStatement ),
                              SMI_STATEMENT_NORMAL |
                              SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );

    qcg::getSmiStmt( sTriggerStatement, & sSmiStmtOrg );
    qcg::setSmiStmt( sTriggerStatement, & sSmiStmt );
    sStage = 2;

    // Statement Text정보의 설정
    sTriggerStatement->myPlan->stmtTextLen = sCache->stmtTextLen;
    sTriggerStatement->myPlan->stmtText = sCache->stmtText;

    //---------------------------------------
    // Trigger Cache Statement의 PVO 수행
    //---------------------------------------

    // Parsing
    IDE_TEST( qcpManager::parseIt( sTriggerStatement ) != IDE_SUCCESS );

    // To Fix PR-10645
    // Parsing을 거치면 DDL로 설정되어 ON절의 Table정보 획득시 Table X Lock을
    // 획득하게 된다.  이 때, 다른 DML이 수행 중이면 Table X Lock을 회득하지
    // 못하여 Recompile이 실패하게 된다.
    // 그러나, Recompile 과정은 DML과 ALTER TRIGGER .. COMPILE
    // 구문의 수행 시에만 수행되고 Meta Table 및 Meta Cache를 변경시키는
    // 작업이 아니다.  따라서, DDL로 설정되어 있는 정보를 DML로
    // 변경시켜 주어야 한다.  Table에 대하여 IS Lock 만을 시도하게 한다.
    sTriggerStatement->myPlan->parseTree->stmtKind = QCI_STMT_MASK_DML;
    sTriggerStatement->myPlan->parseTree->validate = qdnTrigger::validateRecompile;

    sTriggerParseTree =
        (qdnCreateTriggerParseTree * ) sTriggerStatement->myPlan->parseTree;

    // To Fix BUG-10899
    // 트리거 액션 바디는 프로시저 실행루틴을 타는데,
    // 이 속에서 에러가 났을 때 QC_NAME_POSITION으로 에러난 위치를
    // 정확히 찍으려 qcuSqlSourceInfo루틴을 타는데, 이때 이 필드들이 쓰인다.
    sTriggerParseTree->actionBody.stmtText    = sCache->stmtText;
    sTriggerParseTree->actionBody.stmtTextLen = sCache->stmtTextLen;

    // To fix BUG-14584
    // recompile을 replication과 관계없이 수행하기 위해 isRecompile을 TRUE로.
    sTriggerParseTree->isRecompile = ID_TRUE;
    sTriggerParseTree->triggerUserID = aTriggerInfo->userID;

    // BUG-42989 Create trigger with enable/disable option.
    // 기존의 enable 옵션을 유지한다.
    sTriggerParseTree->actionCond.isEnable = aTriggerInfo->enable;

    (void)qdnTrigger::allocProcInfo( sTriggerStatement,
                                     aTriggerInfo->userID );

    // Post-Parsing
    IDE_TEST_RAISE(
        sTriggerStatement->myPlan->parseTree->parse( sTriggerStatement )
        != IDE_SUCCESS, err_trigger_recompile );
    IDE_TEST( qcg::fixAfterParsing( sTriggerStatement ) != IDE_SUCCESS );
    sStage = 3;

    // Validation
    IDE_TEST_RAISE(
        sTriggerStatement->myPlan->parseTree->validate( sTriggerStatement )
        != IDE_SUCCESS, err_trigger_recompile  );
    IDE_TEST( qcg::fixAfterValidation( sTriggerStatement )
              != IDE_SUCCESS );

    // Optimization
    IDE_TEST_RAISE(
        sTriggerStatement->myPlan->parseTree->optimize( sTriggerStatement )
        != IDE_SUCCESS, err_trigger_recompile );
    IDE_TEST( qcg::fixAfterOptimization( sTriggerStatement )
              != IDE_SUCCESS );

    // BUG-42322
    sTriggerParseTree->actionBody.procOID = aTriggerInfo->triggerOID;

    // PROJ-2219 Row-level before update trigger
    // Row-level before update trigger에서
    // new row를 통해 참조하는 column list를 생성한다.
    IDE_TEST( qdnTrigger::makeRefColumnList( sTriggerStatement )
              != IDE_SUCCESS );

    // BUG-16543
    // Meta Table에 정보를 변경한다.
    IDE_TEST( qcmTrigger::removeMetaInfo( sTriggerStatement,
                                          aTriggerInfo->triggerOID )
              != IDE_SUCCESS );
    
    IDE_TEST( qcmTrigger::addMetaInfo( sTriggerStatement,
                                       aTriggerInfo->triggerHandle )
              != IDE_SUCCESS );

    sCache->isValid = ID_TRUE;

    IDE_TEST( qsxRelatedProc::unlatchObjects( sTriggerStatement->spvEnv->procPlanList )
              != IDE_SUCCESS );
    sTriggerStatement->spvEnv->latched = ID_FALSE;

    sStage = 1;
    qcg::setSmiStmt( sTriggerStatement, sSmiStmtOrg );

    IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    // fix BUG-18679
    IDE_EXCEPTION_CONT( already_valid );

    sStage = 0;
    IDE_TEST( sCache->latch.unlock() != IDE_SUCCESS );

    //fix BUG-18158
    if( sAllocFlag == ID_TRUE )
    {
        sTriggerStatement->session = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_trigger_recompile );
    {
        if ( ideIsRebuild() != IDE_SUCCESS )
        {
            (void)sSqlInfo.initWithBeforeMessage(
                QC_QMX_MEM( aStatement ) );

            idlOS::snprintf( sErrorMsg, ID_SIZEOF(sErrorMsg),
                             "%s.%s",
                             aTriggerInfo->userName,
                             aTriggerInfo->triggerName );

            IDE_SET(ideSetErrorCode( qpERR_ABORT_TRIGGER_RECOMPILE,
                                     sErrorMsg,
                                     "\n\n",
                                     sSqlInfo.getBeforeErrMessage() ));

            (void)sSqlInfo.fini();
        }
        else
        {
            // rebuild error가 올라오는 경우
            // error pass
        }
    }
    IDE_EXCEPTION_END;

    sCache->isValid = ID_FALSE;

    switch( sStage )
    {
        case 3 :
            if ( qsxRelatedProc::unlatchObjects( sTriggerStatement->spvEnv->procPlanList )
                 == IDE_SUCCESS )
            {
                sTriggerStatement->spvEnv->latched = ID_FALSE;
            }
            else
            {
                IDE_ERRLOG(IDE_QP_1);
            }

        case 2 :
            qcg::setSmiStmt( sTriggerStatement, sSmiStmtOrg );

            if (sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS)
            {
                IDE_CALLBACK_FATAL("Trigger recompile smiStmt.end() failed");
            }

        case 1:
            // Release Latch
            (void) sCache->latch.unlock();
    }

    //fix BUG-18158
    if( sAllocFlag == ID_TRUE )
    {
        sTriggerStatement->session = NULL;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qdnTrigger::setInvalidTriggerCache4Table( qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *    BUG-16543
 *    qdbAlter::executeDropCol에서 column_id가 변경될 수 있으므로
 *    triggerCache를 invalid로 변경하여 recompile될 수 있게 한다.
 *
 * Implementation :
 *    X Latch를 잡은 후에 invalid로 변경한다.
 *
 ***********************************************************************/

#define IDE_FN "qdnTrigger::setInvalidTriggerCache4Table"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdnTriggerCache            * sCache;
    UInt                         sStage = 0;
    UInt                         i;

    for ( i = 0; i < aTableInfo->triggerCount; i++ )
    {
        IDE_TEST( smiObject::getObjectTempInfo( aTableInfo->triggerInfo[i].triggerHandle,
                                                (void**)&sCache )
                  != IDE_SUCCESS );

        // X Latch 획득
        IDE_TEST( sCache->latch.lockWrite (
                            NULL, /* idvSQL * */
                            NULL /* idvWeArgs* */ )
                  != IDE_SUCCESS );
        sStage = 1;

        sCache->isValid = ID_FALSE;

        sStage = 0;
        IDE_TEST( sCache->latch.unlock()
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void) sCache->latch.unlock();
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdnTrigger::copyRowFromTableRow( qcTemplate       * aTemplate,
                                        qcmTableInfo     * aTableInfo,
                                        qsVariables      * aVariable,
                                        smiTableCursor   * aTableCursor,
                                        void             * aTableRow,
                                        scGRID             aGRID,
                                        qcmColumn        * aTableRowColumns,
                                        qcmTriggerRefType  aRefType )
{
/***********************************************************************
 *
 * Description : PROJ-1075 trigger의 tableRow를 psm호환 가능한
 *               rowtype 변수에 복사.
 *
 * Implementation :
 *             (1) psm호환 rowtype은 해당 column의 다음 컬럼부터
 *                 내부 컬럼이므로 내부컬럼을 순회하며
 *                 각 컬럼마다 mtc::value를 이용하여데이터를 가져옴
 *             (2) trigger tableRowtype도 마찬가지로 순회
 *             (3) memcpy로 actual size만큼 복사.
 *
 ***********************************************************************/
#define IDE_FN "qdnTrigger::copyRowFromTableRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UShort           i;
    UShort           sTable;
    UInt             sSrcActualSize;
    mtcTuple       * sDstTuple;
    void           * sDstValue;
    void           * sSrcValue;
    mtcColumn      * sSrcColumn;
    mtcColumn      * sDstColumn;
    UInt             sColumnOrder;

    sTable = aVariable->variableTypeNode->node.table;

    sDstTuple = &aTemplate->tmplate.rows[sTable];

    // sDstTuple은 rowtype변수의 tuple이므로,
    // 실제 row의 컬럼 개수는 1을 빼야 한다.
    // 맨 앞의 컬럼은 rowtype변수 자체이기 때문이다.
    for ( i = 0; i < sDstTuple->columnCount - 1; i++ )
    {
        IDE_ASSERT( aTableRowColumns != NULL );

        sSrcColumn = aTableRowColumns[i].basicInfo;
        sDstColumn = &sDstTuple->columns[i+1];

        sSrcValue = (void*)mtc::value( sSrcColumn,
                                       aTableRow,
                                       MTD_OFFSET_USE );
        sDstValue = (void*)mtc::value( sDstColumn,
                                       sDstTuple->row,
                                       MTD_OFFSET_USE );

        // PROJ-2002 Column Security
        if ( (sSrcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
             == MTD_ENCRYPT_TYPE_TRUE )
        {
            // 보안 타입을 사용하는 테이블의 trigger 수행시
            // referencing row의 타입은 원본 타입이므로
            // decrypt가 필요하다.

            sColumnOrder = sSrcColumn->column.id & SMI_COLUMN_ID_MASK;

            // decrypt & copy
            IDE_TEST( qcsModule::decryptColumn(
                          aTemplate->stmt,
                          aTableInfo,
                          sColumnOrder,
                          sSrcColumn,   // encrypt column
                          sSrcValue,
                          sDstColumn,   // plain column
                          sDstValue )
                      != IDE_SUCCESS );
        }
        else
        {
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
            if ( (sSrcColumn->module->id == MTD_BLOB_LOCATOR_ID) ||
                 (sSrcColumn->module->id == MTD_CLOB_LOCATOR_ID) ||
                 (sSrcColumn->module->id == MTD_BLOB_ID) ||
                 (sSrcColumn->module->id == MTD_CLOB_ID) )
            {
                IDE_TEST( convertXlobToXlobValue( & aTemplate->tmplate,
                                                  aTableCursor,
                                                  aTableRow,
                                                  aGRID,
                                                  sSrcColumn,
                                                  sSrcValue,
                                                  sDstColumn,
                                                  sDstValue,
                                                  aRefType )
                          != IDE_SUCCESS );
            }
            else
            {
                // 적합성 검사. column사이즈가 동일해야 된다.
                IDE_DASSERT( sSrcColumn->column.size == sDstColumn->column.size );

                sSrcActualSize = sSrcColumn->module->actualSize(
                    sSrcColumn,
                    sSrcValue );

                idlOS::memcpy( sDstValue, sSrcValue, sSrcActualSize );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdnTrigger::copyRowFromValueList( qcTemplate   * aTemplate,
                                         qcmTableInfo * aTableInfo,
                                         qsVariables  * aVariable,
                                         void         * aValueList,
                                         qcmColumn    * aTableRowColumns )
{
/***********************************************************************
 *
 * Description : To fix BUG-12622
 *               Row-level before insert/update trigger 수행 전
 *               new row를 생성하기 위해 호출하는 함수이다.
 *               dml수행전 value list를 rowtype변수에 복사
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort      i;
    UShort      sTable;
    UInt        sSrcActualSize;
    mtcTuple  * sDstTuple;
    void      * sDstValue;
    void      * sSrcValue;
    qcmColumn * sSrcQcmColumn;
    mtcColumn * sSrcColumn;
    mtcColumn * sDstColumn;
    smiValue  * sValueList;
    UInt        sColumnOrder;

    sTable = aVariable->variableTypeNode->node.table;

    sDstTuple = &aTemplate->tmplate.rows[sTable];

    sValueList = (smiValue*)aValueList;

    sSrcQcmColumn = aTableRowColumns;

    // sDstTuple은 rowtype변수의 tuple이고,
    // 맨 앞의 컬럼은 rowtype변수 자체이므로 index를 1부터 시작한다.
    // sSrcColumn은 table의 column 순서대로 정렬되어 있다.
    for ( i = 1; i < sDstTuple->columnCount; i++ )
    {
        if ( sSrcQcmColumn == NULL )
        {
            break;
        }
        else
        {
            sSrcColumn = sSrcQcmColumn->basicInfo;
            sDstColumn = &sDstTuple->columns[i];
        }

        if ( sSrcColumn->column.id !=
             sDstColumn->column.id )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-2002 Column Security
        if ( (sSrcColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
             == MTD_ENCRYPT_TYPE_TRUE )
        {
            // 보안 타입을 사용하는 테이블의 trigger 수행시
            // referencing row의 타입은 원본 타입이므로
            // decrypt가 필요하다.

            // variable null의 형식을 맞춘다.
            if( sValueList->length == 0 )
            {
                sSrcValue = sSrcColumn->module->staticNull;
            }
            else
            {
                sSrcValue = (void*)sValueList->value;
            }

            sDstValue = (void*)mtc::value( sDstColumn,
                                           sDstTuple->row,
                                           MTD_OFFSET_USE );

            // 보안 타입은 MTD_DATA_STORE_MTDVALUE_TRUE 이므로
            // memory와 disk를 구별하지 않는다.

            sColumnOrder = sSrcColumn->column.id & SMI_COLUMN_ID_MASK;

            // decrypt & copy
            IDE_TEST( qcsModule::decryptColumn(
                          aTemplate->stmt,
                          aTableInfo,
                          sColumnOrder,
                          sSrcColumn,   // encrypt column
                          sSrcValue,
                          sDstColumn,   // plain column
                          sDstValue )
                      != IDE_SUCCESS );
        }
        else
        {
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
            IDE_TEST_RAISE( (sSrcColumn->module->id == MTD_BLOB_LOCATOR_ID) ||
                            (sSrcColumn->module->id == MTD_CLOB_LOCATOR_ID) ||
                            (sSrcColumn->module->id == MTD_BLOB_ID) ||
                            (sSrcColumn->module->id == MTD_CLOB_ID),
                            ERR_NOT_SUPPORTED_DATATYPE );

            // 적합성 검사. column사이즈가 동일해야 된다.
            IDE_DASSERT( sSrcColumn->column.size == sDstColumn->column.size );

            if( ( sSrcColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
                == SMI_COLUMN_STORAGE_MEMORY )
            {
                //---------------------------
                // MEMORY COLUMN
                //---------------------------

                // variable null의 형식을 맞춘다.
                if( sValueList->length == 0 )
                {
                    sSrcValue = sSrcColumn->module->staticNull;
                }
                else
                {
                    sSrcValue = (void*)sValueList->value;
                }

                sDstValue = (void*)mtc::value( sDstColumn,
                                               sDstTuple->row,
                                               MTD_OFFSET_USE );

                sSrcActualSize = sSrcColumn->module->actualSize(
                    sSrcColumn,
                    sSrcValue );

                idlOS::memcpy( sDstValue, sSrcValue, sSrcActualSize );
            }
            else
            {
                //---------------------------
                // DISK COLUMN
                //---------------------------

                IDE_ASSERT(sDstColumn->module->storedValue2MtdValue[MTD_COLUMN_COPY_FUNC_NORMAL](
                    sDstColumn->column.size,
                    (UChar*)sDstTuple->row + sDstColumn->column.offset,
                    0,
                    sValueList->length,
                    sValueList->value ) == IDE_SUCCESS);
            }
        }

        sSrcQcmColumn = sSrcQcmColumn->next;
        sValueList++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORTED_DATATYPE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_NOT_SUPPORTED_DATATYPE_SQLTEXT,
                                  "LOB columns are not supported." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdnTrigger::copyValueListFromRow( qcTemplate   * aTemplate,
                                         iduMemory    * aNewValueMem,
                                         qcmTableInfo * aTableInfo,
                                         qsVariables  * aVariable,
                                         void         * aValueList,
                                         qcmColumn    * aTableRowColumns )
{
/***********************************************************************
 *
 * Description : To fix BUG-12622
 *               Row-level before insert/update trigger 수행 후
 *               new row를 value list에 복사하기 위해 호출하는 함수이다.
 *               row로부터 value list를 만든다.
 *               바뀐 컬럼(l-value 속성의 column)만 복사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort         i;
    UShort         sTable;
    UInt           sSrcActualSize;
    mtcTuple     * sSrcTuple;
    void         * sSrcValue;
    void         * sDstValue;
    UInt           sSrcStoringSize;
    void         * sCopiedSrcValue;
    qcmColumn    * sDstQcmColumn;
    mtcColumn    * sSrcColumn;
    mtcColumn    * sDstColumn;
    smiValue     * sValueList;
    qtcNode      * sNode;
    UInt           sColumnOrder;
    UInt           sStoringSize = 0;
    void         * sStoringValue = NULL;

    sTable = aVariable->variableTypeNode->node.table;

    sSrcTuple = &aTemplate->tmplate.rows[sTable];

    sValueList = (smiValue*)aValueList;

    IDE_ERROR( aTableRowColumns != NULL );

    sDstQcmColumn = aTableRowColumns;

    // sSrcTuple은 rowtype변수의 tuple이고,
    // 맨 앞의 컬럼은 rowtype변수 자체이므로 index를 1부터 시작한다.
    // sDstColumn은 table의 column 순서대로 정렬되어 있다.
    for ( i = 1,
             sNode = (qtcNode*)aVariable->variableTypeNode->node.arguments;
          (i < sSrcTuple->columnCount) || (sNode != NULL);
          i++,
             sNode = (qtcNode*)sNode->node.next )
    {
        if ( sDstQcmColumn == NULL )
        {
            break;
        }
        else
        {
            sDstColumn = sDstQcmColumn->basicInfo;
            sSrcColumn = &sSrcTuple->columns[i];
        }

        if ( sDstColumn->column.id !=
             sSrcColumn->column.id )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        // QTC_NODE_LVALUE_ENABLE flag가 set되어있는 경우
        // 변경된 컬럼으로 볼 수 있음.
        if ( ( sNode->lflag & QTC_NODE_LVALUE_MASK ) ==
             QTC_NODE_LVALUE_ENABLE )
        {
            sSrcValue = (void*)mtc::value( sSrcColumn,
                                           sSrcTuple->row,
                                           MTD_OFFSET_USE );

            // PROJ-2002 Column Security
            if ( (sDstColumn->module->flag & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                // 보안 타입을 사용하는 테이블의 trigger 수행시
                // referencing row의 타입은 원본 타입이므로
                // encrypt가 필요하다.

                sColumnOrder = sDstColumn->column.id & SMI_COLUMN_ID_MASK;

                IDU_LIMITPOINT("qdnTrigger::copyValueListFromRow::malloc1");
                IDE_TEST( aNewValueMem->alloc(
                              sDstColumn->column.size,
                              (void**)&sDstValue )
                          != IDE_SUCCESS );

                // encrypt & copy
                IDE_TEST( qcsModule::encryptColumn(
                              aTemplate->stmt,
                              aTableInfo,
                              sColumnOrder,
                              sSrcColumn,   // plain column
                              sSrcValue,
                              sDstColumn,   // encrypt column
                              sDstValue )
                          != IDE_SUCCESS );

                IDE_TEST( qdbCommon::mtdValue2StoringValue( sDstColumn,
                                                            sDstColumn,
                                                            sDstValue,
                                                            &sStoringValue )
                          != IDE_SUCCESS );
                sValueList->value = sStoringValue;

                IDE_TEST( qdbCommon::storingSize( sDstColumn,
                                                  sDstColumn,
                                                  sDstValue,
                                                  &sStoringSize )
                          != IDE_SUCCESS );
                sValueList->length = sStoringSize;
            }
            else
            {
                sSrcActualSize = sSrcColumn->module->actualSize(
                    sSrcColumn,
                    sSrcValue );

                IDE_TEST( qdbCommon::storingSize( sSrcColumn,
                                                  sSrcColumn,
                                                  sSrcValue,
                                                  &sStoringSize )
                          != IDE_SUCCESS );
                sSrcStoringSize = sStoringSize;

                if( sSrcStoringSize == 0 )
                {
                    // variable, lob, disk column에 대한 처리
                    sValueList->value = NULL;
                    sValueList->length = 0;
                }
                else
                {
                    // PROJ-1705
                    // QP에서 smiValue->value는
                    // mtdDataType value를 source로 해서
                    // qdbCommon::mtdValue2StoringValue()함수를 사용해서 구성한다.
                    // 이렇게 구성된 smiValue->value로
                    // qdbCommon::storingValue2mtdValue()함수를 사용해서
                    // 다시 mtdDataType value를 참조하는 경우가 있기때문이다.
                    IDU_LIMITPOINT("qdnTrigger::copyValueListFromRow::malloc2");
                    IDE_TEST( aNewValueMem->alloc(
                                  sSrcActualSize,
                                  (void**)&sCopiedSrcValue )
                              != IDE_SUCCESS );

                    idlOS::memcpy( (void*)sCopiedSrcValue,
                                   sSrcValue,
                                   sSrcActualSize );

                    IDE_TEST( qdbCommon::mtdValue2StoringValue( sSrcColumn,
                                  sSrcColumn,
                                  sCopiedSrcValue,
                                  &sStoringValue)
                              != IDE_SUCCESS );
                    sValueList->value = sStoringValue;

                    sValueList->length = sSrcStoringSize;
                }
            }
        }
        else
        {
            // Nothing to do.
        }

        sDstQcmColumn = sDstQcmColumn->next;
        sValueList++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qdnTrigger::allocProcInfo( qcStatement * aStatement,
                           UInt          aTriggerUserID )
{
    qdnCreateTriggerParseTree * sParseTree;

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    idlOS::memset( &sParseTree->procInfo,
                   0,
                   ID_SIZEOF(qsxProcInfo) );

    sParseTree->procInfo.qmsMem  = QC_QMP_MEM(aStatement);
    sParseTree->procInfo.tmplate = QC_SHARED_TMPLATE(aStatement);

    // BUG-24408
    // trigger의 소유자로 body를 compile해야 한다.
    sParseTree->actionBody.userID = aTriggerUserID;

    sParseTree->actionBody.procInfo = &sParseTree->procInfo;

    aStatement->spvEnv->createProc = &sParseTree->actionBody;

    // validation 시에 잠조되기 때문에 임시로 지정한다.
    aStatement->spvEnv->createProc->stmtText = aStatement->myPlan->stmtText;
    aStatement->spvEnv->createProc->stmtTextLen = aStatement->myPlan->stmtTextLen;

    return IDE_SUCCESS;
}

IDE_RC
qdnTrigger::getTriggerSCN( smOID    aTriggerOID,
                           smSCN  * aTriggerSCN)
{
    void   * sTableHandle;

    sTableHandle = (void *)smiGetTable( aTriggerOID );
    *aTriggerSCN = smiGetRowSCN(sTableHandle);

    return IDE_SUCCESS;
}

IDE_RC
qdnTrigger::executeRenameTable( qcStatement  * aStatement,
                                qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 * Description : table 이름이 변경되었을 때 변경된 이름의 table을
 *               올바르게 참조할 수 있도록 trigger정보를 변경한다.
 *
 * Implementation :
 *   1. 기존 trigger를 parsing하여 변경된 이름의 table을 참조하도록
 *      trigger 구문을 생성한다.
 *   2. 변경된 trigger 구문을 object info에 저장
 *   3. 새로운 trigger의 cache를 생성
 *   4. SYS_TRIGGER_STRINGS_에 기존 trigger 삭제 후 새 trigger 기록
 *   5. 기존 trigger의 cache를 제거
 *
 ***********************************************************************/

    qdnCreateTriggerParseTree * sParseTree;
    qdnTriggerCache          ** sOldCaches = NULL;
    qdnTriggerCache          ** sNewCaches = NULL;
    SChar                     * sSqlStmtBuffer;
    SChar                     * sTriggerStmtBuffer;
    UInt                        sTriggerStmtBufferSize;
    vSLong                      sRowCnt;
    UInt                        sSrcOffset = 0;
    UInt                        sDstOffset = 0;
    UInt                        i;

    // SYS_TRIGGER_STRINGS_ 를 위한 관리 변수
    SChar         sSubString[QCM_TRIGGER_SUBSTRING_LEN * 2 + 2];
    SInt          sCurrPos;
    SInt          sCurrLen;
    SInt          sSeqNo;
    SInt          sSubStringCnt;
    // BUG-42992
    UInt          sNewTableNameLen = 0;
    UInt          sTriggerStmtBufAllocSize = 0;

    if( aTableInfo->triggerCount > 0 )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          &sSqlStmtBuffer)
                  != IDE_SUCCESS );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          &sTriggerStmtBuffer)
                  != IDE_SUCCESS );

        sTriggerStmtBufAllocSize = QD_MAX_SQL_LENGTH;

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( aStatement->qmxMem,
                                            qdnTriggerCache*,
                                            ID_SIZEOF(qdnTriggerCache*) * aTableInfo->triggerCount,
                                            &sOldCaches)
                  != IDE_SUCCESS );

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( aStatement->qmxMem,
                                            qdnTriggerCache*,
                                            ID_SIZEOF(qdnTriggerCache*) * aTableInfo->triggerCount,
                                            &sNewCaches)
                  != IDE_SUCCESS );

        IDE_DASSERT(sOldCaches[0] == NULL);
        IDE_DASSERT(sNewCaches[0] == NULL);

        for ( i = 0;  i < aTableInfo->triggerCount; i++ )
        {
            IDE_TEST( smiObject::getObjectTempInfo( aTableInfo->triggerInfo[i].triggerHandle,
                                                    (void**)&sOldCaches[i] )
                      != IDE_SUCCESS );

            // 1. 기존 trigger를 parsing하여 변경된 이름의 table을 참조하도록 trigger 구문을 생성한다.

            sOldCaches[i]->triggerStatement.myPlan->stmtTextLen = sOldCaches[i]->stmtTextLen;
            sOldCaches[i]->triggerStatement.myPlan->stmtText = sOldCaches[i]->stmtText;

            // parsing
            IDE_TEST( qcpManager::parseIt( &sOldCaches[i]->triggerStatement ) != IDE_SUCCESS );

            sParseTree = (qdnCreateTriggerParseTree *)sOldCaches[i]->triggerStatement.myPlan->parseTree;

            sNewTableNameLen = idlOS::strlen(aTableInfo->name);

            // BUG-42992
            if ( sOldCaches[i]->stmtTextLen + sNewTableNameLen + 1 > sTriggerStmtBufAllocSize )
            {
                // 트리거 변경 전 길이 + 변경하는 테이블 이름의 길이 + null
                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                                  SChar,
                                                  sOldCaches[i]->stmtTextLen + sNewTableNameLen + 1,
                                                  &sTriggerStmtBuffer)
                          != IDE_SUCCESS );

                sTriggerStmtBufAllocSize = sOldCaches[i]->stmtTextLen + sNewTableNameLen + 1;
            }
            else
            {
                // Nothing to do.
            }

            // trigger statement의 table 이름 전까지 복사
            idlOS::memcpy(sTriggerStmtBuffer, sOldCaches[i]->stmtText, sParseTree->tableNamePos.offset);
            sDstOffset = sSrcOffset = sParseTree->tableNamePos.offset;
            // 변경된 table 이름 복사
            idlOS::strncpy(sTriggerStmtBuffer + sDstOffset, aTableInfo->name, QC_MAX_OBJECT_NAME_LEN);
            sDstOffset += sNewTableNameLen;
            sSrcOffset += sParseTree->tableNamePos.size;
            // table 이름 뒷 부분 복사
            idlOS::memcpy(sTriggerStmtBuffer + sDstOffset,
                          sOldCaches[i]->stmtText + sSrcOffset,
                          sOldCaches[i]->stmtTextLen - sSrcOffset);
            sDstOffset += sOldCaches[i]->stmtTextLen - sSrcOffset;

            sTriggerStmtBufferSize = sDstOffset;

            // 2. 변경된 trigger 구문을 object info에 저장
            IDE_TEST( smiObject::setObjectInfo( QC_SMI_STMT( aStatement ),
                                                aTableInfo->triggerInfo[i].triggerHandle,
                                                sTriggerStmtBuffer,
                                                sTriggerStmtBufferSize )
                  != IDE_SUCCESS );

            // 3. 새로운 trigger의 cache를 생성
            IDE_TEST( allocTriggerCache( aTableInfo->triggerInfo[i].triggerHandle,
                                         aTableInfo->triggerInfo[i].triggerOID,
                                         &sNewCaches[i] )
                  != IDE_SUCCESS );

            // 4. SYS_TRIGGER_STRINGS_에 기존 trigger 삭제 후 새 trigger 기록

            idlOS::snprintf( sSqlStmtBuffer, QD_MAX_SQL_LENGTH,
                             "DELETE SYS_TRIGGER_STRINGS_ WHERE TRIGGER_OID="QCM_SQL_BIGINT_FMT,
                             QCM_OID_TO_BIGINT( aTableInfo->triggerInfo[i].triggerOID ));

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStmtBuffer,
                                         & sRowCnt )
                      != IDE_SUCCESS );

            sSubStringCnt = (sTriggerStmtBufferSize / QCM_TRIGGER_SUBSTRING_LEN ) +
                            (((sTriggerStmtBufferSize % QCM_TRIGGER_SUBSTRING_LEN) == 0) ? 0 : 1);

            // qcmTrigger::addMetaInfo에서 일부 코드 복사
            for ( sSeqNo = 0, sCurrLen = 0, sCurrPos = 0;
                  sSeqNo < sSubStringCnt;
                  sSeqNo++ )
            {
                if ( (sTriggerStmtBufferSize - sCurrPos) >=
                     QCM_TRIGGER_SUBSTRING_LEN )
                {
                    sCurrLen = QCM_TRIGGER_SUBSTRING_LEN;
                }
                else
                {
                    sCurrLen = sTriggerStmtBufferSize - sCurrPos;
                }

                // ['] 와 같은 문자를 처리하기 위하여 [\'] 와 같은
                // String으로 변환하여 복사한다.
                IDE_TEST(
                    qcmProc::prsCopyStrDupAppo ( sSubString,
                                                 sTriggerStmtBuffer + sCurrPos,
                                                 sCurrLen )
                    != IDE_SUCCESS );

                sCurrPos += sCurrLen;

                idlOS::snprintf( sSqlStmtBuffer, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_TRIGGER_STRINGS_ VALUES ("
                                 QCM_SQL_UINT32_FMT", "      // 0. TABLE_ID
                                 QCM_SQL_BIGINT_FMT", "      // 1. TRIGGER_OID
                                 QCM_SQL_INT32_FMT", "       // 2. SEQ_NO
                                 QCM_SQL_CHAR_FMT") ",       // 3. SUBSTRING
                                 aTableInfo->tableID,               // 0.
                                 QCM_OID_TO_BIGINT( aTableInfo->triggerInfo[i].triggerOID ),  // 1.
                                 sSeqNo,                            // 2.
                                 sSubString );                      // 3.

                IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                             sSqlStmtBuffer,
                                             & sRowCnt )
                          != IDE_SUCCESS );

                IDE_DASSERT( sRowCnt == 1 );
            }

            /* BUG-45516 RENAME DDL에서 Trigger String을 변경할 때, SYS_TRIGGERS_의 SUBSTRING_CNT와 STRING_LENGTH를 변경하지 않습니다.
             *  - SYS_TRIGGERS_에서 해당하는 TRIGGER_OID로 검색하고, SUBSTRING_CNT, STRING_LENGTH를 변경한다.
             */
            idlOS::snprintf( sSqlStmtBuffer, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_TRIGGERS_ SET"
                             " SUBSTRING_CNT = "QCM_SQL_UINT32_FMT","
                             " STRING_LENGTH = " QCM_SQL_UINT32_FMT" "
                             "WHERE TRIGGER_OID = "QCM_SQL_UINT32_FMT" ",
                             sSubStringCnt,
                             sTriggerStmtBufferSize,
                             QCM_OID_TO_BIGINT( aTableInfo->triggerInfo[i].triggerOID ) );

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStmtBuffer,
                                         & sRowCnt )
                      != IDE_SUCCESS );

            IDE_DASSERT( sRowCnt == 1 );
        }


        // 5. 기존 trigger의 cache를 제거
        for ( i = 0;  i < aTableInfo->triggerCount; i++ )
        {
            IDE_TEST(freeTriggerCache(sOldCaches[i]) != IDE_SUCCESS);
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // error 발생 시 trigger cache 복원
    for ( i = 0;  i < aTableInfo->triggerCount; i++ )
    {
        if( sOldCaches != NULL )
        {
            if( sOldCaches[i] != NULL )
            {
                smiObject::setObjectTempInfo(
                    aTableInfo->triggerInfo[i].triggerHandle,
                    sOldCaches[i]);
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

        if( sNewCaches != NULL )
        {
            if( sNewCaches[i] != NULL )
            {
                freeTriggerCache(sNewCaches[i]);
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

    return IDE_FAILURE;
}

IDE_RC qdnTrigger::executeCopyTable( qcStatement       * aStatement,
                                     qcmTableInfo      * aSourceTableInfo,
                                     qcmTableInfo      * aTargetTableInfo,
                                     qcNamePosition      aNamesPrefix,
                                     qdnTriggerCache *** aTargetTriggerCache )
{
/***********************************************************************
 * Description :
 *      Target Table에 Trigger를 생성한다.
 *          - Source Table의 Trigger 정보를 사용하여, Target Table의 Trigger를 생성한다. (SM)
 *              - TRIGGER_NAME에 사용자가 지정한 Prefix를 붙여서 TRIGGER_NAME을 생성한다.
 *              - Trigger Strings에 생성한 TRIGGER_NAME과 Target Table Name을 적용한다.
 *          - Meta Table에 Target Table의 Trigger 정보를 추가한다. (Meta Table)
 *              - SYS_TRIGGERS_
 *                  - 생성한 TRIGGER_NAME을 사용한다.
 *                  - SM에서 얻은 Trigger OID가 SYS_TRIGGERS_에 필요하다.
 *                  - 변경한 Trigger String으로 SUBSTRING_CNT, STRING_LENGTH를 만들어야 한다.
 *                  - LAST_DDL_TIME을 초기화한다. (SYSDATE)
 *              - SYS_TRIGGER_STRINGS_
 *                  - 갱신한 Trigger Strings을 잘라 넣는다.
 *              - SYS_TRIGGER_UPDATE_COLUMNS_
 *                  - 새로운 TABLE_ID를 이용하여 COLUMN_ID를 만들어야 한다.
 *              - SYS_TRIGGER_DML_TABLES_
 *
 * Implementation :
 *
 ***********************************************************************/

    qdnCreateTriggerParseTree  * sParseTree               = NULL;
    qdnTriggerCache            * sSrcCache                = NULL;
    qdnTriggerCache           ** sTarCache                = NULL;
    void                       * sTriggerHandle           = NULL;
    smOID                        sTriggerOID              = SMI_NULL_OID;
    SChar                      * sTriggerStmtBuffer       = NULL;
    UInt                         sTriggerStmtBufAllocSize = 0;
    UInt                         sTargetTableNameLen      = 0;
    UInt                         sTargetTriggerNameLen    = 0;
    UInt                         sSrcOffset               = 0;
    UInt                         sTarOffset               = 0;
    UInt                         i                        = 0;

    SChar                        sTargetTriggerName[QC_MAX_OBJECT_NAME_LEN + 1];
    qcStatement                  sStatement;
    idBool                       sStatementAlloced        = ID_FALSE;

    *aTargetTriggerCache = NULL;

    if ( aSourceTableInfo->triggerCount > 0 )
    {
        IDE_TEST( qcg::allocStatement( & sStatement,
                                       NULL,
                                       NULL,
                                       NULL )
                  != IDE_SUCCESS );
        sStatementAlloced = ID_TRUE;

        IDU_FIT_POINT( "qdnTrigger::executeCopyTable::STRUCT_ALLOC_WITH_SIZE::sTriggerStmtBuffer",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sTriggerStmtBuffer )
                  != IDE_SUCCESS );

        sTriggerStmtBufAllocSize = QD_MAX_SQL_LENGTH;

        IDU_FIT_POINT( "qdnTrigger::executeCopyTable::STRUCT_CRALLOC_WITH_SIZE::sTarCache",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                            qdnTriggerCache *,
                                            ID_SIZEOF(qdnTriggerCache *) * aSourceTableInfo->triggerCount,
                                            & sTarCache )
                  != IDE_SUCCESS );

        for ( i = 0; i < aSourceTableInfo->triggerCount; i++ )
        {
            IDE_TEST_RAISE( ( idlOS::strlen( aSourceTableInfo->triggerInfo[i].triggerName ) + aNamesPrefix.size ) >
                            QC_MAX_OBJECT_NAME_LEN,
                            ERR_TOO_LONG_OBJECT_NAME );

            // TRIGGER_NAME에 사용자가 지정한 Prefix를 붙여서 TRIGGER_NAME을 생성한다.
            QC_STR_COPY( sTargetTriggerName, aNamesPrefix );
            idlOS::strncat( sTargetTriggerName, aSourceTableInfo->triggerInfo[i].triggerName, QC_MAX_OBJECT_NAME_LEN );

            sTargetTriggerNameLen = idlOS::strlen( sTargetTriggerName );

            IDE_TEST( smiObject::getObjectTempInfo( aSourceTableInfo->triggerInfo[i].triggerHandle,
                                                    (void **) & sSrcCache )
                      != IDE_SUCCESS );

            // Parsing Trigger String
            qcg::setSmiStmt( & sStatement, QC_SMI_STMT( aStatement ) );
            qsxEnv::initialize( sStatement.spxEnv, NULL );

            sStatement.myPlan->stmtText            = sSrcCache->stmtText;
            sStatement.myPlan->stmtTextLen         = sSrcCache->stmtTextLen;
            sStatement.myPlan->encryptedText       = NULL;  /* PROJ-2550 PSM Encryption */
            sStatement.myPlan->encryptedTextLen    = 0;     /* PROJ-2550 PSM Encryption */
            sStatement.myPlan->parseTree           = NULL;
            sStatement.myPlan->plan                = NULL;
            sStatement.myPlan->graph               = NULL;
            sStatement.myPlan->scanDecisionFactors = NULL;

            IDE_TEST( qcpManager::parseIt( & sStatement ) != IDE_SUCCESS );
            sParseTree = (qdnCreateTriggerParseTree *)sStatement.myPlan->parseTree;

            sTargetTableNameLen = idlOS::strlen( aTargetTableInfo->name );

            if ( ( sSrcCache->stmtTextLen + sTargetTriggerNameLen + sTargetTableNameLen + 1 ) >
                 sTriggerStmtBufAllocSize )
            {
                // 변경 전 Trigger 길이 + 변경 후 Trigger Name 길이 + 변경 후 Table Name 길이 + '\0'
                sTriggerStmtBufAllocSize = sSrcCache->stmtTextLen + sTargetTriggerNameLen + sTargetTableNameLen + 1;

                IDU_FIT_POINT( "qdnTrigger::executeCopyTable::STRUCT_ALLOC_WITH_SIZE::sTriggerStmtBuffer-2",
                               idERR_ABORT_InsufficientMemory );

                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                                  SChar,
                                                  sTriggerStmtBufAllocSize,
                                                  & sTriggerStmtBuffer )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            // Trigger Statement의 Trigger Name 전까지 복사
            idlOS::memcpy( sTriggerStmtBuffer,
                           sSrcCache->stmtText,
                           sParseTree->triggerNamePos.offset );
            sTarOffset = sSrcOffset = sParseTree->triggerNamePos.offset;

            // 변경 후 Trigger Name 복사
            idlOS::strncpy( sTriggerStmtBuffer + sTarOffset,
                            sTargetTriggerName,
                            QC_MAX_OBJECT_NAME_LEN );
            sTarOffset += sTargetTriggerNameLen;
            sSrcOffset += sParseTree->triggerNamePos.size;

            // Trigger Statement의 Table Name 전까지 복사
            idlOS::memcpy( sTriggerStmtBuffer + sTarOffset,
                           sSrcCache->stmtText + sSrcOffset,
                           sParseTree->tableNamePos.offset - sSrcOffset );
            sTarOffset += sParseTree->tableNamePos.offset - sSrcOffset;
            sSrcOffset = sParseTree->tableNamePos.offset;

            // 변경 후 Table Name 복사
            idlOS::strncpy( sTriggerStmtBuffer + sTarOffset,
                            aTargetTableInfo->name,
                            QC_MAX_OBJECT_NAME_LEN );
            sTarOffset += sTargetTableNameLen;
            sSrcOffset += sParseTree->tableNamePos.size;

            // Table Name 뒷 부분 복사
            idlOS::memcpy( sTriggerStmtBuffer + sTarOffset,
                           sSrcCache->stmtText + sSrcOffset,
                           sSrcCache->stmtTextLen - sSrcOffset );
            sTarOffset += sSrcCache->stmtTextLen - sSrcOffset;

            sTriggerStmtBuffer[sTarOffset] = '\0';

            /* Source Table의 Trigger 정보를 사용하여, Target Table의 Trigger를 생성한다. (SM) */
            sStatement.myPlan->stmtText    = sTriggerStmtBuffer;
            sStatement.myPlan->stmtTextLen = sTarOffset;

            IDE_TEST( createTriggerObject( & sStatement,
                                           & sTriggerHandle )
                      != IDE_SUCCESS );

            sTriggerOID = smiGetTableId( sTriggerHandle ) ;

            IDE_TEST( allocTriggerCache( sTriggerHandle,
                                         sTriggerOID,
                                         & sTarCache[i] )
                      != IDE_SUCCESS );

            /* Meta Table에 Target Table의 Trigger 정보를 추가한다. (Meta Table) */
            IDE_TEST( qcmTrigger::copyTriggerMetaInfo( aStatement,
                                                       aTargetTableInfo->tableID,
                                                       sTriggerOID,
                                                       sTargetTriggerName,
                                                       aSourceTableInfo->tableID,
                                                       aSourceTableInfo->triggerInfo[i].triggerOID,
                                                       sTriggerStmtBuffer,
                                                       sTarOffset )
                      != IDE_SUCCESS );
        }

        sStatementAlloced = ID_FALSE;
        IDE_TEST( qcg::freeStatement( & sStatement ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    *aTargetTriggerCache = sTarCache;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    if ( sStatementAlloced == ID_TRUE )
    {
        (void)qcg::freeStatement( & sStatement );
    }
    else
    {
        /* Nothing to do */
    }

    freeTriggerCacheArray( sTarCache,
                           aSourceTableInfo->triggerCount );

    return IDE_FAILURE;
}

IDE_RC qdnTrigger::executeSwapTable( qcStatement       * aStatement,
                                     qcmTableInfo      * aSourceTableInfo,
                                     qcmTableInfo      * aTargetTableInfo,
                                     qcNamePosition      aNamesPrefix,
                                     qdnTriggerCache *** aOldSourceTriggerCache,
                                     qdnTriggerCache *** aOldTargetTriggerCache,
                                     qdnTriggerCache *** aNewSourceTriggerCache,
                                     qdnTriggerCache *** aNewTargetTriggerCache )
{
/***********************************************************************
 * Description :
 *      변경한 Trigger Name과 교환한 Table Name을 Trigger Strings에 반영하고 Trigger를 재생성한다.
 *          - Prefix를 지정한 경우, Source의 TRIGGER_NAME에 Prefix를 붙이고,
 *            Target의 TRIGGER_NAME에서 Prefix를 제거한다. (Meta Table)
 *              - SYS_TRIGGERS_
 *                  - TRIGGER_NAME을 변경한다.
 *                  - 변경한 Trigger String으로 SUBSTRING_CNT, STRING_LENGTH를 만들어야 한다.
 *                  - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *          - Trigger Strings에 변경한 Trigger Name과 교환한 Table Name을 적용한다. (SM, Meta Table, Meta Cache)
 *              - Trigger Object의 Trigger 생성 구문을 변경한다. (SM)
 *                  - Call : smiObject::setObjectInfo()
 *              - New Trigger Cache를 생성하고 SM에 등록한다. (Meta Cache)
 *                  - Call : qdnTrigger::allocTriggerCache()
 *              - Trigger Strings을 보관하는 Meta Table을 갱신한다. (Meta Table)
 *                  - SYS_TRIGGER_STRINGS_
 *                      - DELETE & INSERT로 처리한다.
 *          - Trigger를 동작시키는 Column 정보에는 변경 사항이 없다.
 *              - SYS_TRIGGER_UPDATE_COLUMNS_
 *          - 다른 Trigger가 Cycle Check에 사용하는 정보를 갱신한다. (Meta Table)
 *              - SYS_TRIGGER_DML_TABLES_
 *                  - DML_TABLE_ID = Table ID 이면, (TABLE_ID와 무관하게) DML_TABLE_ID를 Peer의 Table ID로 교체한다.
 *          - 참조 Call : qdnTrigger::executeRenameTable()
 *
 * Implementation :
 *
 ***********************************************************************/

    qdnTriggerCache            * sTriggerCache            = NULL;
    qdnTriggerCache           ** sOldSrcCache             = NULL;
    qdnTriggerCache           ** sOldTarCache             = NULL;
    qdnTriggerCache           ** sNewSrcCache             = NULL;
    qdnTriggerCache           ** sNewTarCache             = NULL;
    SChar                      * sTriggerStmtBuffer       = NULL;
    UInt                         sTriggerStmtBufAllocSize = 0;
    SChar                      * sPeerTableNameStr        = NULL;
    UInt                         sPeerTableNameLen        = 0;
    UInt                         sNewTriggerNameLen       = 0;
    UInt                         i                        = 0;

    SChar                        sNewTriggerName[QC_MAX_OBJECT_NAME_LEN + 1];

    *aOldSourceTriggerCache = NULL;
    *aOldTargetTriggerCache = NULL;
    *aNewSourceTriggerCache = NULL;
    *aNewTargetTriggerCache = NULL;

    IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_ALLOC_WITH_SIZE::sTriggerStmtBuffer",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sTriggerStmtBuffer )
              != IDE_SUCCESS );

    sTriggerStmtBufAllocSize = QD_MAX_SQL_LENGTH;

    if ( aSourceTableInfo->triggerCount > 0 )
    {
        IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_CRALLOC_WITH_SIZE::sOldSrcCache",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                            qdnTriggerCache *,
                                            ID_SIZEOF(qdnTriggerCache *) * aSourceTableInfo->triggerCount,
                                            & sOldSrcCache )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_CRALLOC_WITH_SIZE::sNewSrcCache",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                            qdnTriggerCache *,
                                            ID_SIZEOF(qdnTriggerCache *) * aSourceTableInfo->triggerCount,
                                            & sNewSrcCache )
                  != IDE_SUCCESS );

        sPeerTableNameStr = aTargetTableInfo->name;
        sPeerTableNameLen = idlOS::strlen( sPeerTableNameStr );

        for ( i = 0; i < aSourceTableInfo->triggerCount; i++ )
        {
            // TRIGGER_NAME에 사용자가 지정한 Prefix를 붙여서 TRIGGER_NAME을 생성한다.
            if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
            {
                IDE_TEST_RAISE( ( idlOS::strlen( aSourceTableInfo->triggerInfo[i].triggerName ) + aNamesPrefix.size ) >
                                QC_MAX_OBJECT_NAME_LEN,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                QC_STR_COPY( sNewTriggerName, aNamesPrefix );
                idlOS::strncat( sNewTriggerName, aSourceTableInfo->triggerInfo[i].triggerName, QC_MAX_OBJECT_NAME_LEN );
            }
            else
            {
                idlOS::strncpy( sNewTriggerName,
                                aSourceTableInfo->triggerInfo[i].triggerName,
                                QC_MAX_OBJECT_NAME_LEN + 1 );
            }

            sNewTriggerNameLen = idlOS::strlen( sNewTriggerName );

            IDE_TEST( smiObject::getObjectTempInfo( aSourceTableInfo->triggerInfo[i].triggerHandle,
                                                    (void **) & sTriggerCache )
                      != IDE_SUCCESS );
            sOldSrcCache[i] = sTriggerCache;

            if ( ( sTriggerCache->stmtTextLen + sNewTriggerNameLen + sPeerTableNameLen + 1 ) >
                 sTriggerStmtBufAllocSize )
            {
                // 변경 전 Trigger 길이 + 변경 후 Trigger Name 길이 + 변경 후 Table Name 길이 + '\0'
                sTriggerStmtBufAllocSize = sTriggerCache->stmtTextLen + sNewTriggerNameLen + sPeerTableNameLen + 1;

                IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_ALLOC_WITH_SIZE::sTriggerStmtBuffer-2",
                               idERR_ABORT_InsufficientMemory );

                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                                  SChar,
                                                  sTriggerStmtBufAllocSize,
                                                  & sTriggerStmtBuffer )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( makeNewTriggerStringForSwap( aStatement,
                                                   sTriggerCache,
                                                   sTriggerStmtBuffer,
                                                   aSourceTableInfo->triggerInfo[i].triggerHandle,
                                                   aSourceTableInfo->triggerInfo[i].triggerOID,
                                                   aSourceTableInfo->tableID,
                                                   sNewTriggerName,
                                                   sNewTriggerNameLen,
                                                   sPeerTableNameStr,
                                                   sPeerTableNameLen,
                                                   & sNewSrcCache[i] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aTargetTableInfo->triggerCount > 0 )
    {
        IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_CRALLOC_WITH_SIZE::sOldTarCache",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                            qdnTriggerCache *,
                                            ID_SIZEOF(qdnTriggerCache *) * aTargetTableInfo->triggerCount,
                                            & sOldTarCache )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_CRALLOC_WITH_SIZE::sNewTarCache",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_CRALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                            qdnTriggerCache *,
                                            ID_SIZEOF(qdnTriggerCache *) * aTargetTableInfo->triggerCount,
                                            & sNewTarCache )
                  != IDE_SUCCESS );

        sPeerTableNameStr = aSourceTableInfo->name;
        sPeerTableNameLen = idlOS::strlen( sPeerTableNameStr );

        for ( i = 0; i < aTargetTableInfo->triggerCount; i++ )
        {
            // TRIGGER_NAME에서 사용자가 지정한 Prefix를 제거한 TRIGGER_NAME을 생성한다.
            if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
            {
                IDE_TEST_RAISE( idlOS::strlen( aTargetTableInfo->triggerInfo[i].triggerName ) <=
                                (UInt)aNamesPrefix.size,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                IDE_TEST_RAISE( idlOS::strMatch( aTargetTableInfo->triggerInfo[i].triggerName,
                                                 aNamesPrefix.size,
                                                 aNamesPrefix.stmtText + aNamesPrefix.offset,
                                                 aNamesPrefix.size ) != 0,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                idlOS::strncpy( sNewTriggerName,
                                aTargetTableInfo->triggerInfo[i].triggerName + aNamesPrefix.size,
                                QC_MAX_OBJECT_NAME_LEN + 1 );
            }
            else
            {
                idlOS::strncpy( sNewTriggerName,
                                aTargetTableInfo->triggerInfo[i].triggerName,
                                QC_MAX_OBJECT_NAME_LEN + 1 );
            }

            sNewTriggerNameLen = idlOS::strlen( sNewTriggerName );

            IDE_TEST( smiObject::getObjectTempInfo( aTargetTableInfo->triggerInfo[i].triggerHandle,
                                                    (void **) & sTriggerCache )
                      != IDE_SUCCESS );
            sOldTarCache[i] = sTriggerCache;

            if ( ( sTriggerCache->stmtTextLen + sNewTriggerNameLen + sPeerTableNameLen + 1 ) >
                 sTriggerStmtBufAllocSize )
            {
                // 변경 전 Trigger 길이 + 변경 후 Trigger Name 길이 + 변경 후 Table Name 길이 + '\0'
                sTriggerStmtBufAllocSize = sTriggerCache->stmtTextLen + sNewTriggerNameLen + sPeerTableNameLen + 1;

                IDU_FIT_POINT( "qdnTrigger::executeSwapTable::STRUCT_ALLOC_WITH_SIZE::sTriggerStmtBuffer-3",
                               idERR_ABORT_InsufficientMemory );

                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                                  SChar,
                                                  sTriggerStmtBufAllocSize,
                                                  & sTriggerStmtBuffer )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( makeNewTriggerStringForSwap( aStatement,
                                                   sTriggerCache,
                                                   sTriggerStmtBuffer,
                                                   aTargetTableInfo->triggerInfo[i].triggerHandle,
                                                   aTargetTableInfo->triggerInfo[i].triggerOID,
                                                   aTargetTableInfo->tableID,
                                                   sNewTriggerName,
                                                   sNewTriggerNameLen,
                                                   sPeerTableNameStr,
                                                   sPeerTableNameLen,
                                                   & sNewTarCache[i] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* SYS_TRIGGERS_의 TRIGGER_NAME을 변경한다. */
    IDE_TEST( qcmTrigger::renameTriggerMetaInfoForSwap( aStatement,
                                                        aSourceTableInfo,
                                                        aTargetTableInfo,
                                                        aNamesPrefix )
              != IDE_SUCCESS );

    /* 다른 Trigger가 Cycle Check에 사용하는 정보를 갱신한다. (Meta Table) */
    IDE_TEST( qcmTrigger::swapTriggerDMLTablesMetaInfo( aStatement,
                                                        aTargetTableInfo->tableID,
                                                        aSourceTableInfo->tableID )
              != IDE_SUCCESS );

    *aOldSourceTriggerCache = sOldSrcCache;
    *aOldTargetTriggerCache = sOldTarCache;
    *aNewSourceTriggerCache = sNewTarCache;
    *aNewTargetTriggerCache = sNewSrcCache;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NAMES_PREFIX_IS_TOO_LONG )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    freeTriggerCacheArray( sNewSrcCache,
                           aSourceTableInfo->triggerCount );

    freeTriggerCacheArray( sNewTarCache,
                           aTargetTableInfo->triggerCount );

    restoreTempInfo( sOldSrcCache,
                     aSourceTableInfo );

    restoreTempInfo( sOldTarCache,
                     aTargetTableInfo );

    return IDE_FAILURE;
}

IDE_RC qdnTrigger::makeNewTriggerStringForSwap( qcStatement      * aStatement,
                                                qdnTriggerCache  * aTriggerCache,
                                                SChar            * aTriggerStmtBuffer,
                                                void             * aTriggerHandle,
                                                smOID              aTriggerOID,
                                                UInt               aTableID,
                                                SChar            * aNewTriggerName,
                                                UInt               aNewTriggerNameLen,
                                                SChar            * aPeerTableNameStr,
                                                UInt               aPeerTableNameLen,
                                                qdnTriggerCache ** aNewTriggerCache )
{
    qdnCreateTriggerParseTree  * sParseTree               = NULL;
    UInt                         sOldOffset               = 0;
    UInt                         sNewOffset               = 0;

    qcStatement                  sStatement;
    idBool                       sStatementAlloced        = ID_FALSE;

    IDE_TEST( qcg::allocStatement( & sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sStatementAlloced = ID_TRUE;

    // Parsing Trigger String
    qcg::setSmiStmt( & sStatement, QC_SMI_STMT( aStatement ) );
    qsxEnv::initialize( sStatement.spxEnv, NULL );

    sStatement.myPlan->stmtText            = aTriggerCache->stmtText;
    sStatement.myPlan->stmtTextLen         = aTriggerCache->stmtTextLen;
    sStatement.myPlan->encryptedText       = NULL;  /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->encryptedTextLen    = 0;     /* PROJ-2550 PSM Encryption */
    sStatement.myPlan->parseTree           = NULL;
    sStatement.myPlan->plan                = NULL;
    sStatement.myPlan->graph               = NULL;
    sStatement.myPlan->scanDecisionFactors = NULL;

    IDE_TEST( qcpManager::parseIt( & sStatement ) != IDE_SUCCESS );
    sParseTree = (qdnCreateTriggerParseTree *)sStatement.myPlan->parseTree;

    // Trigger Statement의 Trigger Name 전까지 복사
    idlOS::memcpy( aTriggerStmtBuffer,
                   aTriggerCache->stmtText,
                   sParseTree->triggerNamePos.offset );
    sNewOffset = sOldOffset = sParseTree->triggerNamePos.offset;

    // 변경 후 Trigger Name 복사
    idlOS::strncpy( aTriggerStmtBuffer + sNewOffset,
                    aNewTriggerName,
                    QC_MAX_OBJECT_NAME_LEN );
    sNewOffset += aNewTriggerNameLen;
    sOldOffset += sParseTree->triggerNamePos.size;

    // Trigger Statement의 Table Name 전까지 복사
    idlOS::memcpy( aTriggerStmtBuffer + sNewOffset,
                   aTriggerCache->stmtText + sOldOffset,
                   sParseTree->tableNamePos.offset - sOldOffset );
    sNewOffset += sParseTree->tableNamePos.offset - sOldOffset;
    sOldOffset = sParseTree->tableNamePos.offset;

    // 변경 후 Table Name 복사
    idlOS::strncpy( aTriggerStmtBuffer + sNewOffset,
                    aPeerTableNameStr,
                    QC_MAX_OBJECT_NAME_LEN );
    sNewOffset += aPeerTableNameLen;
    sOldOffset += sParseTree->tableNamePos.size;

    // Table Name 뒷 부분 복사
    idlOS::memcpy( aTriggerStmtBuffer + sNewOffset,
                   aTriggerCache->stmtText + sOldOffset,
                   aTriggerCache->stmtTextLen - sOldOffset );
    sNewOffset += aTriggerCache->stmtTextLen - sOldOffset;

    aTriggerStmtBuffer[sNewOffset] = '\0';

    /* Trigger Object의 Trigger 생성 구문을 변경한다. (SM) */
    IDE_TEST( smiObject::setObjectInfo( QC_SMI_STMT( aStatement ),
                                        aTriggerHandle,
                                        aTriggerStmtBuffer,
                                        sNewOffset )
              != IDE_SUCCESS );

    /* New Trigger Cache를 생성하고 SM에 등록한다. (Meta Cache) */
    IDE_TEST( allocTriggerCache( aTriggerHandle,
                                 aTriggerOID,
                                 aNewTriggerCache )
              != IDE_SUCCESS );

    /* Trigger Strings을 보관하는 Meta Table을 갱신한다. (Meta Table) */
    IDE_TEST( qcmTrigger::updateTriggerStringsMetaInfo( aStatement,
                                                        aTableID,
                                                        aTriggerOID,
                                                        sNewOffset )
              != IDE_SUCCESS );

    IDE_TEST( qcmTrigger::removeTriggerStringsMetaInfo( aStatement,
                                                        aTriggerOID )
              != IDE_SUCCESS );

    IDE_TEST( qcmTrigger::insertTriggerStringsMetaInfo( aStatement,
                                                        aTableID,
                                                        aTriggerOID,
                                                        aTriggerStmtBuffer,
                                                        sNewOffset )
              != IDE_SUCCESS );

    sStatementAlloced = ID_FALSE;
    IDE_TEST( qcg::freeStatement( & sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStatementAlloced == ID_TRUE )
    {
        (void)qcg::freeStatement( & sStatement );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC qdnTrigger::convertXlobToXlobValue( mtcTemplate       * aTemplate,
                                           smiTableCursor    * aTableCursor,
                                           void              * aTableRow,
                                           scGRID              aGRID,
                                           mtcColumn         * aSrcLobColumn,
                                           void              * aSrcValue,
                                           mtcColumn         * aDestLobColumn,
                                           void              * aDestValue,
                                           qcmTriggerRefType   aRefType )
{
    smLobLocator         sLocator;
    idBool               sOpened = ID_FALSE;
    UInt                 sInfo = 0;

    mtdLobType         * sLobValue;
    UInt                 sLobLength;
    UInt                 sReadLength;
    idBool               sIsNull;

    IDE_TEST_RAISE( !( (aDestLobColumn->module->id == MTD_BLOB_ID) ||
                       (aDestLobColumn->module->id == MTD_CLOB_ID) ),
                    ERR_CONVERT );

    if ( (aSrcLobColumn->module->id == MTD_BLOB_LOCATOR_ID) ||
         (aSrcLobColumn->module->id == MTD_CLOB_LOCATOR_ID) )
    {
        sLocator = *(smLobLocator *)aSrcValue;
    }
    else if ( (aSrcLobColumn->module->id == MTD_BLOB_ID) ||
              (aSrcLobColumn->module->id == MTD_CLOB_ID) )
    {
        if ( (aSrcLobColumn->column.flag & SMI_COLUMN_STORAGE_MASK)
             == SMI_COLUMN_STORAGE_MEMORY )
        {
            IDE_TEST( mtc::openLobCursorWithRow( aTableCursor,
                                                 aTableRow,
                                                 & aSrcLobColumn->column,
                                                 sInfo,
                                                 SMI_LOB_READ_MODE,
                                                 & sLocator )
                      != IDE_SUCCESS );

            sOpened = ID_TRUE;
        }
        else
        {
            // BUG-16318
            IDE_TEST_RAISE( SC_GRID_IS_NULL(aGRID), ERR_NOT_APPLICABLE );
            //fix BUG-19687

            if( aRefType == QCM_TRIGGER_REF_OLD_ROW )
            {
                /* BUG-43961 trigger를 통해 lob에 접근할 경우 old reference를 사용시
                   old 값을 정상적으로 가져오기 위해서는 READ mode로 접근해야 한다. */ 
                IDE_TEST( mtc::openLobCursorWithGRID( aTableCursor,
                                                      aGRID,
                                                      & aSrcLobColumn->column,
                                                      sInfo,
                                                      SMI_LOB_READ_MODE,
                                                      & sLocator )
                          != IDE_SUCCESS );
            }
            else
            {
                /* BUG-43838 trigger를 통해 lob에 접근할 경우 lob cursor를 
                   READ_WRITE mode로 접근해야 정상적으로 값을 가져올 수 있다. */
                IDE_TEST( mtc::openLobCursorWithGRID( aTableCursor,
                                                      aGRID,
                                                      & aSrcLobColumn->column,
                                                      sInfo,
                                                      SMI_LOB_READ_WRITE_MODE,
                                                      & sLocator )
                          != IDE_SUCCESS );

            }

            sOpened = ID_TRUE;
        }
    }
    else
    {
        IDE_RAISE( ERR_CONVERT );
    }

    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLobLength,
                                        mtc::getStatistics(aTemplate) ) != IDE_SUCCESS );

    if ( sIsNull == ID_TRUE )
    {
        aDestLobColumn->module->null( aDestLobColumn, aDestValue );
    }
    else
    {
        IDE_TEST_RAISE( (UInt)aDestLobColumn->precision < sLobLength, ERR_CONVERT );

        sLobValue = (mtdLobType *)aDestValue;

        IDE_TEST( mtc::readLob( NULL,               /* idvSQL* */
                                sLocator,
                                0,
                                sLobLength,
                                sLobValue->value,
                                & sReadLength )
                  != IDE_SUCCESS );

        sLobValue->length = (SLong)sReadLength;
    }

    if ( sOpened == ID_TRUE )
    {
        sOpened = ID_FALSE;
        IDE_TEST( aTemplate->closeLobLocator( sLocator )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION( ERR_CONVERT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_DISABLE ) );
    }
    IDE_EXCEPTION_END;

    if ( sOpened == ID_TRUE )
    {
        (void) aTemplate->closeLobLocator( sLocator );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

// PROJ-2219 Row-level before update trigger
// Row-level before update trigger에서 new row를 통해
// 참조하는 column list를 생성한다.
IDE_RC qdnTrigger::makeRefColumnList( qcStatement * aQcStmt )
{
    qdnCreateTriggerParseTree * sParseTree;
    qdnTriggerRef             * sRef;
    qtcNode                   * sNode;
    UChar                     * sRefColumnList  = NULL;
    UInt                        sRefColumnCount = 0;
    UInt                        sTableColumnCount;
    UInt                        i;

    sParseTree = (qdnCreateTriggerParseTree *)(aQcStmt->myPlan->parseTree);

    IDE_ERROR( sParseTree != NULL );
    IDE_ERROR( sParseTree->refColumnCount == 0 );
    IDE_ERROR( sParseTree->refColumnList  == NULL );

    sTableColumnCount = sParseTree->tableInfo->columnCount;

    // row-level before update trigger
    if ( ( sParseTree->actionCond.actionGranularity == QCM_TRIGGER_ACTION_EACH_ROW ) &&
         ( sParseTree->triggerEvent.eventTime == QCM_TRIGGER_BEFORE ) &&
         ( ( sParseTree->triggerEvent.eventTypeList->eventType & QCM_TRIGGER_EVENT_UPDATE ) != 0 ) )
    {
        for ( sRef = sParseTree->triggerReference;
              sRef != NULL;
              sRef = sRef->next )
        {
            if ( sRef->refType == QCM_TRIGGER_REF_NEW_ROW )
            {
                // sRefColumnList는 무조건 NULL인 상태이다.
                // NEW ROW는 한 개만 존재하기 때문이다.
                IDE_ERROR( sRefColumnList == NULL );

                IDU_FIT_POINT( "qdnTrigger::makeRefColumnList::alloc::sRefColumnList",
                                idERR_ABORT_InsufficientMemory );

                IDE_TEST( QC_QMP_MEM( aQcStmt )->alloc(
                        ID_SIZEOF(UChar) * sTableColumnCount,
                        (void**)&sRefColumnList )
                    != IDE_SUCCESS);

                sNode = ((qtcNode *) sRef->rowVar->variableTypeNode->node.arguments);

                for ( i = 0; i < sTableColumnCount; i++ )
                {
                    IDE_ERROR( sNode != NULL );

                    if ( (sNode->lflag & QTC_NODE_VALIDATE_MASK) == QTC_NODE_VALIDATE_TRUE )
                    {
                        sRefColumnList[i] = QDN_REF_COLUMN_TRUE;
                        sRefColumnCount++;
                    }
                    else
                    {
                        sRefColumnList[i] = QDN_REF_COLUMN_FALSE;
                    }

                    sNode = (qtcNode *)sNode->node.next;
                }

                sParseTree->refColumnList  = sRefColumnList;
                sParseTree->refColumnCount = sRefColumnCount;
                break;
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
}

// PROJ-2219 Row-level before update trigger
// Invalid 한 trigger가 있으면 compile하고, DML을 rebuild 한다.
IDE_RC qdnTrigger::verifyTriggers( qcStatement   * aQcStmt,
                                   qcmTableInfo  * aTableInfo,
                                   smiColumnList * aUptColumn,
                                   idBool        * aIsNeedRebuild )
{
    qcmTriggerInfo  * sTriggerInfo;
    qdnTriggerCache * sTriggerCache;
    idBool            sNeedAction;
    UInt              i;
    UInt              sStage = 0;

    IDE_ERROR( aIsNeedRebuild != NULL );

    *aIsNeedRebuild = ID_FALSE;

    for ( i = 0; i < aTableInfo->triggerCount; i++ )
    {
        sTriggerInfo = &aTableInfo->triggerInfo[i];

        IDE_TEST( checkCondition( aQcStmt,
                                  & aTableInfo->triggerInfo[i],
                                  QCM_TRIGGER_ACTION_EACH_ROW,
                                  QCM_TRIGGER_BEFORE,
                                  QCM_TRIGGER_EVENT_UPDATE,
                                  aUptColumn,
                                  & sNeedAction,
                                  aIsNeedRebuild )
                  != IDE_SUCCESS );

        if ( sNeedAction == ID_TRUE )
        {
            IDE_TEST( smiObject::getObjectTempInfo( sTriggerInfo->triggerHandle,
                                                    (void**)&sTriggerCache)
                      != IDE_SUCCESS );;

            while ( 1 )
            {
                // S Latch 획득
                IDE_TEST( sTriggerCache->latch.lockRead (
                              NULL, /* idvSQL * */
                              NULL /* idvWeArgs* */ )
                          != IDE_SUCCESS );
                sStage = 1;

                if ( sTriggerCache->isValid != ID_TRUE )
                {
                    // Valid하지 않으면 Recompile 한다.

                    // Release Latch
                    sStage = 0;
                    IDE_TEST( sTriggerCache->latch.unlock() != IDE_SUCCESS );

                    while ( recompileTrigger( aQcStmt,
                                              sTriggerInfo )
                            != IDE_SUCCESS )
                    {
                        // rebuild error라면 다시 recompile을 시도한다.
                        IDE_TEST( ideIsRebuild() != IDE_SUCCESS );
                    }

                    *aIsNeedRebuild = ID_TRUE;

                    continue;
                }
                else
                {
                    // Release Latch
                    sStage = 0;
                    IDE_TEST( sTriggerCache->latch.unlock() != IDE_SUCCESS );

                    break;
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void) sTriggerCache->latch.unlock();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

// BUG-38137 Trigger의 when condition에서 PSM을 호출할 수 없다.
IDE_RC qdnTrigger::checkNoSpFunctionCall( qtcNode * aNode )
{
    qtcNode * sNode;

    for ( sNode = aNode; sNode != NULL; sNode = (qtcNode*)sNode->node.next )
    {
        IDE_TEST( ( sNode->lflag & QTC_NODE_PROC_FUNCTION_MASK )
                  == QTC_NODE_PROC_FUNCTION_TRUE );

        if ( sNode->node.arguments != NULL )
        {
            IDE_TEST( checkNoSpFunctionCall( (qtcNode*)(sNode->node.arguments ) )
                      != IDE_SUCCESS );

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
