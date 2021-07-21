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
 * $Id: qmvQTC.h 88630 2020-09-18 00:40:36Z ahra.cho $
 **********************************************************************/

#ifndef _Q_QMV_QTC_H_
#define _Q_QMV_QTC_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qsParseTree.h>
#include <qcmTableInfo.h>

class qmvQTC
{
public:
    static IDE_RC isGroupExpression(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aExpression,
        idBool            aMakePassNode);

    static IDE_RC isAggregateExpression(
        qcStatement * aStatement,
        qmsSFWGH    * aSFWGH,
        qtcNode     * aExpression);

    static IDE_RC isNestedAggregateExpression(
        qcStatement * aStatement,
        qmsQuerySet * aQuerySet,
        qmsSFWGH    * aSFWGH,
        qtcNode     * aExpression);

    static IDE_RC haveNotNestedAggregate(
        qcStatement * aStatement,
        qmsSFWGH    * aSFWGH,
        qtcNode     * aExpression);

    static IDE_RC isSelectedExpression(
        qcStatement     * aStatement,
        qtcNode         * aExpression,
        qmsTarget       * aTarget);

    static IDE_RC setColumnIDOfOrderBy(
        qtcNode      * aQtcColumn,
        mtcCallBack  * aCallBack,
        idBool       * aFindColumn );

    static IDE_RC setColumnIDForInsert(
        qtcNode      * aQtcColumn,
        mtcCallBack  * aCallBack,
        idBool       * aFindColumn );

    static IDE_RC setColumnID(
        qtcNode      * aQtcColumn,
        mtcTemplate  * aTemplate,
        mtcStack     * aStack,
        SInt           aRemain,
        mtcCallBack  * aCallBack,
        qsVariables ** aArrayVariable,
        idBool       * aIdcFlag,
        qmsSFWGH    ** aColumnSFWGH );

    // PROJ-2533 function object에 대해서 각각의 object에 맞게 node 변경
    static IDE_RC changeModuleToArray( 
        qtcNode      * aNode,
        mtcCallBack  * aCallBack );

    static IDE_RC setColumnIDForGBGS( qcStatement  * aStatement,
                                      qmsSFWGH     * aSFWGHOfCallBack,
                                      qtcNode      * aQtcColumn,
                                      idBool       * aIsFound,
                                      qmsTableRef ** aTableRef );

    static IDE_RC setColumnID4Rid(qtcNode* aQtcColumn, mtcCallBack* aCallBack);

    // BUG-26134
    static IDE_RC searchColumnInOuterQuery(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGHCallBack,
        qtcNode         * aQtcColumn,
        qmsTableRef    ** aTableRef,
        qmsSFWGH       ** aSFGGH );
        
    static IDE_RC setOuterDependencies(
        qmsSFWGH    * aSFWGH,
        qcDepInfo   * aDepInfo );
    
    // PROJ-2415 Grouping Sets Clause
    static IDE_RC addOuterDependencies(
        qmsSFWGH    * aSFWGH,
        qcDepInfo   * aDepInfo );

    static IDE_RC searchColumnInTableInfo(
        qcmTableInfo    * aTableInfo,
        qcNamePosition    aColumnName,
        UShort          * aColOrder,
        idBool          * aIsFound,
        idBool          * aIsLobType );

    static IDE_RC makeOneTupleForPseudoColumn(
        qcStatement     * aStatement,
        qtcNode         * aQtcColumn,
        SChar           * aDataTypeName,
        UInt              aDataTypeLength);

    // PROJ-1413
    static IDE_RC addViewColumnRefList(
        qcStatement     * aStatement,
        qtcNode         * aQtcColumn);

