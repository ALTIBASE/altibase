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
 * $Id: qsxCursor.cpp 87536 2020-05-18 06:56:41Z khkwak $
 **********************************************************************/

/*
  NAME
  qsxCursor.cpp

  DESCRIPTION

  PUBLIC FUNCTION(S)

  PRIVATE FUNCTION(S)

  NOTES

  MODIFIED   (MM/DD/YY)
*/
#include <idl.h>
#include <idu.h>

#include <smi.h>

#include <qcuProperty.h>
#include <qcuError.h>
#include <qc.h>
#include <qsParseTree.h>
#include <qmn.h>
#include <qmo.h>
#include <qmx.h>
#include <qsx.h>
#include <qsxDef.h>
#include <qsxUtil.h>
#include <qsxCursor.h>
#include <qsxExecutor.h>
#include <qsxArray.h>
#include <qmnProject.h>
#include <qsvEnv.h>

IDE_RC qsxCursor::initialize (
    qsxCursorInfo            * aCurInfo,
    idBool                     aIsSQLCursor )
{
#define IDE_FN "IDE_RC qsxCursor::initialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    QSX_CURSOR_INFO_INIT( aCurInfo );

    if ( aIsSQLCursor == ID_TRUE )
    {
        QSX_CURSOR_SET_SQL_CURSOR_TRUE( aCurInfo );
    }
    else
    {
        QSX_CURSOR_SET_SQL_CURSOR_FALSE( aCurInfo );
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qsxCursor::finalize( qsxCursorInfo * aCurInfo,
                            qcStatement   * aQcStmt )
{
#define IDE_FN "IDE_RC qsxCursor::finalize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ( QSX_CURSOR_IS_OPEN( aCurInfo ) == ID_TRUE )
    {
        IDE_TEST ( close( aCurInfo, aQcStmt )
                   != IDE_SUCCESS );
    }

    // BUG-38767
    if ( QSX_CURSOR_IS_NEED_STMT_FREE( aCurInfo ) == ID_TRUE )
    {
        IDE_TEST( qcd::freeStmt( aCurInfo->hStmt,
                                 ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    QSX_CURSOR_INFO_INIT( aCurInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxCursor::setCursorSpec (
    qsxCursorInfo            * aCurInfo,
    qsVariableItems          * aCursorParaDecls )
{
#define IDE_FN "IDE_RC qsxCursor::setCursorSpec"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aCurInfo-> mCursorParaDecls   = aCursorParaDecls;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qsxCursor::open ( qsxCursorInfo   * aCurInfo,
                         qcStatement     * aQcStmt,
                         qtcNode         * aOpenArgNodes,
                         qcTemplate      * aCursorTemplate,
                         UInt              aSqlIdx,
                         idBool            aIsDefiner )
{
    qtcNode         * sOpenArg;
    qsVariableItems * sOpenParam;
    qsVariables     * sParaVar;
    qciStmtType       sStmtType;
    qsUsingParam    * sUsingParam;
    mtcColumn       * sMtcColumn;
    UShort            sBindParamId = 0;
    void            * sValue;
    UInt              sValueSize;
    vSLong            sAffectedRowCount;
    idBool            sResultSetExist;
    idBool            sNextRecordExist;
    UInt              sUserID       = QC_EMPTY_USER_ID;
    qcStatement     * sExecQcStmt   = NULL;
    qsCursors       * sCursor       = NULL;
    qsxStmtList     * sStmtList     = aQcStmt->spvEnv->mStmtList;
    qsxStmtList2    * sStmtList2    = aQcStmt->spvEnv->mStmtList2;
    UInt              sStage        = 0;
    UInt              sPoolCount    = 0; 

    // BUG-37961
    IDE_DASSERT( aCurInfo->mCursor != NULL );

    sCursor       = aCurInfo->mCursor;
    sUserID       = QCG_GET_SESSION_USER_ID( aQcStmt );

    IDE_TEST_RAISE( QSX_CURSOR_IS_OPEN( aCurInfo ) == ID_TRUE, err_cursor_already_open);

    if( sCursor->common.objectID != QS_EMPTY_OID )
    {
        IDE_DASSERT( aCursorTemplate->tmplate.stackBuffer == NULL );

        QC_CONNECT_TEMPLATE_STACK(
            aCursorTemplate,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackBuffer,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackCount,
            QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain );
        aCursorTemplate->stmt = QC_PRIVATE_TMPLATE(aQcStmt)->stmt;
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-45306 PSM AUTHID */
    if ( aIsDefiner == ID_TRUE )
    { 
        /* BUG-38164
           BUG-38164 이전 aExecInfo->mUserID를 session의 userID에 설정해주었다.
           package 지원 이전에는 다른 PSM객체에 선언된 cursor를 사용할 수 없었으나,
           package 지원 후에는 다른 PSM객체(package)에 선언된 cursor를 사용 가능하다.
           따라서, 객체를 생성한 userID를 세션에 설정해주면,
           다른 package의 선언된 cursor를 참조 할 경우, cursor 선언 시 참조한 table을 찾지 못 하거나,
           객체를 생성한 유저가 소유한 동일한 이름을 가진 table에 접근하여 잘못된 결과가 나오게 된다.
           그렇게 때문에, cursor의 경우, cursor를 포함하고 있는 package의 userID를 넘겨줘야 한다. */
        QCG_SET_SESSION_USER_ID( aQcStmt,
                                 sCursor->userID );
    }
    else
    {
        // Nothing to do.
    }

    if( aCurInfo->hStmt == NULL )
    {
        // BUG-38767
        if ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) )
        {
            IDE_ERROR( aSqlIdx != ID_UINT_MAX );

            sPoolCount = aQcStmt->session->mQPSpecific.mStmtListInfo.mStmtPoolCount;

            IDU_FIT_POINT_RAISE( "qsxCursor::open::is_unused", err_sql_index );
            IDE_TEST_RAISE( aSqlIdx >= sPoolCount, err_sql_index );

            if ( sStmtList != NULL )
            {
                if ( QSX_STMT_LIST_IS_UNUSED( sStmtList->mStmtPoolStatus, aSqlIdx )
                     == ID_TRUE )
                {
                    // stmt alloc
                    IDE_TEST( qcd::allocStmt( aQcStmt,
                                              &aCurInfo->hStmt )
                              != IDE_SUCCESS );

                    QSX_CURSOR_SET_NEED_STMT_FREE_TRUE( aCurInfo );

                    sStmtList->mStmtPool[aSqlIdx] = aCurInfo->hStmt;
                }
                else
                {
                    aCurInfo->hStmt = (void*)sStmtList->mStmtPool[aSqlIdx];
                }
            }

            if ( sStmtList2 != NULL )
            {
                if ( QSX_STMT_LIST_IS_UNUSED( sStmtList2->mStmtPoolStatus, aSqlIdx )
                     == ID_TRUE )
                {
                    // stmt alloc
                    IDE_TEST( qcd::allocStmt( aQcStmt,
                                              &aCurInfo->hStmt )
                              != IDE_SUCCESS );

                    QSX_CURSOR_SET_NEED_STMT_FREE_TRUE( aCurInfo );

                    sStmtList2->mStmtPool[aSqlIdx] = aCurInfo->hStmt;
                }
                else
                {
                    aCurInfo->hStmt = (void*)sStmtList2->mStmtPool[aSqlIdx];
                }
            }
        }
        else
        {
            // stmt alloc
            IDE_TEST( qcd::allocStmt( aQcStmt,
                                      &aCurInfo->hStmt )
                      != IDE_SUCCESS );

            QSX_CURSOR_SET_NEED_STMT_FREE_TRUE( aCurInfo );
        }
    }
    else
    {
        IDE_TEST( qcd::freeStmt( aCurInfo->hStmt,
                                 ID_FALSE )
                  != IDE_SUCCESS );
    }

    aCurInfo->mRowCount = 0;
    aCurInfo->mStmtType = QCI_STMT_SELECT;

    QSX_CURSOR_SET_ROWCOUNT_NULL_FALSE( aCurInfo );
    QSX_CURSOR_SET_OPEN_FALSE( aCurInfo );

    if ( aOpenArgNodes != NULL  )
    {
        for (sOpenParam = aCurInfo->mCursorParaDecls,
             sOpenArg   = aOpenArgNodes;
             sOpenParam != NULL && sOpenArg != NULL ;
             sOpenParam = sOpenParam->next,
             sOpenArg   = (qtcNode* )sOpenArg->node.next )
        {
            sParaVar = (qsVariables*)sOpenParam;

            /* cursor의 parameter는 IN Type만 올 수 있다. */
            IDE_TEST( qsxUtil::calculateAndAssign ( QC_QMX_MEM( aQcStmt ),
                                                    sOpenArg,
                                                    QC_PRIVATE_TMPLATE(aQcStmt),
                                                    sParaVar->variableTypeNode,
                                                    aCursorTemplate,
                                                    ID_FALSE )
                      != IDE_SUCCESS );
        }
    }

    // BUG-38767
    /* PROJ-2197 PSM Renewal
     * 1. mmStmt에서 qcStmt를 얻어와서
     * 2. callDepth를 설정한다.
     * qcd를 통해 실행하면 qcStmt의 상속관계가 없기 때문에
     * stack overflow로 server가 비정상 종료할 수 있다. */
    IDE_TEST( qcd::getQcStmt( aCurInfo->hStmt,
                              &sExecQcStmt )
              != IDE_SUCCESS );

    IDE_TEST( qcd::prepare( aCurInfo->hStmt,
                            aQcStmt,
                            sExecQcStmt,
                            &sStmtType,
                            sCursor->mCursorSql->sqlText,
                            sCursor->mCursorSql->sqlTextLen,
                            ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (sStmtType != QCI_STMT_SELECT) &&
                    (sStmtType != QCI_STMT_SELECT_FOR_UPDATE) &&
                    (sStmtType != QCI_STMT_SELECT_FOR_FIXED_TABLE ),
                    err_not_query );

    // BUG-43158 Enhance statement list caching in PSM
    if ( ( ( sStmtList != NULL ) || ( sStmtList2 != NULL ) ) &&
         ( QSX_CURSOR_IS_NEED_STMT_FREE( aCurInfo ) == ID_TRUE ) )
    {
        IDU_FIT_POINT_RAISE( "qsxCursor::open::set_used", err_sql_index );
        IDE_TEST_RAISE( aSqlIdx >= sPoolCount, err_sql_index );

        QSX_CURSOR_SET_NEED_STMT_FREE_FALSE( aCurInfo );

        if ( sStmtList != NULL )
        {
            QSX_STMT_LIST_SET_USED( sStmtList->mStmtPoolStatus, aSqlIdx );
        }

        if ( sStmtList2 != NULL )
        {
            QSX_STMT_LIST_SET_USED( sStmtList2->mStmtPoolStatus, aSqlIdx );
        }
    }
    else
    {
        // Nothing to do.
    }

    for( sUsingParam = sCursor->mCursorSql->usingParams;
         sUsingParam != NULL;
         sUsingParam = sUsingParam->next )
    {
        sMtcColumn = QTC_TMPL_COLUMN( aCursorTemplate,
                                      sUsingParam->paramNode );

        IDE_TEST( qcd::bindParamInfoSet( aCurInfo->hStmt,
                                         sMtcColumn,
                                         sBindParamId,
                                         sUsingParam->inOutType )
                  != IDE_SUCCESS );

        sBindParamId++;
    }

    sBindParamId = 0;

    for( sUsingParam = sCursor->mCursorSql->usingParams;
         sUsingParam != NULL;
         sUsingParam = sUsingParam->next )
    {
        IDE_TEST( qtc::calculate( sUsingParam->paramNode,
                                  aCursorTemplate )
                  != IDE_SUCCESS );

        sMtcColumn = QTC_TMPL_COLUMN( aCursorTemplate,
                                      sUsingParam->paramNode );

        sValue = aCursorTemplate->tmplate.stack[0].value;

        sValueSize = sMtcColumn->module->actualSize(
            sMtcColumn,
            sValue );

        IDE_TEST( qcd::bindParamData( aCurInfo->hStmt,
                                      sValue,
                                      sValueSize,
                                      sBindParamId,
                                      aQcStmt->qmxMem,
                                      NULL,
                                      sUsingParam->inOutType )
                  != IDE_SUCCESS );

        sBindParamId++;
    }

    IDE_TEST( qcd::execute( aCurInfo->hStmt,
                            aQcStmt,
                            NULL,
                            &sAffectedRowCount,
                            &sResultSetExist,
                            &sNextRecordExist,
                            ID_TRUE )
              != IDE_SUCCESS );
    sStage = 1;

    if ( sNextRecordExist == ID_TRUE )
    {
        QSX_CURSOR_SET_NEXT_RECORD_EXIST_TRUE( aCurInfo );
    }
    else
    {
        QSX_CURSOR_SET_NEXT_RECORD_EXIST_FALSE( aCurInfo );
    }

    QSX_CURSOR_SET_OPEN_TRUE( aCurInfo );

    // BUG-34331
    // Cursor를 open하면 mCursorsInUse에 추가한다.
    QSX_ENV_ADD_CURSORS_IN_USE( QC_QSX_ENV(aQcStmt), aCurInfo );

    /* PROJ-2586 PSM Parameters and return without precision */
    if ( aOpenArgNodes != NULL  )
    {
        for (sOpenParam = aCurInfo->mCursorParaDecls,
             sOpenArg   = aOpenArgNodes;
             (sOpenParam != NULL) && (sOpenArg != NULL);
             sOpenParam = sOpenParam->next,
             sOpenArg   = (qtcNode* )sOpenArg->node.next )
        {
            sParaVar   = (qsVariables*)sOpenParam;
            sMtcColumn = QTC_TMPL_COLUMN( aCursorTemplate,
                                          sParaVar->variableTypeNode );

            IDE_TEST( qsxUtil::finalizeParamAndReturnColumnInfo( sMtcColumn ) != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    QCG_SET_SESSION_USER_ID( aQcStmt, sUserID );

    // BUG-37961
    if( sCursor->common.objectID != QS_EMPTY_OID )
    {
        QC_DISCONNECT_TEMPLATE_STACK( aCursorTemplate );
        aCursorTemplate->stmt = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cursor_already_open);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_CURSOR_ALREADY_OPEN));
    }
    IDE_EXCEPTION(err_not_query);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NOT_QUERY));
    }
    IDE_EXCEPTION(err_sql_index)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_INTERNAL_SERVER_ERROR_ARG,
                                 "[qsxCursor::open_sql_index_is_wrong]"));
    }
    IDE_EXCEPTION_END;

    if ( aOpenArgNodes != NULL  )
    {
        for (sOpenParam = aCurInfo->mCursorParaDecls,
             sOpenArg   = aOpenArgNodes;
             (sOpenParam != NULL) && (sOpenArg != NULL) ;
             sOpenParam = sOpenParam->next,
             sOpenArg   = (qtcNode* )sOpenArg->node.next )
        {
            sParaVar   = (qsVariables*)sOpenParam;
            sMtcColumn = QTC_TMPL_COLUMN( aCursorTemplate,
                                          sParaVar->variableTypeNode );

            (void)qsxUtil::finalizeParamAndReturnColumnInfo( sMtcColumn );
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( sStage == 1 )
    {
        (void)qcd::endFetch( aCurInfo->hStmt );
    }
    else
    {
        // Nothing to do.
    }

    QCG_SET_SESSION_USER_ID( aQcStmt, sUserID );

    // BUG-37961
    if ( sCursor->common.objectID != QS_EMPTY_OID )
    {
        QC_DISCONNECT_TEMPLATE_STACK( aCursorTemplate );
        aCursorTemplate->stmt = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsxCursor::close( qsxCursorInfo * aCurInfo,
                         qcStatement   * aQcStmt )
{
#define IDE_FN "IDE_RC qsxCursor::close"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxEnvInfo    * sQsxEnv;
    qsxCursorInfo * sCurrCurInfo;
    qsxCursorInfo * sPrevCurInfo = NULL;

    IDE_TEST_RAISE( QSX_CURSOR_IS_OPEN( aCurInfo ) != ID_TRUE, err_invalid_cursor);

    sQsxEnv = QC_QSX_ENV(aQcStmt);

    sCurrCurInfo = QSX_ENV_CURSORS_IN_USE( sQsxEnv );

    // BUG-34331
    // Cursor를 close하면 mCursorsInUse에서 제거한다.
    while( sCurrCurInfo != NULL )
    {
        if( sCurrCurInfo == aCurInfo )
        {
            if( sPrevCurInfo != NULL )
            {
                sPrevCurInfo->mNext = aCurInfo->mNext;
            }
            else // QSX_ENV_CURSORS_IN_USE( sQsxEnv ) == aCurInfo
            {
                QSX_ENV_CURSORS_IN_USE( sQsxEnv ) = aCurInfo->mNext;
            }

            // mCursorsInUseFence위치의 cursor를 닫으면
            // mCursorsInUseFence를 다음 cursor로 변경함.
            if( sQsxEnv->mCursorsInUseFence == aCurInfo )
            {
                sQsxEnv->mCursorsInUseFence = aCurInfo->mNext;
            }
            else
            {
                // Nothing to do.
            }

            aCurInfo->mNext = NULL;
            break;
        }
        else
        {
            sPrevCurInfo = sCurrCurInfo;
            sCurrCurInfo = sCurrCurInfo->mNext;
        }
    }

    IDE_TEST( qcd::endFetch( aCurInfo->hStmt )
              != IDE_SUCCESS );

    aCurInfo->mRowCount = 0;

    QSX_CURSOR_SET_ROWCOUNT_NULL_TRUE( aCurInfo );
    QSX_CURSOR_SET_OPEN_FALSE( aCurInfo );
    QSX_CURSOR_SET_END_OF_CURSOR_FALSE( aCurInfo );

    // BUG-38767
    if ( QSX_CURSOR_IS_NEED_STMT_FREE( aCurInfo ) == ID_TRUE )
    {
        IDE_TEST( qcd::freeStmt( aCurInfo->hStmt,
                                 ID_TRUE )
                  != IDE_SUCCESS );
        aCurInfo->hStmt           = NULL;
        QSX_CURSOR_SET_NEED_STMT_FREE_FALSE( aCurInfo );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_cursor);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INVALID_CURSOR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsxCursor::fetchInto (qsxCursorInfo   * aCurInfo,
                             qcStatement     * aQcStmt,
                             qmsInto         * aIntoVarNodes,
                             qmsLimit        * aLimit )
{
    mtcColumn       * sMtcColumn;
    UInt              sStage = 0;
    iduMemoryStatus   sQmxMemStatus;
    qtcNode         * sNode;
    void            * sValue;
    void            * sColumnValue;
    qciBindData     * sBindColumnDataList = NULL;
    UShort            sBindColumnId = 0;
    qcmColumn       * sQcmColumn;
    /* BUG-41242 */
    vSLong            sRowCount = 0;
    qtcNode         * sIndexNode = NULL;
    void            * sIndexValue = NULL;
    ULong             sLimitCount = ((ULong)MTD_INTEGER_MAXIMUM);
    idBool            sNextRecordExist;

    IDE_TEST_RAISE ( QSX_CURSOR_IS_OPEN( aCurInfo ) != ID_TRUE, err_invalid_cursor);

    IDE_TEST( QC_QMX_MEM(aQcStmt)-> getStatus( &sQmxMemStatus )
              != IDE_SUCCESS);
    sStage = 1;

    if ( QSX_CURSOR_IS_NEXT_RECORD_EXIST( aCurInfo ) == ID_FALSE )
    {
        QSX_CURSOR_SET_END_OF_CURSOR_TRUE( aCurInfo );
        IDE_CONT( ERR_PASS );
    }
    else
    {
        // Nothing do to.
    }

    /* BUG-41242 */
    if ( aIntoVarNodes->bulkCollect == ID_TRUE )
    { 
        if ( aLimit != NULL )
        {
            IDE_TEST( qmsLimitI::getCountValue( QC_PRIVATE_TMPLATE(aQcStmt),
                                                aLimit,
                                                &sLimitCount )
                      != IDE_SUCCESS );
            IDE_TEST_CONT( sLimitCount == 0, ERR_PASS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sLimitCount = 1;
    }

    while ( ID_TRUE )
    {
        for ( sNode = aIntoVarNodes->intoNodes;
              sNode != NULL;
              sNode = (qtcNode*)sNode->node.next )
        {
            if ( aIntoVarNodes->bulkCollect == ID_TRUE )
            {
                IDE_DASSERT( sRowCount < (vSLong)MTD_INTEGER_MAXIMUM );

                sIndexNode   = (qtcNode*)sNode->node.arguments;
                sIndexValue  = QTC_TMPL_FIXEDDATA( QC_PRIVATE_TMPLATE(aQcStmt),
                                                   sIndexNode );

                IDE_DASSERT( QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                              sIndexNode )->module->id  == MTD_INTEGER_ID );

                *(mtdIntegerType*)sIndexValue =
                    (mtdIntegerType)(sRowCount + 1);
            }
            else
            {
                // Nothing to do.
            }
        }

        sBindColumnId = 0;
        sBindColumnDataList = NULL;

        // bindColumnDataList생성
        IDE_DASSERT( aIntoVarNodes->intoNodes != NULL );

        sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                      aIntoVarNodes->intoNodes );

        if ( (sMtcColumn->type.dataTypeId == MTD_ROWTYPE_ID) ||
             (sMtcColumn->type.dataTypeId == MTD_RECORDTYPE_ID) )
        {
            IDE_TEST( qtc::calculate( aIntoVarNodes->intoNodes,
                                      QC_PRIVATE_TMPLATE(aQcStmt) )
                      != IDE_SUCCESS );

            sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

            for ( sQcmColumn = ((qtcModule*)(sMtcColumn->module))->typeInfo->columns;
                  sQcmColumn != NULL;
                  sQcmColumn = sQcmColumn->next )
            {
                sColumnValue = (void*)mtc::value( sQcmColumn->basicInfo,
                                                  sValue,
                                                  MTD_OFFSET_USE );

                IDE_TEST( qcd::addBindColumnDataList( aQcStmt->qmxMem,
                                                      &sBindColumnDataList,
                                                      sQcmColumn->basicInfo,
                                                      sColumnValue,
                                                      sBindColumnId )
                          != IDE_SUCCESS );

                sBindColumnId++;
            }

        }
        else
        {
            for ( sNode = aIntoVarNodes->intoNodes;
                  sNode != NULL;
                  sNode = (qtcNode*)sNode->node.next )
            {
                sMtcColumn = QTC_TMPL_COLUMN( QC_PRIVATE_TMPLATE(aQcStmt),
                                              sNode );

                IDE_TEST( qtc::calculate( sNode,
                                          QC_PRIVATE_TMPLATE(aQcStmt) )
                          != IDE_SUCCESS );

                sValue = QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack[0].value;

                IDE_TEST( qcd::addBindColumnDataList( aQcStmt->qmxMem,
                                                      &sBindColumnDataList,
                                                      sMtcColumn,
                                                      sValue,
                                                      sBindColumnId )
                          != IDE_SUCCESS );

                sBindColumnId++;
            }
        }

        if ( sRowCount == 0 )
        {
            // rowtype의 내부적인 column개수와 into에 명시된 column 개수가
            // 동일한지 확인
            IDE_TEST( qcd::checkBindColumnCount( aCurInfo->hStmt,
                                                 sBindColumnId )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // fetch
        IDE_TEST( qcd::fetch( aQcStmt,
                              QC_QMX_MEM(aQcStmt),
                              aCurInfo->hStmt,
                              sBindColumnDataList,
                              &sNextRecordExist )
                  != IDE_SUCCESS );

        aCurInfo->mRowCount++;

        if ( sNextRecordExist == ID_TRUE )
        {
            QSX_CURSOR_SET_NEXT_RECORD_EXIST_TRUE( aCurInfo );
        }
        else
        {
            QSX_CURSOR_SET_NEXT_RECORD_EXIST_FALSE( aCurInfo );
        }

        /* BUG-41242 */
        sRowCount++;
        sLimitCount--;

        if ( sLimitCount == 0 )
        {
            break;
        }
        else if ( sNextRecordExist == ID_FALSE )
        {
            break;
        }
        else
        {
            IDE_TEST_RAISE( sRowCount >= (vSLong)MTD_INTEGER_MAXIMUM,
                            ERR_TOO_MANY_ROWS );
        }
    }

    IDE_EXCEPTION_CONT(ERR_PASS);

    sStage=0;
    IDE_TEST( QC_QMX_MEM(aQcStmt)-> setStatus( &sQmxMemStatus )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_cursor);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_INVALID_CURSOR));
    }
    IDE_EXCEPTION(ERR_TOO_MANY_ROWS);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_TOO_MANY_ROWS));
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            (void) QC_QMX_MEM(aQcStmt)->setStatus( &sQmxMemStatus );
        default:
            break;
    }

    return IDE_FAILURE;
}
