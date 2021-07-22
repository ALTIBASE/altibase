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
 * $Id: qsvPkgStmts.cpp 87967 2020-07-07 05:02:47Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcmUser.h>
#include <qcmPkg.h>
#include <qcuSqlSourceInfo.h>
#include <qcg.h>
#include <qsvPkgStmts.h>
#include <qsvProcStmts.h>
#include <qsvProcVar.h>
#include <qsvProcType.h>
#include <qsvCursor.h>
#include <qsvEnv.h>
#include <qsv.h>
#include <qmv.h>
#include <qcmSynonym.h>
#include <qtcCache.h>

extern mtdModule    mtdBoolean;
extern mtdModule    mtdVarchar;
extern mtdModule    mtdChar;
extern mtdModule    mtdInteger;

extern mtfModule mtfTo_nchar;
extern mtfModule mtfTo_char;
extern mtfModule mtfTo_date;
extern mtfModule mtfTo_number;
extern mtfModule mtfCast;

IDE_RC qsvPkgStmts::parseSpecBlock( qcStatement * aStatement,
                                    qsProcStmts * aProcStmts )
{
/***********************************************************************
 *
 * Description : BUG-41847
 *
 *    Package Spec의 block에 있는 subprogram에 대해 ID를 부여하고,
 *    parse함수를 호출한다.
 *
 ***********************************************************************/

    qsPkgStmtBlock    * sBLOCK        = NULL;
    qsPkgStmts        * sCurrStmt     = NULL;
    qsPkgSubprograms  * sCurrSub      = NULL;
    UInt                sSubID        = 0;

    aStatement->spvEnv->currStmt = aProcStmts;
    
    sBLOCK = (qsPkgStmtBlock *)aProcStmts;

    /* subprogramID를 부여한다. */
    for( sCurrStmt = sBLOCK->subprograms;
         sCurrStmt != NULL;
         sCurrStmt = sCurrStmt->next )
    {
        sCurrSub = (qsPkgSubprograms *)sCurrStmt;

        if( sCurrStmt->stmtType != QS_OBJECT_MAX )
        {
            sSubID++;

            sCurrSub->subprogramID   = sSubID;
            sCurrSub->subprogramType = QS_PUBLIC_SUBPROGRAM;
            IDE_TEST( sCurrStmt->parse( aStatement,
                                        sCurrStmt )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    aStatement->spvEnv->currStmt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStatement->spvEnv->currStmt = NULL;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::parseBodyBlock( qcStatement * aStatement,
                                    qsProcStmts * aProcStmts )
{
/**************************************************************************
 *
 * Description : BUG-41847
 *
 *    - Package Body의 block에 있는 subprogram에 대해 ID를 부여하고,
 *     parse함수를 호출한다.
 *    - Private subprogramID는 validate함수에서 동일한 subprogram인 경우,
 *     동일한 subprogramID로 통일시킨다.
 *
 **************************************************************************/

    qsPkgParseTree   * sPkgSpecParseTree = NULL;
    qsPkgStmtBlock   * sPkgSpecBLOCK     = NULL;
    qsPkgStmtBlock   * sPkgBodyBLOCK     = NULL;
    qsPkgStmts       * sPkgSpecStmt      = NULL;
    qsPkgStmts       * sCurrStmt         = NULL;
    qsPkgSubprograms * sCurrSub          = NULL;
    UInt               sSubID            = 0;

    IDE_DASSERT( aStatement->spvEnv->pkgPlanTree != NULL );

    aStatement->spvEnv->currStmt = aProcStmts;

    sPkgSpecParseTree = aStatement->spvEnv->pkgPlanTree;
    sPkgSpecBLOCK     = sPkgSpecParseTree->block;
    sSubID            = sPkgSpecParseTree->subprogramCount;
    sPkgBodyBLOCK     = (qsPkgStmtBlock *)aProcStmts;

    /* subprogramID를 부여한다. */
    for( sPkgSpecStmt = sPkgSpecBLOCK->subprograms;
         sPkgSpecStmt != NULL;
         sPkgSpecStmt = sPkgSpecStmt->next )
    {
        if( sPkgSpecStmt->stmtType != QS_OBJECT_MAX )
        {
            sCurrStmt = sPkgBodyBLOCK->subprograms;
            IDE_TEST( checkSubprogramDef( aStatement,
                                          sCurrStmt,
                                          sPkgSpecStmt )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    for( sCurrStmt = sPkgBodyBLOCK->subprograms;
         sCurrStmt != NULL;
         sCurrStmt = sCurrStmt->next )
    {
        if( sCurrStmt->stmtType != QS_OBJECT_MAX )
        {
            sCurrSub = (qsPkgSubprograms *)sCurrStmt;

            // private 서브 프로그램인 경우
            // ID 를 할당해 주어야 validate 가 가능하다.
            if( sCurrSub->subprogramID == QS_PSM_SUBPROGRAM_ID )
            {
                sSubID++;

                sCurrSub->subprogramID   = sSubID;
                sCurrSub->subprogramType = QS_PRIVATE_SUBPROGRAM;
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST( sCurrStmt->parse( aStatement,
                                        sCurrStmt )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    aStatement->spvEnv->currStmt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStatement->spvEnv->currStmt = NULL;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::validateSpecBlock( qcStatement       * aStatement,
                                       qsProcStmts       * aProcStmts,
                                       qsProcStmts       * aParentStmt,
                                       qsValidatePurpose   aPurpose )
{
/*****************************************************************************
 *
 * Description : 
 *
 *    Package Spec의 block에 대한 validation을 한다.
 *        - exception variable
 *            ㄴ동일한 exception 변수명이 존재하는지 확인
 *        - variable
 *            ㄴ동일한 variable 명이 존재하는지 확인
 *            ㄴvariable type에 따라 validation
 *        - validate subprogram header
 *            ㄴprocedure / function ~ as 구문 사이에
 *              있는 정보에 대해 validation
 *        - validate subprogram body
 *            ㄴspec에 선언된 variable과 subprogram 이름이 중복 되는지 검사
 *            ㄴas ~ end 사이에 대해서 validation
 *            ㄴvariable , statement 등등
 *
 ****************************************************************************/

    qsPkgParseTree    * sParseTree;
    qsPkgStmtBlock    * sBLOCK = (qsPkgStmtBlock *)aProcStmts;
    qsVariableItems   * sCurrItem;
    qsVariableItems   * sNextItem;
    qsPkgStmts        * sCurrStmt;
    qsPkgStmts        * sNextStmt;
    qsPkgSubprograms  * sCurrSub;
    qsPkgSubprograms  * sNextSub;
    qsProcParseTree   * sCurrSubParseTree;
    qsProcParseTree   * sNextSubParseTree;
    qsCursors         * sCursor;
    qcuSqlSourceInfo    sqlInfo;

    QS_SET_PARENT_STMT( aProcStmts, aParentStmt );

    aStatement->spvEnv->currStmt = aProcStmts;

    sParseTree = (qsPkgParseTree *)(aStatement->myPlan->parseTree);
   
    // exception
    for( sCurrItem = sBLOCK->variableItems;
         sCurrItem != NULL;
         sCurrItem = sCurrItem->next )
    {
        if( sCurrItem->itemType == QS_EXCEPTION )
        {
            // BUG-27442
            // Validate length of Label name
            if( sCurrItem->name.size > QC_MAX_OBJECT_NAME_LEN )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrItem->name );
                IDE_RAISE( ERR_MAX_NAME_LEN_OVERFLOW );
            }
            else
            {
                // Nothing to do.
            }

            // check duplicate exception name
            for( sNextItem = sCurrItem->next;
                 sNextItem != NULL;
                 sNextItem = sNextItem->next )
            {
                if( sNextItem->itemType == QS_EXCEPTION )
                {
                    if( QC_IS_NAME_MATCHED( sNextItem->name, sCurrItem->name )
                        == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrItem->name );
                        IDE_RAISE( ERR_DUP_VARIABLE_ITEM );
                    }
                }
            }

            QS_INIT_EXCEPTION_VAR( aStatement, (qsExceptionDefs *)sCurrItem );
        }
    }

    // check duplication variable
    for( sCurrItem = sBLOCK->variableItems; 
         sCurrItem != NULL;
         sCurrItem = sCurrItem->next )
    {
        // To fix BUG-14129
        // 현재 validate중인 declare item을 링크.
        aStatement->spvEnv->currDeclItem = sCurrItem;
       
        if( ( sCurrItem->itemType == QS_VARIABLE ) ||
            ( sCurrItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
            ( sCurrItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
            ( sCurrItem->itemType == QS_CURSOR ) ||
            ( sCurrItem->itemType == QS_TYPE ) )
        {
            for( sNextItem = sCurrItem->next;
                 sNextItem != NULL;
                 sNextItem = sNextItem->next )
            {
                if( ( sNextItem->itemType == QS_VARIABLE ) ||
                    ( sNextItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                    ( sNextItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
                    ( sNextItem->itemType == QS_CURSOR ) ||
                    ( sNextItem->itemType == QS_TYPE ) )
                {
                    if ( QC_IS_NAME_MATCHED( sNextItem->name, sCurrItem->name ) )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrItem->name );
                        IDE_RAISE( ERR_DUP_VARIABLE_ITEM );
                    }
                }
            }

            // validation of variable
            if( ( sCurrItem->itemType == QS_VARIABLE ) ||
                ( sCurrItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                ( sCurrItem->itemType == QS_TRIGGER_OLD_VARIABLE ) )
            {
                IDE_TEST( qsvProcVar::validateLocalVariable(
                              aStatement,
                              (qsVariables *)sCurrItem )
                          != IDE_SUCCESS ); 
            }
            else if( sCurrItem->itemType == QS_CURSOR )
            {
                sCursor = (qsCursors *)sCurrItem;
                IDE_TEST( qsvProcStmts::parseCursor( aStatement,
                                                     sCursor )
                          != IDE_SUCCESS );

                IDE_TEST( qsvCursor::validateCursorDeclare(
                              aStatement,
                              (qsCursors *)sCurrItem )
                          != IDE_SUCCESS );
            }
            else if( sCurrItem->itemType == QS_TYPE )
            {
                IDE_TEST( qsvProcType::validateTypeDeclare(
                              aStatement,
                              (qsTypes *)sCurrItem,
                              & sCurrItem->name,
                              ID_FALSE )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            switch ( sCurrItem->itemType )
            {
                /* BUG-38509 autonomous transaction
                   autonomous transaction은 package(spec , body)의
                   declare_section에는 정의하지 못 함 */
                case QS_PRAGMA_AUTONOMOUS_TRANS:
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrItem->name );
                    IDE_RAISE( ERR_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANS );
                    break;
                /* BUG-41240 EXCEPTION_INIT Pragma */
                case QS_PRAGMA_EXCEPTION_INIT:
                    IDE_TEST( qsvProcStmts::validatePragmaExcepInit( aStatement,
                                                                     sBLOCK->variableItems,
                                                                     (qsPragmaExceptionInit*)sCurrItem )
                              != IDE_SUCCESS ); 
                    break;
                default:
                    break;
            }
        }
    } /* end for */

    aStatement->spvEnv->currDeclItem = NULL;
   
    /* 서브 프로그램의 인자의 정보를 알기 위해서
       parameter 및 return 정보에 대한 validation을 해야 한다. */
    for( sCurrStmt = sBLOCK->subprograms;
         sCurrStmt != NULL;
         sCurrStmt = sCurrStmt->next )
    {
        if( sCurrStmt->stmtType != QS_OBJECT_MAX )
        {
            IDE_DASSERT( sCurrStmt->state == QS_PKG_STMTS_PARSE_FINISHED );

            sCurrSub = (qsPkgSubprograms *)sCurrStmt;
            IDE_TEST( sCurrStmt->validateHeader( aStatement,
                                                 sCurrStmt )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    /* subprogram의 중복 여부를 검사하고,
       subprogram의 body( variable 및 statement )에 대한 validation을 한다. */
    for( sCurrStmt = sBLOCK->subprograms;
         sCurrStmt != NULL;
         sCurrStmt = sCurrStmt->next )
    {
        if( sCurrStmt->stmtType != QS_OBJECT_MAX )
        {
            sCurrSub = (qsPkgSubprograms *)sCurrStmt;
            sCurrSubParseTree = sCurrSub->parseTree;

            // check duplication subprogram name
            // 변수명과 비교
            for( sCurrItem = sBLOCK->variableItems;
                 sCurrItem != NULL;
                 sCurrItem = sCurrItem->next )
            {
                if( ( sCurrItem->itemType == QS_VARIABLE ) ||
                    ( sCurrItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                    ( sCurrItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
                    ( sCurrItem->itemType == QS_CURSOR ) ||
                    ( sCurrItem->itemType == QS_TYPE ) )
                {
                    if ( QC_IS_NAME_MATCHED( sCurrItem->name, sCurrSubParseTree->procNamePos ) )
                    {
                        sqlInfo.setSourceInfo( aStatement, &( sCurrSubParseTree->procNamePos ) );
                        IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                    }
                }
            }

            for( sNextStmt = sCurrStmt->next; 
                 sNextStmt != NULL;
                 sNextStmt = sNextStmt->next )
            {
                if( sNextStmt->stmtType != QS_OBJECT_MAX )
                {
                    sNextSub = (qsPkgSubprograms *)sNextStmt;
                    sNextSubParseTree = sNextSub->parseTree;

                    if( isSubprogramEqual( sCurrSub,
                                           sNextSub ) == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement, &( sNextSubParseTree->procNamePos ) );
                        IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }

            IDE_DASSERT( sCurrStmt->state == QS_PKG_STMTS_VALIDATE_HEADER_FINISHED );

            IDE_TEST( sCurrStmt->validateBody( aStatement, sCurrStmt, aPurpose ) != IDE_SUCCESS );
        }
        else
        {
            // BUG-37655
            IDE_TEST( validatePragmaRestrictReferences( aStatement,
                                                        sBLOCK,
                                                        sCurrStmt )
                      != IDE_SUCCESS )
        }
    }

    IDE_DASSERT( sParseTree->block->bodyStmts == NULL ) ;
    IDE_DASSERT( sParseTree->block->exception == NULL ) ;

    aStatement->spvEnv->currStmt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUP_VARIABLE_ITEM );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_DUPLICATE_VARIABLE_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MAX_NAME_LEN_OVERFLOW )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DUP_PKG_SUBPROGRAM )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_DUPLICATE_PKG_SUBPROGRAM,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANS)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANSACTION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    aStatement->spvEnv->currStmt = NULL;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::validateBodyBlock( qcStatement       * aStatement,
                                       qsProcStmts       * aProcStmts,
                                       qsProcStmts       * aParentStmt,
                                       qsValidatePurpose   aPurpose )
{
/*****************************************************************************************
 *
 * Description : 
 *
 *    Package Spec의 block에 대한 validation을 한다.
 *        - exception variable
 *            ㄴ spec / body에 동일한 exception 변수명이 존재하는지 확인
 *        - variable
 *            ㄴspec / body에 동일한 variable 명이 존재하는지 확인
 *            ㄴvariable type에 따라 validation
 *        - validate subprogram header
 *            ㄴprocedure / function ~ as 구문 사이에
 *              있는 정보에 대해 validation
 *        - validate subprogram body
 *            ㄴspec / body에 선언된 variable과 subprogram 이름이 중복 되는지 검사
 *            ㄴprivate subprogram의 subprogramID 보정
 *            ㄴas ~ end 사이에 대해서 validation
 *            ㄴvariable , statement 등등
 *        - initialize section에 대한 parse 및 validation
 *        - exception block에 대한 validation
 *
 ****************************************************************************************/

    qsPkgParseTree      * sSpecParseTree;
    qsPkgStmtBlock      * sBLOCK;
    qsPkgStmtBlock      * sSpecBLOCK;
    qsProcStmts         * sStmt;
    qsVariableItems     * sCurrItem;
    qsVariableItems     * sNextItem;
    qsVariableItems     * sSpecItem;
    qsPkgStmts          * sCurrStmt;
    qsPkgStmts          * sNextStmt;
    qsPkgSubprograms    * sCurrSub;
    qsPkgSubprograms    * sNextSub;
    qsProcParseTree     * sCurrSubParseTree;
    qsProcParseTree     * sNextSubParseTree;
    qsCursors           * sCursor;
    qcuSqlSourceInfo      sqlInfo;
    idBool                sExistDefinition = ID_FALSE;

    qsPkgStmts       * sPkgSubprogramStmt       = NULL;
    qsPkgSubprograms * sPkgSubprogramInfo       = NULL;

    IDE_DASSERT( aStatement->spvEnv->pkgPlanTree != NULL );

    QS_SET_PARENT_STMT( aProcStmts, aParentStmt );

    aStatement->spvEnv->currStmt = aProcStmts;

    sSpecParseTree =  aStatement->spvEnv->pkgPlanTree;
    sBLOCK         = (qsPkgStmtBlock *)aProcStmts;
    sSpecBLOCK     = sSpecParseTree->block;

    aStatement->spvEnv->currStmt = aProcStmts;

    IDE_DASSERT( sSpecParseTree != NULL ); 

    // exception
    for( sCurrItem = sBLOCK->variableItems;
         sCurrItem != NULL;
         sCurrItem = sCurrItem->next )
    {
        if( sCurrItem->itemType == QS_EXCEPTION )
        {
            // BUG-27442
            // Validate length of Label name
            if( sCurrItem->name.size > QC_MAX_OBJECT_NAME_LEN )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrItem->name );
                IDE_RAISE( ERR_MAX_NAME_LEN_OVERFLOW );
            }
            else
            {
                // Nothing to do.
            }

            // check duplicate exception name
            for( sNextItem = sCurrItem->next;
                 sNextItem != NULL;
                 sNextItem = sNextItem->next )
            {
                if( sNextItem->itemType == QS_EXCEPTION )
                {
                    if( QC_IS_NAME_MATCHED( sNextItem->name, sCurrItem->name )
                        == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrItem->name );
                        IDE_RAISE( ERR_DUP_VARIABLE_ITEM );
                    }
                }
            }

            // create package body일 때는 package spec의
            // 변수와도 비교해야 한다.
            for( sSpecItem = sSpecBLOCK->variableItems;
                 sSpecItem != NULL ;
                 sSpecItem = sSpecItem->next )
            {
                if( sSpecItem->itemType == QS_EXCEPTION )
                {
                    if( QC_IS_NAME_MATCHED( sSpecItem->name, sCurrItem->name )
                        == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrItem->name );
                        IDE_RAISE( ERR_DUP_VARIABLE_ITEM );
                    }
                }
            }

            QS_INIT_EXCEPTION_VAR( aStatement, (qsExceptionDefs *)sCurrItem );
        }
    }

    // check duplication variable
    for( sCurrItem = sBLOCK->variableItems;
         sCurrItem != NULL;
         sCurrItem = sCurrItem->next )
    {
        // To fix BUG-14129
        // 현재 validate중인 declare item을 링크.
        aStatement->spvEnv->currDeclItem = sCurrItem;

        if( ( sCurrItem->itemType == QS_VARIABLE ) ||
            ( sCurrItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
            ( sCurrItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
            ( sCurrItem->itemType == QS_CURSOR ) ||
            ( sCurrItem->itemType == QS_TYPE ) )
        {
            for( sNextItem = sCurrItem->next ;
                 sNextItem != NULL ;
                 sNextItem = sNextItem->next )
            {
                if( ( sNextItem->itemType == QS_VARIABLE ) ||
                    ( sNextItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                    ( sNextItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
                    ( sNextItem->itemType == QS_CURSOR ) ||
                    ( sNextItem->itemType == QS_TYPE ) )
                {
                    if ( QC_IS_NAME_MATCHED( sNextItem->name, sCurrItem->name ) )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrItem->name );
                        IDE_RAISE(ERR_DUP_VARIABLE_ITEM);
                    }
                }
            }

            // create package body일 때는 package spec의
            // 변수와도 비교해야 한다.
            for( sSpecItem = sSpecParseTree->block->variableItems ;
                 sSpecItem != NULL ;
                 sSpecItem = sSpecItem->next )
            {
                if( ( sSpecItem->itemType == QS_VARIABLE ) ||
                    ( sSpecItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                    ( sSpecItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
                    ( sSpecItem->itemType == QS_CURSOR ) ||
                    ( sSpecItem->itemType == QS_TYPE ) )
                {
                    if ( QC_IS_NAME_MATCHED( sSpecItem->name, sCurrItem->name ) )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrItem->name );
                        IDE_RAISE(ERR_DUP_VARIABLE_ITEM);
                    }
                }
            }

            // validation of variable
            if( ( sCurrItem->itemType == QS_VARIABLE ) ||
                ( sCurrItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                ( sCurrItem->itemType == QS_TRIGGER_OLD_VARIABLE ) )
            {
                IDE_TEST(qsvProcVar::validateLocalVariable(
                        aStatement,
                        (qsVariables *)sCurrItem)
                    != IDE_SUCCESS);
            }
            else if( sCurrItem->itemType == QS_CURSOR )
            {
                sCursor = (qsCursors *)sCurrItem;
                IDE_TEST( qsvProcStmts::parseCursor( aStatement,
                                                     sCursor )
                          != IDE_SUCCESS );

                IDE_TEST(qsvCursor::validateCursorDeclare(
                        aStatement,
                        (qsCursors *)sCurrItem)
                    != IDE_SUCCESS);
            }
            else if( sCurrItem->itemType == QS_TYPE )
            {
                IDE_TEST( qsvProcType::validateTypeDeclare(
                        aStatement,
                        (qsTypes*)sCurrItem,
                        & sCurrItem->name,
                        ID_FALSE )
                    != IDE_SUCCESS );
            }
        }
        else
        {
            switch ( sCurrItem->itemType )
            {
                /* BUG-38509 autonomous transaction
                   autonomous transaction은 package(spec , body)의
                   declare_section에는 정의하지 못 함 */
                case QS_PRAGMA_AUTONOMOUS_TRANS:
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrItem->name );
                    IDE_RAISE( ERR_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANS );
                    break;
                /* BUG-41240 EXCEPTION_INIT Pragma */
                case QS_PRAGMA_EXCEPTION_INIT:
                    IDE_TEST( qsvProcStmts::validatePragmaExcepInit( aStatement,
                                                                     sBLOCK->variableItems,
                                                                     (qsPragmaExceptionInit*)sCurrItem )
                              != IDE_SUCCESS );
                    break;
                default:
                    break;
            }
        }
    }

    aStatement->spvEnv->currDeclItem = NULL;

    /* 서브 프로그램의 인자의 정보를 알기 위해서
       parameter 및 return 정보에 대한 validation을 해야 한다. */
    for( sCurrStmt = sBLOCK->subprograms;
         sCurrStmt != NULL;
         sCurrStmt = sCurrStmt->next )
    {
        if( sCurrStmt->stmtType != QS_OBJECT_MAX )
        {
            if ( sCurrStmt->state == QS_PKG_STMTS_PARSE_FINISHED )
            {
                sCurrSub = (qsPkgSubprograms *)sCurrStmt;
                IDE_TEST( sCurrStmt->validateHeader( aStatement, sCurrStmt ) != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            /* BUG-37655
               package body에서는 pragma restrict_reference를 사용할 수 없다.
               pragma restrict_reference는 package spec에서만 가능하다. */
            IDE_RAISE( ERR_INVALID_PRAGMA_RESTRICT_REFERENCE );
        }
    }

    /* subprogram의 중복 여부를 검사하고,
       동일한 private subprogram에 대해서는 동일한 subprogramID를 셋팅한다.
       이후, subprogram의 body( variable 및 statement )에 대한 validation을 한다. */
    for( sCurrStmt = sBLOCK->subprograms;
         sCurrStmt != NULL;
         sCurrStmt = sCurrStmt->next )
    {
        sExistDefinition = ID_FALSE;

        if( sCurrStmt->stmtType != QS_OBJECT_MAX )
        {
            sCurrSub = (qsPkgSubprograms *)sCurrStmt;
            sCurrSubParseTree = sCurrSub->parseTree;

            for( sCurrItem = sBLOCK->variableItems;
                 sCurrItem != NULL;
                 sCurrItem = sCurrItem->next )
            {
                if( ( sCurrItem->itemType == QS_VARIABLE ) ||
                    ( sCurrItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                    ( sCurrItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
                    ( sCurrItem->itemType == QS_CURSOR ) ||
                    ( sCurrItem->itemType == QS_TYPE ) )
                {
                    if ( QC_IS_NAME_MATCHED( sCurrItem->name, sCurrSubParseTree->procNamePos ) )
                    {
                        sqlInfo.setSourceInfo( aStatement, &( sCurrSubParseTree->procNamePos ) );
                        IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                    }
                }
            }

            // create package body일 때는 package spec의
            // 변수와도 비교해야 한다.
            for( sSpecItem = sSpecParseTree->block->variableItems;
                 sSpecItem != NULL;
                 sSpecItem = sSpecItem->next )
            {
                if( ( sSpecItem->itemType == QS_VARIABLE ) ||
                    ( sSpecItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
                    ( sSpecItem->itemType == QS_TRIGGER_OLD_VARIABLE ) ||
                    ( sSpecItem->itemType == QS_CURSOR ) ||
                    ( sSpecItem->itemType == QS_TYPE ) )
                {
                    if ( QC_IS_NAME_MATCHED( sSpecItem->name, sCurrSubParseTree->procNamePos ) )
                    {
                        sqlInfo.setSourceInfo( aStatement, &( sCurrSubParseTree->procNamePos ) );
                        IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                    }
                }
            }

            for( sNextStmt = sCurrStmt->next;
                 sNextStmt != NULL;
                 sNextStmt = sNextStmt->next )
            {
                if( sNextStmt->stmtType != QS_OBJECT_MAX )
                {
                    sNextSub = (qsPkgSubprograms *)sNextStmt;
                    sNextSubParseTree = sNextSub->parseTree;

                    if( isSubprogramEqual( sCurrSub,
                                           sNextSub ) == ID_TRUE )
                    {
                        if( sCurrSubParseTree->block == NULL && sNextSubParseTree->block != NULL )
                        {
                            sNextSub->subprogramID   = sCurrSub->subprogramID;
                            sNextSub->subprogramType = QS_PRIVATE_SUBPROGRAM;
                            sExistDefinition         = ID_TRUE;
                        }
                        else if( sCurrSubParseTree->block != NULL && sNextSubParseTree->block == NULL )
                        {
                            sqlInfo.setSourceInfo( aStatement, &( sNextSubParseTree->procNamePos ) );
                            IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                        }
                        else
                        {
                            sqlInfo.setSourceInfo( aStatement, &( sNextSubParseTree->procNamePos ) );
                            IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                        }
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

            if( (sCurrSubParseTree->block == NULL) &&
                (sCurrSubParseTree->procType == QS_NORMAL) &&
                (sExistDefinition == ID_FALSE ) )
            {
                /* BUG-39220
                   subprogram의 definition도 있어야 한다. */
                sqlInfo.setSourceInfo( aStatement,
                                       &(sCurrSubParseTree->procNamePos) );
                IDE_RAISE( ERR_NOT_DEFINE_SUBPROGRAM );
            }
            else
            {
                // Nothing to do.
            }

            IDE_DASSERT( sCurrStmt->state == QS_PKG_STMTS_VALIDATE_HEADER_FINISHED );

            IDE_TEST( sCurrStmt->validateBody( aStatement, sCurrStmt, aPurpose ) != IDE_SUCCESS );

            // BUG-47971 Package global variable의 분산 실행을 제한합니다.
            // Package body의 subprogram을 validation해야 package global variable 참조 여부를 알 수 있다.
            // Pakcage spec의 subprogram definition에 package global variable 참조 여부를 복사해준다.
            for ( sPkgSubprogramStmt = sSpecBLOCK->subprograms;
                  sPkgSubprogramStmt != NULL;
                  sPkgSubprogramStmt = sPkgSubprogramStmt->next )
            {
                if ( sPkgSubprogramStmt->stmtType != QS_OBJECT_MAX )
                {
                    sPkgSubprogramInfo = (qsPkgSubprograms *)sPkgSubprogramStmt;

                    if ( sPkgSubprogramInfo->subprogramID == sCurrSub->subprogramID )
                    {
                        sPkgSubprogramInfo->parseTree->referToPkgVar = sCurrSubParseTree->referToPkgVar;
                        break;
                    }
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    aStatement->spvEnv->isPkgInitializeSection = ID_TRUE;

    // parse & validate body(initialize section)
    for( sStmt = sBLOCK->bodyStmts ;
         sStmt != NULL ;
         sStmt = sStmt->next )
    {
        aStatement->spvEnv->currSubprogram = NULL;
        aStatement->spvEnv->createProc     = NULL;

        IDE_TEST( sStmt->parse( aStatement, sStmt ) != IDE_SUCCESS );

        IDE_TEST( sStmt->validate( aStatement , sStmt, aProcStmts, aPurpose )
                  != IDE_SUCCESS );
    }

    if( sBLOCK->exception != NULL )
    {
        IDE_TEST( qsvProcStmts::validateExceptionHandler( aStatement,
                                                          NULL,
                                                          sBLOCK,
                                                          aParentStmt,
                                                          aPurpose )
                  != IDE_SUCCESS );
    }

    aStatement->spvEnv->isPkgInitializeSection = ID_FALSE;

    aStatement->spvEnv->currStmt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUP_VARIABLE_ITEM );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_DUPLICATE_VARIABLE_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MAX_NAME_LEN_OVERFLOW )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DUP_PKG_SUBPROGRAM )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_DUPLICATE_PKG_SUBPROGRAM,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_PRAGMA_RESTRICT_REFERENCE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_INVALID_PRAGMA_RESTRICT_REFERENCE ) );
    }
    IDE_EXCEPTION( ERR_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANS )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_CANNOT_SPECIFIED_PRAGMA_AUTONOMOUS_TRANSACTION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_DEFINE_SUBPROGRAM );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_UNDEFINED_SUBPROGRAM,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    aStatement->spvEnv->isPkgInitializeSection = ID_FALSE;

    aStatement->spvEnv->currStmt = NULL;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::makeRelatedObjects(
    qcStatement      * aStatement,
    qcNamePosition   * aUserName,
    qcNamePosition   * aTableName,
    qcmSynonymInfo   * aSynonymInfo,
    UInt               aTableID,
    SInt               aObjectType)
{
    qsRelatedObjects   * sCurrObject;
    qsRelatedObjects   * sObject;

    // make new related object
    IDE_TEST( makeNewRelatedObject( aStatement,
                                    aUserName,
                                    aTableName,
                                    aSynonymInfo,
                                    aTableID,
                                    aObjectType,
                                    &sCurrObject )
              != IDE_SUCCESS);

    // search same object
    for ( sObject = aStatement->spvEnv->relatedObjects;
          sObject != NULL;
          sObject = sObject->next)
    {
        if ( (sObject->objectType == sCurrObject->objectType) &&
             (idlOS::strMatch(sObject->userName.name,
                              sObject->userName.size,
                              sCurrObject->userName.name,
                              sCurrObject->userName.size) == 0) &&
             (idlOS::strMatch(sObject->objectName.name,
                              sObject->objectName.size,
                              sCurrObject->objectName.name,
                              sCurrObject->objectName.size) == 0) )
        {
            // found
            break;
        }
    }

    // connect
    if (sObject == NULL)
    {
        sCurrObject->next = aStatement->spvEnv->relatedObjects;
        aStatement->spvEnv->relatedObjects = sCurrObject;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::makeNewRelatedObject(
    qcStatement       * aStatement,
    qcNamePosition    * aUserName,
    qcNamePosition    * aTableName,
    qcmSynonymInfo    * aSynonymInfo,
    UInt                aTableID,
    SInt                aObjectType,
    qsRelatedObjects ** aObject)
{
    qsRelatedObjects * sCurrObject;
    UInt               sConnectUserID = QCG_GET_SESSION_USER_ID(aStatement);
    idBool             sExist         = ID_FALSE;
    iduVarMemList    * sMemory        = QC_QMP_MEM( aStatement );
    qsOID              sPkgOID        = QS_EMPTY_OID;
    UInt               sUserID;

    IDE_ASSERT( aSynonymInfo != NULL );

    IDE_TEST( STRUCT_ALLOC( sMemory,
                            qsRelatedObjects,
                            &sCurrObject ) != IDE_SUCCESS );

    // object type
    sCurrObject->userID       = 0;
    sCurrObject->objectID     = QS_EMPTY_OID;
    sCurrObject->objectType   = aObjectType;
    sCurrObject->tableID      = aTableID;
    sCurrObject->next         = NULL;

    if( sCurrObject->objectType == QS_TABLE )
    {
        // Nothing To Do
    }
    else if( ( sCurrObject->objectType == QS_PKG ) ||
             ( sCurrObject->objectType == QS_PKG_BODY ) )
    {
        IDE_TEST( qcmSynonym::resolvePkg( aStatement,
                                          *aUserName,
                                          *aTableName,
                                          &(sCurrObject->objectID),
                                          &(sCurrObject->userID),
                                          &sExist,
                                          aSynonymInfo )
                  != IDE_SUCCESS );
    }
    else /* QS_PROC or QS_FUNC */
    {
        IDE_DASSERT( sCurrObject->objectType == QS_PROC );
        IDE_DASSERT( sCurrObject->objectType == QS_FUNC );
    }

    if( sExist == ID_FALSE )
    {
        sCurrObject->objectID = QS_EMPTY_OID;
    }
    else
    {
        // Nothing To Do
    }

    if( aSynonymInfo->isSynonymName == ID_TRUE )
    {
        //-------------------------------------
        // SYNONYM USER NAME
        //-------------------------------------
        sCurrObject->userName.size
            = idlOS::strlen( aSynonymInfo->objectOwnerName );

        IDE_TEST( sMemory->alloc( sCurrObject->userName.size + 1,
                                  (void**)&(sCurrObject->userName.name) )
                  != IDE_SUCCESS );

        idlOS::memcpy( sCurrObject->userName.name,
                       &(aSynonymInfo->objectOwnerName),
                       sCurrObject->userName.size );

        sCurrObject->userName.name[sCurrObject->userName.size] = '\0';

        IDE_TEST( qcmUser::getUserID( aStatement,
                                      aSynonymInfo->objectOwnerName,
                                      (UInt)(sCurrObject->userName.size),
                                      &(sCurrObject->userID) )
                  != IDE_SUCCESS);

        //-------------------------------------
        // SYNONYM OBJECT NAME
        //-------------------------------------
        sCurrObject->objectName.size = idlOS::strlen( aSynonymInfo->objectName );

        IDE_TEST( sMemory->alloc( sCurrObject->objectName.size + 1,
                                  (void**)&(sCurrObject->objectName.name) )
                  != IDE_SUCCESS );

        idlOS::memcpy( sCurrObject->objectName.name,
                       &(aSynonymInfo->objectName),
                       sCurrObject->objectName.size );

        sCurrObject->objectName.name[sCurrObject->objectName.size] = '\0';

        /* BUG-39821 set objectNamePos */
        if ( QC_IS_NULL_NAME( (*aUserName) ) == ID_FALSE )
        {
            /* aUserNamed에 들어올 수 있는 것은 user name 또는 synonym name이다.
               이를 구분하여 sCurrObject->objectNamePos에 설정해줘야 한다.

               ex) syn1이 pkg1의 synonym이라고 가정한다.
                   sys.syn1.func1 일 경우에는 aUserName에 user name이 존재한다.
                   syn1.rec1.field1 일 경우에는 aUserName에 package name이 존재한다. */
            if ( qcmUser::getUserID( aStatement, *aUserName, &sUserID ) 
                 == IDE_SUCCESS )
            {
                SET_POSITION( sCurrObject->objectNamePos, *aTableName );
            }
            else
            {
                IDE_CLEAR();
                SET_POSITION( sCurrObject->objectNamePos, *aUserName );
            }
        }
        else
        {
            SET_POSITION( sCurrObject->objectNamePos, *aTableName );
        }
    }
    else /* isSynonymName == ID_FALSE */
    {
        //-------------------------------------
        // USER NAME / OBJECT NAME
        //-------------------------------------

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                          SChar,
                                          QC_MAX_OBJECT_NAME_LEN + 1,
                                          &(sCurrObject->userName.name) )
                  != IDE_SUCCESS );

        // set user name
        sCurrObject->userName.size = aUserName->size;

        if( sCurrObject->userName.size > 0 ) 
        {
            /* check package name
               aUserName으로 package가 존재하는지 검색 */
            IDE_TEST ( qcmPkg::getPkgExistWithEmptyByNamePtr( aStatement,
                                                              sCurrObject->userID,
                                                              (SChar*)(aUserName->stmtText +
                                                                       aUserName->offset),
                                                              aUserName->size,
                                                              QS_PKG,
                                                              &sPkgOID )
                       != IDE_SUCCESS );

            /* aUserName은 package name 이다. */
            if ( sPkgOID == sCurrObject->objectID )
            {
                // connect user name
                IDE_TEST( qcmUser::getUserName( aStatement,
                                                sConnectUserID,
                                                sCurrObject->userName.name )
                          != IDE_SUCCESS );

                sCurrObject->userName.size
                    = idlOS::strlen(sCurrObject->userName.name);
                sCurrObject->userID = sConnectUserID;

                // set object name
                sCurrObject->objectName.size = aUserName->size;

                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                                  SChar,
                                                  (sCurrObject->objectName.size+1),
                                                  &(sCurrObject->objectName.name ) )
                          != IDE_SUCCESS );

                QC_STR_COPY( sCurrObject->objectName.name, *aUserName );

                // objectNamePos
                SET_POSITION( sCurrObject->objectNamePos, *aUserName );
            }
            else /* aUserName은 user name 이다. */
            {
                // set user name
                sCurrObject->userName.size = aUserName->size;

                QC_STR_COPY( sCurrObject->userName.name, *aUserName );

                IDE_TEST( qcmUser::getUserID( aStatement, *aUserName,
                                              &(sCurrObject->userID) )
                          != IDE_SUCCESS);

                // set object name
                sCurrObject->objectName.size = aTableName->size;

                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                                  SChar,
                                                  (sCurrObject->objectName.size+1),
                                                  &(sCurrObject->objectName.name) )
                          != IDE_SUCCESS );

                QC_STR_COPY( sCurrObject->objectName.name, *aTableName );

                // objectNamePos
                SET_POSITION( sCurrObject->objectNamePos, *aTableName );
            }
        }
        else /* userNamePos == NULL */
        {
            // connect user name
            IDE_TEST( qcmUser::getUserName( aStatement,
                                            sConnectUserID,
                                            sCurrObject->userName.name )
                      != IDE_SUCCESS );

            sCurrObject->userName.size = idlOS::strlen( sCurrObject->userName.name );
            sCurrObject->userID = sConnectUserID;

            // set object name
            sCurrObject->objectName.size = aTableName->size;

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                              SChar,
                                              (sCurrObject->objectName.size+1),
                                              &(sCurrObject->objectName.name) )
                      != IDE_SUCCESS );

            QC_STR_COPY( sCurrObject->objectName.name, *aTableName );

            // objectNamePos
            SET_POSITION( sCurrObject->objectNamePos, *aTableName );
        }
    }

    // return
    *aObject = sCurrObject;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::makePkgSynonymList( qcStatement    * aStatement,
                                        qcmSynonymInfo * aSynonymInfo,
                                        qcNamePosition   aUserName,
                                        qcNamePosition   aTableName,
                                        qsOID            aPkgID )
{
    iduVarMemList     * sMemory = QC_QMP_MEM( aStatement );
    qsSynonymList     * sSynonym;
    idBool              sExist = ID_FALSE;
    UInt                sUserID;

    if( aSynonymInfo->isSynonymName == ID_TRUE )
    {
        IDE_DASSERT( QC_IS_NULL_NAME( aTableName ) == ID_FALSE );

        for( sSynonym = aStatement->spvEnv->objectSynonymList;
             sSynonym != NULL;
             sSynonym = sSynonym->next )
        {
            if( QC_IS_NULL_NAME( aUserName ) == ID_TRUE )
            {
                if ( ( sSynonym->userName[0] == '\0' ) &&
                     ( idlOS::strMatch( aTableName.stmtText + aTableName.offset,
                                        aTableName.size,
                                        sSynonym->objectName,
                                        idlOS::strlen( sSynonym->objectName ) ) == 0 ) &&
                     ( sSynonym->isPublicSynonym == aSynonymInfo->isPublicSynonym ) )
                {
                    IDE_DASSERT( sSynonym->objectID == aPkgID );

                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // userName이 있는 정보가 object정보 일 경우
                if ( ( idlOS::strMatch( aUserName.stmtText + aUserName.offset,
                                        aUserName.size,
                                        sSynonym->objectName,
                                        idlOS::strlen( sSynonym->objectName ) ) == 0 ) &&
                     ( sSynonym->isPublicSynonym == aSynonymInfo->isPublicSynonym ) )
                {
                    IDE_DASSERT( sSynonym->objectID == aPkgID );

                    sExist = ID_TRUE;
                    break;
                }
                // user
                else if ( ( idlOS::strMatch( aUserName.stmtText + aUserName.offset,
                                             aUserName.size,
                                             sSynonym->userName,
                                             idlOS::strlen( sSynonym->userName ) ) == 0 ) &&
                          ( idlOS::strMatch( aTableName.stmtText + aTableName.offset,
                                             aTableName.size,
                                             sSynonym->objectName,
                                             idlOS::strlen( sSynonym->objectName ) ) == 0 ) &&
                          ( sSynonym->isPublicSynonym == aSynonymInfo->isPublicSynonym ) )
                {
                    IDE_DASSERT( sSynonym->objectID == aPkgID );

                    sExist = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            } 
        } // for

        if( sExist == ID_FALSE )
        {
            IDE_TEST( sMemory->alloc( ID_SIZEOF(qsSynonymList),
                                      (void**)&sSynonym )
                      != IDE_SUCCESS );

            if( QC_IS_NULL_NAME( aUserName ) == ID_TRUE )
            {
                sSynonym->userName[0] = '\0';

                idlOS::strncpy( sSynonym->objectName,
                                aTableName.stmtText + aTableName.offset,
                                aTableName.size );
                sSynonym->objectName[aTableName.size] = '\0';
            }
            else
            {
                /* check user name
                   aUserName으로 해당 이름의 user가 있는지 검사 */
                if ( qcmUser::getUserID( aStatement,
                                         aUserName,
                                         & sUserID )
                     == IDE_SUCCESS )
                {
                    idlOS::strncpy( sSynonym->userName,
                                    aUserName.stmtText + aUserName.offset,
                                    aUserName.size );
                    sSynonym->userName[aUserName.size] = '\0';

                    idlOS::strncpy( sSynonym->objectName,
                                    aTableName.stmtText + aTableName.offset,
                                    aTableName.size );
                    sSynonym->objectName[aTableName.size] = '\0';
                }
                else
                {
                    IDE_CLEAR();
                    sSynonym->userName[0] = '\0';

                    idlOS::strncpy( sSynonym->objectName,
                                    aUserName.stmtText + aUserName.offset,
                                    aUserName.size );
                    sSynonym->objectName[aUserName.size] = '\0';
                }
            }

            sSynonym->isPublicSynonym = aSynonymInfo->isPublicSynonym;
            sSynonym->objectID = aPkgID;

            sSynonym->next = aStatement->spvEnv->objectSynonymList;

            // 연결한다.
            aStatement->spvEnv->objectSynonymList = sSynonym;
        }
    }
    else
    {
        // Nothing to do.
    } // is not synonym_name

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::parseCreateProc( qcStatement * aStatement,
                                     qsPkgStmts  * aPkgStmt )
{
    qcuSqlSourceInfo   sqlInfo;
    qsPkgSubprograms * sSubprogram = NULL;
    qsProcParseTree  * sParseTree  = NULL;

    aStatement->spvEnv->currSubprogram = aPkgStmt;
    sSubprogram                        = (qsPkgSubprograms *)aPkgStmt;
    sParseTree                         = sSubprogram->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->procNamePos) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->procNamePos) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    // validation시에 참조되기 때문에 임시로 지정한다.
    // PVO가 완료되면 정상적인 값으로 지정된다.
    // subprogram의 pasreTree는 하나의 object의 parseTree라 아닌,
    // package 안에 있는 각 subprogram(procedure/function)의 parseTree이다.
    // 그렇기 때문에, userName과 userID가 처음 쓰레기값이 들어 있어서
    // package와 동일하게 맞춰준다.
    sParseTree->stmtText       = aStatement->myPlan->stmtText;
    sParseTree->stmtTextLen    = aStatement->myPlan->stmtTextLen;
    sParseTree->userNamePos    = aStatement->spvEnv->createPkg->userNamePos;
    sParseTree->userID         = aStatement->spvEnv->createPkg->userID;
    sParseTree->objectNameInfo = aStatement->spvEnv->createPkg->objectNameInfo;
    /* BUG-45306 PSM AUTHID 
       package subprogram의 authid는 package의 authid와 동일하게 설정해야 한다. */
    sParseTree->isDefiner      = aStatement->spvEnv->createPkg->isDefiner;

    IDE_TEST( qsv::checkHostVariable( aStatement ) != IDE_SUCCESS );

    aPkgStmt->state = QS_PKG_STMTS_PARSE_FINISHED;

    aStatement->spvEnv->currSubprogram = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    aStatement->spvEnv->currSubprogram = NULL;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::validateCreateProcHeader( qcStatement       * aStatement,
                                              qsPkgStmts        * aPkgStmt ) 
{
    qsPkgSubprograms   * sSubprogram;
    qsProcParseTree    * sParseTree;
    idBool               sIsPrivate    = ID_FALSE;
    qsPkgStmts         * sOriPkgStmt   = NULL;
    qsProcParseTree    * sOriParseTree = NULL;
    qsProcStmts        * sOriProcStmt  = NULL;

    sSubprogram = ( qsPkgSubprograms * )( aPkgStmt );
    sParseTree  = sSubprogram->parseTree;

    sOriPkgStmt   = aStatement->spvEnv->currSubprogram; 
    sOriParseTree = aStatement->spvEnv->createProc;
    sOriProcStmt  = aStatement->spvEnv->currStmt;

    aStatement->spvEnv->currSubprogram = aPkgStmt;
    aStatement->spvEnv->createProc     = sParseTree;
    aStatement->spvEnv->currStmt       = NULL;

    // qsProcParseTree의 OID설정
    // subprogram의 OID는 package spec의 OID를 따른다.
    if( aStatement->spvEnv->createPkg->objType == QS_PKG )
    {
        sParseTree->procOID = aStatement->spvEnv->createPkg->pkgOID;
    }
    else
    {
        // Private Subprogram
        if( sSubprogram->subprogramID > aStatement->spvEnv->pkgPlanTree->subprogramCount )
        {
            sParseTree->procOID = QS_EMPTY_OID;
            sIsPrivate = ID_TRUE;
        }
        else
        {
            sParseTree->procOID = aStatement->spvEnv->pkgPlanTree->pkgOID;
        }
        sParseTree->pkgBodyOID = aStatement->spvEnv->createPkg->pkgOID;
    }

    // validation parameter
    IDE_TEST( qsvProcVar::validateParaDef( aStatement, sParseTree->paraDecls )
              != IDE_SUCCESS );

    IDE_TEST( qsvProcVar::setParamModules( aStatement,
                                           sParseTree->paraDeclCount,
                                           sParseTree->paraDecls,
                                           (mtdModule***)&(sParseTree->paramModules),
                                           &(sParseTree->paramColumns) )
              != IDE_SUCCESS );

    /* BUG-39220
       subprogram 이름과 parameter 갯수만 같을 수 있으므로,
       parameter 및 return 대한 상세한 비교가 필요하다. */
    if ( aStatement->spvEnv->createPkg->objType == QS_PKG_BODY )
    {
        IDE_TEST( compareParametersAndReturnVar( aStatement,
                                                 sIsPrivate,
                                                 ID_TRUE,
                                                 sSubprogram )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( sParseTree->objType == QS_FUNC )
    {
        IDE_TEST( validateCreateFuncHeader( aStatement,
                                            aPkgStmt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    aPkgStmt->state = QS_PKG_STMTS_VALIDATE_HEADER_FINISHED;

    aStatement->spvEnv->currSubprogram = sOriPkgStmt; 
    aStatement->spvEnv->createProc     = sOriParseTree;
    aStatement->spvEnv->currStmt       = sOriProcStmt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aStatement->spvEnv->currSubprogram = sOriPkgStmt; 
    aStatement->spvEnv->createProc     = sOriParseTree;
    aStatement->spvEnv->currStmt       = sOriProcStmt;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::validateCreateProcBody( qcStatement       * aStatement,
                                            qsPkgStmts        * aPkgStmt,
                                            qsValidatePurpose   aPurpose )
{
    qsPkgParseTree   * sPkgParseTree = NULL;
    qsPkgSubprograms * sSubprogram   = NULL;
    qsProcParseTree  * sParseTree    = NULL;
    qsLabels         * sCurrLabel    = NULL;

    sPkgParseTree = aStatement->spvEnv->createPkg;
    sSubprogram   = ( qsPkgSubprograms * )( aPkgStmt );
    sParseTree    = sSubprogram->parseTree;

    aStatement->spvEnv->currSubprogram = aPkgStmt;
    aStatement->spvEnv->createProc     = sParseTree;
    aStatement->spvEnv->currStmt       = NULL;

    if( sParseTree->block != NULL )
    {
        // set label name (= procedure name)
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qsLabels, &sCurrLabel )
                  != IDE_SUCCESS );

        SET_POSITION( sCurrLabel->namePos, sParseTree->procNamePos );
        sCurrLabel->id      = qsvEnv::getNextId( aStatement->spvEnv );
        sCurrLabel->stmt    = NULL;
        sCurrLabel->next    = NULL;

        sParseTree->block->common.parentLabels = sCurrLabel;
        // PR-24408
        QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

        // parsing body
        IDE_TEST( sParseTree->block->common.parse( aStatement,
                                                   (qsProcStmts *)(sParseTree->block) )
                  != IDE_SUCCESS );

        // validation body
        IDE_TEST( sParseTree->block->common.validate( aStatement,
                                                      (qsProcStmts *)(sParseTree->block),
                                                      NULL,
                                                      aPurpose )
                  != IDE_SUCCESS );
    }
    else
    {
        if( (sParseTree->procType == QS_EXTERNAL_C) ||
            (sParseTree->procType == QS_INTERNAL_C) )
        {
            IDE_TEST( qsv::validateExtproc( aStatement, sParseTree ) != IDE_SUCCESS )
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( (sPkgParseTree->objType == QS_PKG_BODY) && 
         (aPkgStmt->stmtType == QS_FUNC) &&
         ((aStatement->spvEnv->flag & QSV_ENV_RETURN_STMT_MASK)
          == QSV_ENV_RETURN_STMT_ABSENT) &&
         ((sParseTree->block != NULL ) ||
          (sParseTree->procType != QS_NORMAL)) )
    {
        IDE_RAISE(ERR_NOT_HAVE_RETURN_STMT);

    }
    else
    {
        // Nothing to do.
    }

    aPkgStmt->state = QS_PKG_STMTS_VALIDATE_BODY_FINISHED;

    aStatement->spvEnv->currSubprogram = NULL;
    aStatement->spvEnv->createProc     = NULL;
    aStatement->spvEnv->currStmt       = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_HAVE_RETURN_STMT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_STMT ) );
    }
    IDE_EXCEPTION_END;

    aStatement->spvEnv->currSubprogram = NULL;
    aStatement->spvEnv->createProc     = NULL;
    aStatement->spvEnv->currStmt       = NULL;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::validateCreateFuncHeader( qcStatement       * aStatement,
                                              qsPkgStmts        * aPkgStmt )
{
    qsProcParseTree    * sParseTree;
    mtcColumn          * sReturnTypeNodeColumn;
    qcuSqlSourceInfo     sqlInfo;
    qsVariables        * sReturnTypeVar;
    idBool               sIsPrivate = ID_FALSE;
    idBool               sFound     = ID_FALSE;

    sParseTree     = ((qsPkgSubprograms *)aPkgStmt)->parseTree;
    sReturnTypeVar = sParseTree->returnTypeVar;

    // 적합성 검사. return타입은 반드시 있어야 함.
    IDE_DASSERT( sReturnTypeVar != NULL );

    // validation return data type
    if( sReturnTypeVar->variableType == QS_COL_TYPE )
    {
        // return value에서 column type을 허용하는 경우는 단 2가지.
        // (1) table_name.column_name%TYPE
        // (2) user_name.table_name.column_name%TYPE
        if( QC_IS_NULL_NAME( sReturnTypeVar->variableTypeNode->tableName ) == ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnTypeVar->variableTypeNode->position );
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
        else
        {
            IDE_TEST( qsvProcVar::checkAttributeColType( aStatement,
                                                         sReturnTypeVar )
                      != IDE_SUCCESS );

            sReturnTypeVar->variableType = QS_PRIM_TYPE;
        }
    }
    else if( sReturnTypeVar->variableType == QS_PRIM_TYPE )
    {
        // Nothing to do.
    }
    else if( sReturnTypeVar->variableType == QS_ROW_TYPE )
    {
        if( QC_IS_NULL_NAME( sReturnTypeVar->variableTypeNode->userName ) == ID_TRUE )
        {
            // PROJ-1075 rowtype 생성 허용.
            // (1) user_name.table_name%ROWTYPE
            // (2) table_name%ROWTYPE
            IDE_TEST( qsvProcVar::checkAttributeRowType( aStatement, sReturnTypeVar )
                      != IDE_SUCCESS );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnTypeVar->variableTypeNode->columnName );
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
    }
    else if( sReturnTypeVar->variableType == QS_UD_TYPE )
    {
        // PROJ-1075 parameter에 udt 허용.
        // typeset_name.type_name 와 user_name.typeset_name.type_name 만 허용해야 함.
        /* BUG-41720
           동일 package에 선언된 array type을 return value에 사용 가능해야 한다. */
        if( (QC_IS_NULL_NAME( sReturnTypeVar->variableTypeNode->tableName ) == ID_TRUE) &&
            (aStatement->spvEnv->createPkg == NULL) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnTypeVar->variableTypeNode->columnName );
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
        else
        {
            IDE_TEST( qsvProcVar::makeUDTVariable( aStatement,
                                                   sReturnTypeVar )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // 적합성 검사. 이 이외의 타입은 절대로 올 수 없음.
        IDE_DASSERT(0);
    }

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            mtcColumn,
                            &(sParseTree->returnTypeColumn) )
              != IDE_SUCCESS );

    sReturnTypeNodeColumn = QTC_STMT_COLUMN( aStatement,
                                             sReturnTypeVar->variableTypeNode );

    // to fix BUG-5773
    sReturnTypeNodeColumn->column.id     = 0;
    sReturnTypeNodeColumn->column.offset = 0;

    // sReturnTypeNodeColumn->flag = 0;
    // to fix BUG-13600 precision, scale을 복사하기 위해
    // column argument값은 저장해야 함.
    sReturnTypeNodeColumn->flag = sReturnTypeNodeColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;

    sReturnTypeNodeColumn->type.languageId = sReturnTypeNodeColumn->language->id;

    /* PROJ-2586 PSM Parameters and return without precision
       아래 조건 중 하나만 만족하면 precision을 조정하기 위한 함수 호출.

       조건 1. QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 1이면서
               datatype의 flag에 QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT 일 것.
       조건 2. QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 2 */
    /* 왜 아래쪽에 복사했는가?
       alloc전에 하면, mtcColumn에 셋팅했던 flag 정보가 날라감. */
    if( ((QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 1) &&
          (((sReturnTypeVar->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK)
            == QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT))) /* 조건1 */ ||
        (QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 2) /* 조건2 */ )
    {
        IDE_TEST( qsv::setPrecisionAndScale( aStatement,
                                             sReturnTypeVar )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    /* BUG-44382 clone tuple 성능개선 */
    // 복사와 초기화가 필요함
    qtc::setTupleColumnFlag(
        QTC_STMT_TUPLE( aStatement, sReturnTypeVar->variableTypeNode ),
        ID_TRUE,
        ID_TRUE );

    mtc::copyColumn( sParseTree->returnTypeColumn,
                     sReturnTypeNodeColumn );

    // mtdModule 설정
    sParseTree->returnTypeModule = (mtdModule *)sReturnTypeNodeColumn->module;

    if ( aStatement->spvEnv->createPkg->objType == QS_PKG_BODY )
    {
        /* BUG-39220
           subprogram 이름과 parameter 갯수만 같을 수 있으므로,
           parameter 및 return 대한 상세한 비교가 필요하다. */
        if ( sParseTree->procOID == QS_EMPTY_OID )
        {
            sIsPrivate = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( compareParametersAndReturnVar( aStatement,
                                                 sIsPrivate,
                                                 ID_FALSE,
                                                 (qsPkgSubprograms*)aPkgStmt )
                  != IDE_SUCCESS );

        /* PROJ-1685 , BUG-41818
           external procedure에서 return type으로 지원하지 않는 type에 대해
           에러를 발생해야 한다 */
        if( sParseTree->block == NULL )
        {
            // external procedure
            sFound = ID_FALSE;

            switch( sParseTree->returnTypeColumn->type.dataTypeId )
            {
                case MTD_BIGINT_ID:
                case MTD_BOOLEAN_ID:
                case MTD_SMALLINT_ID:
                case MTD_INTEGER_ID:
                case MTD_REAL_ID:
                case MTD_DOUBLE_ID:
                case MTD_CHAR_ID:
                case MTD_VARCHAR_ID:
                case MTD_NCHAR_ID:
                case MTD_NVARCHAR_ID:
                case MTD_NUMERIC_ID:
                case MTD_NUMBER_ID:
                case MTD_FLOAT_ID:
                case MTD_DATE_ID:
                case MTD_INTERVAL_ID:
                    sFound = ID_TRUE;
                    break;
                default:
                    break;
            }

            if ( sFound == ID_FALSE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sParseTree->returnTypeVar->common.name );

                IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // normal procedure
            sFound = ID_TRUE;

            switch( sParseTree->returnTypeColumn->type.dataTypeId )
            {
                case MTD_LIST_ID:
                    // list type을 반환하는 user-defined function은 생성할 수 없다.
                    sFound = ID_FALSE;
                    break;
                default:
                    break;
            }

            if ( sFound == ID_FALSE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sParseTree->returnTypeVar->common.name );

                IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
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

    /* PROJ-2452 Cache for DETERMINISTIC function */
    IDE_TEST( qtcCache::setIsCachableForFunction( sParseTree->paraDecls,
                                                  sParseTree->paramModules,
                                                  sParseTree->returnTypeModule,
                                                  &sParseTree->isDeterministic,
                                                  &sParseTree->isCachable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORTED_DATATYPE );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_PROC_NOT_SUPPORTED_DATATYPE_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::checkSubprogramDef( qcStatement * aStatement,
                                        qsPkgStmts  * aBodyStmt,
                                        qsPkgStmts  * aSpecStmt )
{
    // subprogram의 name 비교
    qcuSqlSourceInfo   sqlInfo;
    qsPkgStmts       * sBodyStmt;
    qsPkgSubprograms * sBodySub;
    qsPkgSubprograms * sSpecSub;
    qsProcParseTree  * sBodyParseTree;
    qsProcParseTree  * sSpecParseTree;
    idBool             sExist       = ID_FALSE;
    SChar              sSubprogramName[QC_MAX_OBJECT_NAME_LEN+1];

    sSpecSub       = (qsPkgSubprograms *)aSpecStmt;
    sSpecParseTree = sSpecSub->parseTree;

    for( sBodyStmt = aBodyStmt;
         sBodyStmt != NULL;
         sBodyStmt = sBodyStmt->next )
    {
        if( sBodyStmt->stmtType != QS_OBJECT_MAX )
        { 
            sBodySub = (qsPkgSubprograms *)sBodyStmt;
            sBodyParseTree = sBodySub->parseTree;

            if( sBodySub->subprogramID == QS_PSM_SUBPROGRAM_ID )
            {
                if ( QC_IS_NAME_MATCHED( sBodyParseTree->procNamePos, sSpecParseTree->procNamePos ) &&
                     ( sBodyStmt->stmtType == aSpecStmt->stmtType ) && 
                     ( sBodyParseTree->paraDeclCount == sSpecParseTree->paraDeclCount ) ) 
                {
                    if( sBodyParseTree->block != NULL )
                    {
                        sBodySub->subprogramID   = sSpecSub->subprogramID;
                        sBodySub->subprogramType = QS_PUBLIC_SUBPROGRAM;
                        sBodyStmt->flag          = aSpecStmt->flag;
                        sExist                   = ID_TRUE;
                        break;
                    }
                    else
                    {
                        if( (sBodyParseTree->procType == QS_EXTERNAL_C) ||
                            (sBodyParseTree->procType == QS_INTERNAL_C) )
                        {
                            sBodySub->subprogramID   = sSpecSub->subprogramID;
                            sBodySub->subprogramType = QS_PUBLIC_SUBPROGRAM;
                            sBodyStmt->flag          = aSpecStmt->flag;
                            sExist                   = ID_TRUE;
                            break;
                        }
                        else
                        {
                            sqlInfo.setSourceInfo( aStatement, &( sBodyParseTree->procNamePos ) );
                            IDE_RAISE( ERR_DUP_PKG_SUBPROGRAM );
                        }
                    }
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
            // Nothing to do.
        }
    }

    if( sExist == ID_FALSE )
    {
        QC_STR_COPY( sSubprogramName, sSpecParseTree->procNamePos );

        IDE_RAISE( ERR_NOT_DEFINED_ALL_SUBPROGRAM );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUP_PKG_SUBPROGRAM )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_DUPLICATE_PKG_SUBPROGRAM,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_DEFINED_ALL_SUBPROGRAM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_INVALID_OR_MISSING_SUBPROGRAM, sSubprogramName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::getPkgSubprogramID( qcStatement    * aStatement,
                                        qtcNode        * aNode,
                                        qcNamePosition   aUserName,
                                        qcNamePosition   aTableName,
                                        qcNamePosition   aColumnName,
                                        qsxPkgInfo     * aPkgInfo,
                                        UInt           * aSubprogramID )
{
/********************************************************
 * 본 함수에서는 subprogramID를 셋팅한다.
 *
 *  No | userName | tableName | columnName | pkgName
 * ------------------------------------------------
 * (1) |          |   package | subprogram |
 * (2) |     user |   package | subprogram |
 * (3) |          |   package |   variable |
 * (4) |  package |    record |      field |
 * (5) |     user |   package |   variable |
 * (6) |     user |   package |     record | field
 *********************************************************/
    qsPkgParseTree          * sParseTree = aPkgInfo->planTree;
    UInt                      sSubprogramID = QS_PSM_SUBPROGRAM_ID;
    qsPkgSubprogramType       sSubprogramType = QS_NONE_TYPE;
    idBool                    sExist = ID_FALSE;
    qsVariableItems         * sVariable;
    mtcNode                 * sNode;
    UInt                      sArgCount;
    qcNamePosition            sColumnName;

    SET_EMPTY_POSITION( sColumnName );

    if( QC_IS_NULL_NAME( aUserName ) == ID_TRUE )
    { 
        // (1) , (3)

        // BUG-41386
        IDE_DASSERT( QC_IS_NAME_MATCHED( aTableName, sParseTree->pkgNamePos ) );
        sColumnName = aColumnName;
    }
    else
    {
        // BUG-37163
        // user name과 package name이 동일한 경우, user name을 package name으로 판단하는 경우가 발생한다.
        // package name과 user name이 동일하지 않고, package name일 때 package name으로 판단한다.
        if ( ( QC_IS_NAME_MATCHED( aUserName, sParseTree->pkgNamePos ) ) &&
             ( QC_IS_NAME_MATCHED( aUserName, aTableName ) == ID_FALSE) )
        {
            // (4)
            sColumnName = aTableName;
        }
        else
        {
            // (2) , (5) , (6)
            sColumnName = aColumnName;
        }
    }

    // BUG-41262 PSM overloading
    // 인자가 estimate 되어야 PSM overloading 이 가능하다.
    for( sNode = aNode->node.arguments, sArgCount = 0;
         sNode != NULL;
         sNode = sNode->next )
    {
        if ( ( ((qtcNode*)sNode)->lflag & QTC_NODE_SP_PARAM_NAME_MASK )
               == QTC_NODE_SP_PARAM_NAME_TRUE )
        {
            continue;
        }
        else
        {
            sArgCount++;
        }

        if ( ( aStatement->spvEnv->flag & QSV_ENV_ESTIMATE_ARGU_MASK )
               == QSV_ENV_ESTIMATE_ARGU_TRUE )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qtc::estimate( (qtcNode*)sNode,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 NULL,
                                 NULL,
                                 NULL )
                  != IDE_SUCCESS );
    }

    // BUG-41262 PSM overloading
    IDE_TEST( matchingSubprogram( aStatement,
                                  sParseTree,
                                  aNode,
                                  sArgCount,
                                  sColumnName,
                                  ID_FALSE,
                                  &sExist,
                                  &sSubprogramType,
                                  &sSubprogramID )
              != IDE_SUCCESS );

    // check variable name
    if( sExist == ID_FALSE )
    {
        for( sVariable = sParseTree->block->variableItems ;
             sVariable != NULL;
             sVariable = sVariable->next )
        {
            // fix BUG-41302
            if ( ( sVariable->itemType == QS_VARIABLE ) &&
                 QC_IS_NAME_MATCHED( sColumnName, sVariable->name ) )
            {
                sSubprogramID = QS_PKG_VARIABLE_ID;
                sExist = ID_TRUE;
                break;
            }
            else
            {
                sExist = ID_FALSE;
            }
        }
    }
    else
    {
        // Nothing to do.
        // subprogram 존재
    }

    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_NOT_EXIST_SUBPROGRAM );

    *aSubprogramID = sSubprogramID;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SUBPROGRAM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_NOT_EXIST_SUBPROGRAM_AND_VARIABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::getMyPkgSubprogramID( qcStatement         * aStatement,
                                          qtcNode             * aNode,
                                          qsPkgSubprogramType * aSubprogramType,
                                          UInt                * aSubprogramID )
{
/*************************************************************************************
 * 본 함수에서는  private subprogram의 ID를 찾는다.
 * create package body 시 사용된다.
 *************************************************************************************/

    qsPkgParseTree          * sParseTree     = aStatement->spvEnv->createPkg;
    qsPkgParseTree          * sSpecParseTree = aStatement->spvEnv->pkgPlanTree;
    UInt                      sSubprogramID = QS_PSM_SUBPROGRAM_ID;
    qsPkgSubprogramType       sSubprogramType = QS_NONE_TYPE;
    idBool                    sExist = ID_FALSE;
    UInt                      sUserID;
    mtcNode                 * sNode;
    UInt                      sArgCount;

    // check userName 
    if( QC_IS_NULL_NAME( aNode->userName ) == ID_FALSE )
    {
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      aNode->userName,
                                      &sUserID )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sUserID != sParseTree->userID,
                        ERR_NOT_EXIST_SUBPROGRAM );
    }
    else
    {
        // Nothing to do.
    }

    // check pkgName
    if( QC_IS_NULL_NAME( aNode->tableName ) == ID_FALSE )
    { 
        IDE_TEST_RAISE( QC_IS_NAME_MATCHED( aNode->tableName,
                                            sParseTree->pkgNamePos )
                        != ID_TRUE,
                        ERR_NOT_EXIST_SUBPROGRAM );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-41262 PSM overloading
    // 인자가 estimate 되어야 PSM overloading 이 가능하다.
    for( sNode = aNode->node.arguments, sArgCount = 0;
         sNode != NULL;
         sNode = sNode->next )
    {
        if ( ( ((qtcNode*)sNode)->lflag & QTC_NODE_SP_PARAM_NAME_MASK )
               == QTC_NODE_SP_PARAM_NAME_TRUE )
        {
            continue;
        }
        else
        {
            sArgCount++;
        }

        if ( ( aStatement->spvEnv->flag & QSV_ENV_ESTIMATE_ARGU_MASK )
               == QSV_ENV_ESTIMATE_ARGU_TRUE )
        {
            continue;
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( qtc::estimate( (qtcNode*)sNode,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 NULL,
                                 NULL,
                                 NULL )
                  != IDE_SUCCESS );
    }

    // BUG-41262 PSM overloading
    // 먼저 pkg spec 에서 찾는다. private 서브 프로그램은 없다.

    // BUG-41695 create or replace package 에서도 호출될 수 있다.
    if ( sSpecParseTree != NULL )
    {
        IDE_TEST( matchingSubprogram( aStatement,
                                      sSpecParseTree,
                                      aNode,
                                      sArgCount,
                                      aNode->columnName,
                                      ID_TRUE,
                                      &sExist,
                                      &sSubprogramType,
                                      &sSubprogramID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // private 서브 프로그램을 pkg body 에서 찾는다.
    if ( sExist == ID_FALSE )
    {
        IDE_TEST( matchingSubprogram( aStatement,
                                      sParseTree,
                                      aNode,
                                      sArgCount,
                                      aNode->columnName,
                                      ID_FALSE,
                                      &sExist,
                                      &sSubprogramType,
                                      &sSubprogramID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    *aSubprogramType = sSubprogramType;
    *aSubprogramID   = sSubprogramID;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SUBPROGRAM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_NOT_EXIST_SUBPROGRAM_AND_VARIABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::makeUsingSubprogramList( qcStatement         * aStatement, 
                                             qtcNode             * aNode )
{
    qsProcStmtSql      * sSql;
    qsUsingSubprograms * sCurrNode        = NULL;
    qsUsingSubprograms * sNewNode         = NULL;

    sSql = (qsProcStmtSql *)( aStatement->spvEnv->currStmt );

    // BUG-38099
    IDE_TEST_CONT( sSql == NULL, skip_make_usingsubprogram );

    IDE_TEST_CONT( qsvProcStmts::isSQLType(sSql->common.stmtType) != ID_TRUE, 
                   skip_make_usingsubprogram);

    sCurrNode = sSql->usingSubprograms;

    /* BUG-38456
       columnName과 position의 offset이 다르다는 것은
       aNode의 tableName 또는 userName이 명시되어있다는 의미이다. */
    IDE_TEST_CONT( aNode->columnName.offset != aNode->position.offset,
                   skip_make_usingsubprogram );

    IDE_TEST( QC_QME_MEM(aStatement)->alloc( ID_SIZEOF(qsUsingSubprograms),
                                             (void**)&sNewNode )
              != IDE_SUCCESS );

    sNewNode->subprogramNode = aNode;
    sNewNode->next = NULL;

    if( sCurrNode == NULL )
    {
        sSql->usingSubprograms = sNewNode;
    }
    else
    {
        if( sNewNode->subprogramNode->position.offset <
            sCurrNode->subprogramNode->position.offset )
        {
            // newNode가 제일 작을 경우
            sNewNode->next = sCurrNode;
            sSql->usingSubprograms = sNewNode;
        }
        else
        {
            while( sCurrNode->next != NULL )
            {
                if( sNewNode->subprogramNode->position.offset <
                    sCurrNode->next->subprogramNode->position.offset )
                {
                    sNewNode->next = sCurrNode->next;
                    sCurrNode->next = sNewNode;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                sCurrNode = sCurrNode->next;
            }

            // new Node가 제일 클 경우
            if( sCurrNode->next == NULL )
            {
                sCurrNode->next = sNewNode;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_make_usingsubprogram );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-37655
IDE_RC qsvPkgStmts::validatePragmaRestrictReferences( qcStatement    * aStatement,
                                                      qsPkgStmtBlock * aBLOCK,
                                                      qsPkgStmts     * aPkgStmt )
{
/***********************************************************************
 *
 * Description :
 *    validate pragma restrict references
 *
 * Implementation :
 *    (1) deault/특정 subprogram 확인
 *        (a) default
 *            (a-1) default option이면 적용
 *            (a-2) 그렇지 않을 경우(해당 subprogram만 적용된 option일 경우)
 *                  default option을 적용하지 않는다.
 *        (b) 특정 subprogram
 *            (b-1) defualt option이 존재할 경우, 초기화하고 옵션 적용
 ***********************************************************************/

    qsPkgStmts                 * sCurrStmt;
    qsPkgSubprograms           * sCurrSub;
    qsProcParseTree            * sCurrSubParseTree;
    qsRestrictReferences       * sRestrictReferences;
    qsRestrictReferencesOption * sRestrictReferencesOptions;
    ULong                        sFlag = 0;   // option들의 or 연산
    idBool                       sIsExist = ID_FALSE;
    qcuSqlSourceInfo             sqlInfo;

    sRestrictReferences = (qsRestrictReferences *)aPkgStmt;

    for( sRestrictReferencesOptions = sRestrictReferences->options;
         sRestrictReferencesOptions != NULL;
         sRestrictReferencesOptions = sRestrictReferencesOptions->next )
    {
        switch( sRestrictReferencesOptions->option )
        {
            case QS_PRAGMA_RESTRICT_REFERENCES_RNDS :
                sFlag |= QS_PRAGMA_RESTRICT_REFERENCES_RNDS;
                break;
            case QS_PRAGMA_RESTRICT_REFERENCES_WNDS :
                sFlag |= QS_PRAGMA_RESTRICT_REFERENCES_WNDS;
                break;
        }
    }

    // default인지 아닌지 확인
    if( QC_IS_NULL_NAME( sRestrictReferences->subprogramName ) == ID_TRUE )
    {
        for( sCurrStmt = aBLOCK->subprograms;
             sCurrStmt != NULL;
             sCurrStmt = sCurrStmt->next )
        {
            if( sCurrStmt->stmtType != QS_OBJECT_MAX )
            {
                if( QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION(
                        sCurrStmt->flag,
                        QS_PRAGMA_RESTRICT_REFERENCES_SUBPROGRAM )
                    == ID_FALSE )
                {
                    sCurrStmt->flag &= QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED;
                    sCurrStmt->flag = sFlag;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                // subprogram이 아님.
                // Nothing to do.
            }
        }
    }
    else
    {
        for( sCurrStmt = aBLOCK->subprograms;
             sCurrStmt != aPkgStmt;
             sCurrStmt = sCurrStmt->next )
        {
            if( sCurrStmt->stmtType != QS_OBJECT_MAX )
            {
                sCurrSub = (qsPkgSubprograms *)sCurrStmt;
                sCurrSubParseTree = sCurrSub->parseTree;

                if ( QC_IS_NAME_MATCHED( sCurrSubParseTree->procNamePos, sRestrictReferences->subprogramName ) )
                {
                    if( QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION(
                            sCurrStmt->flag,
                            QS_PRAGMA_RESTRICT_REFERENCES_SUBPROGRAM )
                        == ID_FALSE )
                    {
                        sCurrStmt->flag &= QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED;
                        sCurrStmt->flag = sFlag;
                    }
                    else
                    {
                        sCurrStmt->flag |= sFlag;
                    }

                    sCurrStmt->flag |= QS_PRAGMA_RESTRICT_REFERENCES_SUBPROGRAM;
                    sIsExist = ID_TRUE;

                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // subprogram이 아님.
                // Nothing to do.
            }
        }

        if( sIsExist == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, & sRestrictReferences->subprogramName );
            IDE_RAISE( ERR_NOT_EXIST_SUBPROGRAM_IN_PACKAGE );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SUBPROGRAM_IN_PACKAGE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_UNDECLARED_SUBPROGRAM,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::checkPragmaRestrictReferencesOption( qcStatement * aStatement,
                                                         qsPkgStmts  * aPkgStmt,
                                                         qsProcStmts * aProcStmt,
                                                         idBool        aIsPkgInitSection )
{
    qcuSqlSourceInfo   sqlInfo;
    qsPkgStmts       * sPkgStmt;
    qsPkgStmts       * sTmpStmt;
    qsPkgParseTree   * sPkgSpecParseTree;
    qsProcParseTree  * sProcParseTree;
    idBool             sIsErrMsg = ID_FALSE;

    if( aPkgStmt != NULL )
    {
        sProcParseTree = ((qsPkgSubprograms *)aPkgStmt)->parseTree;

        if( sProcParseTree->procType == QS_NORMAL )
        {
            /* BUG-38509 autonomous transaction 
               autonomous transaction block일 때는 pragma restrict references 는 적용되지 않는다 */
            if( ( aPkgStmt->flag != QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED ) &&
                ( sProcParseTree->block->isAutonomousTransBlock == ID_FALSE ) )
            {
                if( QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION(
                        aPkgStmt->flag,
                        QS_PRAGMA_RESTRICT_REFERENCES_RNDS )
                    == ID_TRUE )
                {
                    if( qsvProcStmts::isFetchType( aProcStmt->stmtType ) == ID_TRUE )
                    {
                        IDE_RAISE( ERR_VIOLATES_PRAGMA )
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

                if( QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION(
                        aPkgStmt->flag,
                        QS_PRAGMA_RESTRICT_REFERENCES_WNDS )
                    == ID_TRUE )
                {
                    if( qsvProcStmts::isDMLType( aProcStmt->stmtType ) == ID_TRUE )
                    {
                        IDE_RAISE( ERR_VIOLATES_PRAGMA )
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
                // 제약이 존재하지 않음.
                // Nothing to do.
            } /* aPkgStmt->flag == QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED */
        }
        else
        {
            // external procedure / function
            // Nothing to do.
        }
    }
    else
    {
        IDE_DASSERT( aPkgStmt == NULL );

        /* BUG-39003
           aPkgStmt == NULL일 경우
           1. validate variable.
           2. validate initialize section.

           aIsPkgInitSection == ID_FALSE일 경우는 variable에 대한 validation일 때이다. */
        IDE_TEST_CONT( aIsPkgInitSection != ID_TRUE , is_not_initialize_section );

        /* initialize section */
        sPkgSpecParseTree = aStatement->spvEnv->pkgPlanTree;

        if( sPkgSpecParseTree != NULL )
        {
            for( sPkgStmt = sPkgSpecParseTree->block->subprograms;
                 sPkgStmt != NULL;
                 sPkgStmt = sPkgStmt->next )
            {
                if( sPkgStmt->stmtType != QS_OBJECT_MAX )
                {
                    if( sPkgStmt->flag != QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED )
                    {
                        if( QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION(
                                sPkgStmt->flag,
                                QS_PRAGMA_RESTRICT_REFERENCES_RNDS )
                            == ID_TRUE )
                        {
                            if( aProcStmt->stmtType == QS_PROC_STMT_SELECT )
                            {
                                sIsErrMsg = ID_TRUE;
                                break;
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

                        if( QS_CHECK_PRAGMA_RESTRICT_REFERENCES_CONDITION(
                                sPkgStmt->flag,
                                QS_PRAGMA_RESTRICT_REFERENCES_WNDS )
                            == ID_TRUE )
                        {
                            if( (aProcStmt->stmtType == QS_PROC_STMT_INSERT) ||
                                (aProcStmt->stmtType == QS_PROC_STMT_DELETE) ||
                                (aProcStmt->stmtType == QS_PROC_STMT_MOVE) ||
                                (aProcStmt->stmtType == QS_PROC_STMT_MERGE) ||
                                (aProcStmt->stmtType == QS_PROC_STMT_UPDATE) )
                            {
                                sIsErrMsg = ID_TRUE;
                                break;
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
                        // Nothing to do.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }

            if( sIsErrMsg == ID_TRUE )
            {
                for( sTmpStmt = aStatement->spvEnv->createPkg->block->subprograms ;
                     sTmpStmt != NULL ;
                     sTmpStmt = sTmpStmt->next )
                {
                    if ( QC_IS_NAME_MATCHED( ((qsPkgSubprograms *)sPkgStmt)->parseTree->procNamePos,
                                             ((qsPkgSubprograms *)sTmpStmt)->parseTree->procNamePos ) )
                    {
                        IDE_RAISE( ERR_VIOLATES_PRAGMA )
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    IDE_EXCEPTION_CONT( is_not_initialize_section );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_VIOLATES_PRAGMA)
    {
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_VIOLATES_ASSOCIATED_PRAGMA) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::getPkgSubprogramPlanTree( qcStatement          * aStatement,
                                              qsPkgParseTree       * aPkgPlanTree,
                                              qsObjectType           aPkgType,
                                              UInt                   aSubprogramID,
                                              qsProcParseTree     ** aSubprogramPlanTree )
{
/*****************************************************************************
 *
 * Description : 
 *
 *    Package subprogram의 plan tree를 찾는다.
 *
 * Implementation :
 *
 * (1) 동일한 package에서 호출할 경우
 *    - public subprogram
 *        ㄴpackage spec에 있는지 확인
 *        ㄴpackage body에서 동일 plan tree를 찾음
 *            a. header 부분에 대한 validation이 되어 있지 않다면,
 *              이를 validation 함.
 *    - private subprogram
 *        ㄴ현재 validation 중인 subprogram 앞에
 *         선언 또는 정의 된 plan tree한에서 찾음.
 * (2) 외부 package의 subprogram을 호출할 경우
 *     - aPkgParseTree에서 aSubprogramID와 동일한 plan tree를 찾아 반환
 *
 ****************************************************************************/

    qsPkgParseTree   * sPkgSpecPlanTree    = NULL;
    qsProcParseTree  * sSubprogramPlanTree = NULL;
    qsPkgStmts       * sPkgStmt            = NULL;
    qsPkgStmts       * sCurrPkgStmt        = NULL;
    qsPkgSubprograms * sSubprogram         = NULL;
    idBool             sIsPublic           = ID_FALSE;

    IDE_DASSERT( aSubprogramID != QS_PSM_SUBPROGRAM_ID );

    sCurrPkgStmt = aStatement->spvEnv->currSubprogram;

    if ( aPkgPlanTree == aStatement->spvEnv->createPkg )
    {
        /* 동일한 package 내에서 호출 */
        if ( aPkgType == QS_PKG )
        {
            sPkgSpecPlanTree = aPkgPlanTree;
        }
        else
        {
            sPkgSpecPlanTree = aStatement->spvEnv->pkgPlanTree;
        }

        sPkgStmt = sPkgSpecPlanTree->block->subprograms;

        while ( sPkgStmt != NULL )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;

                if( sSubprogram->subprogramID == aSubprogramID )
                {
                    sSubprogramPlanTree = sSubprogram->parseTree;
                    sIsPublic           = ID_TRUE;
                    break;
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

            if ( sPkgStmt == sCurrPkgStmt )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }

            sPkgStmt = sPkgStmt->next;
        }

        if ( aPkgType == QS_PKG_BODY )
        {
            sSubprogramPlanTree = NULL;

            /* public subporgram인 경우, package body에서 정의되는데,
               이 때 아직 header부분에 대한 validation이 안 되어 있을 경우,
               해당 subprogram의 header를 validation 한다.
               이렇게 함으로써, spec의 template에 접근하지 않아도 된다. */
            for ( sPkgStmt = aPkgPlanTree->block->subprograms;
                  sPkgStmt != NULL;
                  sPkgStmt = sPkgStmt->next )
            {
                if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
                {
                    sSubprogram = (qsPkgSubprograms*)sPkgStmt;
                    if ( sSubprogram->subprogramID == aSubprogramID )
                    {
                        if ( sPkgStmt->state == QS_PKG_STMTS_PARSE_FINISHED )
                        {
                            IDE_TEST( sPkgStmt->validateHeader( aStatement,
                                                                sPkgStmt )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                        }

                        sSubprogramPlanTree = sSubprogram->parseTree;
                        break;
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

                /* private subprogram인 경우,i
                   현재 validation 중인 subprogram의
                   앞에서 선언 또는 정의 된 subprogram만 호출 가능하다. */
                if ( (sIsPublic == ID_FALSE) &&
                     (sPkgStmt == sCurrPkgStmt) )
                {
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
    }
    else
    {
        sPkgStmt     = aPkgPlanTree->block->subprograms;

        while ( sPkgStmt != NULL )
        {
            if ( sPkgStmt->stmtType != QS_OBJECT_MAX )
            {
                sSubprogram = (qsPkgSubprograms*)sPkgStmt;

                if( sSubprogram->subprogramID == aSubprogramID )
                {
                    sSubprogramPlanTree = sSubprogram->parseTree;
                    break;
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

            sPkgStmt = sPkgStmt->next;
        }
    }

    IDE_DASSERT( sSubprogramPlanTree != NULL );

    *aSubprogramPlanTree = sSubprogramPlanTree;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-39220
   subprogram 이름과 parameter 갯수만 같을 수 있으므로,
   parameter 및 return 대한 상세한 비교가 필요하다. */
IDE_RC qsvPkgStmts::compareParametersAndReturnVar( qcStatement      * aStatement,
                                                   idBool             aIsPrivate,
                                                   idBool             aCheckParameters,
                                                   qsPkgSubprograms * aCurrSubprogram )
{
    qsPkgParseTree   * sPkgSpecParseTree        = NULL;
    qsPkgParseTree   * sPkgBodyParseTree        = NULL;
    qsProcParseTree  * sDeclSubprogramParseTree = NULL;
    qsProcParseTree  * sCurrSubprogramParseTree = NULL;
    qsPkgStmts       * sCurrStmt                = NULL;
    qsPkgStmts       * sPkgSubprogramStmt       = NULL;
    qsPkgSubprograms * sPkgSubprogramInfo       = NULL;
    qsVariableItems  * sDeclParamItem;
    qsVariableItems  * sDefParamItem;
    qsVariables      * sDeclParam;
    qsVariables      * sDefParam;
    mtcColumn        * sDeclColumn;
    mtcColumn        * sDefColumn; 
    UInt               i;
    idBool             sIsSame = ID_TRUE;
    qcuSqlSourceInfo   sqlInfo;
    SChar              sDeclSubprogramName[QC_MAX_OBJECT_NAME_LEN+1];

    IDE_DASSERT( aStatement->spvEnv->createPkg->objType == QS_PKG_BODY );

    sPkgSpecParseTree        = aStatement->spvEnv->pkgPlanTree;
    sPkgBodyParseTree        = aStatement->spvEnv->createPkg;
    sCurrStmt                = aStatement->spvEnv->currSubprogram;
    sCurrSubprogramParseTree = aCurrSubprogram->parseTree;

    if ( (sCurrSubprogramParseTree->block == NULL) &&
         (sCurrSubprogramParseTree->expCallSpec == NULL) )
    {
        // Nothing to do.
    }
    else
    {
        if ( aIsPrivate == ID_FALSE )
        {
            for ( sPkgSubprogramStmt = sPkgSpecParseTree->block->subprograms;
                  sPkgSubprogramStmt != NULL;
                  sPkgSubprogramStmt = sPkgSubprogramStmt->next )
            {
                if ( sPkgSubprogramStmt->stmtType != QS_OBJECT_MAX )
                {
                    sPkgSubprogramInfo = (qsPkgSubprograms *)sPkgSubprogramStmt;

                    if ( sPkgSubprogramInfo->subprogramID == aCurrSubprogram->subprogramID )
                    {
                        sDeclSubprogramParseTree = sPkgSubprogramInfo->parseTree;
                        break;
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
        else /* private subprogram */
        {
            for ( sPkgSubprogramStmt = sPkgBodyParseTree->block->subprograms;
                  sPkgSubprogramStmt != sCurrStmt;
                  sPkgSubprogramStmt = sPkgSubprogramStmt->next )
            {
                if ( sPkgSubprogramStmt->stmtType != QS_OBJECT_MAX )
                {
                    sPkgSubprogramInfo = (qsPkgSubprograms *)sPkgSubprogramStmt;

                    if ( sPkgSubprogramInfo->subprogramID == aCurrSubprogram->subprogramID )
                    {
                        sDeclSubprogramParseTree = sPkgSubprogramInfo->parseTree;
                        break;
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

        if( sDeclSubprogramParseTree != NULL )
        {
            if ( aCheckParameters == ID_TRUE )
            {
                sDeclSubprogramParseTree->procType = sCurrSubprogramParseTree->procType;

                for ( sDeclParamItem = sDeclSubprogramParseTree->paraDecls,
                      sDefParamItem = sCurrSubprogramParseTree->paraDecls, i = 0;
                      sDeclParamItem != NULL;
                      sDeclParamItem = sDeclParamItem->next,
                      sDefParamItem = sDefParamItem->next, i++ )
                {
                    sDeclParam = (qsVariables *)sDeclParamItem;
                    sDefParam  = (qsVariables *)sDefParamItem;

                    if ( QC_IS_NAME_MATCHED( sDeclParamItem->name, sDefParamItem->name ) &&
                         ( sDeclParam->inOutType == sDefParam->inOutType ) ) 
                    {
                        sDeclColumn = &sDeclSubprogramParseTree->paramColumns[i];
                        sDefColumn = &sCurrSubprogramParseTree->paramColumns[i];

                        if ( (sDeclColumn->precision == sDefColumn->precision) &&
                             (sDeclColumn->type.dataTypeId == sDefColumn->type.dataTypeId) )
                        {
                            sIsSame = ID_TRUE;
                        }
                        else
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }
                    else
                    {
                        sIsSame = ID_FALSE;
                        break;
                    }
                }
            }
            else /* check return */
            {
                sDeclColumn = sDeclSubprogramParseTree->returnTypeColumn;
                sDefColumn = sCurrSubprogramParseTree->returnTypeColumn;

                if ( (sDeclColumn->precision == sDefColumn->precision) &&
                     (sDeclColumn->type.dataTypeId == sDefColumn->type.dataTypeId) )
                {
                    if ( (sDeclSubprogramParseTree->isDeterministic == ID_FALSE) &&
                         (sCurrSubprogramParseTree->isDeterministic == ID_TRUE) )
                    {
                        sIsSame = ID_FALSE;
                    }
                    else
                    {
                        sCurrSubprogramParseTree->isDeterministic = sDeclSubprogramParseTree->isDeterministic;
                        sIsSame = ID_TRUE;
                    }
                }
                else
                {
                    sIsSame = ID_FALSE;
                }
            }

            if ( sIsSame == ID_FALSE )
            {
                if ( aIsPrivate == ID_TRUE )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &(sDeclSubprogramParseTree->procNamePos) );
                    IDE_RAISE( ERR_NOT_DEFINE_SUBPROGRAM );
                }
                else
                {
                    QC_STR_COPY( sDeclSubprogramName, sDeclSubprogramParseTree->procNamePos );

                    IDE_RAISE( ERR_NOT_DEFINED_ALL_SUBPROGRAM );
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // package body 내에서 definition만 할 수도 있음.
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_DEFINED_ALL_SUBPROGRAM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_INVALID_OR_MISSING_SUBPROGRAM, sDeclSubprogramName ) );
    }
    IDE_EXCEPTION( ERR_NOT_DEFINE_SUBPROGRAM );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_UNDEFINED_SUBPROGRAM,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qsvPkgStmts::isSubprogramEqual( qsPkgSubprograms * aCurrent,
                                       qsPkgSubprograms * aNext )
{
/***********************************************************************
 *
 * Description : 서브프로그램이 중복이 없는지 검사할때 사용한다.
 *
 *    1. 서브 프로그램의 이름
 *    2. 서브 프로그램의 타입
 *    3. 서브 프로그램의 인자 갯수
 *    4. 서브 프로그램의 인자 이름
 *    5. 서브 프로그램의 인자 in out 타입
 *    6. 서브 프로그램의 인자 데이타 타입
 *    7. 리턴 값의 데이타 타입
 *
 ***********************************************************************/

    qsProcParseTree * sCurrSubParseTree = aCurrent->parseTree;
    qsProcParseTree * sNextSubParseTree = aNext->parseTree;
    qsVariableItems * sCurrParameters;
    qsVariableItems * sNextParameters;
    qsVariables     * sCurrParam;
    qsVariables     * sNextParam;
    mtcColumn       * sCurrColumn;
    mtcColumn       * sNextColumn;
    UInt              i;

    if ( QC_IS_NAME_MATCHED( sCurrSubParseTree->procNamePos,
                             sNextSubParseTree->procNamePos ) != ID_TRUE )
    {
        IDE_CONT( NOT_EUQAL );
    }
    else
    {
        // nothing to do.
    }

    if ( aCurrent->common.stmtType != aNext->common.stmtType )
    {
        IDE_CONT( NOT_EUQAL );
    }
    else
    {
        // nothing to do.
    }

    if ( sCurrSubParseTree->paraDeclCount != sNextSubParseTree->paraDeclCount )
    {
        IDE_CONT( NOT_EUQAL );
    }
    else
    {
        // nothing to do.
    }

    for ( sCurrParameters = sCurrSubParseTree->paraDecls,
          sNextParameters = sNextSubParseTree->paraDecls, i = 0;
          sCurrParameters != NULL && sNextParameters != NULL;
          sCurrParameters = sCurrParameters->next,
          sNextParameters = sNextParameters->next, i++ )
    {
        sCurrParam = (qsVariables *)sCurrParameters;
        sNextParam = (qsVariables *)sNextParameters;

        if ( QC_IS_NAME_MATCHED( sCurrParam->common.name,
                                 sNextParam->common.name ) != ID_TRUE )
        {
            IDE_CONT( NOT_EUQAL );
        }
        else
        {
            // nothing to do.
        }

        if( sCurrParam->inOutType != sNextParam->inOutType )
        {
            IDE_CONT( NOT_EUQAL );
        }
        else
        {
            // nothing to do.
        }

        sCurrColumn = &sCurrSubParseTree->paramColumns[i];
        sNextColumn = &sNextSubParseTree->paramColumns[i];

        if ( sCurrColumn->type.dataTypeId != sNextColumn->type.dataTypeId )
        {
            IDE_CONT( NOT_EUQAL );
        }
        else
        {
            // nothing to do.
        }
    }

    if ( aCurrent->common.stmtType == QS_FUNC )
    {
        if ( sCurrSubParseTree->returnTypeColumn->type.dataTypeId !=
             sNextSubParseTree->returnTypeColumn->type.dataTypeId )
        {
            IDE_CONT( NOT_EUQAL );
        }
        else
        {
            // nothing to do.
        }
    }
    else
    {
        // nothing to do.
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( NOT_EUQAL );

    return ID_FALSE;
}

IDE_RC qsvPkgStmts::matchingSubprogram( qcStatement         * aStatement,
                                        qsPkgParseTree      * aParseTree,
                                        qtcNode             * aNode,
                                        UInt                  aArgCount,
                                        qcNamePosition        aColumnName,
                                        idBool                aExactly,
                                        idBool              * aExist,
                                        qsPkgSubprogramType * aSubprogramType,
                                        UInt                * aSubprogramID )
{
/***********************************************************************
 *
 * Description : 인자에 알맞는 서브 프로그램을 찾는다.
 *
 *    1. 서브 프로그램의 이름
 *    2. 서브 프로그램의 타입
 *    3. 서브 프로그램의 인자 갯수
 *    4. 서브 프로그램의 인자 데이타 타입
 *
 ***********************************************************************/
    idBool                    sExist = ID_FALSE;
    idBool                    sTooManyMatch = ID_FALSE;
    UInt                      sSubprogramID = QS_PSM_SUBPROGRAM_ID;
    qsPkgSubprogramType       sSubprogramType = QS_NONE_TYPE;
    qsExecParseTree         * sExeParseTree;
    qsObjectType              sCallStmtType;
    qsPkgStmts              * sCurrPkgStmt;
    qsPkgSubprograms        * sCurrSubprogram;
    qsVariableItems         * sCurrParameters;
    qsVariables             * sCurrParam;
    qsProcParseTree         * sCurrSubParseTree;
    mtcNode                 * sNode;
    mtcColumn               * sColumn;
    mtcColumn               * sParaColumn;
    UInt                      i, j;
    UInt                      sUnneedParamCnt;
    UInt                      sFriendly;
    UInt                      sTotalFriendly = 0;
    UInt                      sSelFriendly = ID_UINT_MAX;
    qcuSqlSourceInfo          sqlInfo;

    sExeParseTree = (qsExecParseTree *)aStatement->myPlan->parseTree;

    if ( sExeParseTree->leftNode == NULL )
    {
        sCallStmtType = QS_PROC;
    }
    else
    {
        sCallStmtType = QS_FUNC;
    }

    for( sCurrPkgStmt = aParseTree->block->subprograms;
         sCurrPkgStmt != NULL;
         sCurrPkgStmt = sCurrPkgStmt->next )
    {
        if( sCurrPkgStmt->stmtType == QS_OBJECT_MAX )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        sCurrSubprogram = (qsPkgSubprograms *)sCurrPkgStmt;
        sCurrSubParseTree = sCurrSubprogram->parseTree;

        for ( sCurrParameters  = sCurrSubParseTree->paraDecls,
              sUnneedParamCnt = 0;
              sCurrParameters != NULL;
              sCurrParameters  = sCurrParameters->next )
        {
            sCurrParam = (qsVariables *)sCurrParameters;

            // default, refCousor 일때는 인자를 입력하지 않아도 된다.
            if ( (sCurrParam->defaultValueNode != NULL) ||
                 (sCurrParam->variableType == QS_REF_CURSOR_TYPE) )
            {
                sUnneedParamCnt++;
            }
        }

        // 아직 header에 대한 validation이 안된 경우에는 skip
        if ( (sCurrPkgStmt->state == QS_PKG_STMTS_INIT_STATE) ||
             (sCurrPkgStmt->state == QS_PKG_STMTS_PARSE_FINISHED) )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        // private 서브 프로그램일때는 선언만 찾는다.
        if ( (sCurrSubprogram->subprogramType == QS_PRIVATE_SUBPROGRAM) &&
             (sCurrSubParseTree->block != NULL) )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        if( QC_IS_NAME_MATCHED( aColumnName,
                                sCurrSubParseTree->procNamePos ) != ID_TRUE )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        if ( (aArgCount > sCurrSubParseTree->paraDeclCount) ||
             (aArgCount < sCurrSubParseTree->paraDeclCount - sUnneedParamCnt) )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        if ( sCurrPkgStmt->stmtType != sCallStmtType )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        for( sNode = aNode->node.arguments,
             i = 0, sTotalFriendly = 0;
             sNode != NULL;
             sNode = sNode->next, i++ )
        {
            // named 파라미터일때는 이름으로 찾는다.
            if ( ( ((qtcNode*)sNode)->lflag & QTC_NODE_SP_PARAM_NAME_MASK )
                 == QTC_NODE_SP_PARAM_NAME_TRUE )
            {
                for ( sCurrParameters = sCurrSubParseTree->paraDecls, j = 0;
                      sCurrParameters != NULL;
                      sCurrParameters = sCurrParameters->next, j++ )
                {
                    sCurrParam = (qsVariables *)sCurrParameters;

                    if ( QC_IS_NAME_MATCHED( sCurrParam->common.name,
                                             ((qtcNode*)sNode)->position ) == ID_TRUE )
                    {
                        break;
                    }
                    else
                    {
                        // nothing to do.
                    }
                }

                if ( sCurrParameters != NULL )
                {
                    sNode = sNode->next;

                    sColumn = QTC_STMT_COLUMN( aStatement, (qtcNode*)sNode );

                    sParaColumn = &(sCurrSubParseTree->paramColumns[j]);
                }
                else
                {
                    sTotalFriendly = ID_UINT_MAX;
                    break;
                }
            }
            else
            {
                sColumn = QTC_STMT_COLUMN( aStatement, (qtcNode*)sNode );

                sParaColumn = &(sCurrSubParseTree->paramColumns[i]);
            }

            // bind 변수일때
            if ( sColumn->type.dataTypeId == MTD_UNDEF_ID )
            {
                continue;
            }
            else
            {
                // nothing to do
            }

            // 컨버젼까지 고려해서 타입 매칭을 한다.
            IDE_TEST( calTypeFriendly( aStatement,
                                       sNode,
                                       sColumn,
                                       sParaColumn,
                                       &sFriendly )
                      != IDE_SUCCESS );

            if( sFriendly != ID_UINT_MAX )
            {
                sTotalFriendly += sFriendly;
            }
            else
            {
                // 컨버젼이 불가능한 경우이다.
                sTotalFriendly = sFriendly;
                break;
            }
        } // end for

        if( sTotalFriendly != ID_UINT_MAX )
        {
            if ( sTotalFriendly < sSelFriendly )
            {
                sExist          = ID_TRUE;
                sTooManyMatch   = ID_FALSE;
                sSubprogramID   = sCurrSubprogram->subprogramID;
                sSubprogramType = sCurrSubprogram->subprogramType;
                sSelFriendly    = sTotalFriendly;
            }
            else if ( sTotalFriendly == sSelFriendly )
            {
                if( sExist == ID_TRUE )
                {
                    sTooManyMatch = ID_TRUE;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }

        if ( sCurrPkgStmt == aStatement->spvEnv->currSubprogram )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sTooManyMatch == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, & aColumnName );

        // 찾아난 함수가 2개 이상일때는 에러를 발생시킨다.
        IDE_TEST_RAISE( sExist == ID_TRUE, ERR_TOO_MANY_MATCH );
    }
    else
    {
        // Nothing to do.
    }

    // overloading 된 함수를 못찾았을때는
    // estimate 정보를 제외하고 찾는다.
    if ( (aExactly == ID_FALSE) &&
         (sExist == ID_FALSE) )
    {
        for( sCurrPkgStmt = aParseTree->block->subprograms;
             sCurrPkgStmt != NULL;
             sCurrPkgStmt = sCurrPkgStmt->next )
        {
            if( sCurrPkgStmt->stmtType == QS_OBJECT_MAX )
            {
                continue;
            }
            else
            {
                // nothing to do
            }

            sCurrSubprogram = (qsPkgSubprograms *)sCurrPkgStmt;
            sCurrSubParseTree = sCurrSubprogram->parseTree;

            if( QC_IS_NAME_MATCHED( aColumnName,
                                    sCurrSubParseTree->procNamePos )
                == ID_TRUE )
            {
                sSubprogramID   = sCurrSubprogram->subprogramID;
                sSubprogramType = sCurrSubprogram->subprogramType;
                sExist          = ID_TRUE;
                break;
            }
            else
            {
                sExist = ID_FALSE;
            }

            if ( sCurrPkgStmt == aStatement->spvEnv->currSubprogram )
            {
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
        // subprogram 존재
    }

    *aSubprogramType = sSubprogramType;
    *aSubprogramID   = sSubprogramID;
    *aExist          = sExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_MATCH )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_TOO_MANY_MATCH_THIS_CALL,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::calTypeFriendly( qcStatement * aStatement,
                                     mtcNode     * aArgNode,
                                     mtcColumn   * aArgColumn,
                                     mtcColumn   * aParamColumn,
                                     UInt        * aFriendly )
{
/***********************************************************************
 *
 * Description : 컨버젼까지 고려해서 타입 매칭을 한다.
 *
 *    타입정보를 알수 있을때는 2개 타입간의 컨버젼 노드가 몇개가 생성되는지를 계산한다.
 *    타입정보를 알수 없을때는 타입 그룹을 통해서 판단한다.
 *
 ***********************************************************************/

    const mtdModule * sModule;
    const mtvTable  * sMtvTable;
    mtcColumn       * sColumn;
    UInt              sFromNo;
    UInt              sToNo;
    ULong             sArgTypeGroup;
    UInt              sParamTypeGroup;

    *aFriendly = ID_UINT_MAX;

    if ( aArgColumn->module != NULL )
    {
        // 인자가 estimate가 되어 있는 경우
        sColumn = aArgColumn;
    }
    else
    {
        if (aArgNode->module == &mtfCast)
        {
            sColumn = QTC_STMT_COLUMN( aStatement, (qtcNode*)(aArgNode->funcArguments) );
        }
        else
        {
            sColumn = NULL;
        }
    }

    // 인자의 타입을 알수 있는 경우
    if ( sColumn != NULL )
    {
        // PSM 용 타입들은 아래 로직을 이용할수 없다.
        if ( (sColumn->type.dataTypeId < MTD_UDT_ID_MIN) &&
             (aParamColumn->type.dataTypeId < MTD_UDT_ID_MIN) )
        {
            IDE_TEST(mtd::moduleById( &sModule,
                                      sColumn->type.dataTypeId )
                    != IDE_SUCCESS);
            sFromNo = sModule->no;

            IDE_TEST(mtd::moduleById( &sModule,
                                      aParamColumn->type.dataTypeId )
                    != IDE_SUCCESS);
            sToNo = sModule->no;

            IDE_TEST(mtv::tableByNo(&sMtvTable, sToNo, sFromNo) != IDE_SUCCESS);

            if ( sMtvTable->cost != MTV_COST_INFINITE )
            {
                *aFriendly = sMtvTable->count;
            }
            else
            {
                *aFriendly = ID_UINT_MAX;
            }
        }
        else
        {
            // PSM 타입은 동일한 경우에만 허용한다.
            if ( sColumn->type.dataTypeId ==
                 aParamColumn->type.dataTypeId )
            {
                *aFriendly = 0;
            }
            else
            {
                *aFriendly = ID_UINT_MAX;
            }
        }
    }
    else
    {
        if ( (aArgNode->module == &mtfTo_nchar) ||
             (aArgNode->module == &mtfTo_char) )
        {
            sArgTypeGroup   = MTD_GROUP_TEXT;
        }
        else if (aArgNode->module == &mtfTo_number)
        {
            sArgTypeGroup   = MTD_GROUP_NUMBER;
        }
        else if (aArgNode->module == &mtfTo_date)
        {
            sArgTypeGroup   = MTD_GROUP_DATE;
        }
        else
        {
            sArgTypeGroup   = MTD_GROUP_MAXIMUM;
        }

        sParamTypeGroup = aParamColumn->module->flag & MTD_GROUP_MASK;

        // 동일한 타입 그룹일때는 QSV_TYPE_GROUP_PENALTY 만
        // 서로 다른 그룹이고 한쪽이 TEXT 그룹일 때는 QSV_TYPE_GROUP_PENALTY +1
        // 서로 다른 그룹이고 TEXT 그룹이 아닐때는 허용하지 않는다.
        if ( sArgTypeGroup == MTD_GROUP_TEXT )
        {
            if ( sParamTypeGroup == MTD_GROUP_TEXT )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY;
            }
            else
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 1;
            }
        }
        else if ( sArgTypeGroup == MTD_GROUP_NUMBER )
        {
            if ( sParamTypeGroup == MTD_GROUP_NUMBER )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 0;
            }
            else if ( sParamTypeGroup == MTD_GROUP_TEXT )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 1;
            }
            else
            {
                *aFriendly = ID_UINT_MAX;
            }
        }
        else if ( sArgTypeGroup == MTD_GROUP_DATE )
        {
            if ( sParamTypeGroup == MTD_GROUP_DATE )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 0;
            }
            else if ( sParamTypeGroup == MTD_GROUP_TEXT )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 1;
            }
            else
            {
                *aFriendly = ID_UINT_MAX;
            }
        }
        else if ( sArgTypeGroup == MTD_GROUP_MISC )
        {
            if ( sParamTypeGroup == MTD_GROUP_MISC )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 0;
            }
            else if ( sParamTypeGroup == MTD_GROUP_TEXT )
            {
                *aFriendly = QSV_TYPE_GROUP_PENALTY + 1;
            }
            else
            {
                *aFriendly = ID_UINT_MAX;
            }
        }
        else
        {
            *aFriendly = ID_UINT_MAX;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::getPkgLocalException( qcStatement  * aStatement,
                                          qsExceptions * aException,
                                          idBool       * aFind )
{
    qsPkgParseTree  * sPkgSpecParseTree = NULL;
    qsPkgParseTree  * sPkgBodyParseTree = NULL;
    qsVariableItems * sVariableItem     = NULL;
    idBool            sFind             = ID_FALSE;
    UInt              sUserID           = QC_EMPTY_USER_ID; 
    qsOID             sPkgSpecOID       = QS_EMPTY_OID; 

    IDE_DASSERT( QC_IS_NULL_NAME(aException->exceptionNamePos ) == ID_FALSE );

    if ( aStatement->spvEnv->pkgPlanTree == NULL )
    {
        /* create [or replace ]package */
        sPkgSpecParseTree = aStatement->spvEnv->createPkg;
        sPkgBodyParseTree = NULL;
        sPkgSpecOID       = QS_EMPTY_OID;
    }
    else
    {
        /* create [or replace ]package body */
        sPkgSpecParseTree = aStatement->spvEnv->pkgPlanTree;
        sPkgBodyParseTree = aStatement->spvEnv->createPkg;
        sPkgSpecOID       = sPkgSpecParseTree->pkgOID;
    }

    if ( QC_IS_NULL_NAME(aException->userNamePos) == ID_TRUE )
    {
        if ( qcmUser::getUserID( aStatement,
                                 aException->userNamePos,
                                 &sUserID )
             == IDE_SUCCESS )
        {
            if ( sUserID == aStatement->spvEnv->createPkg->userID )
            {
                // Nothing to do.
            }
            else
            {
                IDE_CONT( IS_NOT_PACKAGE_LOCAL_EXCEPTION );
            }
        }
        else
        {
            IDE_CLEAR();
            IDE_CONT( IS_NOT_PACKAGE_LOCAL_EXCEPTION );
        }
    }    
    else
    {
        // Nothing to do.
    }

    if ( QC_IS_NULL_NAME(aException->labelNamePos) == ID_FALSE )
    {
        if ( QC_IS_NAME_MATCHED( aException->labelNamePos,
                                 aStatement->spvEnv->createPkg->pkgNamePos )
             == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            IDE_CONT( IS_NOT_PACKAGE_LOCAL_EXCEPTION );
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( sPkgBodyParseTree != NULL )
    {
        for( sVariableItem = sPkgBodyParseTree->block->variableItems;
             sVariableItem != NULL;
             sVariableItem = sVariableItem->next )
        {
            if ( (sVariableItem->itemType == QS_EXCEPTION) &&
                 (QC_IS_NAME_MATCHED( sVariableItem->name,
                                      aException->exceptionNamePos )
                  == ID_TRUE) )
            {
                aException->id            = ((qsExceptionDefs *)sVariableItem)->id;
                aException->objectID      = QS_EMPTY_OID;
                /* BUG-41240 EXCEPTION_INIT Pragma */
                aException->errorCode     = ((qsExceptionDefs *)sVariableItem)->errorCode;
                aException->userErrorCode = ((qsExceptionDefs *)sVariableItem)->userErrorCode;
                sFind                     = ID_TRUE;
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

    if ( sFind == ID_FALSE )
    {
        for( sVariableItem = sPkgSpecParseTree->block->variableItems;
             sVariableItem != NULL;
             sVariableItem = sVariableItem->next )
        {
            if ( (sVariableItem->itemType == QS_EXCEPTION) &&
                 (QC_IS_NAME_MATCHED( sVariableItem->name,
                                      aException->exceptionNamePos )
                  == ID_TRUE) )
            {
                aException->id            = ((qsExceptionDefs *)sVariableItem)->id;
                aException->objectID      = sPkgSpecOID;
                /* BUG-41240 EXCEPTION_INIT Pragma */
                aException->errorCode     = ((qsExceptionDefs *)sVariableItem)->errorCode;
                aException->userErrorCode = ((qsExceptionDefs *)sVariableItem)->userErrorCode;
                sFind                     = ID_TRUE;
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

    IDE_EXCEPTION_CONT( IS_NOT_PACKAGE_LOCAL_EXCEPTION );

    *aFind = sFind;

    return IDE_SUCCESS;
} 

IDE_RC qsvPkgStmts::makeRelatedPkgSpecToPkgBody( qcStatement * aStatement,
                                                 qsxPkgInfo  * aPkgSpecInfo )
{
    iduVarMemList    * sMemory           = NULL;
    qsPkgParseTree   * sPkgBodyParseTree = NULL;
    qsRelatedObjects * sCurrObject       = NULL;
    qsRelatedObjects * sObject           = NULL;
    qsxProcPlanList  * sNewObjectPlan    = NULL;
    qsxProcPlanList  * sObjectPlan       = NULL;

    sMemory           = QC_QMP_MEM( aStatement );
    sPkgBodyParseTree = (qsPkgParseTree *)(aStatement->myPlan->parseTree);

    /* make related object list */
    IDE_TEST( STRUCT_ALLOC( sMemory,
                            qsRelatedObjects,
                            &sCurrObject )
              != IDE_SUCCESS );

    sCurrObject->next       = NULL;
    sCurrObject->userID     = aPkgSpecInfo->planTree->userID;
    sCurrObject->tableID    = 0;
    sCurrObject->objectID   = aPkgSpecInfo->pkgOID;
    sCurrObject->objectType = aPkgSpecInfo->objType;

    SET_POSITION( sCurrObject->objectNamePos, sPkgBodyParseTree->pkgNamePos );

    // setting userName
    if ( QC_IS_NULL_NAME( aPkgSpecInfo->planTree->userNamePos ) == ID_TRUE )
    {
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                          SChar,
                                          QC_MAX_OBJECT_NAME_LEN + 1,
                                          &(sCurrObject->userName.name ) )
                  != IDE_SUCCESS );

        IDE_TEST( qcmUser::getUserName( aStatement,
                                        aPkgSpecInfo->planTree->userID,
                                        sCurrObject->userName.name )
                  != IDE_SUCCESS );
        sCurrObject->userName.size = idlOS::strlen(sCurrObject->userName.name);
    }
    else
    {
        sCurrObject->userName.size = aPkgSpecInfo->planTree->userNamePos.size;

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                          SChar,
                                          (sCurrObject->userName.size + 1),
                                          &(sCurrObject->userName.name ) )
                  != IDE_SUCCESS );

        QC_STR_COPY( sCurrObject->userName.name, aPkgSpecInfo->planTree->userNamePos );
    }

    sCurrObject->objectName.size = aPkgSpecInfo->planTree->pkgNamePos.size;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                      SChar,
                                      (sCurrObject->objectName.size+1),
                                      &(sCurrObject->objectName.name ) )
              != IDE_SUCCESS );

    QC_STR_COPY( sCurrObject->objectName.name, aPkgSpecInfo->planTree->pkgNamePos );

    /* connect related object list */
    for ( sObject = aStatement->spvEnv->relatedObjects;
          sObject != NULL;
          sObject = sObject->next )
    {
        if ( (sObject->objectType == sCurrObject->objectType) &&
             (idlOS::strMatch(sObject->userName.name,
                              sObject->userName.size,
                              sCurrObject->userName.name,
                              sCurrObject->userName.size) == 0) &&
             (idlOS::strMatch(sObject->objectName.name,
                              sObject->objectName.size,
                              sCurrObject->objectName.name,
                              sCurrObject->objectName.size) == 0) )
        {
            // found
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sObject == NULL )
    {
        sCurrObject->next = aStatement->spvEnv->relatedObjects;
        aStatement->spvEnv->relatedObjects = sCurrObject;
    }
    else
    {
        // Nothing to do.
    }

    /* make prepare plan list */
    IDE_TEST( STRUCT_ALLOC( sMemory,
                            qsxProcPlanList,
                            & sNewObjectPlan )
              != IDE_SUCCESS );

    sNewObjectPlan->objectID           = aPkgSpecInfo->pkgOID;
    sNewObjectPlan->modifyCount        = aPkgSpecInfo->modifyCount;
    sNewObjectPlan->objectType         = QS_PKG;
    sNewObjectPlan->objectSCN          = smiGetRowSCN( smiGetTable( aPkgSpecInfo->pkgOID ) ); /* PROJ-2268 */
    sNewObjectPlan->planTree           = aPkgSpecInfo->planTree;
    sNewObjectPlan->pkgBodyOID         = QS_EMPTY_OID;
    sNewObjectPlan->pkgBodyModifyCount = 0;
    sNewObjectPlan->pkgBodySCN         = SM_SCN_INIT; /* PROJ-2268 */
    sNewObjectPlan->next               = NULL;

    for ( sObjectPlan = aStatement->spvEnv->procPlanList;
          sObjectPlan != NULL;
          sObjectPlan = sObjectPlan->next )
    {
        if ( sObjectPlan->objectID == sNewObjectPlan->objectID )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sObjectPlan == NULL )
    {
        sNewObjectPlan->next = aStatement->spvEnv->procPlanList;
        aStatement->spvEnv->procPlanList = sNewObjectPlan;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvPkgStmts::inheritRelatedObjectFromPkgSpec( qcStatement * aStatement,
                                                     qsxPkgInfo * aPkgSpecInfo )
{
    iduVarMemList    * sMemory           = NULL;
    qsRelatedObjects * sCurrObject       = NULL;
    qsRelatedObjects * sRelObject        = NULL;
    qsRelatedObjects * sObject           = NULL;
    SChar            * sPkgSpecStmtText  = NULL;

    sMemory = QC_QMP_MEM( aStatement );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                      SChar,
                                      (aPkgSpecInfo->planTree->stmtTextLen + 1),
                                      &sPkgSpecStmtText )
              != IDE_SUCCESS );

    idlOS::memcpy( sPkgSpecStmtText,
                   aPkgSpecInfo->planTree->stmtText,
                   aPkgSpecInfo->planTree->stmtTextLen );
    sPkgSpecStmtText[aPkgSpecInfo->planTree->stmtTextLen] = '\0';

    for ( sObject = aPkgSpecInfo->relatedObjects;
          sObject != NULL;
          sObject = sObject->next )
    {
        IDE_TEST( STRUCT_ALLOC( sMemory,
                                qsRelatedObjects,
                                &sCurrObject )
                  != IDE_SUCCESS );

        sCurrObject->next = NULL;
        sCurrObject->userID  = sObject->userID;
        sCurrObject->tableID = sObject->tableID;
        sCurrObject->objectID = sObject->objectID;
        sCurrObject->objectType = sObject->objectType;

        // setting userName
        sCurrObject->userName.size = sObject->userName.size;

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                          SChar,
                                          (sCurrObject->userName.size + 1),
                                          &(sCurrObject->userName.name ) )
                  != IDE_SUCCESS );

        idlOS::memcpy( sCurrObject->userName.name,
                       sObject->userName.name,
                       sCurrObject->userName.size );
        sCurrObject->userName.name[sCurrObject->userName.size] = '\0';

        // setting objectName
        sCurrObject->objectName.size = sObject->objectName.size;

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( sMemory,
                                          SChar,
                                          (sCurrObject->objectName.size + 1),
                                          &(sCurrObject->objectName.name ) )
                  != IDE_SUCCESS );

        idlOS::memcpy( sCurrObject->objectName.name,
                       sObject->objectName.name,
                       sCurrObject->objectName.size );
        sCurrObject->objectName.name[sCurrObject->objectName.size] = '\0';

        // setting objectNamePos
        sCurrObject->objectNamePos.stmtText = sPkgSpecStmtText;
        sCurrObject->objectNamePos.offset   = sObject->objectNamePos.offset;
        sCurrObject->objectNamePos.size     = sObject->objectNamePos.size;

        for ( sRelObject = aStatement->spvEnv->relatedObjects;
              sRelObject != NULL;
              sRelObject = sRelObject->next )
        {
            if ( (sRelObject->objectType == sCurrObject->objectType) &&
                 (idlOS::strMatch(sRelObject->userName.name,
                                  sRelObject->userName.size,
                                  sCurrObject->userName.name,
                                  sCurrObject->userName.size) == 0) &&
                 (idlOS::strMatch(sRelObject->objectName.name,
                                  sRelObject->objectName.size,
                                  sCurrObject->objectName.name,
                                  sCurrObject->objectName.size) == 0) )
            {
                // found
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        if ( sRelObject == NULL )
        {
            sCurrObject->next = aStatement->spvEnv->relatedObjects;
            aStatement->spvEnv->relatedObjects = sCurrObject;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( aPkgSpecInfo->relatedObjects != NULL )
    {
        IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree( aStatement,
                                                          aPkgSpecInfo->relatedObjects->objectID,
                                                          aPkgSpecInfo->relatedObjects->objectType,
                                                          &(aStatement->spvEnv->procPlanList) )
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

/* BUG-41847
   package 변수의 default 값으로 function을 사용할 수 있어야 합니다. */
idBool qsvPkgStmts::checkIsRecursiveSubprogramCall( qsPkgStmts * aCurrPkgStmt,
                                                    UInt         aTargetSubprogramID )
{

    idBool sIsRecursiveCall  = ID_FALSE;
    UInt   sCurrSubprogramID = QS_INIT_SUBPROGRAM_ID;

    if ( aCurrPkgStmt != NULL )
    {
        sCurrSubprogramID = ((qsPkgSubprograms *)aCurrPkgStmt)->subprogramID;

        if ( sCurrSubprogramID == aTargetSubprogramID )
        {
            sIsRecursiveCall = ID_TRUE;
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

    return sIsRecursiveCall;
}
