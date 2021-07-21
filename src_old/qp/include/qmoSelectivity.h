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
 * $Id:  $
 *
 * Description : Selectivity Manager
 *
 *        - Unit predicate 에 대한 selectivity 계산
 *        - qmoPredicate 에 대한 selectivity 계산
 *        - qmoPredicate list 에 대한 통합 selectivity 계산
 *        - qmoPredicate wrapper list 에 대한 통합 selectivity 계산
 *        - 각 graph 에 대한 selectivity 계산
 *        - 각 graph 에 대한 output record count 계산
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qtcDef.h>
#include <qcmTableInfo.h>
#include <qmoPredicate.h>
#include <qmoCnfMgr.h>

#ifndef _Q_QMO_NEW_SELECTIVITY_H_
#define _Q_QMO_NEW_SELECTIVITY_H_ 1

//---------------------------------------------------
// feasibility로 수행가능한 predicate이 존재하지 않는 경우,
// selectivity를 계산할 수 없음을 반환한다.
//---------------------------------------------------
# define QMO_SELECTIVITY_NOT_EXIST               (1)

//---------------------------------------------------
// selectivity를 예측할수 없을때 사용한다.
//---------------------------------------------------
# define QMO_SELECTIVITY_UNKNOWN               (0.05)

//---------------------------------------------------
// PROJ-1353 
// qmgGrouping 에 대한 ROLLUP factor
//---------------------------------------------------
# define QMO_SELECTIVITY_ROLLUP_FACTOR           (0.75)

//---------------------------------------------------
// Selectivity를 관리하기 위한 함수
//---------------------------------------------------

class qmoSelectivity
{
public:

    //-----------------------------------------------
    // SCAN 관련 selectivity 계산
    //-----------------------------------------------

    // qmoPredicate.mySelectivity 계산
    // (BUGBUG : host 변수 값이 바인딩 전, 후 무관)
    static IDE_RC setMySelectivity( qcStatement   * aStatement ,
                                    qmoStatistics * aStatInfo,
                                    qcDepInfo     * aDepInfo,
                                    qmoPredicate  * aPredicate );

    // qmoPredicate.totalSelectivity 계산
    // (BUGBUG : host 변수 값이 바인딩 전, 후 무관)
    static IDE_RC setTotalSelectivity( qcStatement   * aStatement,
                                       qmoStatistics * aStatInfo,
                                       qmoPredicate  * aPredicate );

    // execution time에 selectivity 다시 계산
    // host 변수가 있는 경우 바인딩된 값을 참조하여 구한다.
    // qmoPredicate.mySelectivityOffset 과
    // qmoPredicate.totalSelectivityOffset 을 획득한다.
    static IDE_RC recalculateSelectivity( qcTemplate    * aTemplate,
                                          qmoStatistics * aStatInfo,
                                          qcDepInfo     * aDepInfo,
                                          qmoPredicate  * aRootPredicate );

    // index의 keyNDV 를 이용하여 selectivity 를 계산한다.
    // 사용하지 못하는 경우에는 getSelectivity4PredWrapper 를 호출함
    static IDE_RC getSelectivity4KeyRange( qcTemplate      * aTemplate,
                                           qmoStatistics   * aStatInfo,
                                           qmoIdxCardInfo  * aIdxCardInfo,
                                           qmoPredWrapper  * aKeyRange,
                                           SDouble         * aSelectivity,
                                           idBool            aInExecutionTime );

    // scan method 와 관련된 qmoPredWrapper list 에 대한 통합 selectivity 반환
    static IDE_RC getSelectivity4PredWrapper( qcTemplate     * aTemplate,
                                              qmoPredWrapper * aPredWrapper,
                                              SDouble        * aSelectivity,
                                              idBool           aInExecutionTime );

    // qmoPredicate list 에 대한 통합 totalSelectivity 반환
    // qmgSelection and qmgHierarchy 최적화 과정에서 이용
    static IDE_RC getTotalSelectivity4PredList( qcStatement  * aStatement,
                                                qmoPredicate * aPredList,
                                                SDouble      * aSelectivity,
                                                idBool         aInExecutionTime );

