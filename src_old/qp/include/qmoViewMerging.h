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
 * $Id: qmoViewMerging.h 23857 2008-03-19 02:36:53Z sungminee $
 *
 * Description :
 *     PROJ-1413 Simple View Merging을 위한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_VIEW_MERGING_H_
#define _O_QMO_VIEW_MERGING_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmoDependency.h>

#define MAKE_TUPLE_RETRY_COUNT    (100)

#define DEFAULT_VIEW_NAME         ((SChar*) "NONAME")
#define DEFAULT_VIEW_NAME_LEN     (6)

//---------------------------------------------------
// View Merging의 rollback을 위한 자료구조
//---------------------------------------------------

typedef struct qmoViewRollbackInfo
{
    //---------------------------------------------------
    // Hint절의 rollback 정보
    //---------------------------------------------------

    idBool                    hintMerged;
    
    // join method hint의 merge전 마지막 노드를 기록한다.
    qmsJoinMethodHints      * lastJoinMethod;
    
    // table access hint의 merge전 마지막 노드를 기록한다.
    qmsTableAccessHints     * lastTableAccess;

    // BUG-22236
    // 이전 interResultType을 기록한다. 
    qmoInterResultType        interResultType;    // DISK/MEMORY

    //---------------------------------------------------
    // Target List의 rollback 정보
    //---------------------------------------------------

    idBool                    targetMerged;
    
    //---------------------------------------------------
    // From 절의 rollback 정보
    //---------------------------------------------------

    idBool                    fromMerged;
    
    // 현재 query block의 from 절 merge전 첫번째 from을 기록한다.
    qmsFrom                 * firstFrom;
    
    // 하위 query block의 from 절 merge전 마지막 from을 기록한다.
    qmsFrom                 * lastFrom;

    // merge전 tuple variable의 이름을 기록한다.
    qcNamePosition          * oldAliasName;
    
    // merge후 tuple variable의 이름을 기록한다.
    qcNamePosition          * newAliasName;
    
    //---------------------------------------------------
    // Where 절의 rollback 정보
    //---------------------------------------------------

    idBool                    whereMerged;
    
    // merge전 현재 where절의 노드를 기록한다.
    qtcNode                 * currentWhere;

    // merge전 하위 where절의 노드를 기록한다.
    qtcNode                 * underWhere;
    
} qmoViewRollbackInfo;


class qmoViewMerging
{
public:

    //------------------------------------------
    // (simple) view merging의 수행
    //------------------------------------------
    
    static IDE_RC  doTransform( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet );

    static IDE_RC  doTransformSubqueries( qcStatement  * aStatement,
                                          qtcNode      * aNode );

    static IDE_RC  doTransformForMultiDML( qcStatement  * aStatement,
                                           qmsQuerySet  * aQuerySet );
private:

    //------------------------------------------
    // simple view merging의 수행
    //------------------------------------------

    // query block을 순회하며 simple view merging 수행
    static IDE_RC  processTransform( qcStatement  * aStatement,
                                     qcParseTree  * aParseTree,
                                     qmsQuerySet  * aQuerySet );

    // query set을 순회하며 simple view merging 수행
    static IDE_RC  processTransformForQuerySet( qcStatement  * aStatement,
                                                qmsQuerySet  * aQuerySet,
                                                idBool       * aIsTransformed );

    // joined table의 하위 query set을 순회하며 simple view merging 수행
    static IDE_RC  processTransformForJoinedTable( qcStatement  * aStatement,
                                                   qmsFrom      * aFrom );

    // Expression에 포함된 subquery들을 찾아 simple view merging 수행
    static IDE_RC  processTransformForExpression( qcStatement * aStatement,
                                                  qtcNode     * aNode );

    //------------------------------------------
    // simple view 검사
    //------------------------------------------

    // query block이 simple query인지 검사
    static IDE_RC  isSimpleQuery( qmsParseTree * aParseTree,
                                  idBool       * aIsSimpleQuery );

    //------------------------------------------
    // merge 조건 검사
    //------------------------------------------

    // 현재 query block과 하위 query block의 merge 조건 검사
    static IDE_RC  canMergeView( qcStatement  * aStatement,
                                 qmsSFWGH     * aCurrentSFWGH,
                                 qmsSFWGH     * aUnderSFWGH,
                                 qmsTableRef  * aUnderTableRef,
                                 idBool       * aCanMergeView );

    // 현재 query block의 merge 조건 검사
    static IDE_RC  checkCurrentSFWGH( qmsSFWGH     * aSFWGH,
                                      idBool       * aCanMerge );
    
    // 하위 query block의 merge 조건 검사
    static IDE_RC  checkUnderSFWGH( qcStatement  * aStatement,
                                    qmsSFWGH     * aSFWGH,
                                    qmsTableRef  * aTableRef,
                                    idBool       * aCanMerge );

    // dependency 검사
    static IDE_RC  checkDependency( qmsSFWGH     * aCurrentSFWGH,
                                    qmsSFWGH     * aUnderSFWGH,
                                    idBool       * aCanMerge );

    // normal form 검사
    static IDE_RC  checkNormalForm( qcStatement  * aStatement,
                                    qmsSFWGH     * aCurrentSFWGH,
                                    qmsSFWGH     * aUnderSFWGH,
                                    idBool       * aCanMerge );
    
    //------------------------------------------
    // merging
    //------------------------------------------

