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
 * $Id$
 *
 * Description :
 *     Statistical Information Manager
 *
 *     각종 실시간 통계 정보의 추출을 담당한다.
 **********************************************************************/

#ifndef _O_QMO_COST_H_
#define _O_QMO_COST_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoCostDef.h>
#include <qmoJoinMethod.h>

class qmoCost
{
public:

    /***********************************************************************
    * Object Scan Cost
    ***********************************************************************/

    static SDouble    getTableFullScanCost( qmoSystemStatistics * aSystemStat,
                                            qmoStatistics       * aTableStat,
                                            SDouble               aFilterCost,
                                            SDouble             * aMemCost,
                                            SDouble             * aDiskCost );

    static SDouble    getTableRIDScanCost( qmoStatistics       * aTableStat,
                                           SDouble             * aMemCost,
                                           SDouble             * aDiskCost  );

    static SDouble    getIndexScanCost( qcStatement         * aStatement,
                                        mtcColumn           * aColumns,
                                        qmoPredicate        * aJoinPred,
                                        qmoStatistics       * aTableStat,
                                        qmoIdxCardInfo      * aIndexStat,
                                        SDouble               aKeyRangeSelectivity,
                                        SDouble               aKeyFilterSelectivity,
                                        SDouble               aKeyRangeCost,
                                        SDouble               aKeyFilterCost,
                                        SDouble               aFilterCost,
                                        SDouble             * aMemCost,
                                        SDouble             * aDiskCost );

    static SDouble    getIndexBtreeScanCost( qmoSystemStatistics * aSystemStat,
                                             qmoStatistics       * aTableStat,
                                             qmoIdxCardInfo      * aIndexStat,
                                             SDouble               aKeyRangeSelectivity,
                                             SDouble               aKeyRangeCost,
                                             SDouble               aKeyFilterCost,
                                             SDouble               aFilterCost,
                                             SDouble             * aMemCost,
                                             SDouble             * aDiskCost );

    /***********************************************************************
    * Join Cost
    ***********************************************************************/

    static SDouble    getFullNestedJoinCost( qmgGraph    * aLeft,
                                             qmgGraph    * aRight,
                                             SDouble       aFilterCost,
                                             SDouble     * aMemCost,
                                             SDouble     * aDiskCost );

    static SDouble    getFullStoreJoinCost( qmgGraph            * aLeft,
                                            qmgGraph            * aRight,
                                            SDouble               aFilterCost,
                                            qmoSystemStatistics * aSystemStat,
                                            idBool                aUseRightDiskTemp,
                                            SDouble             * aMemCost,
                                            SDouble             * aDiskCost );

