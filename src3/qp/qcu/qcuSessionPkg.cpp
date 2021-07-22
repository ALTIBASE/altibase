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
 * $Id: qcuSessionPkg.cpp 91010 2021-06-17 01:33:11Z hykim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <qcuProperty.h>
#include <qcuSessionPkg.h>
#include <iduFileStream.h>
#include <qcg.h>
#include <qsvEnv.h>
#include <qsxCursor.h>
#include <qsxArray.h>
#include <qcuSqlSourceInfo.h>
#include <qsxUtil.h>
#include <qsParseTree.h>
#include <qcmPkg.h>

IDE_RC qcuSessionPkg::finalizeSessionPkgInfoList( qcSessionPkgInfo ** aSessionPkgInfo )
{
    qcSessionPkgInfo * sSessionPkg;
    qcSessionPkgInfo * sNext;

    sSessionPkg = *aSessionPkgInfo;
    while( sSessionPkg != NULL )
    {
        sNext = sSessionPkg->next;

        sSessionPkg->pkgInfoMem->destroy();
        (void)iduMemMgr::free( sSessionPkg->pkgInfoMem );
        (void)iduMemMgr::free( sSessionPkg );

        sSessionPkg = sNext;
    }

    *aSessionPkgInfo = NULL;

    return IDE_SUCCESS;
}

