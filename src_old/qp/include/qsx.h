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
 * $Id: qsx.h 86373 2019-11-19 23:12:16Z khkwak $
 **********************************************************************/

#ifndef _Q_QSX_H_
#define _Q_QSX_H_ 1

#include <qc.h>
#include <qsParseTree.h>
#include <qsxProc.h>
#include <qsxPkg.h>

#define QSX_CHECK_AT_PSM_BLOCK( aQsxEnvInfo , aProcPlanTree ) \
    ( ((aQsxEnvInfo->mExecPWVerifyFunc == ID_FALSE) && \
       (aProcPlanTree->procType == QS_NORMAL) && \
       (aProcPlanTree->block->isAutonomousTransBlock == ID_TRUE))?ID_TRUE:ID_FALSE )

struct qcmProcedures;
struct qcmPkgs;

typedef IDE_RC (*qsxJobForProc) ( smiStatement   * aSmiStmt,
                                  qcmProcedures  * aProc,
                                  idBool           aIsUseTx );

typedef IDE_RC (*qsxJobForPkg)  ( smiStatement   * aSmiStmt,
                                  qcmPkgs        * aPkg );

enum qsxMPIOption
{
    QSX_MPI_NONE = 0,
    QSX_MPI_ALWAYS_REPLACE = 1
};

class qsx
{
private :
    static IDE_RC removeProcMeta(qcStatement * aStatement);

    static IDE_RC doJobForEachProc (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem,
        idBool            aIsUseTx,
        qsxJobForProc     aJobFunc);

    // PROJ-1073 Package
    static IDE_RC doJobForEachPkg (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem,
        qsObjectType      aObjType,
        qsxJobForPkg      aJobFunc);

    static IDE_RC dropProcOrFuncCommon(qcStatement     * aStatement,
                                       qsOID             aProcOID,
                                       UInt              aUserID,
                                       SChar           * aProcName,
                                       UInt              aProcNameSize );

    // PROJ-1073 Package
    static IDE_RC dropPkgCommon( qcStatement * aStatement,
                                 qsOID         aPkgSpecOID,
                                 qsOID         aPkgBodyOID,
                                 UInt          aUserID,
                                 SChar       * aPkgName,
                                 UInt          aPkgNameSize,
                                 qsPkgOption   aOption );

    /* BUG-38844 drop package 시 meta 정보 삭제 */
    static IDE_RC dropPkgCommonForMeta( qcStatement * aStatement,
                                        qsOID         aPkgOID,
                                        UInt          aUserID,
                                        SChar       * aPkgName,
                                        UInt          aPkgNameSize,
                                        qsObjectType  aPkgType );

    /* BUG-38844 drop package 시 qsxPkgInfo free */
    static IDE_RC dropPkgCommonForPkgInfo( qsOID          aPkgOID,
                                           qsxPkgInfo   * aPkgInfo );

    static IDE_RC alterPkgCommon( qcStatement         * aStatement,
                                  qsAlterPkgParseTree * aPlanTree,
                                  qsOID                 aPkgOID );

    /* BUG-43113 autonomous_transaction */
    static IDE_RC execAT( qsxExecutorInfo * aExecInfo,
                          qcStatement     * aQcStmt,
                          mtcStack        * aStack,
                          SInt              aStackRemain );

public:

    /* PROJ-2197 PSM Renewal
     * PSM을 생성할 때 prepare memory를 온전히 보호하기 위해서
     * PSM을 생성한 이후에 prepare memory를 새로 만든다. */
    static IDE_RC makeNewPreparedMem( qcStatement * aStatement );

    static IDE_RC makeProcInfoMembers(
                  qsxProcInfo        * aProcInfo,
                  qcStatement        * aStatement );

    static IDE_RC makePkgInfoMembers(
        qsxPkgInfo         * aPkgInfo,
        qcStatement        * aStatement );

    // BUG-45571 Upgrade meta for authid clause of PSM
    static IDE_RC loadAllProcForMetaUpgrade (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem);

    static IDE_RC loadAllProc (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem);

    static IDE_RC unloadAllProc (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem);

    // PROJ-1073 Package
    static IDE_RC loadAllPkgSpec (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem);

    static IDE_RC loadAllPkgBody (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem);

