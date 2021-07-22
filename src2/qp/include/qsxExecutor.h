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
 * $Id: qsxExecutor.h 90088 2021-02-26 05:02:04Z khkwak $
 **********************************************************************/

#ifndef _O_QSX_EXECUTOR_H_
#define _O_QSX_EXECUTOR_H_ 1

#include <iduMemory.h>
#include <qc.h>
#include <qcuSqlSourceInfo.h>
#include <qcd.h>
#include <qtc.h>
#include <qsParseTree.h>
#include <qsxDef.h>
#include <qsxEnv.h>

struct qsxCursorInfo;

// qsxExecutor should be created per procedure/funcation call
typedef struct qsxExecutorInfo
{
    // plan tree used for recursive call
    qsProcParseTree       * mProcPlanTree;

    // RETURN statement stores return value into mProcCallSpec->return_val
    idBool                  mIsReturnValueValid;

    qsxFlowControl          mFlow;
    // expr id of label, exception,
    SInt                    mFlowId;
    idBool                  mFlowIdSetBySystem;

    qsProcStmtRaise       * mRecentRaise;

    // a pointer to WHILE, FOR, CURSORFOR LOOP to EXIT.
    // NULL if to EXIT to adjacent LOOP.
    qsProcStmts           * mExitLoopStmt;

    qsProcStmts           * mContLoopStmt;
    qsxCursorInfo         * mSqlCursorInfo;

    qcTemplate            * mTriggerTmplate;

    // fix BUG-36522
    SChar                   mSqlErrorMessage[MAX_ERROR_MSG_LEN + 1];    

    // PROJ-1073 Package
    qsPkgParseTree        * mPkgPlanTree;
    qcTemplate            * mPkgTemplate;

    mtcStack              * mSourceTemplateStackBuffer;
    mtcStack              * mSourceTemplateStack;
    UInt                    mSourceTemplateStackCount;
    UInt                    mSourceTemplateStackRemain;

    UInt                    mDefinerUserID;
    UInt                    mCurrentUserID;      /* BUG-45306 PSM AUTHID */
    idBool                  mIsDefiner;          /* BUG-45306 PSM AUTHID */

    // BUG-42322
    SChar                 * mObjectType;
    SChar                 * mUserAndObjectName;
    qsOID                   mObjectID;
} qsxExecutorInfo;

typedef struct intProcUtilFuncArg
{
    UInt     mOption;
    SInt     mResult;
    void   * mMemory;
    void   * mFunction;
    void   * mArgument1;
    void   * mArgument2;
    void  ** mArgument3;
} intProcUtilFuncArg;

class qsxExecutor
{
private:
    static void unsetFlowControl(
        qsxExecutorInfo  * aExecInfo,
        idBool             aIsClearIdeError = ID_TRUE );

    static void raiseSystemException(
        qsxExecutorInfo * aExecInfo,
        SInt              aExcpId);

    static void raiseConvertedException(
        qsxExecutorInfo * aExecInfo,
        UInt              aIdeErrorCode);

    // PROJ-1335, To fix BUG-12475 GOTO 지원
    // 해당 statement에 labelID와 같은 것이 존재하는지
    // 검사하고, 존재하면 aLabel에 label의 포인터를 넘겨줌
    static idBool findLabel( qsProcStmts *  aProcStmt,
                             SInt           aLabelID,
                             qsLabels    ** aLabel );

    static IDE_RC catchException (
        qsxExecutorInfo     * aExecInfo,
        qcStatement         * aQcStmt,
        qsExceptionHandlers * aExcpHdlrs);

    static IDE_RC getRaisedExceptionName( qsxExecutorInfo * aExecInfo,
                                          qcStatement     * aQcStmt );

    static IDE_RC registerLabel (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcStmts );

    static IDE_RC notSupportedError (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcStmts );

    static IDE_RC setArgumentValuesFromSourceTemplate (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt );

    static IDE_RC setOutArgumentValuesToSourceTemplate (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qcTemplate      * aSourceTemplate );

    static IDE_RC execNonSelectDML (
        qsxExecutorInfo     * aExecInfo,
        qcStatement         * aQcStmt,
        qsProcStmts         * aProcSql);

    static IDE_RC initializeSqlCursor( qsxExecutorInfo  * aExecInfo,
                                       qcStatement      * aQcStmt );

    static IDE_RC bindParam( qcStatement   * aQcStmt,
                             QCD_HSTMT       aHstmt,
                             qsUsingParam  * aUsingParam,
                             qciBindData  ** aOutBindParamDataList,
                             idBool          aIsFirst );