IDE_RC qcuSessionPkg::makeSessionPkgInfo( qcSessionPkgInfo ** aNewSessionPkgInfo )
{
    qcSessionPkgInfo * sNewPkgInfo;

    // qcSessionPkgInfo 및 template 할당
    IDE_TEST( qcg::allocSessionPkgInfo( &sNewPkgInfo ) != IDE_SUCCESS );

    *aNewSessionPkgInfo = sNewPkgInfo;
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcuSessionPkg::delSessionPkgInfo( qcStatement * aStatement, qsOID aPkgOID )
{
    qcSessionPkgInfo * sCurrSessionPkg = NULL;
    qcSessionPkgInfo * sPrevSessionPkg = NULL;
    qcSessionPkgInfo * sNextSessionPkg = NULL;

    for( sCurrSessionPkg = aStatement->session->mQPSpecific.mSessionPkg ;
         sCurrSessionPkg != NULL ;
         sCurrSessionPkg = sCurrSessionPkg->next )
    {
        sNextSessionPkg = sCurrSessionPkg->next;

        if( aPkgOID == sCurrSessionPkg->pkgOID )
        {
            if( sPrevSessionPkg != NULL )
            {
                sPrevSessionPkg->next = sNextSessionPkg;
            }
            else
            {
                // Prev가 없으면 제일 처음
                aStatement->session->mQPSpecific.mSessionPkg = sNextSessionPkg;
            }

            sCurrSessionPkg->pkgInfoMem->destroy();
            IDE_TEST( iduMemMgr::free( sCurrSessionPkg->pkgInfoMem ) != IDE_SUCCESS );
            IDE_TEST( iduMemMgr::free( sCurrSessionPkg ) != IDE_SUCCESS );

            break;
        }
        else
        {
            sPrevSessionPkg = sCurrSessionPkg;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcuSessionPkg::searchPkgInfoFromSession( qcStatement        * aStatement,
                                                qsxPkgInfo         * aPkgInfo,
                                                mtcStack           * aStack,
                                                SInt                 aStackRemain,
                                                qcTemplate        ** aTemplate    /* output */ )
{
    idBool              sFoundPkgInfo  = ID_FALSE;
    qcSessionPkgInfo  * sSessionPkg    = NULL;
    qcSessionPkgInfo  * sNewSessionPkg = NULL;
    qcTemplate        * sTemplate      = NULL;
    qcTemplate        * sTmpTemplate   = NULL;
    SInt                sStage         = 0;
    qsxPkgInfo        * sPkgBodyInfo   = NULL;

    for( sSessionPkg = aStatement->session->mQPSpecific.mSessionPkg ;
         sSessionPkg != NULL ;
         sSessionPkg = sSessionPkg->next )
    {
        if( sSessionPkg->pkgOID == aPkgInfo->pkgOID )
        {
            /* PROJ-2268 Reuse Catalog Table Slot
             * Session에 저장된 Pkg 객체에 변경이 있었는지 확인한다. */
            if ( smiValidatePlanHandle( smiGetTable( sSessionPkg->pkgOID ),
                                        sSessionPkg->pkgSCN ) 
                != IDE_SUCCESS )
            {
                /* Pkg에 변경이 있었다. 등록된 정보를 지운다. */
                IDE_TEST( delSessionPkgInfo( aStatement, sSessionPkg->pkgOID ) != IDE_SUCCESS );
                sFoundPkgInfo = ID_FALSE;
                break;
            }
            else
            {
                /* nothing to do ... */
            }

            if( sSessionPkg->modifyCount == aPkgInfo->modifyCount )
            {
                sFoundPkgInfo = ID_TRUE;
                sTemplate = sSessionPkg->pkgTemplate;
                break;
            }
            // 동일한 OID를 가진 package는 존재하나,
            // 잘못된 template을 정보를 가지고 있는 있으므로 지운다.
            else
            {
                IDE_TEST( delSessionPkgInfo( aStatement, sSessionPkg->pkgOID ) != IDE_SUCCESS );
                sFoundPkgInfo = ID_FALSE;
                break;
            }
        }
        else
        {
            sFoundPkgInfo = ID_FALSE;
        }
    }

    if( sFoundPkgInfo == ID_FALSE )
    {
        // session에 package 정보를 저장할 노드를 생성한다.
        IDE_TEST( makeSessionPkgInfo( &sNewSessionPkg ) != IDE_SUCCESS );
        sStage = 1;

        // save infomation
        sNewSessionPkg->pkgOID      = aPkgInfo->pkgOID;
        sNewSessionPkg->modifyCount = aPkgInfo->modifyCount;
        
        /* PROJ-2268 Reuse Catalog Table Slot */
        sNewSessionPkg->pkgSCN      = smiGetRowSCN( smiGetTable( aPkgInfo->pkgOID ) );

        // connect pkgInfoList
        sNewSessionPkg->next = aStatement->session->mQPSpecific.mSessionPkg;
        aStatement->session->mQPSpecific.mSessionPkg = sNewSessionPkg;

        IDE_TEST( qcg::cloneAndInitTemplate( sNewSessionPkg->pkgInfoMem,
                                             aPkgInfo->tmplate,
                                             sNewSessionPkg->pkgTemplate )
                  != IDE_SUCCESS );

        /* BUG-41847
           package 변수의 default 값으로 function을 사용할 수 있어야 합니다.
           해당 함수에서 package 변수 초기화 및 initialize section을 수행한다. */
        IDE_TEST( qsx::pkgInitWithNode( aStatement,
                                        aStack,
                                        aStackRemain,
                                        aPkgInfo->planTree,
                                        sNewSessionPkg->pkgTemplate )
                  != IDE_SUCCESS );

        if( aPkgInfo->objType == QS_PKG )
        {
            /* BUG-38844
               동일한 이름을 가진 package body의 qsxPkgInfo를 얻어온다. */
            IDE_TEST( getPkgInfo( aStatement,
                                  aPkgInfo->planTree->userID,
                                  aPkgInfo->planTree->pkgNamePos,
                                  QS_PKG_BODY,
                                  &sPkgBodyInfo )
                      != IDE_SUCCESS );

            if ( sPkgBodyInfo != NULL )
            {
                IDE_TEST( searchPkgInfoFromSession( aStatement,
                                                    sPkgBodyInfo,
                                                    QC_PRIVATE_TMPLATE(aStatement)->tmplate.stack,
                                                    QC_PRIVATE_TMPLATE(aStatement)->tmplate.stackRemain,
                                                    &sTmpTemplate )
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

        sTemplate = sNewSessionPkg->pkgTemplate;

        sFoundPkgInfo = ID_TRUE;
    }

    sStage = 0;

    *aTemplate = sTemplate; 

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStage == 1 )
    {
        // error 나면 생성한 노드는 무조건 삭제
        (void)delSessionPkgInfo( aStatement, aPkgInfo->pkgOID );
    }

    return IDE_FAILURE;
}

IDE_RC qcuSessionPkg::getTmplate( qcStatement  * aQcStmt,
                                  smOID          aPkgOID,
                                  mtcStack     * aStack,
                                  SInt           aStackRemain,
                                  mtcTemplate ** aMtcTmplate )
{
    qsxPkgInfo          * sPkgInfo;
    qcTemplate          * sTemplate;

    *aMtcTmplate = NULL;

    IDE_TEST(qsxPkg::getPkgInfo( aPkgOID,
                                 &sPkgInfo)
             != IDE_SUCCESS);

    IDE_TEST( searchPkgInfoFromSession( aQcStmt,
                                        sPkgInfo,
                                        aStack,
                                        aStackRemain,
                                        &sTemplate ) 
              != IDE_SUCCESS );

    *aMtcTmplate = ( mtcTemplate * )sTemplate;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qcuSessionPkg::initPkgVariable( qsxExecutorInfo * aExecInfo,
                                       qcStatement     * aStatement,
                                       qsxPkgInfo      * aPkgInfo,
                                       qcTemplate      * aTemplate )
{
    qsVariables       * sVariable;
    qsVariableItems   * sVariableItem;
    qsCursors         * sCursor;
    qsxCursorInfo     * sCursorInfo;
    qsxArrayInfo     ** sArrayInfo;
    qcuSqlSourceInfo    sSqlInfo;
    /* BUG-37854 */
    qsPkgStmtBlock    * sPkgBlock;
    mtcStack            sAssignStack[2];        // assign를 위한 variable

    idBool              sCopyRef = ID_FALSE;
    SChar             * sErrorMsg = NULL; 
    qsxStackFrame       sStackInfo = QC_QSX_ENV(aStatement)->mPrevStackInfo;

    sPkgBlock = aPkgInfo->planTree->block;

    /* BUG-37854 */
    if ( sPkgBlock->common.useDate == ID_TRUE )
    {
        IDE_TEST( qtc::setDatePseudoColumn( aTemplate ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    for( sVariableItem = sPkgBlock->variableItems;
         sVariableItem != NULL;
         sVariableItem = sVariableItem->next )
    {
        // BUG-42322
        qsxEnv::setStackInfo( QC_QSX_ENV(aStatement),
                              aExecInfo->mObjectID,
                              sVariableItem->lineno,
                              aExecInfo->mObjectType,
                              aExecInfo->mUserAndObjectName );

        switch( sVariableItem->itemType )
        {
            case QS_VARIABLE :
                // set default value or null
                sVariable = ( qsVariables * ) sVariableItem;

                // PROJ-1904 Extend UDT
                if ( sVariable->nocopyType == QS_COPY )
                {
                    sCopyRef = ID_FALSE;
                }
                else
                {
                    sCopyRef = ID_TRUE;
                }

                if( sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                {
                    sArrayInfo = ((qsxArrayInfo**) QTC_TMPL_FIXEDDATA( aTemplate,
                                                                       sVariable->variableTypeNode ));

                    *sArrayInfo = NULL;

                    if ( sCopyRef == ID_FALSE )
                    {
                        IDE_TEST( qsxArray::initArray( aStatement,
                                                       sArrayInfo,
                                                       sVariable->typeInfo->columns,
                                                       aStatement->mStatistics )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else if ( (sVariable->variableType == QS_ROW_TYPE) ||
                          (sVariable->variableType == QS_RECORD_TYPE) )
                {
                    IDE_TEST( qsxUtil::initRecordVar( aStatement,
                                                      aTemplate,
                                                      sVariable->variableTypeNode,
                                                      sCopyRef )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                if ( sVariable->defaultValueNode != NULL )
                {
                    if( qtc::calculate( sVariable->defaultValueNode,
                                        aTemplate )
                        != IDE_SUCCESS)
                    {
                        sSqlInfo.setSourceInfo( aStatement,
                                                &(sVariable->defaultValueNode->position) );
                        IDE_RAISE(err_pass_wrap_sqltext);
                    }
                    else
                    {
                        // stack의 두번째 부분에 default의 결과값 세팅
                        sAssignStack[1].column = aTemplate->tmplate.stack[0].column;
                        sAssignStack[1].value  = aTemplate->tmplate.stack[0].value;
                    }

                    aTemplate->tmplate.stack++;
                    aTemplate->tmplate.stackRemain--;

                    // variable type node
                    sAssignStack[0].column = QTC_TMPL_COLUMN( aTemplate , sVariable->variableTypeNode );
                    sAssignStack[0].value = ( void * )( (SChar*) aTemplate->tmplate.rows[ sVariable->variableTypeNode->node.table ].row +
                                                        sAssignStack[0].column->column.offset );
                    // assign
                    IDE_TEST( qsxUtil::assignValue ( QC_QMX_MEM( aStatement ),
                                                     sAssignStack[1].column, /* Source column */
                                                     sAssignStack[1].value,  /* Source value  */
                                                     sAssignStack[0].column, /* Dest column   */
                                                     sAssignStack[0].value,  /* Dest value    */
                                                     &aTemplate->tmplate,    /* Dest template */
                                                     sCopyRef )
                              != IDE_SUCCESS );

                    aTemplate->tmplate.stack--;
                    aTemplate->tmplate.stackRemain++;
                }
                else
                {
                    // PROJ-1904 Extend UDT
                    // UDT variable은 초기화한 상태임.
                    if ( (sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE) ||
                         (sVariable->variableType == QS_ROW_TYPE) ||
                         (sVariable->variableType == QS_RECORD_TYPE) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        IDE_TEST( qsxUtil::assignNull ( sVariable->variableTypeNode,
                                                        aTemplate )
                                  != IDE_SUCCESS );
                    }
                }

                break;
            case QS_CURSOR :
                sCursor     = ( qsCursors * ) sVariableItem;

                sCursorInfo = (qsxCursorInfo *) QTC_TMPL_FIXEDDATA(
                    aTemplate,
                    sCursor->cursorTypeNode );

                IDE_TEST( qsxCursor::initialize( sCursorInfo,
                                                 ID_FALSE )
                          != IDE_SUCCESS );

                sCursorInfo->mCursor = sCursor;

                (void)qsxCursor::setCursorSpec( sCursorInfo,
                                                sCursor->paraDecls );

                // BUG-44716 Initialize & finalize parameters of Cursor
                IDE_TEST( qsxExecutor::initVariableItems( aExecInfo,
                                                          aStatement,
                                                          sCursorInfo->mCursorParaDecls,
                                                          ID_TRUE )
                          != IDE_SUCCESS );
                break;
            case QS_EXCEPTION :
            case QS_TYPE :
            case QS_PRAGMA_AUTONOMOUS_TRANS :
            case QS_PRAGMA_EXCEPTION_INIT :
            case QS_TRIGGER_NEW_VARIABLE:
            case QS_TRIGGER_OLD_VARIABLE:
                break;
            default:
                break;
        }
    }

    QC_QSX_ENV(aStatement)->mPrevStackInfo = sStackInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_pass_wrap_sqltext);
    {
        // To fix BUG-13208
        // system_유저가 만든 프로시져는 내부공개 안함.
        if( aPkgInfo->planTree->userID == QC_SYSTEM_USER_ID )
        {
            qsxEnv::setErrorCode( QC_QSX_ENV(aStatement) );
            qsxEnv::setErrorMessage( QC_QSX_ENV(aStatement) );
        }
        else
        {
            // set original error code.
            qsxEnv::setErrorCode( QC_QSX_ENV(aStatement) );

            // BUG-43998
            // PSM 생성 오류 발생시 오류 발생 위치를 한 번만 출력하도록 합니다.
            if ( ideHasErrorPosition() == ID_FALSE )
            {
                (void)sSqlInfo.initWithBeforeMessage( QC_QMX_MEM(aStatement) );

                // BUG-42322
                if ( QCU_PSM_SHOW_ERROR_STACK == 1 )
                {
                    sErrorMsg = sSqlInfo.getErrMessage();
                }
                else // QCU_PSM_SHOW_ERROR_STACK = 0
                {
                    sErrorMsg = aExecInfo->mSqlErrorMessage;

                    IDE_DASSERT( sVariableItem != NULL );

                    idlOS::snprintf( sErrorMsg,
                                     MAX_ERROR_MSG_LEN + 1,
                                     "\nat \"%s\", line %"ID_INT32_FMT"",
                                     aExecInfo->mUserAndObjectName,
                                     sVariableItem->lineno );
                }

                IDE_SET(
                    ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                    sSqlInfo.getBeforeErrMessage(),
                                    sErrorMsg));
                (void)sSqlInfo.fini();
            }

            // set sophisticated error message.
            qsxEnv::setErrorMessage( QC_QSX_ENV(aStatement) );
        }
    }
    IDE_EXCEPTION_END;

    QC_QSX_ENV(aStatement)->mPrevStackInfo = sStackInfo;

    return IDE_FAILURE;
}

// PROJ-1073 Package BUGBUG
IDE_RC qcuSessionPkg::initPkgCursors( qcStatement * aQcStmt, qsOID aObjectID )
{
    qsxPkgInfo        * sPkgInfo = NULL;
    qsPkgParseTree    * sPlanTree;
    qsVariableItems   * sVarItems;
    qsCursors         * sCursor;
    qsxCursorInfo     * sCursorInfo;
    qcTemplate        * sTemplate;

    IDE_TEST( qsxPkg::getPkgInfo ( aObjectID, &sPkgInfo )
              != IDE_SUCCESS );

    IDE_TEST( searchPkgInfoFromSession(
                    aQcStmt,
                    sPkgInfo,
                    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                    &sTemplate)
              != IDE_SUCCESS);

    sPlanTree = sPkgInfo->planTree;

    for( sVarItems = sPlanTree->block->variableItems ;
         sVarItems != NULL;
         sVarItems = sVarItems->next )
    {
        switch( sVarItems->itemType )
        {
            case QS_CURSOR:
                sCursor     = ( qsCursors * ) sVarItems;

                sCursorInfo = (qsxCursorInfo *) QTC_TMPL_FIXEDDATA(
                    sTemplate,
                    sCursor->cursorTypeNode );

                if ( QSX_CURSOR_IS_OPEN( sCursorInfo ) == ID_FALSE )
                {
                    IDE_TEST( qsxCursor::initialize( sCursorInfo,
                                                     ID_FALSE )
                              != IDE_SUCCESS );

                    sCursorInfo->mCursor = sCursor;

                    (void)qsxCursor::setCursorSpec( sCursorInfo,
                                                    sCursor->paraDecls );
                }
                else
                {
                    // Nothing to do.
                }
                break;
            case QS_VARIABLE :
            case QS_TRIGGER_NEW_VARIABLE :
            case QS_TRIGGER_OLD_VARIABLE :
            case QS_EXCEPTION :
            case QS_TYPE :
            case QS_PRAGMA_AUTONOMOUS_TRANS :
            case QS_PRAGMA_EXCEPTION_INIT :
                break;
            default:
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcuSessionPkg::finiPkgCursors( qcStatement * aQcStmt,
                                      qsOID         aObjectID )
{
    qsxPkgInfo        * sPkgInfo = NULL;
    qsPkgParseTree    * sPlanTree;
    qsVariableItems   * sVarItems; 
    qsCursors         * sCursor;
    qsxCursorInfo     * sCursorInfo;
    qcTemplate        * sTemplate;

    IDE_TEST( qsxPkg::getPkgInfo ( aObjectID, &sPkgInfo )
              != IDE_SUCCESS );

    IDE_TEST( searchPkgInfoFromSession(
                    aQcStmt,
                    sPkgInfo,
                    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stack,
                    QC_PRIVATE_TMPLATE(aQcStmt)->tmplate.stackRemain,
                    &sTemplate )
              != IDE_SUCCESS);

    sPlanTree = sPkgInfo->planTree;

    for( sVarItems = sPlanTree->block->variableItems ;
         sVarItems != NULL;
         sVarItems = sVarItems->next )
    {
        switch( sVarItems->itemType )
        {
            case QS_CURSOR:
                sCursor  = ( qsCursors * ) sVarItems;

                // BUG-27037 QP Code Sonar
                IDE_ASSERT( sCursor->cursorTypeNode != NULL );

                sCursorInfo = (qsxCursorInfo *) QTC_TMPL_FIXEDDATA(
                    sTemplate,
                    sCursor->cursorTypeNode );

                IDE_TEST( qsxCursor::finalize( sCursorInfo,
                                               aQcStmt )
                          != IDE_SUCCESS );
                break;
            case QS_VARIABLE :
            case QS_TRIGGER_NEW_VARIABLE :
            case QS_TRIGGER_OLD_VARIABLE :
            case QS_EXCEPTION :
            case QS_TYPE :
            case QS_PRAGMA_AUTONOMOUS_TRANS :
            case QS_PRAGMA_EXCEPTION_INIT :
                break;
            default:
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcuSessionPkg::getPkgInfo( qcStatement     * aStatement,
                                  UInt              aUserID,
                                  qcNamePosition    aPkgName,
                                  qsObjectType      aObjectType,
                                  qsxPkgInfo     ** aPkgInfo )
{
    qsOID              sPkgOID;
    qsxPkgInfo       * sPkgInfo = NULL;
    qsxPkgObjectInfo * sPkgObjectInfo = NULL;
    // get package body info from meta.
    smiTrans           sTrans;
    smiStatement       sSmiStmt;
    smiStatement     * sSmiStmtOri;
    smiStatement     * sDummySmiStmt;
    UInt               sSmiStmtFlag = 0;
    UInt               sStage = 0;

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_UNTOUCHABLE;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sTrans.begin( &sDummySmiStmt, aStatement->mStatistics )
             != IDE_SUCCESS);
    sStage = 2;

    IDE_TEST(sSmiStmt.begin( aStatement->mStatistics,
                             sDummySmiStmt,
                             sSmiStmtFlag )
             != IDE_SUCCESS);

    qcg::getSmiStmt( aStatement, &sSmiStmtOri );
    qcg::setSmiStmt( aStatement, &sSmiStmt);
    sStage = 3;

    IDE_TEST(
        qcmPkg::getPkgExistWithEmptyByNamePtr(
            aStatement,
            aUserID,
            (SChar*)(aPkgName.stmtText + aPkgName.offset),
            aPkgName.size,
            aObjectType,
            &sPkgOID)
        != IDE_SUCCESS);

    if ( sPkgOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::getPkgObjectInfo( sPkgOID,
                                            & sPkgObjectInfo )
                  != IDE_SUCCESS );

        if ( sPkgObjectInfo != NULL )
        {
            IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );
            IDE_DASSERT( sPkgInfo != NULL );
        }
        else
        {
            // on boot up case
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    sStage = 2;
    qcg::setSmiStmt( aStatement, sSmiStmtOri );
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST(sTrans.destroy( aStatement->mStatistics ) != IDE_SUCCESS);

    *aPkgInfo = sPkgInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 3:
            qcg::setSmiStmt( aStatement, sSmiStmtOri );
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
            /* fall through */
        case 2:
            (void)sTrans.rollback();
            /* fall through */
        case 1:
            (void)sTrans.destroy( aStatement->mStatistics );
            /* fall through */
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcuSessionPkg::finiPkgVariable( qcStatement * aStatement,
                                       qsxPkgInfo  * aPkgInfo,
                                       qcTemplate  * aTemplate )
{
    qsVariableItems * sVariableItem = NULL;
    qsCursors       * sCursor       = NULL;
    qsxCursorInfo   * sCursorInfo   = NULL;
    qsVariables     * sVariable     = NULL;
    qsxArrayInfo   ** sArrayInfo    = NULL;
    qsPkgStmtBlock  * sPkgBlock     = NULL;

    sPkgBlock = aPkgInfo->planTree->block;

    for( sVariableItem = sPkgBlock->variableItems;
         sVariableItem != NULL;
         sVariableItem = sVariableItem->next )
    {
        switch( sVariableItem->itemType )
        {
            case QS_VARIABLE :
                sVariable = ( qsVariables * ) sVariableItem;

                if( sVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                {
                    // BUG-27037 QP Code Sonar
                    IDE_ERROR( sVariable->variableTypeNode != NULL );

                    sArrayInfo = (qsxArrayInfo**) QTC_TMPL_FIXEDDATA( aTemplate, 
                                                                      sVariable->variableTypeNode );   

                    IDE_TEST_RAISE( *sArrayInfo == NULL, ERR_INVALID_ARRAY );

                    IDE_TEST( qsxArray::truncateArray( *sArrayInfo ) 
                              != IDE_SUCCESS ); 
                }
                else
                {
                    // Nothing to do.
                }
                break;
            case QS_CURSOR :
                sCursor = (qsCursors *) sVariableItem;

                // BUG-27037 QP Code Sonar
                IDE_ERROR( sCursor->cursorTypeNode != NULL );

                sCursorInfo = (qsxCursorInfo*) QTC_TMPL_FIXEDDATA( aTemplate,
                                                                   sCursor->cursorTypeNode );

                // BUG-44716 Initialize & finalize parameters of Cursor
                IDE_TEST( qsxExecutor::finiVariableItems( aStatement,
                                                          sCursorInfo->mCursorParaDecls )
                          != IDE_SUCCESS );

                IDE_TEST( qsxCursor::finalize( sCursorInfo,
                                               aStatement )
                          != IDE_SUCCESS );
                break;
            case QS_TRIGGER_NEW_VARIABLE:
            case QS_TRIGGER_OLD_VARIABLE:
            case QS_EXCEPTION : 
            case QS_TYPE :
            case QS_PRAGMA_AUTONOMOUS_TRANS :
            case QS_PRAGMA_EXCEPTION_INIT :
                break;
            default:
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARRAY );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_UNINITIALIZED_ARRAY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