    // 현재 query block과 하위 (view) query block의 merge 수행
    static IDE_RC  processMerging( qcStatement  * aStatement,
                                   qmsSFWGH     * aCurrentSFWGH,
                                   qmsSFWGH     * aUnderSFWGH,
                                   qmsFrom      * aUnderFrom,
                                   idBool       * aIsMerged );

    // hint 절의 merge 수행
    static IDE_RC  mergeForHint( qmsSFWGH            * aCurrentSFWGH,
                                 qmsSFWGH            * aUnderSFWGH,
                                 qmsFrom             * aUnderFrom,
                                 qmoViewRollbackInfo * aRollbackInfo,
                                 idBool              * aIsMerged );

    // hint 절의 rollback 수행
    static IDE_RC  rollbackForHint( qmsSFWGH            * aCurrentSFWGH,
                                    qmoViewRollbackInfo * aRollbackInfo );

    // target list의 merge 수행
    static IDE_RC  mergeForTargetList( qcStatement         * aStatement,
                                       qmsSFWGH            * aUnderSFWGH,
                                       qmsTableRef         * aUnderTableRef,
                                       qmoViewRollbackInfo * aRollbackInfo,
                                       idBool              * aIsMerged );

    // target이 순수 컬럼일 때의 merge 수행
    static IDE_RC  mergeForTargetColumn( qcStatement         * aStatement,
                                         qmsColumnRefList    * aColumnRef,
                                         qtcNode             * aTargetColumn,
                                         idBool              * aIsMerged );

    // target이 상수일 때의 merge 수행
    static IDE_RC  mergeForTargetValue( qcStatement         * aStatement,
                                        qmsColumnRefList    * aColumnRef,
                                        qtcNode             * aTargetColumn,
                                        idBool              * aIsMerged );

    // target이 expression일 때의 merge 수행
    static IDE_RC  mergeForTargetExpression( qcStatement         * aStatement,
                                             qmsSFWGH            * aUnderSFWGH,
                                             qmsColumnRefList    * aColumnRef,
                                             qtcNode             * aTargetColumn,
                                             idBool              * aIsMerged );

    // target list의 rollback 수행
    static IDE_RC  rollbackForTargetList( qmsTableRef         * aUnderTableRef,
                                          qmoViewRollbackInfo * aRollbackInfo );

    // from 절의 merget 수행
    static IDE_RC  mergeForFrom( qcStatement         * aStatement,
                                 qmsSFWGH            * aCurrentSFWGH,
                                 qmsSFWGH            * aUnderSFWGH,
                                 qmsFrom             * aUnderFrom,
                                 qmoViewRollbackInfo * aRollbackInfo,
                                 idBool              * aIsMerged );

    // tuple variable의 생성
    static IDE_RC  makeTupleVariable( qcStatement    * aStatement,
                                      qcNamePosition * aViewName,
                                      qcNamePosition * aTableName,
                                      idBool           aIsNewTableName,
                                      qcNamePosition * aNewTableName,
                                      idBool         * aIsCreated );

    // from 절의 rollback 수행
    static IDE_RC  rollbackForFrom( qmsSFWGH            * aCurrentSFWGH,
                                    qmsSFWGH            * aUnderSFWGH,
                                    qmoViewRollbackInfo * aRollbackInfo );

    // where 절의 merge 수행
    static IDE_RC  mergeForWhere( qcStatement         * aStatement,
                                  qmsSFWGH            * aCurrentSFWGH,
                                  qmsSFWGH            * aUnderSFWGH,
                                  qmoViewRollbackInfo * aRollbackInfo,
                                  idBool              * aIsMerged );

    // where 절의 rollback 수행
    static IDE_RC  rollbackForWhere( qmsSFWGH            * aCurrentSFWGH,
                                     qmsSFWGH            * aUnderSFWGH,
                                     qmoViewRollbackInfo * aRollbackInfo );

    // merge된 view의 제거
    static IDE_RC  removeMergedView( qmsSFWGH     * aSFWGH );

    //------------------------------------------
    // validation
    //------------------------------------------

    // query set의 validation 수행
    static IDE_RC  validateQuerySet( qcStatement  * aStatement,
                                     qmsQuerySet  * aQuerySet );

    // SFWGH의 validation 수행
    static IDE_RC  validateSFWGH( qcStatement  * aStatement,
                                  qmsSFWGH     * aSFWGH );

    // order by 절의 validation 수행
    static IDE_RC  validateOrderBy( qcStatement  * aStatement,
                                    qmsParseTree * aParseTree );
    
public:
    // qtcNode의 validation 수행
    static IDE_RC  validateNode( qcStatement  * aStatement,
                                 qtcNode      * aNode );
private:
    // BUG-27526
    // over 절의 validation 수행
    static IDE_RC  validateNode4OverClause( qcStatement  * aStatement,
                                            qtcNode      * aNode );

    // 제거된 view의 dependency 검사
    static IDE_RC  checkViewDependency( qcStatement  * aStatement,
                                        qcDepInfo    * aDepInfo );
    
    // PROJ-2418
    // qmsFrom에 있는 Lateral View에 대해 다시 Validation
    static IDE_RC  validateFrom( qmsFrom * aFrom );

    //------------------------------------------
    // 기타 후처리 함수
    //------------------------------------------

    // merge된 view에 대한 동일 view reference 제거
    static IDE_RC  modifySameViewRef( qcStatement  * aStatement );
};

#endif /* _O_QMO_VIEW_MERGING_H_ */