    static void adjustErrorMsg( qsxExecutorInfo  * aExecInfo,
                                qcStatement      * aQcStmt,
                                qsProcStmts      * aProcStmts,
                                qcuSqlSourceInfo * aSqlInfo,
                                idBool             aIsSetPosition );

    // BUG-37273
    static IDE_RC processIntoClause( qsxExecutorInfo * aExecInfo,
                                     qcStatement     * aQcStmt,
                                     QCD_HSTMT         aHstmt,
                                     qmsInto         * aIntoVariables,
                                     idBool            aIsIntoVarRecordType, 
                                     idBool            aRecordExist );

    // TASK-7244 Global PSM - commit/rollback/savepoint/DDL 정책
    static IDE_RC checkRestrictionsForShard( qcStatement * aQcStmt );

public:

    static IDE_RC initVariableItems(
        qsxExecutorInfo     * aExecInfo,
        qcStatement         * aQcStmt,
        qsVariableItems     * aVarItems,
        idBool                aCheckOutParam );

    static IDE_RC finiVariableItems(
        qcStatement         * aQcStmt,
        qsVariableItems     * aVarItems );

    static IDE_RC initialize ( qsxExecutorInfo  * aExecInfo,
                               qsProcParseTree  * aProcPlanTree,
                               qsPkgParseTree   * aPkgParseTree,
                               qcTemplate       * aPkgTemplate,
                               qcTemplate       * aTmplate );

    static IDE_RC execPlan (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        mtcStack        * aStack,
        SInt              aStackRemain );

    static IDE_RC execBlock (
        qsxExecutorInfo * aExecInfo,
        qcStatement       * aQcStmt,
        qsProcStmts       * aProcBlock);

    static IDE_RC execStmtList (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcStmts);

    static IDE_RC execInvoke (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execSelect (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execUpdate (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execDelete (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execInsert (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execMove (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execMerge (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execSavepoint (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execCommit (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execRollback (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql);

    static IDE_RC execIf (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcIf);

    // PROJ-1335, To Fix BUG-12475
    // GOTO 문을 위해 추가함
    static IDE_RC execThen (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcThen);

    // PROJ-1335, To Fix BUG-12475
    // GOTO 문을 위해 추가함
    static IDE_RC execElse (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcElse);

    static IDE_RC execWhile (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcWhile);

    static IDE_RC execFor (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcFor);

    static IDE_RC execCursorFor (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcCursorFor);

    static IDE_RC execExit (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcExit);

    static IDE_RC execContinue (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcContinue);

    static IDE_RC execOpen (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcOpen);

    static IDE_RC execFetch (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcFetch);

    static IDE_RC execClose (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcClose);

    static IDE_RC execAssign (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcAssign);

    static IDE_RC execRaise (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcRaise);

    static IDE_RC execReturn (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcReturn);

    // does nothing
    static IDE_RC execLabel (
        qsxExecutorInfo * aExecInfo,
        qcStatement       * aQcStmt,
        qsProcStmts       * aProcLabel );

    static IDE_RC execNull (
        qsxExecutorInfo * aExecInfo,
        qcStatement       * aQcStmt,
        qsProcStmts       * aProcLabel );

    // not available statements
    static IDE_RC execSetTrans (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcStmt);

    static IDE_RC execGoto (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcGoto);


    static IDE_RC execExecImm (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcExecImm );

    static IDE_RC execOpenFor (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcOpenFor );

    /* BUG-24383 Support enqueue statement at PSM */
    static IDE_RC execEnqueue (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        qsProcStmts     * aProcSql );

    // PROJ-1685
    static IDE_RC execExtproc( qsxExecutorInfo * aExecInfo,
                               qcStatement     * aQcStmt );

    // PROJ-2717
    static IDE_RC execIntproc( qsxExecutorInfo * aExecInfo,
                               qcStatement     * aQcStmt );

    // PROJ-1073 Package
    static IDE_RC execPkgPlan (
        qsxExecutorInfo * aExecInfo,
        qcStatement     * aQcStmt,
        mtcStack        * aStack,
        SInt              aStackRemain );

    static IDE_RC execPkgBlock (
        qsxExecutorInfo   * aExecInfo,
        qcStatement       * aQcStmt,
        qsProcStmts       * aProcBlock);

    /* BUG-43160 */
    static void setRaisedExcpErrorMsg( qsxExecutorInfo  * aExecInfo,
                                       qcStatement      * aQcStmt,
                                       qsProcStmts      * aProcStmts,
                                       qcuSqlSourceInfo * aSqlInfo,
                                       idBool             aIsSetPosition );

    static IDE_RC utilFunc4IntProc( intProcUtilFuncArg * aArg );
};

#endif