    //-----------------------------------------------
    // JOIN 관련 selectivity, output record count, joinOrderFactor 계산
    //-----------------------------------------------

    // Join 에 대한 qmoPredicate.mySelectivity 계산
    static IDE_RC setMySelectivity4Join( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qmoPredicate * aJoinPredList,
                                         idBool         aIsSetNext );

    // qmgJoin 에 대한 selectivity 계산
    static IDE_RC setJoinSelectivity( qcStatement  * aStatement,
                                      qmgGraph     * aGraph,
                                      qmoPredicate * aJoinPred,
                                      SDouble      * aSelectivity );

    // qmgLeftOuter, qmgFullOuter 에 대한 selectivity 계산
    static IDE_RC setOuterJoinSelectivity( qcStatement  * aStatement,
                                           qmgGraph     * aGraph,
                                           qmoPredicate * aWherePred,
                                           qmoPredicate * aOnJoinPred,
                                           qmoPredicate * aOneTablePred,
                                           SDouble      * aSelectivity );

    // qmgLeftOuter 에 대한 output record count 계산
    static IDE_RC setLeftOuterOutputCnt( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qcDepInfo    * aLeftDepInfo,
                                         qmoPredicate * aWherePred,
                                         qmoPredicate * aOnJoinPred,
                                         qmoPredicate * aOneTablePred,
                                         SDouble        aLeftOutputCnt,
                                         SDouble        aRightOutputCnt,
                                         SDouble      * aOutputRecordCnt );

    // qmgFullOuter 에 대한 output record count 계산
    static IDE_RC setFullOuterOutputCnt( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qcDepInfo    * aLeftDepInfo,
                                         qmoPredicate * aWherePred,
                                         qmoPredicate * aOnJoinPred,
                                         qmoPredicate * aOneTablePred,
                                         SDouble        aLeftOutputCnt,
                                         SDouble        aRightOutputCnt,
                                         SDouble      * aOutputRecordCnt );

    // Join ordering 관련 JoinOrderFactor, JoinSize 계산
    static IDE_RC setJoinOrderFactor( qcStatement  * aStatement,
                                      qmgGraph     * aJoinGraph,
                                      qmoPredicate * aJoinPred,
                                      SDouble      * aJoinOrderFactor,
                                      SDouble      * aJoinSize );

    //-----------------------------------------------
    // 이 외 graph 에 대한 selectivity, output record count 계산
    //-----------------------------------------------

    // qmgHierarchy 에 대한 selectivity 계산
    static IDE_RC setHierarchySelectivity( qcStatement  * aStatement,
                                           qmoPredicate * aWherePredList,
                                           qmoCNF       * aConnectByCNF,
                                           qmoCNF       * aStartWithCNF,
                                           idBool         aInExecutionTime,
                                           SDouble      * aSelectivity );

    // qmgCounting 에 대한 selectivity 계산
    static IDE_RC setCountingSelectivity( qmoPredicate * aStopkeyPred,
                                          SLong          aStopRecordCnt,
                                          SDouble        aInputRecordCnt,
                                          SDouble      * aSelectivity );

    // qmgCounting 에 대한 output record count 계산
    static IDE_RC setCountingOutputCnt( qmoPredicate * aStopkeyPred,
                                        SLong          aStopRecordCnt,
                                        SDouble        aInputRecordCnt,
                                        SDouble      * aOutputRecordCnt );

    // qmgProjection 에 대한 selectivity 계산
    static IDE_RC setProjectionSelectivity( qmsLimit * aLimit,
                                            SDouble    aInputRecordCnt,
                                            SDouble  * aSelectivity );

    // qmgGrouping 에 대한 selectivity 계산
    static IDE_RC setGroupingSelectivity( qmgGraph     * aGraph,
                                          qmoPredicate * aHavingPred,
                                          SDouble        aInputRecordCnt,
                                          SDouble      * aSelectivity );

