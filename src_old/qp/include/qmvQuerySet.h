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
 * $Id: qmvQuerySet.h 86122 2019-09-04 07:20:21Z donovan.seo $
 **********************************************************************/

#ifndef _Q_QMV_QUERY_SET_H_
#define _Q_QMV_QUERY_SET_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmmParseTree.h>

class qmvQuerySet
{
public:
    static IDE_RC validate(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet,
        qmsSFWGH    * aOuterQuerySFWGH,
        qmsFrom     * aOuterQueryFrom,
        SInt          aDepth);

    static IDE_RC validateQmsTableRef(
        qcStatement * aStatement,
        qmsSFWGH    * aSFWGH,
        qmsTableRef * aTableRef,
        UInt          aFlag,
        UInt          aIsNullableFlag ); // PR-13597

    static IDE_RC validateTable(
        qcStatement * aStatement,
        qmsTableRef * aTableRef,
        UInt          aFlag,
        UInt          aIsNullableFlag);
        
    static IDE_RC validateView(
        qcStatement * aStatement,
        qmsSFWGH    * aSFWGH,
        qmsTableRef * aTableRef,
        UInt          aIsNullableFlag);

    // PROJ-2582 recursive with
    static IDE_RC validateRecursiveView( qcStatement * aStatement,
                                         qmsSFWGH    * aSFWGH,
                                         qmsTableRef * aTableRef,
                                         UInt          aIsNullableFlag);

    static IDE_RC validateTopRecursiveView( qcStatement * aStatement,
                                            qmsSFWGH    * aSFWGH,
                                            qmsTableRef * aTableRef,
                                            UInt          aIsNullableFlag);

    static IDE_RC validateBottomRecursiveView( qcStatement * aStatement,
                                               qmsSFWGH    * aSFWGH,
                                               qmsTableRef * aTableRef,
                                               UInt          aIsNullableFlag);

    static IDE_RC validateQmsSFWGH(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet,
        qmsSFWGH    * aSFWGH);

    static IDE_RC validateWhere(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH);

    static IDE_RC validateGroupBy(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH);

    static IDE_RC validateHaving(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH);

    static IDE_RC validateHierarchy(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH);

    static IDE_RC makeTableInfo(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qcmColumn       * aColumnAlias,
        qcmTableInfo   ** aTableInfo,
        smOID             aObjectID );     // BUG-37981
                                           // cursor for loop 시 cursor의 objectID를 넘기기 위한 parameter
    
    static IDE_RC estimateTargetCount(
        qcStatement * aStatement,
        SInt        * aTargetCount);

    static IDE_RC validateHints(
        qcStatement * aStatement,
        qmsSFWGH    * aSFWGH);

    // BUG-20625
    static IDE_RC changeTargetForCommunication(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet);    

    static IDE_RC validateQmsTarget(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet,
        qmsSFWGH    * aSFWGH);

    static IDE_RC validateInlineView( qcStatement * aStatement,
                                      qmsSFWGH    * aSFWGH,
                                      qmsTableRef * aTableRef,
                                      UInt          aIsNullableFlag );

    static IDE_RC addDecryptFuncForNode(
        qcStatement  * aStatement,
        qtcNode      * aNode,
        qtcNode     ** aNewNode );

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    static IDE_RC addBLobLocatorFuncForNode(
        qcStatement  * aStatement,
        qtcNode      * aNode,
        qtcNode     ** aNewNode );

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    static IDE_RC addCLobLocatorFuncForNode(
        qcStatement  * aStatement,
        qtcNode      * aNode,
        qtcNode     ** aNewNode );

    static IDE_RC makeTupleForInlineView(
        qcStatement * aStatement,
        qmsTableRef * aTableRef,
        UShort        aTupleID,
        UInt          aIsNullableFlag);

    // PROJ-1362
    static IDE_RC addLobLocatorFunc(
        qcStatement * aStatement,
        qmsTarget   * aTarget );

    static IDE_RC searchHostVarAndAddCastOper(
        qcStatement   * aStatement,
        qtcNode       * aNode,
        qtcNode      ** aNewNode,
        idBool          aContainRootsNext );

    // BUG-38273
    static idBool checkInnerJoin( qmsFrom * aFrom );

    static IDE_RC innerJoinToNoJoin( qcStatement * aStatement,
                                     qmsSFWGH    * aSFWGH,
                                     qmsFrom     * aFrom );

    static IDE_RC validateQmsFromWithOnCond( qmsQuerySet * aQuerySet,
                                             qmsSFWGH    * aSFWGH,
                                             qmsFrom     * aFrom,
                                             qcStatement * aStatement,
                                             UInt          aIsNullableFlag ); // PR-13597

    static IDE_RC validateJoin( qcStatement * aStatement,
                                qmsFrom     * aFrom,
                                qmsSFWGH    * aSFWGH );

    static IDE_RC convertAnsiInnerJoin( qcStatement * aStatement,
                                        qmsSFWGH    * aSFWGH );

private:
    static IDE_RC validateGroupOneColumn(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aNode );

    static void findHintTableInFromClause(
        qcStatement     * aStatement,
        qmsFrom         * aFrom,
        qmsHintTables   * aHintTable);

    static IDE_RC expandAllTarget(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH,
        qmsFrom         * aFrom,
        qmsTarget       * aCurrTarget,
        qmsTarget      ** aFirstTarget,
        qmsTarget      ** aLastTarget);