    static IDE_RC unloadAllPkg (
        smiStatement    * aSmiStmt,
        iduMemory       * aIduMem);

    static IDE_RC doRecompile(
        qcStatement    * aStatement,
        qsOID            aProcOID,
        qsxProcInfo    * aProcInfo,
        idBool           aIsUseTx );

    static IDE_RC doPkgRecompile(
        qcStatement    * aStatement,
        qsOID            aPkgOID,
        qsxPkgInfo     * aPkgInfo,
        idBool           aIsUseTx );


    static IDE_RC createProcOrFunc(qcStatement * aStatement);

    static IDE_RC replaceProcOrFunc(qcStatement * aStatement);

    // PROJ-1073 Package
    static IDE_RC createPkg(qcStatement * aStatement);

    static IDE_RC createPkgBody(qcStatement * aStatement);

    static IDE_RC replacePkgOrPkgBody(qcStatement * aStatement);

    static IDE_RC dropProcOrFunc(qcStatement * aStatement);

    // PROJ-1073 Package
    static IDE_RC dropPkg(qcStatement * aStatement);

    static IDE_RC alterProcOrFunc(qcStatement * aStatement);

    // PROJ-1073 Package
    static IDE_RC alterPkg(qcStatement * aStatement);

    static IDE_RC executeProcOrFunc(qcStatement * aStatement);

    static IDE_RC executePkgAssign( qcStatement * aQcStmt );

    static IDE_RC executeAB(qcStatement * aQcStmt);

    static IDE_RC callProcWithNode (
                  qcStatement            * aStatement,
                  qsProcParseTree        * aPlanTree,
                  qtcNode                * aProcCallSpecNode,
                  qcTemplate             * aPkgTemplate,    // PROJ-1073 Package
                  qcTemplate             * aTmplate );

    static IDE_RC callProcWithStack (
                  qcStatement            * aStatement,
                  qsProcParseTree        * aPlanTree,
                  mtcStack               * aStack,
                  SInt                     aStackRemain,
                  qcTemplate             * aPkgTemplate,    // PROJ-1073 Package
                  qcTemplate             * aTmplate );

    static IDE_RC rebuildQsxProcInfoPrivilege(
        qcStatement     * aStatement,
        qsOID             aProcOID );

    // PROJ-1073 Package
    static IDE_RC rebuildQsxPkgInfoPrivilege(
        qcStatement     * aStatement,
        qsOID             aPkgOID );

    static IDE_RC addObject( iduList       * aList,
                             void          * aObject,
                             iduMemory     * aNodeMemory );

    static UShort getResultSetCount( qcStatement * aStatement );

    static IDE_RC getResultSetInfo( qcStatement * aStatement,
                                    UShort        aResultSetID,
                                    void       ** aResultSetStmt,
                                    idBool      * aRecordExist );

    // PROJ-1073 Package
    static IDE_RC pkgInitWithNode (
        qcStatement            * aStatement,
        mtcStack               * aStack,
        SInt                     aStackRemain,
        qsPkgParseTree         * aPlanTree,
        qcTemplate             * aPkgTemplate );

    static IDE_RC pkgInitWithStack (
                  qcStatement       * aStatement,
                  qsPkgParseTree    * aPlanTree,
                  mtcStack          * aStack,
                  SInt                aStackRemain,
                  qcTemplate        * aPkgTemplate );

    // BUG-36203 PSM Optimize
    static IDE_RC makeTemplateCache( qcStatement * aStatement,
                                     qsxProcInfo * aProcInfo,
                                     qcTemplate  * aTemplate );

    static IDE_RC freeTemplateCache( qsxProcInfo * aProcInfo );

    /* PROJ-2446 ONE SOURCE */
    static IDE_RC qsxLoadProcByProcOID( smiStatement   * aSmiStmt,
                                        smOID            aProcOID,
                                        idBool           aCreateProcInfo,
                                        idBool           aIsUseTx );

    /* PROJ-2446 ONE SOURCE FOR SUPPOTING PACKAGE */
    static IDE_RC qsxLoadPkgByPkgOID( smiStatement   * aSmiStmt,
                                      smOID            aPkgOID,
                                      idBool           aCreatePkgInfo );
};

#endif  // _Q_QSX_H_