    // qmgGrouping 에 대한 output record count 계산
    static IDE_RC setGroupingOutputCnt( qcStatement      * aStatement,
                                        qmgGraph         * aGraph,
                                        qmsConcatElement * aGroupBy,
                                        SDouble            aInputRecordCnt,
                                        SDouble            aSelectivity,
                                        SDouble          * aOutputRecordCnt );

    // qmgDistinction 에 대한 output record count 계산
    static IDE_RC setDistinctionOutputCnt( qcStatement * aStatement,
                                           qmsTarget   * aTarget,
                                           SDouble       aInputRecordCnt,
                                           SDouble     * aOutputRecordCnt );

    // qmgSet 에 대한 output record count 계산
    static IDE_RC setSetOutputCnt( qmsSetOpType   aSetOpType,
                                   SDouble        aLeftOutputRecordCnt,
                                   SDouble        aRightOutputRecordCnt,
                                   SDouble      * aOutputRecordCnt );

    // qmoPredicate 에 대한 mySelectivity 값 반환
    static SDouble getMySelectivity( qcTemplate   * aTemplate,
                                     qmoPredicate * aPredicate,
                                     idBool         aInExecutionTime );

    // PROJ-2582 recursive with
    static IDE_RC setSetRecursiveOutputCnt( SDouble   aLeftOutputRecordCnt,
                                            SDouble   aRightOutputRecordCnt,
                                            SDouble * aOutputRecordCnt );
    
private:

    //-----------------------------------------------
    // SCAN 관련 selectivity 계산
    //-----------------------------------------------

    /* qmoPredicate.mySelectivity */

    // execution time때 불리는 setMySelectivity 함수
    // host 변수에 값이 바인딩된 후 scan method 재구축을 위해
    // template->data + qmoPredicate.mySelectivityOffset 에
    // mySelectivity 를 재설정한다. (qmo::optimizeForHost 참조)
    static IDE_RC setMySelectivityOffset( qcTemplate    * aTemplate,
                                          qmoStatistics * aStatInfo,
                                          qcDepInfo     * aDepInfo,
                                          qmoPredicate  * aPredicate );

    /* qmoPredicate.totalSelectivity */

    // execution time때 불리는 setTotalSelectivity 함수
    // host 변수에 값이 바인딩된 후 scan method 재구축을 위해
    // template->data + qmoPredicate.totalSelectivityOffset 에
    // totalSelectivity 를 재설정한다. (qmo::optimizeForHost 참조)
    static IDE_RC setTotalSelectivityOffset( qcTemplate    * aTemplate,
                                             qmoStatistics * aStatInfo,
                                             qmoPredicate  * aPredicate );

    // qmoPredicate 에 대한 totalSelectivity 값 반환
    inline static SDouble getTotalSelectivity( qcTemplate   * aTemplate,
                                               qmoPredicate * aPredicate,
                                               idBool         aInExecutionTime );

    // setTotalSelectivity 에서 호출되어 통합된 total selectivity 계산
    static IDE_RC getIntegrateMySelectivity( qcTemplate    * aTemplate,
                                             qmoStatistics * aStatInfo,
                                             qmoPredicate  * aPredicate,
                                             SDouble       * aTotalSelectivity,
                                             idBool          aInExecutionTime );

    /* Unit selectivity */

    // Unit predicate 에 대한 scan selectivity 계산
    static IDE_RC getUnitSelectivity( qcTemplate    * aTemplate,
                                      qmoStatistics * aStatInfo,
                                      qcDepInfo     * aDepInfo,
                                      qmoPredicate  * aPredicate,
                                      qtcNode       * aCompareNode,
                                      SDouble       * aSelectivity,
                                      idBool          aInExecutionTime );
                                       
    // =, !=(<>) 에 대한 unit selectivity 계산
    static IDE_RC getEqualSelectivity( qcTemplate    * aTemplate,
                                       qmoStatistics * aStatInfo,
                                       qcDepInfo     * aDepInfo,
                                       qmoPredicate  * aPredicate,
                                       qtcNode       * aCompareNode,
                                       SDouble       * aSelectivity );