    static IDE_RC expandTarget(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH,
        qmsTarget       * aCurrTarget,
        qmsTarget      ** aFirstTarget,
        qmsTarget      ** aLastTarget );

    static IDE_RC validateTargetExp(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH,
        qmsTarget       * aCurrTarget );

    static IDE_RC getTableRef(
        qcStatement       * aStatement,
        qmsFrom           * aFrom,
        UInt                aUserID,
        qcNamePosition      aTableName,
        qmsTableRef      ** aTableRef);

    static IDE_RC setSameViewRef(
        qcStatement * aStatement,
        qmsSFWGH    * aSFWGH,
        qmsTableRef * aTableRef);

    static IDE_RC makeTargetListForTableRef(
        qcStatement     * aStatement,
        qmsQuerySet     * aQuerySet,
        qmsSFWGH        * aSFWGH,
        qmsTableRef     * aTableRef,
        qmsTarget      ** aFirstTarget,
        qmsTarget      ** aLastTarget);

    static IDE_RC estimateOneFromTreeTargetCount(
        qcStatement       * aStatement,
        qmsFrom           * aFrom,
        SInt              * aTargetCount);

    static IDE_RC estimateOneTableTargetCount(
        qcStatement       * aStatement,
        qmsFrom           * aFrom,
        UInt                aUserID,
        qcNamePosition      aTableName,
        SInt              * aTargetCount);

    static IDE_RC getTargetCountFromTableRef(
        qcStatement * aStatement,
        qmsTableRef * aTableRef,
        SInt        * aTargetCount);

    // PROJ-1495
    static IDE_RC validatePushPredView(
        qcStatement * aStatement,
        qmsTableRef * aTableRef,
        idBool      * aIsValid );

    // PROJ-1495
    static IDE_RC validateUnionAllQuerySet(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet,
        idBool      * aIsValid );

    // PROJ-1495
    static IDE_RC validatePushPredHintQuerySet(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet,
        idBool      * aIsValid );

    // BUG-16928
    static IDE_RC getSubqueryTarget(
        qcStatement  * aStatement,
        qtcNode      * aNode,
        qtcNode     ** aTargetNode );

    // BUG-20272
    static IDE_RC addAssignNode(
        qcStatement   * aStatement,
        qmsSelectType   aSelectType, 
        qmsTarget     * aTarget );

    // PROJ-2002 Column Security
    static IDE_RC addDecryptFunc(
        qcStatement  * aStatement,
        qmsTarget    * aTarget );

    // BUG-34234
    static IDE_RC addCastOper(
        qcStatement  * aStatement,
        qmsTarget    * aTarget );

    // PROJ-1789 Scrollable Updatable Cursor
    static IDE_RC setTargetColumnInfo(qcStatement* aStatement,
                                      qmsTarget  * aTarget);

    /* PROJ-1832 New database link */
    static IDE_RC validateRemoteTable( qcStatement * aStatement,
                                       qmsTableRef * aTableRef );

    static void copyRemoteQueryString( SChar * aDest, qcNamePosition * aSrc );

    /* PROJ-1071 Parallel Query */
    static void validateParallelHint(qcStatement* aStatement, qmsSFWGH* aSFWGH);
    static void setParallelDegreeOnAllTables(qmsFrom* aFrom, UInt aDegree);

    // BUG-38946
    static void getDisplayName( qtcNode        * aNode,
                                qcNamePosition * aNamePos );

    static void copyDisplayName( qtcNode         * aNode,
                                 qmsNamePosition * aNamePos );

    // PROJ-2418
    // Lateral View의 outerQuery / outerFrom 설정
    static void setLateralOuterQueryAndFrom( qmsQuerySet * aViewQuerySet,
                                             qmsTableRef * aViewTableRef,
                                             qmsSFWGH    * aOuterSFWGH );

    // PROJ-2418
    // Lateral View의 outerFrom이 될 Join Tree 반환
    static void getLateralOuterFrom( qmsQuerySet  * aViewQuerySet,
                                     qmsTableRef  * aViewTableRef,
                                     qmsFrom      * aFrom,
                                     qmsFrom     ** aOuterFrom );

    // BUG-39522
    // Validate All DISTINCT Targets
    static idBool isAllDistinctTargets( qcStatement * aStatement,
                                        qmsSFWGH    * aSFWGH,
                                        qmsFrom     * aFrom );

    // BUG-39522
    // Validate All DISTINCT Targets
    static idBool compareKeyColumnWithTargets( qcStatement * aStatement,
                                               qmsSFWGH    * aSFWGH,
                                               mtcColumn   * aKeyColumn );

    // PROJ-2582 recursive with
    static IDE_RC addCastFuncForNode( qcStatement  * aStatement,
                                      qtcNode      * aNode,
                                      mtcColumn    * aCastType,
                                      qtcNode     ** aNewNode );

    /* BUG-45692 start with prior fatal */
    static IDE_RC searchStartWithPrior( qcStatement * aStatement,
                                        qtcNode     * aNode );

    /* BUG-45742 connect by aggregation function fatal */
    static IDE_RC searchConnectByAggr( qcStatement * aStatement,
                                       qtcNode     * aNode );
};

#endif  // _Q_QMV_QUERY_SET_H_
