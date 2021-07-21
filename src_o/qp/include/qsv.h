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
 * $Id: qsv.h 90176 2021-03-10 10:14:18Z seulki $
 **********************************************************************/

#ifndef _Q_QSV_H_
#define _Q_QSV_H_ 1

#include <qc.h>
#include <qsParseTree.h>
#include <qsvEnv.h>
#include <qsxProc.h>
#include <qcuSqlSourceInfo.h>

#include <qmmParseTree.h>

// BUG-41243 Name-based Argument Passing
// 선언된 Parameter에 Argument의 전달 여부 / 전달 유형
enum qsvArgPassType
{
    QSV_ARG_NOT_PASSED = 0,  // 전달되지 않음
    QSV_ARG_PASS_POSITIONAL, // Parameter 위치로 전달   (e.g.) proc1('val')
    QSV_ARG_PASS_NAMED       // Parameter 이름으로 전달 (e.g.) proc1(P1 => 'val')
};

typedef struct qsvArgPassInfo
{
    qsvArgPassType   mType;
    qtcNode        * mArg;
} qsvArgPassInfo;

class qsv
{
public:
    static IDE_RC parseCreateProc(qcStatement * aStatement);

    // PROJ-1073 Package
    static IDE_RC parseCreatePkg(qcStatement * aStatement);

    static IDE_RC validateCreateProc(qcStatement * aStatement);

    static IDE_RC validateReplaceProc(qcStatement * aStatement);
    
    static IDE_RC validateReplaceProcForRecompile(qcStatement * aStatement);

    static IDE_RC validateCreateFunc(qcStatement * aStatement);

    static IDE_RC validateReplaceFunc(qcStatement * aStatement);
    
    // PROJ-1073 Package
    static IDE_RC validateCreatePkg(qcStatement * aStatement);

    static IDE_RC validateReplacePkg(qcStatement * aStatement);
   
    static IDE_RC validateCreatePkgBody(qcStatement * aStatement);
  
    static IDE_RC validateReplacePkgBody(qcStatement * aStatement);

    static IDE_RC validateDropProc(qcStatement * aStatement);

    // PROJ-1073 Package
    static IDE_RC validateDropPkg(qcStatement * aStatement);

    static IDE_RC validateAltProc(qcStatement * aStatement);

    // PROJ-1073 Package
    static IDE_RC validateAltPkg(qcStatement * aStatement);

    static IDE_RC parseExeProc(qcStatement * aStatement);

    static IDE_RC validateExeProc(qcStatement * aStatement);

    static IDE_RC validateExeFunc(qcStatement * aStatement);

    static IDE_RC parseAB(qcStatement * aStatement);
    static IDE_RC validateAB(qcStatement * aStatement);

    static IDE_RC validateArgumentsWithParser(
        qcStatement     * aStatement,
        SChar           * aStmtText,
        qtcNode         * aCallSpec,
        qsVariableItems * aParaDecls,
        UInt              aParaDeclCount,
        iduList         * aRefCurList,
        idBool            aIsRootStmt );

    static IDE_RC validateArgumentsAfterParser(
        qcStatement     * aStatement,
        qtcNode         * aCallSpec,
        qsVariableItems * aParaDecls,
        idBool            aAllowSubquery );

    static IDE_RC validateArgumentsWithoutParser(
        qcStatement     * aStatement,
        qtcNode         * aCallSpec,
        qsVariableItems * aParaDecls,
        UInt              aParaDeclCount,
        idBool            aAllowSubquery );

    static IDE_RC checkNoSubquery(
        qcStatement       * aStatement,
        qtcNode           * aNode,
        qcuSqlSourceInfo  * aSourceInfo);

    static IDE_RC checkUserAndProcedure(
        qcStatement     * aStatement,
        qcNamePosition    aUserName,
        qcNamePosition    aProcName,
        UInt            * aUserID,
        qsOID           * aProcOID);
    
    // PROJ-1073 Package
    static IDE_RC checkUserAndPkg(
        qcStatement    * aStatement,
        qcNamePosition   aUserName,
        qcNamePosition   aPkgName,
        UInt           * aUserID,
        qsOID          * aSpecOID,
        qsOID          * aBodyOID);

    static IDE_RC checkHostVariable(qcStatement * aStatement);


    // To Fix BUG-11921(A3) 11920(A4)

    static IDE_RC createExecParseTreeOnCallSpecNode(
        qtcNode     * aCallSpecNode,
        mtcCallBack * aCallBack );

    static UShort getResultSetCount( qcStatement * aStatement );

    // PROJ-1685
    static IDE_RC validateExtproc( qcStatement * aStatement, qsProcParseTree * aParseTree );

    // PROJ-1073 Package
    static IDE_RC parseExecPkgAssign( qcStatement * aQcStmt );
    static IDE_RC validateExecPkgAssign( qcStatement * aQcStmt );

    /* PROJ-2586 PSM Parameters and return without precision */
    static IDE_RC setPrecisionAndScale( qcStatement * aStatement,
                                        qsVariables * aParamAndReturn );

    // BUG-42322
    static IDE_RC setObjectNameInfo( qcStatement        * aStatement,
                                     qsObjectType         aObjectType,
                                     UInt                 aUserID,
                                     qcNamePosition       aObjectNamePos,
                                     qsObjectNameInfo   * aObjectInfo );
private:

    static IDE_RC checkRecursiveCall(
        qcStatement * aStatement,
        idBool      * aRecursiveCall);

    // PROJ-1073 Package
    static IDE_RC checkMyPkgSubprogramCall(
        qcStatement * aStatement,
        idBool      * aMyPkgSubprogram,
        idBool      * aRecursiveCall );

    static IDE_RC validateReturnDef(
        qcStatement * aStatement );

    // BUG-41243 Name-based Argument Passing
    // Name-based Argument 검증을 하고, 올바른 위치로 Argument를 조정
    static IDE_RC validateNamedArguments( qcStatement     * aStatement,
                                          qtcNode         * aCallSpec,
                                          qsVariableItems * aParaDecls,
                                          UInt              aParaDeclCnt,
                                          qsvArgPassInfo ** aArgPassArray );
};

#endif  // _Q_QSV_H_