    // IS NULL, IS NOT NULL 에 대한 unit selectivity 계산
    static IDE_RC getIsNullSelectivity( qcTemplate    * aTemplate,
                                        qmoStatistics * aStatInfo,
                                        qmoPredicate  * aPredicate,
                                        qtcNode       * aCompareNode,
                                        SDouble       * aSelectivity );

    // >, >=, <, <=, BETWEEN, NOT BETWEEN 에 대한 unit selectivity 계산
    static IDE_RC getGreaterLessBeetweenSelectivity( qcTemplate    * aTemplate,
                                                     qmoStatistics * aStatInfo,
                                                     qmoPredicate  * aPredicate,
                                                     qtcNode       * aCompareNode,
                                                     SDouble       * aSelectivity,
                                                     idBool          aInExecutionTime );

    // getGreaterLessBeetweenSelectivity 에서 호출되어 MIN, MAX selectivity 계산
    static IDE_RC getMinMaxSelectivity( qcTemplate    * aTemplate,
                                        qmoStatistics * aStatInfo,
                                        qtcNode       * aCompareNode,
                                        qtcNode       * aColumnNode,
                                        qtcNode       * aValueNode,
                                        SDouble       * aSelectivity );

    // getMinMaxSelectivity, getIntegrateSelectivity 에서 호출
    // NUMBER group 의 min-max selectivity 계산을 위해 double 형 변환값 반환
    static IDE_RC getConvertToDoubleValue( mtcColumn     * aColumnColumn,
                                           mtcColumn     * aMaxValueColumn,
                                           mtcColumn     * aMinValueColumn,
                                           void          * aColumnMaxValue,
                                           void          * aColumnMinValue,
                                           void          * aMaxValue,
                                           void          * aMinValue,
                                           mtdDoubleType * aDoubleColumnMaxValue,
                                           mtdDoubleType * aDoubleColumnMinValue,
                                           mtdDoubleType * aDoubleMaxValue,
                                           mtdDoubleType * aDoubleMinValue );

    // LIKE, NOT LIKE 에 대한 unit selectivity 계산
    static IDE_RC getLikeSelectivity( qcTemplate    * aTemplate,
                                      qmoStatistics * aStatInfo,
                                      qmoPredicate  * aPredicate,
                                      qtcNode       * aCompareNode,
                                      SDouble       * aSelectivity,
                                      idBool          aInExecutionTime );

    // getLikeSelectivity 에서 호출되어 keyLength 반환
    static IDE_RC getLikeKeyLength( const mtdCharType   * aSource,
                                    const mtdCharType   * aEscape,
                                    UShort              * aKeyLength );

    // IN, NOT IN 에 대한 unit selectivity 계산
    static IDE_RC getInSelectivity( qcTemplate    * aTemplate,
                                    qmoStatistics * aStatInfo,
                                    qcDepInfo     * aDepInfo,
                                    qmoPredicate  * aPredicate,
                                    qtcNode       * aCompareNode,
                                    SDouble       * aSelectivity );

    // EQUALS, NOTEQUALS 에 대한 unit selectivity 계산
    static IDE_RC getEqualsSelectivity( qmoStatistics * aStatInfo,
                                        qmoPredicate  * aPredicate,
                                        qtcNode       * aCompareNode,
                                        SDouble       * aSelectivity );

    // LNNVL 에 대한 unit selectivity 계산
    static IDE_RC getLnnvlSelectivity( qcTemplate    * aTemplate,
                                       qmoStatistics * aStatInfo,
                                       qcDepInfo     * aDepInfo,
                                       qmoPredicate  * aPredicate,
                                       qtcNode       * aCompareNode,
                                       SDouble       * aSelectivity,
                                       idBool          aInExecutionTime );

    //-----------------------------------------------
    // JOIN 관련 selectivity 계산
    //-----------------------------------------------

