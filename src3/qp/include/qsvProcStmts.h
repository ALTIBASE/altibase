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
 * $Id: qsvProcStmts.h 87042 2020-03-26 08:29:59Z khkwak $
 **********************************************************************/

#ifndef _Q_QSV_PROC_STMTS_H_
#define _Q_QSV_PROC_STMTS_H_ 1

#include <qc.h>
#include <qsParseTree.h>
#include <qcmSynonym.h>

#define QSV_SQL_BUFFER_SIZE (1024)
#define QSV_CHECK_AT_TRIGGER_ACTION_BODY_BLOCK( aCrtProcParseTree ) \
    ( aCrtProcParseTree->block->isAutonomousTransBlock == ID_TRUE ? ID_TRUE : ID_FALSE )

class qsvProcStmts
{
public:
    static IDE_RC parse(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC parseBlock(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateBlock( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC parseSql( qcStatement * aStatement,
                            qsProcStmts * aProcStmts );

    static IDE_RC initSqlStmtForParse( qcStatement * aStatement,
                                       qsProcStmts * aProcStmts );

    static IDE_RC parseCursor( qcStatement * aStatement,
                               qsCursors   * aCursor );

    static IDE_RC initCursorStmtForParse( qcStatement * aStatement,
                                          qsCursors   * aCursor );

    static IDE_RC validateSql( qcStatement * aStatement,
                               qsProcStmts * aProcStmts,
                               qsProcStmts * aParentStmt,
                               qsValidatePurpose aPurpose );

    static IDE_RC parseIf(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateIf( qcStatement * aStatement,
                              qsProcStmts * aProcStmts,
                              qsProcStmts * aParentStmt,
                              qsValidatePurpose aPurpose );

    static IDE_RC parseWhile(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateWhile( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC parseFor(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateFor( qcStatement * aStatement,
                               qsProcStmts * aProcStmts,
                               qsProcStmts * aParentStmt,
                               qsValidatePurpose aPurpose );

    static IDE_RC parseExit(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateExit( qcStatement * aStatement,
                                qsProcStmts * aProcStmts,
                                qsProcStmts * aParentStmt,
                                qsValidatePurpose aPurpose );

    static IDE_RC validateContinue( qcStatement * aStatement,
                                    qsProcStmts * aProcStmts,
                                    qsProcStmts * aParentStmt,
                                    qsValidatePurpose aPurpose );

    static IDE_RC validateGoto( qcStatement * aStatement,
                                qsProcStmts * aProcStmts,
                                qsProcStmts * aParentStmt,
                                qsValidatePurpose aPurpose );

    static IDE_RC validateNull( qcStatement * aStatement,
                                qsProcStmts * aProcStmts,
                                qsProcStmts * aParentStmt,
                                qsValidatePurpose aPurpose );

    static IDE_RC parseAssign(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateAssign( qcStatement * aStatement,
                                  qsProcStmts * aProcStmts,
                                  qsProcStmts * aParentStmt,
                                  qsValidatePurpose aPurpose );

    static IDE_RC validateRaise( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC parseReturn(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateReturn( qcStatement * aStatement,
                                  qsProcStmts * aProcStmts,
                                  qsProcStmts * aParentStmt,
                                  qsValidatePurpose aPurpose );

    static IDE_RC validateLabel( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC parseThen(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);
    
    static IDE_RC validateThen( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC parseElse(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);
    
    static IDE_RC validateElse( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose aPurpose );
    
    static IDE_RC makeRelatedObjects(
        qcStatement     * aStatement,
        qcNamePosition  * aUserName,
        qcNamePosition  * aObjectName,
        qcmSynonymInfo  * aSynonymInfo,
        UInt              aTableID,
        SInt              aObjectType);

    static IDE_RC makeNewRelatedObject(
        qcStatement       * aStatement,
        qcNamePosition    * aUserName,
        qcNamePosition    * aObjectName,
        qcmSynonymInfo    * aSynonymInfo,
        UInt                aTableID,
        SInt                aObjectType,
        qsRelatedObjects ** aObject);

    // PROJ-1446
    // synonym으로 참조되는 PSM의 기록
    static IDE_RC makeProcSynonymList(
        qcStatement           * aStatement,
        struct qcmSynonymInfo * aSynonymInfo,
        qcNamePosition          aUserName,
        qcNamePosition          aObjectName,
        smOID                   aProcID );
    
    static IDE_RC connectAllVariables(
        qcStatement          * aStatement,
        qsLabels             * aLabels,
        qsVariableItems      * aVariableItems,
        idBool                 aInLoop,
        qsAllVariables      ** aOldAllVariables);

    static void disconnectAllVariables(
        qcStatement          * aStatement,
        qsAllVariables       * aOldAllVariables);

    // PROJ-1386 Dynamic-SQL  - BEGIN -
    static IDE_RC validateExecImm( qcStatement     * aStatement,
                                   qsProcStmts     * aProcStmts,
                                   qsProcStmts     * aParentStmt,
                                   qsValidatePurpose aPurpose );

    static IDE_RC validateOpenFor( qcStatement     * aStatement,
                                   qsProcStmts     * aProcStmts,
                                   qsProcStmts     * aParentStmt,
                                   qsValidatePurpose aPurpose );

    static IDE_RC validateFetch( qcStatement     * aStatement,
                                 qsProcStmts     * aProcStmts,
                                 qsProcStmts     * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC validateClose( qcStatement     * aStatement,
                                 qsProcStmts     * aProcStmts,
                                 qsProcStmts     * aParentStmt,
                                 qsValidatePurpose aPurpose );
    // PROJ-1386 Dynamic-SQL  - END -

    /* PROJ-2197 PSM Renewal
     * BUG-36988 Query Trans
     * PSM 변수를 qsUsingParam에 추가한다. */
    static IDE_RC makeUsingParam( qsVariables  * aVariable,
                                  qtcNode      * aQtcColumn,
                                  mtcCallBack  * aCallBack);

    // PROJ-1073 Package
    static IDE_RC getExceptionFromPkg(
        qcStatement    * aStatement,
        qsExceptions   * aException,
        idBool         * aFind);

    static IDE_RC getException(
        qcStatement     * aStatement,
        qsExceptions    * aException);

    static IDE_RC searchExceptionFromPkg( qsxPkgInfo   * aPkgInfo,
                                          qsExceptions * aException,
                                          idBool       * aFind );

    // BUG-36988
    static IDE_RC queryTrans( qcStatement * aQcStmt,
                              qsProcStmts * aProcStmts );

    // BUG-36902
    static IDE_RC setUseDate( qsvEnvInfo * aEnvInfo );

    static idBool isSQLType( qsProcStmtType aStmtType );

    /* BUG-24383 Support enqueue statement at PSM */
    static idBool isDMLType( qsProcStmtType aStmtType );

    /* BUG-24383 Support enqueue statement at PSM */
    static idBool isFetchType( qsProcStmtType aStmtType );

    static IDE_RC validateExceptionHandler( qcStatement     * aStatement,
                                            qsProcStmtBlock * aProcBLOCK,
                                            qsPkgStmtBlock  * aPkgBLOCK,
                                            qsProcStmts     * aParentStmt,
                                            qsValidatePurpose aPurpose );

    // BUG-37273 support bulk collect to execute immediate statement
    static IDE_RC validateIntoClause( qcStatement * aStatement,
                                      qmsTarget   * aTarget,
                                      qmsInto     * aIntoVars );

    // BUG-41707 chunking bulk collections of reference cursor
    static IDE_RC validateIntoClauseForRefCursor( qcStatement * aStatement,
                                                  qsProcStmts * aProcStmts,
                                                  qmsInto     * aIntoVars );

    /* BUG-41240 EXCEPTION_INIT Pragma */
    static IDE_RC validatePragmaExcepInit( qcStatement           * aStatement,
                                           qsVariableItems       * aVariableItems,
                                           qsPragmaExceptionInit * aPragmaExcepInit );

private:
    static IDE_RC checkSelectIntoClause(
        qcStatement     * aStatement,
        qsProcStmts     * aProcStmts,
        qmsQuerySet     * aQuerySet);

    static IDE_RC checkIndexVariableOfFor(
        qcStatement * aStatement,
        qtcNode     * aCounter,
        qtcNode     * aBound);

    // PROJ-1359 Trigger
    // Trigger Action Body로 적합한 Statement인지 검사하고
    // DML Statement의 경우 Modify Table 목록을 추가한다.
    static IDE_RC checkTriggerActionBody( qcStatement * aStatement,
                                          qsProcStmts * aProcStmts );

    // PROJ-1335 GOTO 지원
    // label을 parent stmt에 연결한다.
    static IDE_RC setLabelToParentStmt( qcStatement * aStatement,
                                        qsProcStmts * aParentStmt,
                                        qsLabels    * aLabel );

    // BUG-36988
    static IDE_RC makeBindVar( qcStatement     * aQcStmt,
                               qsProcStmtSql   * aSql,
                               iduVarMemString * aSqlBuffer,
                               qsUsingParam   ** aUsingParam,
                               SInt            * aOrgSqlTextOffset,
                               qcNamePosition ** aInBindVars,
                               SInt            * aInBindCount,
                               SInt            * aOutBindCount,
                               SInt            * aRecIndex,
                               SInt            * aRecFieldCount );

    // BUG-37501
    static IDE_RC connectChildLabel( qcStatement * aStatement,
                                     qsProcStmts * aSrcProcStmt,
                                     qsProcStmts * aDestProcStmt );

    // BUG-37273 support bulk collect to execute immediate statement
    static IDE_RC validateIntoClauseInternal( qcStatement * aStatement,
                                              qmsInto     * aIntoVars,
                                              UInt          aTargetCount,
                                              idBool      * aExistsRecordVar,
                                              idBool        aIsExecImm,
                                              idBool        alsRefCur, /* BUG-41707 */
                                              UInt        * aIntoVarCount /* BUG-41707 */ );

    /* BUG-41240 EXCEPTION_INIT Pragma */
    static idBool checkDupErrorCode( qsExceptions * aTarget1,
                                     qsExceptions * aTarget2 );
};
#endif  // _Q_QSV_PROC_STMTS_H_