    static SDouble    getIndexNestedJoinCost( qmgGraph            * aLeft,
                                              qmoStatistics       * aTableStat,
                                              qmoIdxCardInfo      * aIndexStat,
                                              qmoSystemStatistics * aSystemStat,
                                              SDouble               aRightMemCost,
                                              SDouble               aRightDiskCost,
                                              SDouble               aFilterCost,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getFullNestedJoin4SemiCost( qmgGraph    * aLeft,
                                                  qmgGraph    * aRight,
                                                  SDouble       aJoinSelectivty,
                                                  SDouble       aFilterCost,
                                                  SDouble     * aMemCost,
                                                  SDouble     * aDiskCost );

    static SDouble    getIndexNestedJoin4SemiCost( qmgGraph            * aLeft,
                                                   qmoStatistics       * aTableStat,
                                                   qmoIdxCardInfo      * aIndexStat,
                                                   qmoSystemStatistics * aSystemStat,
                                                   SDouble               aFilterCost,
                                                   SDouble             * aMemCost,
                                                   SDouble             * aDiskCost );

    static SDouble    getFullNestedJoin4AntiCost( qmgGraph    * aLeft,
                                                  qmgGraph    * aRight,
                                                  SDouble       aJoinSelectivty,
                                                  SDouble       aFilterCost,
                                                  SDouble     * aMemCost,
                                                  SDouble     * aDiskCost );

    static SDouble    getIndexNestedJoin4AntiCost( qmgGraph            * aLeft,
                                                   qmoStatistics       * aTableStat,
                                                   qmoIdxCardInfo      * aIndexStat,
                                                   qmoSystemStatistics * aSystemStat,
                                                   SDouble               aFilterCost,
                                                   SDouble             * aMemCost,
                                                   SDouble             * aDiskCost );

    static SDouble    getInverseIndexNestedJoinCost( qmgGraph            * aLeft,
                                                     qmoStatistics       * aTableStat,
                                                     qmoIdxCardInfo      * aIndexStat,
                                                     qmoSystemStatistics * aSystemStat,
                                                     idBool                aUseLeftDiskTemp,
                                                     SDouble               aFilterCost,
                                                     SDouble             * aMemCost,
                                                     SDouble             * aDiskCost );

    static SDouble    getAntiOuterJoinCost( qmgGraph          * aLeft,
                                            SDouble             aRightMemCost,
                                            SDouble             aRightDiskCost,
                                            SDouble             aFilterCost,
                                            SDouble           * aMemCost,
                                            SDouble           * aDiskCost );

    static SDouble    getOnePassHashJoinCost( qmgGraph            * aLeft,
                                              qmgGraph            * aRight,
                                              SDouble               aFilterCost,
                                              qmoSystemStatistics * aSystemStat,
                                              idBool                aUseRightDiskTemp,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getTwoPassHashJoinCost( qmgGraph            * aLeft,
                                              qmgGraph            * aRight,
                                              SDouble               aFilterCost,
                                              qmoSystemStatistics * aSystemStat,
                                              idBool                aUseLeftDiskTemp,
                                              idBool                aUseRightDiskTemp,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getInverseHashJoinCost( qmgGraph            * aLeft,
                                              qmgGraph            * aRight,
                                              SDouble               aFilterCost,
                                              qmoSystemStatistics * aSystemStat,
                                              idBool                aUseRightDiskTemp,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getOnePassSortJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                              qmgGraph            * aLeft,
                                              qmgGraph            * aRight,
                                              SDouble               aFilterCost,
                                              qmoSystemStatistics * aSystemStat,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getTwoPassSortJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                              qmgGraph            * aLeft,
                                              qmgGraph            * aRight,
                                              SDouble               aFilterCost,
                                              qmoSystemStatistics * aSystemStat,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getInverseSortJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                              qmgGraph            * aLeft,
                                              qmgGraph            * aRight,
                                              SDouble               aFilterCost,
                                              qmoSystemStatistics * aSystemStat,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost );

    static SDouble    getMergeJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                        qmgGraph            * aLeft,
                                        qmgGraph            * aRight,
                                        qmoAccessMethod     * aLeftSelMethod,
                                        qmoAccessMethod     * aRightSelMethod,
                                        SDouble               aFilterCost,
                                        qmoSystemStatistics * aSystemStat,
                                        SDouble             * aMemCost,
                                        SDouble             * aDiskCost );

    /***********************************************************************
    * Temp Table Cost
    ***********************************************************************/
    static SDouble    getMemSortTempCost( qmoSystemStatistics * aSystemStat,
                                          qmgCostInfo         * aCostInfo,
                                          SDouble               aSortNodeCnt );

    static SDouble    getMemHashTempCost( qmoSystemStatistics * aSystemStat,
                                          qmgCostInfo         * aCostInfo,
                                          SDouble               aHashNodeCnt );

    static SDouble    getMemStoreTempCost( qmoSystemStatistics * aSystemStat,
                                           qmgCostInfo         * aCostInfo );

    static SDouble    getDiskSortTempCost( qmoSystemStatistics * aSystemStat,
                                           qmgCostInfo         * aCostInfo,
                                           SDouble               aSortNodeCnt,
                                           SDouble               aSortNodeLen );

    static SDouble    getDiskHashTempCost( qmoSystemStatistics * aSystemStat,
                                           qmgCostInfo         * aCostInfo,
                                           SDouble               aHashNodeCnt,
                                           SDouble               aHashNodeLen );

    static SDouble    getDiskHashGroupCost( qmoSystemStatistics * aSystemStat,
                                            qmgCostInfo         * aCostInfo,
                                            SDouble               aGroupCnt,
                                            SDouble               aHashNodeCnt,
                                            SDouble               aHashNodeLen );

    static SDouble    getDiskStoreTempCost( qmoSystemStatistics * aSystemStat,
                                            SDouble               aStoreRowCnt,
                                            SDouble               aStoreNodeLen,
                                            UShort                aTempTableType );

    /***********************************************************************
    * Filter Cost
    ***********************************************************************/
    static SDouble    getKeyRangeCost( qmoSystemStatistics * aSystemStat,
                                       qmoStatistics       * aTableStat,
                                       qmoIdxCardInfo      * aIndexStat,
                                       qmoPredWrapper      * aKeyRange,
                                       SDouble               aKeyRangeSelectivity );

    static SDouble    getKeyFilterCost( qmoSystemStatistics * aSystemStat,
                                        qmoPredWrapper      * aKeyFilter,
                                        SDouble               aLoopCnt );

    static SDouble    getFilterCost4PredWrapper( qmoSystemStatistics * aSystemStat,
                                                 qmoPredWrapper      * aFilter,
                                                 SDouble               aLoopCnt );

    static SDouble    getFilterCost( qmoSystemStatistics * aSystemStat,
                                     qmoPredicate        * aFilter,
                                     SDouble               aLoopCnt );

    static SDouble    getTargetCost( qmoSystemStatistics * aSystemStat,
                                     qmsTarget           * aTarget,
                                     SDouble               aLoopCnt );

    static SDouble    getAggrCost( qmoSystemStatistics * aSystemStat,
                                   qmsAggNode          * aAggrNode,
                                   SDouble               aLoopCnt );

private:

    static SDouble    getIndexWithTableCost( qmoSystemStatistics * aSystemStat,
                                             qmoStatistics       * aTableStat,
                                             qmoIdxCardInfo      * aIndexStat,
                                             SDouble               aKeyRangeSelectivity,
                                             SDouble               aKeyFilterSelectivity,
                                             SDouble             * aMemCost,
                                             SDouble             * aDiskCost );

};

#endif /* _O_QMO_COST_H_ */