    /* PROJ-2469 Optimize View Materialization
     *
     * 조정 가능성이 있는, Target에 의해 등록된 참조 리스트를 생성하는 함수
     *
     * Args :  aTargetOrder     - 몇 번 째 Target에서 참조하는지 순번
     *         aViewTargetOrder - Grouping Sets의 Transparent View의 처리를 위한 인자
     */
    static IDE_RC addViewColumnRefListForTarget(
        qcStatement     * aStatement,
        qtcNode         * aQtcColumn,
        UShort            aTargetOrder,
        UShort            aViewTargetOrder );
 
    static IDE_RC addOuterColumn(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn);

    static IDE_RC setOuterColumns( qcStatement * aSQStatement,
                                   qcDepInfo   * aSQDepInfo,
                                   qmsSFWGH    * aSQSFWGH,
                                   qtcNode     * aNode );

    static IDE_RC extendOuterColumns( qcStatement * aSQStatement,
                                      qcDepInfo   * aSQDepInfo,
                                      qmsSFWGH    * aSQSFWGH );

    // PROJ-2418
    // QuerySet의 lateralDepInfo 설정
    static IDE_RC setLateralDependencies( qmsSFWGH  * aSFWGH,
                                          qcDepInfo * aLateralDepInfo );

    // PROJ-2418
    // From의 lateralDepInfo 획득
    static IDE_RC getFromLateralDepInfo( qmsFrom   * aFrom,
                                         qcDepInfo * aFromLateralDepInfo );

    // BUG-39567
    // Set Operation을 고려한 lateralDepInfo 마지막 설정
    static IDE_RC setLateralDependenciesLast( qmsQuerySet * aLateralQuerySet );

private:
    static IDE_RC checkSubquery4IsGroup(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGHofOuterQuery,
        qmsQuerySet     * aQuerySet,
        idBool            aMakePassNode ); /* BUG-48128 */

    static IDE_RC checkSubquery4IsAggregation(
        qmsSFWGH        * aSFWGHofOuterQuery,
        qmsQuerySet     * aQuerySet);

    static IDE_RC searchColumnInFromTree(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn,
        qmsFrom         * aFrom,
        qmsTableRef    ** aTableRef);

    static IDE_RC searchSequence(
        qcStatement     * aStatement,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn);

    static void findSeqCache(
        qcParseSeqCaches    * aParseSeqCaches,
        qcmSequenceInfo     * aSequenceInfo,
        qcParseSeqCaches   ** aSeqCache);

    static IDE_RC addSeqCache(
        qcStatement         * aStatement,
        qcmSequenceInfo     * aSequenceInfo,
        qtcNode             * aQtcColumn,
        qcParseSeqCaches   ** aSeqCaches);

    static void moveSeqCacheFromCurrToNext(
        qcStatement     * aStatement,
        qcmSequenceInfo * aSequenceInfo );

    /* PROJ-2209 DBTIMEZONE */
    static IDE_RC searchDatePseudoColumn(
        qcStatement     * aStatement,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn);

    static IDE_RC searchLevel(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn);

    // BUG-41311 table function
    static IDE_RC searchLoopLevel(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn);

    static IDE_RC searchLoopValue(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn);

    static IDE_RC searchRownum(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn);

    static IDE_RC searchConnectByIsLeaf(
        qcStatement     * aStatement,
        qmsSFWGH        * aSFWGH,
        qtcNode         * aQtcColumn,
        idBool          * aFindColumn );

    // PROJ-2687 Shard aggregation transform
    static IDE_RC setColumnIDForShardTransView( qcStatement     * aStatement,
                                                qmsSFWGH        * aSFWGHOfCallBack,
                                                qtcNode         * aQtcColumn,
                                                idBool          * aIsFound,
                                                qmsTableRef    ** aTableRef );

    /* TASK-7219 */
    static IDE_RC setColumnIDOfOrderByForShard( qcStatement  * aStatement,
                                                qmsSFWGH     * aSFWGH,
                                                qtcNode      * aQtcColumn,
                                                idBool       * aIsFound );
};

#endif  // _Q_QMV_QTC_H_