    // Outer join ON 절의 one table predicate 에 대한 mySelectivity 계산
    static IDE_RC setMySelectivity4OneTable( qcStatement  * aStatement,
                                             qmgGraph     * aLeftGraph,
                                             qmgGraph     * aRightGraph,
                                             qmoPredicate * aOneTablePred );

    // Join predicate list 에 대한 통합 mySelectivity 반환
    static IDE_RC getMySelectivity4PredList( qmoPredicate  * aPredList,
                                             SDouble       * aSelectivity );

    // BUG-37918 join selectivity 개선
    static SDouble getEnhanceSelectivity4Join( qcStatement  * aStatement,
                                               qmgGraph     * aGraph,
                                               qmoStatistics* aStatInfo,
                                               qtcNode      * aNode);

    // Join predicate 에 대한 unit selectivity 를 반환하고
    // 다음 인덱스 사용가능여부를 세팅한다.
    static IDE_RC getUnitSelectivity4Join( qcStatement * aStatement,
                                           qmgGraph    * aGraph,
                                           qtcNode     * aCompareNode,
                                           idBool      * aIsEqual,
                                           SDouble     * aSelectivity );

    // qmoPredicate list 에 대한 통합 semi join selectivity 반환
    static IDE_RC getSemiJoinSelectivity( qcStatement   * aStatement,
                                          qmgGraph      * aGraph,
                                          qcDepInfo     * aLeftDepInfo,
                                          qmoPredicate  * aPredList,
                                          idBool          aIsLeftSemi,
                                          idBool          aIsSetNext,
                                          SDouble       * aSelectivity );

    // Join predicate 에 대한 semi join unit selectivity 반환
    static IDE_RC getUnitSelectivity4Semi( qcStatement  * aStatement,
                                           qmgGraph     * aGraph,
                                           qcDepInfo    * aLeftDepInfo,
                                           qtcNode      * aCompareNode,
                                           idBool         aIsLeftSemi,
                                           SDouble      * aSelectivity );

    // qmoPredicate list 에 대한 통합 anti join selectivity 반환
    static IDE_RC getAntiJoinSelectivity( qcStatement   * aStatement,
                                          qmgGraph      * aGraph,
                                          qcDepInfo     * aLeftDepInfo,
                                          qmoPredicate  * aPredList,
                                          idBool          aIsLeftAnti,
                                          idBool          aIsSetNext,
                                          SDouble       * aSelectivity );

    // Join predicate 에 대한 anti join unit selectivity 반환
    static IDE_RC getUnitSelectivity4Anti( qcStatement  * aStatement,
                                           qmgGraph     * aGraph,
                                           qcDepInfo    * aLeftDepInfo,
                                           qtcNode      * aCompareNode,
                                           idBool         aIsLeftAnti,
                                           SDouble      * aSelectivity );

    //-----------------------------------------------
    // JOIN ordering 관련
    //-----------------------------------------------

    // setJoinOrderFactor 함수에서 호출
    // 최적화 이전(predicate 미구분 상태) join predicate의 selectivity 계산
    static IDE_RC getSelectivity4JoinOrder( qcStatement  * aStatement,
                                            qmgGraph     * aJoinGraph,
                                            qmoPredicate * aJoinPred,
                                            SDouble      * aSelectivity );

    // getSelectivity4JoinOrder 함수에서 호출
    // 최적화 이전(predicate 미구분 상태) join predicate의 selectivity 보정
    static IDE_RC getReviseSelectivity4JoinOrder (
                                          qcStatement    * aStatement,
                                          qmgGraph       * aJoinGraph,
                                          qcDepInfo      * aChildDepInfo,
                                          qmoIdxCardInfo * aIdxCardInfo,
                                          qmoPredicate   * aPredicate,
                                          SDouble        * aSelectivity,
                                          idBool         * aIsRevise );

    // getReviseSelectivity4JoinOrder 함수에서 호출
    // Equal Predicate 인지 검사
    static IDE_RC isEqualPredicate( qmoPredicate * aPredicate,
                                    idBool       * aIsEqual );
};

#endif  /* _Q_QMO_NEW_SELECTIVITY_H_ */

