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
 *     Cost 계산식
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qmoCost.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <smiTableSpace.h>

// TASK-6699 TPC-H 성능 개선
// 패널티값은 TPC-H 수행시간에 적합하게 설정된 값이다.
//#define QMO_COST_NL_JOIN_PENALTY (6.0)
#define QMO_COST_HASH_JOIN_PENALTY (3.0)

extern mtfModule mtfNotLike;
extern mtfModule mtfLike;
extern mtfModule mtfOr;

/***********************************************************************
* Object Scan Cost
***********************************************************************/

SDouble qmoCost::getTableFullScanCost( qmoSystemStatistics * aSystemStat,
                                       qmoStatistics       * aTableStat,
                                       SDouble               aFilterCost,
                                       SDouble             * aMemCost,
                                       SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : full scan cost
 *
 *****************************************************************************/
    SDouble sCost;

    *aMemCost   = ( aTableStat->totalRecordCnt *
                    aTableStat->readRowTime ) + aFilterCost;

    *aDiskCost  = ( aSystemStat->multiIoScanTime * aTableStat->pageCnt );

    sCost       = *aMemCost + *aDiskCost;
    
    return sCost;
}

SDouble qmoCost::getTableRIDScanCost( qmoStatistics       * aTableStat,
                                      SDouble             * aMemCost,
                                      SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : rid scan cost
 *
 *****************************************************************************/

    SDouble sCost;

    *aMemCost  = aTableStat->readRowTime;

    // 버퍼 hit 상황을 가정하고 디스크 비용은 0 으로 처리합니다.
    *aDiskCost = 0;

    sCost       = *aMemCost + *aDiskCost;
    
    return sCost;
}

SDouble qmoCost::getIndexScanCost( qcStatement         * aStatement,
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
                                   SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : index scan cost
 *               btree, rtree scan 의 코스트를 구한다.
 *****************************************************************************/

    SDouble     sCost;
    SDouble     sMemCost;
    SDouble     sDiskCost;
    UInt        i,j;
    mtcColumn * sTableColumn;
    mtcColumn * sKeyColumn;
    idBool      sWithTableScan = ID_FALSE;

    //--------------------------------------
    // table 스캔을 해야하는지 판단
    //--------------------------------------

    if ( ( aColumns->column.flag & SMI_COLUMN_STORAGE_MASK )
         == SMI_COLUMN_STORAGE_DISK )
    {
        // BUG-39854
        // predicate 없을때는 table scan 을 하게 된다.
        if ( aJoinPred == NULL )
        {
            sWithTableScan = ID_TRUE;
        }
        else
        {
            for ( i=0; i < aTableStat->columnCnt; i++ )
            {
                sTableColumn = aColumns + i;

                for ( j=0; j < aIndexStat->index->keyColCount; j++ )
                {
                    sKeyColumn = &(aIndexStat->index->keyColumns[j]);

                    if ( sTableColumn->column.id == sKeyColumn->column.id )
                    {
                        break;
                    }
                }

                // key 컬럼와 동일한 컬럼이 없는 경우 질의문에 사용되는 컬럼인지 확인한다.
                if ( j == aIndexStat->index->keyColCount )
                {
                    if ( (sTableColumn->flag & MTC_COLUMN_USE_COLUMN_MASK) ==
                         MTC_COLUMN_USE_COLUMN_TRUE )
                    {
                        sWithTableScan = ID_TRUE;
                        break;
                    }
                }
            } // for
        } // if
    }
    else
    {
        // BUG-38152
        // predicate 없을때는 table scan 을 하게 된다.
        if ( aJoinPred == NULL )
        {
            sWithTableScan = ID_TRUE;
        }
        else
        {
            // BUG-36514
            // mem index 일때는 table scan cost 가 추가되지 않는다.
            // 포인터를 이용하여 바로 record 에 접근이 가능하다.
            sWithTableScan = ID_FALSE;
        }
    }

    //--------------------------------------
    // Index scan cost
    //--------------------------------------
    sCost = getIndexBtreeScanCost( aStatement->mSysStat,
                                   aTableStat,
                                   aIndexStat,
                                   aKeyRangeSelectivity,
                                   aKeyRangeCost,
                                   aKeyFilterCost,
                                   aFilterCost,
                                   aMemCost,
                                   aDiskCost );

    //--------------------------------------
    // table scan cost
    //--------------------------------------
    if( sWithTableScan == ID_TRUE )
    {
        sCost += getIndexWithTableCost( aStatement->mSysStat,
                                        aTableStat,
                                        aIndexStat,
                                        aKeyRangeSelectivity,
                                        aKeyFilterSelectivity,
                                        &sMemCost,
                                        &sDiskCost );

        *aMemCost  = sMemCost  + *aMemCost;
        *aDiskCost = sDiskCost + *aDiskCost;
    }
    else
    {
        // nothing todo
    }

    //--------------------------------------
    // OPTIMIZER_MEMORY_INDEX_COST_ADJ ( BUG-43736 )
    //--------------------------------------
    *aMemCost *= QCG_GET_SESSION_OPTIMIZER_MEMORY_INDEX_COST_ADJ(aStatement);
    *aMemCost /= 100;

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_MEMORY_INDEX_COST_ADJ );

    //--------------------------------------
    // OPTIMIZER_DISK_INDEX_COST_ADJ
    //--------------------------------------
    *aDiskCost *= QCG_GET_SESSION_OPTIMIZER_DISK_INDEX_COST_ADJ(aStatement);
    *aDiskCost /= 100;

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_DISK_INDEX_COST_ADJ );

    sCost = *aMemCost + *aDiskCost;
    
    return sCost;
}

SDouble qmoCost::getIndexBtreeScanCost( qmoSystemStatistics * aSystemStat,
                                        qmoStatistics       * aTableStat,
                                        qmoIdxCardInfo      * aIndexStat,
                                        SDouble               aKeyRangeSelectivity,
                                        SDouble               aKeyRangeCost,
                                        SDouble               aKeyFilterCost,
                                        SDouble               aFilterCost,
                                        SDouble             * aMemCost,
                                        SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : getIndexBtreeScanCost
 *               sLeafPageCnt 는 통계정보로 알수 없으므로 다음과 같이 추정하여 사용한다.
 *               전체 레코드 갯수 / avgSlotCount
 *****************************************************************************/

    SDouble     sCost;
    SDouble     sLeafPageCnt;

    sLeafPageCnt = idlOS::ceil( aTableStat->totalRecordCnt / aIndexStat->avgSlotCount );

    *aMemCost  = ( aIndexStat->indexLevel * aTableStat->readRowTime ) +
                   aKeyRangeCost  +
                   aKeyFilterCost +
                   aFilterCost;

    if( aIndexStat->pageCnt > 0 )
    {
        *aDiskCost = ( aSystemStat->singleIoScanTime *
                      (aIndexStat->indexLevel + ( sLeafPageCnt * aKeyRangeSelectivity )) );
    }
    else
    {
        *aDiskCost = 0;
    }

    sCost      = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getIndexWithTableCost( qmoSystemStatistics * aSystemStat,
                                        qmoStatistics       * aTableStat,
                                        qmoIdxCardInfo      * aIndexStat,
                                        SDouble               aKeyRangeSelectivity,
                                        SDouble               aKeyFilterSelectivity,
                                        SDouble             * aMemCost,
                                        SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : getIndexWithTableCost
 *               index에 없는 컬럼들을 참조할때 발생하는 비용을 계산한다.
 *****************************************************************************/

    SDouble sCost;
    SDouble sIndexSelectivity = aKeyRangeSelectivity * aKeyFilterSelectivity;

    *aMemCost  = ( aTableStat->totalRecordCnt *
                   sIndexSelectivity *
                   aTableStat->readRowTime );

    if( aIndexStat->pageCnt > 0 )
    {
        *aDiskCost = ( aIndexStat->clusteringFactor *
                       sIndexSelectivity *
                       aSystemStat->singleIoScanTime );
    }
    else
    {
        *aDiskCost = 0;
    }

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

/***********************************************************************
* Join Cost
***********************************************************************/

SDouble qmoCost::getFullNestedJoinCost( qmgGraph    * aLeft,
                                        qmgGraph    * aRight,
                                        SDouble       aFilterCost,
                                        SDouble     * aMemCost,
                                        SDouble     * aDiskCost )
{
/******************************************************************************
 *
 * Description : full nested loop join cost
 *               left scan cost + ( left output * right scan cost )
 *****************************************************************************/

    SDouble        sCost;
    qmgCostInfo  * sLeftCostInfo;
    qmgCostInfo  * sRightCostInfo;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    *aMemCost =  sLeftCostInfo->totalAccessCost +
               ( sLeftCostInfo->outputRecordCnt * sRightCostInfo->totalAccessCost ) +
                 aFilterCost;

    *aDiskCost =  sLeftCostInfo->totalDiskCost +
                ( sLeftCostInfo->outputRecordCnt * sRightCostInfo->totalDiskCost );

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getFullStoreJoinCost( qmgGraph            * aLeft,
                                       qmgGraph            * aRight,
                                       SDouble               aFilterCost,
                                       qmoSystemStatistics * aSystemStat,
                                       idBool                aUseRightDiskTemp,
                                       SDouble             * aMemCost,
                                       SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : full stored nested loop join cost
 *               left scan cost + right scan cost + right temp build cost
 *               ( left output * right temp scan cost )
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sRightTempScanCost;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    // BUGBUG sRightTempScanCost 의 값을 보정해야 한다.
    // 메모리 디스크 템프테이블일때를 고려해야함
    if( aRight->type == QMG_SELECTION ||
        aRight->type == QMG_SHARD_SELECT || // PROJ-2638
        aRight->type == QMG_PARTITION )
    {
        sRightTempScanCost = ( sRightCostInfo->outputRecordCnt *
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        // Join 그래프가 오는 경우 레코드의 읽기시간을 알수 없어서 1로 가정한다.
        sRightTempScanCost = sRightCostInfo->outputRecordCnt;
    }

    if( aUseRightDiskTemp == ID_FALSE )
    {
        sRightMemTempCost =  getMemStoreTempCost( aSystemStat,
                                                  sRightCostInfo );
        sRightDiskTempCost = 0;
    }
    else
    {
        sRightMemTempCost  = getMemStoreTempCost( aSystemStat,
                                                  sRightCostInfo );

        sRightDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                   sRightCostInfo->outputRecordCnt,
                                                   sRightCostInfo->recordSize,
                                                   QMO_COST_STORE_SORT_TEMP );
    }

    *aMemCost = sLeftCostInfo->totalAccessCost +
                sRightCostInfo->totalAccessCost +
                sRightMemTempCost +
              ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
                aFilterCost;

    *aDiskCost =  sLeftCostInfo->totalDiskCost +
                  sRightCostInfo->totalDiskCost +
                  sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getIndexNestedJoinCost( qmgGraph            * aLeft,
                                         qmoStatistics       * aTableStat,
                                         qmoIdxCardInfo      * aIndexStat,
                                         qmoSystemStatistics * aSystemStat,
                                         SDouble               aRightMemCost,
                                         SDouble               aRightDiskCost,
                                         SDouble               aFilterCost,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : index nested loop join cost
 *               left scan cost + ( left output * right scan cost )
 *****************************************************************************/

    SDouble          sCost;
    SDouble          sTotalReadPageCnt;
    SDouble          sMaxReadPageCnt;
    qmgCostInfo    * sLeftCostInfo;

    sLeftCostInfo  = &(aLeft->costInfo);

    // TASK-6699 TPC-H 성능 개선
    /* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
    *aMemCost =  sLeftCostInfo->totalAccessCost +
               ( sLeftCostInfo->outputRecordCnt * (aRightMemCost + aSystemStat->mIndexNlJoinPenalty) ) +
                 aFilterCost;

    // BUG-37125 tpch plan optimization
    // Disk index 의 경우 인덱스의 page 갯수가 buffer 사이즈보다 큰 경우가 있다.
    if( aIndexStat->pageCnt > 0 )
    {
        // 중복을 고려하지 않은 read page 갯수
        sTotalReadPageCnt = sLeftCostInfo->outputRecordCnt *
                                ( aRightDiskCost / aSystemStat->singleIoScanTime );

        // 중복을 고려한 read page 갯수
        sMaxReadPageCnt = IDL_MIN( sTotalReadPageCnt,
                                   aTableStat->pageCnt + aIndexStat->pageCnt );

        // 버퍼 사이즈보다 큰 경우 버퍼 리플레이스가 일어난다.
        if( smiGetBufferPoolSize() > sMaxReadPageCnt )
        {
            *aDiskCost = sLeftCostInfo->totalDiskCost +
                            ( sMaxReadPageCnt * aSystemStat->singleIoScanTime );
        }
        else
        {
            *aDiskCost = sLeftCostInfo->totalDiskCost +
                            ( sTotalReadPageCnt * aSystemStat->singleIoScanTime );
        }
    }
    else
    {
        *aDiskCost = 0;
    }

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getFullNestedJoin4SemiCost( qmgGraph    * aLeft,
                                             qmgGraph    * aRight,
                                             SDouble       aJoinSelectivty,
                                             SDouble       aFilterCost,
                                             SDouble     * aMemCost,
                                             SDouble     * aDiskCost )
{
/******************************************************************************
 *
 * Description : left scan cost +
 *               left output * ( semi 비용 * 선택도 +
                                 full scan 비용 * (1 - 선택도) )
 *               semi 비용      = right scan cost / 2
 *               full scan 비용 = right scan cost
 *****************************************************************************/

    SDouble        sCost;
    qmgCostInfo  * sLeftCostInfo;
    qmgCostInfo  * sRightCostInfo;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    *aMemCost = sLeftCostInfo->totalAccessCost +
                    sLeftCostInfo->outputRecordCnt *
                    ( ( sRightCostInfo->totalAccessCost / 2 ) * aJoinSelectivty +
                            sRightCostInfo->totalAccessCost * ( 1 - aJoinSelectivty ) ) +
                    aFilterCost;

    *aDiskCost = sLeftCostInfo->totalDiskCost +
                    sLeftCostInfo->outputRecordCnt *
                    ( ( sRightCostInfo->totalDiskCost / 2 ) * aJoinSelectivty +
                             sRightCostInfo->totalDiskCost * ( 1 - aJoinSelectivty ) );

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getIndexNestedJoin4SemiCost( qmgGraph            * aLeft,
                                              qmoStatistics       * aTableStat,
                                              qmoIdxCardInfo      * aIndexStat,
                                              qmoSystemStatistics * aSystemStat,
                                              SDouble               aFilterCost,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : left scan cost + ( left output * index level )
                 semi,anti 조인의 경우 1개의 key 값에 대해서 index 를 한번만 사용한다.
 *****************************************************************************/

    SDouble          sCost;
    SDouble          sTotalReadPageCnt;
    SDouble          sMaxReadPageCnt;
    qmgCostInfo    * sLeftCostInfo;

    sLeftCostInfo  = &(aLeft->costInfo);

    // TASK-6699 TPC-H 성능 개선
    /* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
    *aMemCost = sLeftCostInfo->totalAccessCost +
                ( sLeftCostInfo->outputRecordCnt * 0.5 *
                    ( aIndexStat->indexLevel *
                      aTableStat->readRowTime +
                      aSystemStat->mIndexNlJoinPenalty ) ) +
                aFilterCost;

    if( aIndexStat->pageCnt > 0 )
    {
        // tpch Q21 semi index nl join cost 를 낮추어야 합니다.
        sTotalReadPageCnt = sLeftCostInfo->outputRecordCnt * 0.5 *
                            aIndexStat->indexLevel;

        sMaxReadPageCnt = IDL_MIN( sTotalReadPageCnt, aIndexStat->pageCnt );

        // 버퍼 사이즈보다 큰 경우 버퍼 리플레이스가 일어난다.
        if( smiGetBufferPoolSize() > sMaxReadPageCnt )
        {
            *aDiskCost = sLeftCostInfo->totalDiskCost +
                            ( sMaxReadPageCnt * aSystemStat->singleIoScanTime );
        }
        else
        {
            *aDiskCost = sLeftCostInfo->totalDiskCost +
                            ( sTotalReadPageCnt * aSystemStat->singleIoScanTime );
        }
    }
    else
    {
        *aDiskCost = 0;
    }

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getFullNestedJoin4AntiCost( qmgGraph    * aLeft,
                                             qmgGraph    * aRight,
                                             SDouble       aJoinSelectivty,
                                             SDouble       aFilterCost,
                                             SDouble     * aMemCost,
                                             SDouble     * aDiskCost )
{
/******************************************************************************
 *
 * Description : left scan cost +
 *               left output * ( anti 비용 * (1 - 선택도) +
                                 full scan 비용 * 선택도 )
 *               anti 비용       = right scan cost / 2
 *               full scan 비용  = right scan cost
 *****************************************************************************/

    SDouble        sCost;
    qmgCostInfo  * sLeftCostInfo;
    qmgCostInfo  * sRightCostInfo;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    *aMemCost = sLeftCostInfo->totalAccessCost +
                    sLeftCostInfo->outputRecordCnt *
                    ( ( sRightCostInfo->totalAccessCost / 2 ) * ( 1 - aJoinSelectivty ) +
                            sRightCostInfo->totalAccessCost * aJoinSelectivty ) +
                    aFilterCost;

    *aDiskCost = sLeftCostInfo->totalDiskCost +
                    sLeftCostInfo->outputRecordCnt *
                    ( ( sRightCostInfo->totalDiskCost / 2 ) * ( 1 - aJoinSelectivty ) +
                             sRightCostInfo->totalDiskCost * aJoinSelectivty );

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getIndexNestedJoin4AntiCost( qmgGraph            * aLeft,
                                              qmoStatistics       * aTableStat,
                                              qmoIdxCardInfo      * aIndexStat,
                                              qmoSystemStatistics * aSystemStat,
                                              SDouble               aFilterCost,
                                              SDouble             * aMemCost,
                                              SDouble             * aDiskCost )
{
/******************************************************************************
*
* Description : same getIndexNestedJoin4SemiCost
*****************************************************************************/

    SDouble sCost;

    sCost = getIndexNestedJoin4SemiCost( aLeft,
                                         aTableStat,
                                         aIndexStat,
                                         aSystemStat,
                                         aFilterCost,
                                         aMemCost,
                                         aDiskCost );
    
    return sCost;
}

SDouble qmoCost::getInverseIndexNestedJoinCost( qmgGraph            * aLeft,
                                                qmoStatistics       * aTableStat,
                                                qmoIdxCardInfo      * aIndexStat,
                                                qmoSystemStatistics * aSystemStat,
                                                idBool                aUseLeftDiskTemp,
                                                SDouble               aFilterCost,
                                                SDouble             * aMemCost,
                                                SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : PROJ-2385 Inverse Index NL for Semi Join
 *
 *               left scan cost + left hashing cost + ( left output * right index level )
 *               Semi 조인의 경우 1개의 key 값에 대해서 index 를 한번만 사용한다.
 *****************************************************************************/

    SDouble          sCost;
    SDouble          sTotalReadPageCnt;
    SDouble          sMaxReadPageCnt;
    qmgCostInfo    * sLeftCostInfo;
    SDouble          sLeftMemTempCost;
    SDouble          sLeftDiskTempCost;

    sLeftCostInfo = &(aLeft->costInfo);

    // LEFT는 Distinct Hashing 된다
    if ( aUseLeftDiskTemp == ID_FALSE )
    {
        sLeftMemTempCost = getMemHashTempCost( aSystemStat,
                                               sLeftCostInfo,
                                               1 ); // nodeCnt

        sLeftDiskTempCost = 0;
    }
    else
    {
        sLeftMemTempCost  = getMemHashTempCost( aSystemStat,
                                                sLeftCostInfo,
                                                1 ); // nodeCnt

        sLeftDiskTempCost = getDiskHashTempCost( aSystemStat,
                                                 sLeftCostInfo,
                                                 1, // nodeCnt
                                                 sLeftCostInfo->recordSize );
    }

    // TASK-6699 TPC-H 성능 개선
    /* BUG-48135 NL Join Penalty 값을 조절할수 있는 property 추가 */
    *aMemCost = sLeftCostInfo->totalAccessCost +
                sLeftMemTempCost +
                ( sLeftCostInfo->outputRecordCnt *
                    ( aIndexStat->indexLevel *
                      aTableStat->readRowTime +
                      aSystemStat->mIndexNlJoinPenalty ) ) +
                aFilterCost;

    if( aIndexStat->pageCnt > 0 )
    {
        sTotalReadPageCnt = sLeftCostInfo->outputRecordCnt *
                            aIndexStat->indexLevel;

        sMaxReadPageCnt = IDL_MIN( sTotalReadPageCnt, aIndexStat->pageCnt );

        // 버퍼 사이즈보다 큰 경우 버퍼 리플레이스가 일어난다.
        if( smiGetBufferPoolSize() > sMaxReadPageCnt )
        {
            *aDiskCost = sLeftCostInfo->totalDiskCost +
                            ( sMaxReadPageCnt * aSystemStat->singleIoScanTime );
        }
        else
        {
            *aDiskCost = sLeftCostInfo->totalDiskCost +
                            ( sTotalReadPageCnt * aSystemStat->singleIoScanTime );
        }
    }
    else
    {
        *aDiskCost = 0;
    }

    *aDiskCost += sLeftDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getAntiOuterJoinCost( qmgGraph          * aLeft,
                                       SDouble             aRightMemCost,
                                       SDouble             aRightDiskCost,
                                       SDouble             aFilterCost,
                                       SDouble           * aMemCost,
                                       SDouble           * aDiskCost )
{
/******************************************************************************
 *
 * Description : anti join cost
 *               left scan cost + ( left output * right scan cost )
 *****************************************************************************/

    SDouble        sCost;
    qmgCostInfo  * sLeftCostInfo;

    sLeftCostInfo  = &(aLeft->costInfo);

    *aMemCost =  sLeftCostInfo->totalAccessCost +
               ( sLeftCostInfo->outputRecordCnt * aRightMemCost ) +
                 aFilterCost;

    *aDiskCost =  sLeftCostInfo->totalDiskCost +
                ( sLeftCostInfo->outputRecordCnt * aRightDiskCost );

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getOnePassHashJoinCost( qmgGraph            * aLeft,
                                         qmgGraph            * aRight,
                                         SDouble               aFilterCost,
                                         qmoSystemStatistics * aSystemStat,
                                         idBool                aUseRightDiskTemp,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : one pass hash join
 *               left scan cost + right scan cost + right temp build cost
 *               ( left output * right temp scan cost )
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sRightTempScanCost;
    SDouble          sFilterLoop;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    // BUGBUG sRightTempScanCost 의 값을 보정해야 한다.
    // 메모리 디스크 템프테이블일때를 고려해야함
    if( aRight->type == QMG_SELECTION ||
        aRight->type == QMG_SHARD_SELECT || // PROJ-2638
        aRight->type == QMG_PARTITION )
    {
        sRightTempScanCost = ( aSystemStat->mHashTime +
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = aSystemStat->mHashTime;
    }

    if( aUseRightDiskTemp == ID_FALSE )
    {
        sRightMemTempCost = getMemHashTempCost( aSystemStat,
                                                sRightCostInfo,
                                                1 ); // nodeCnt

        sRightDiskTempCost = 0;
    }
    else
    {
        sRightMemTempCost = getMemHashTempCost( aSystemStat,
                                                sRightCostInfo,
                                                1 ); // nodeCnt

        sRightDiskTempCost = getDiskHashTempCost( aSystemStat,
                                                  sRightCostInfo,
                                                  1, // nodeCnt
                                                  sRightCostInfo->recordSize );
    }

    // TASK-6699 TPC-H 성능 개선
    // PROJ-2553 에서 추가된 Hash Temp Table 에 최적화된 계산식
    if ( sLeftCostInfo->outputRecordCnt >
         (QMC_MEM_PART_HASH_MAX_PART_COUNT * QMC_MEM_PART_HASH_AVG_RECORD_COUNT) )
    {
        sFilterLoop = sLeftCostInfo->outputRecordCnt / QMC_MEM_PART_HASH_MAX_PART_COUNT;
    }
    else
    {
        sFilterLoop = QMC_MEM_PART_HASH_AVG_RECORD_COUNT;
    }

    if( aUseRightDiskTemp == ID_FALSE )
    {
        sRightTempScanCost += QMO_COST_HASH_JOIN_PENALTY;

        *aMemCost =  sLeftCostInfo->totalAccessCost +
                     sRightCostInfo->totalAccessCost +
                     sRightMemTempCost +
                   ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
                     aFilterCost * sFilterLoop;
    }
    else
    {
        // 기존 계산식을 그대로 이용한다.
        *aMemCost =  sLeftCostInfo->totalAccessCost +
                     sRightCostInfo->totalAccessCost +
                     sRightMemTempCost +
                   ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
                     aFilterCost;
    }

    *aDiskCost =  sLeftCostInfo->totalDiskCost +
                  sRightCostInfo->totalDiskCost +
                  sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;
    
    return sCost;
}

SDouble qmoCost::getTwoPassHashJoinCost( qmgGraph            * aLeft,
                                         qmgGraph            * aRight,
                                         SDouble               aFilterCost,
                                         qmoSystemStatistics * aSystemStat,
                                         idBool                aUseLeftDiskTemp,
                                         idBool                aUseRightDiskTemp,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : two pass hash join
 *               left scan cost + right scan cost +
 *               left temp build cost + right temp build cost
 *               ( left output * right temp scan cost )
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sLeftMemTempCost;
    SDouble          sLeftDiskTempCost;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sLeftTempScanCost;
    SDouble          sRightTempScanCost;
    SDouble          sFilterLoop;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    // BUGBUG sRightTempScanCost 의 값을 보정해야 한다.
    if( aLeft->type == QMG_SELECTION ||
        aLeft->type == QMG_SHARD_SELECT || // PROJ-2638
        aLeft->type == QMG_PARTITION )
    {
        sLeftTempScanCost = ( sLeftCostInfo->outputRecordCnt *
                              aLeft->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sLeftTempScanCost = sLeftCostInfo->outputRecordCnt;
    }

    if( aRight->type == QMG_SELECTION ||
        aRight->type == QMG_SHARD_SELECT || // PROJ-2638
        aRight->type == QMG_PARTITION )
    {
        sRightTempScanCost = ( aSystemStat->mHashTime +
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = aSystemStat->mHashTime;
    }

    if( aUseLeftDiskTemp == ID_FALSE )
    {
        sLeftMemTempCost = getMemHashTempCost( aSystemStat,
                                               sLeftCostInfo,
                                               1 ); // nodeCnt

        sLeftDiskTempCost = 0;
    }
    else
    {
        sLeftMemTempCost = getMemHashTempCost( aSystemStat,
                                               sLeftCostInfo,
                                               1 ); // nodeCnt

        sLeftDiskTempCost = getDiskHashTempCost( aSystemStat,
                                                 sLeftCostInfo,
                                                 1, // nodeCnt
                                                 sLeftCostInfo->recordSize );
    }

    if( aUseRightDiskTemp == ID_FALSE )
    {
        sRightMemTempCost = getMemHashTempCost( aSystemStat,
                                                sRightCostInfo,
                                                1 ); // nodeCnt

        sRightDiskTempCost = 0;
    }
    else
    {
        sRightMemTempCost = getMemHashTempCost( aSystemStat,
                                                sRightCostInfo,
                                                1 ); // nodeCnt

        sRightDiskTempCost = getDiskHashTempCost( aSystemStat,
                                                  sRightCostInfo,
                                                  1, // nodeCnt
                                                  sRightCostInfo->recordSize );
    }

    // TASK-6699 TPC-H 성능 개선
    // PROJ-2553 에서 추가된 Hash Temp Table 에 최적화된 계산식
    if ( sLeftCostInfo->outputRecordCnt >
         (QMC_MEM_PART_HASH_MAX_PART_COUNT * QMC_MEM_PART_HASH_AVG_RECORD_COUNT) )
    {
        sFilterLoop = sLeftCostInfo->outputRecordCnt / QMC_MEM_PART_HASH_MAX_PART_COUNT;
    }
    else
    {
        sFilterLoop = QMC_MEM_PART_HASH_AVG_RECORD_COUNT;
    }

    if( aUseRightDiskTemp == ID_FALSE )
    {
        sRightTempScanCost += QMO_COST_HASH_JOIN_PENALTY;

        *aMemCost =  sLeftCostInfo->totalAccessCost +
                     sRightCostInfo->totalAccessCost +
                     sLeftMemTempCost +
                     sRightMemTempCost +
                   ( sLeftTempScanCost + ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) ) +
                     aFilterCost * sFilterLoop;
    }
    else
    {
        *aMemCost =  sLeftCostInfo->totalAccessCost +
                     sRightCostInfo->totalAccessCost +
                     sLeftMemTempCost +
                     sRightMemTempCost +
                   ( sLeftTempScanCost + ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) ) +
                     aFilterCost;
    }

    *aDiskCost =  sLeftCostInfo->totalDiskCost +
                  sRightCostInfo->totalDiskCost +
                  sLeftDiskTempCost +
                  sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getInverseHashJoinCost( qmgGraph            * aLeft,
                                         qmgGraph            * aRight,
                                         SDouble               aFilterCost,
                                         qmoSystemStatistics * aSystemStat,
                                         idBool                aUseRightDiskTemp,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : inverse hash join (PROJ-2339)
 *
 *               left scan cost + right scan cost + right temp build cost
 *               ( left output * right temp scan cost )
 *               ( right output * right temp scan cost )
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sRightTempScanCost;
    SDouble          sFilterLoop;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    // BUGBUG sRightTempScanCost 의 값을 보정해야 한다.
    // 메모리 디스크 템프테이블일때를 고려해야함
    if ( ( aRight->type == QMG_SELECTION ) ||
         ( aRight->type == QMG_SHARD_SELECT ) || // PROJ-2638
         ( aRight->type == QMG_PARTITION ) )
    {
        sRightTempScanCost = ( aSystemStat->mHashTime +
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = aSystemStat->mHashTime;
    }

    if ( aUseRightDiskTemp == ID_FALSE )
    {
        sRightMemTempCost = getMemHashTempCost( aSystemStat,
                                                sRightCostInfo,
                                                1 ); // nodeCnt

        sRightDiskTempCost = 0;
    }
    else
    {
        sRightMemTempCost = getMemHashTempCost( aSystemStat,
                                                sRightCostInfo,
                                                1 ); // nodeCnt

        sRightDiskTempCost = getDiskHashTempCost( aSystemStat,
                                                  sRightCostInfo,
                                                  1, // nodeCnt
                                                  sRightCostInfo->recordSize );
    }

    // TASK-6699 TPC-H 성능 개선
    // PROJ-2553 에서 추가된 Hash Temp Table 에 최적화된 계산식
    if ( sLeftCostInfo->outputRecordCnt >
         (QMC_MEM_PART_HASH_MAX_PART_COUNT * QMC_MEM_PART_HASH_AVG_RECORD_COUNT) )
    {
        sFilterLoop = sLeftCostInfo->outputRecordCnt / QMC_MEM_PART_HASH_MAX_PART_COUNT;
    }
    else
    {
        sFilterLoop = QMC_MEM_PART_HASH_AVG_RECORD_COUNT;
    }

    if( aUseRightDiskTemp == ID_FALSE )
    {
        sRightTempScanCost += QMO_COST_HASH_JOIN_PENALTY;

        *aMemCost = sLeftCostInfo->totalAccessCost +
                    sRightCostInfo->totalAccessCost +
                    sRightMemTempCost +
                  ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
                  ( sRightCostInfo->outputRecordCnt * sRightTempScanCost ) +
                    aFilterCost * sFilterLoop;
    }
    else
    {
        *aMemCost = sLeftCostInfo->totalAccessCost +
                    sRightCostInfo->totalAccessCost +
                    sRightMemTempCost +
                  ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
                  ( sRightCostInfo->outputRecordCnt * sRightTempScanCost ) +
                    aFilterCost;
    }

    *aDiskCost = sLeftCostInfo->totalDiskCost +
                 sRightCostInfo->totalDiskCost +
                 sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getOnePassSortJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                         qmgGraph            * aLeft,
                                         qmgGraph            * aRight,
                                         SDouble               aFilterCost,
                                         qmoSystemStatistics * aSystemStat,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : one pass sort join
 *               left scan cost + right scan cost + right temp build cost
 *               ( left output * right temp scan cost )
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sRightTempScanCost;
    UInt             sFlag;
    UInt             sTempType;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    if( aRight->type == QMG_SELECTION ||
        aRight->type == QMG_SHARD_SELECT || // PROJ-2638
        aRight->type == QMG_PARTITION )
    {
        sRightTempScanCost = ( sRightCostInfo->outputRecordCnt *
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = sRightCostInfo->outputRecordCnt;
    }

    sFlag     = aJoinMethodCost->flag;
    sTempType = (sFlag & QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    switch ( (sFlag & QMO_JOIN_RIGHT_NODE_MASK) )
    {
        case QMO_JOIN_RIGHT_NODE_NONE :
            sRightTempScanCost  = 0;
            sRightMemTempCost   = 0;
            sRightDiskTempCost  = 0;
            break;

        case QMO_JOIN_RIGHT_NODE_STORE :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                           sRightCostInfo->outputRecordCnt,
                                                           sRightCostInfo->recordSize,
                                                           QMO_COST_STORE_SORT_TEMP );
            }
            break;

        case QMO_JOIN_RIGHT_NODE_SORTING :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt

                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt

                sRightDiskTempCost = getDiskSortTempCost( aSystemStat,
                                                          sRightCostInfo,
                                                          1, // nodeCnt
                                                          sRightCostInfo->recordSize );
            }
            break;

        default :
            sRightMemTempCost   = 0;
            sRightDiskTempCost  = 0;
            IDE_DASSERT ( 0 );
    }

    *aMemCost =  sLeftCostInfo->totalAccessCost +
                 sRightCostInfo->totalAccessCost +
                 sRightMemTempCost +
               ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
                 aFilterCost;

    *aDiskCost = sLeftCostInfo->totalDiskCost +
                 sRightCostInfo->totalDiskCost +
                 sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getTwoPassSortJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                         qmgGraph            * aLeft,
                                         qmgGraph            * aRight,
                                         SDouble               aFilterCost,
                                         qmoSystemStatistics * aSystemStat,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : two pass sort join
 *               left scan cost + right scan cost +
 *               left temp build cost + right temp build cost
 *               ( left output * right temp scan cost )
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sLeftMemTempCost;
    SDouble          sLeftDiskTempCost;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sLeftTempScanCost;
    SDouble          sRightTempScanCost;
    UInt             sFlag;
    UInt             sTempType;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    //--------------------------------------
    // temp table Scan Cost
    //--------------------------------------
    if( aLeft->type == QMG_SELECTION ||
        aLeft->type == QMG_SHARD_SELECT || // PROJ-2638
        aLeft->type == QMG_PARTITION )
    {
        sLeftTempScanCost = ( sLeftCostInfo->outputRecordCnt *
                              aLeft->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sLeftTempScanCost = sLeftCostInfo->outputRecordCnt;
    }

    if( aRight->type == QMG_SELECTION ||
        aRight->type == QMG_SHARD_SELECT || // PROJ-2638
        aRight->type == QMG_PARTITION )
    {
        sRightTempScanCost = ( sRightCostInfo->outputRecordCnt *
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = sRightCostInfo->outputRecordCnt;
    }

    //--------------------------------------
    // temp table build Cost
    //--------------------------------------
    sFlag     = aJoinMethodCost->flag;
    sTempType = (sFlag & QMO_JOIN_METHOD_LEFT_STORAGE_MASK);

    switch ( (sFlag & QMO_JOIN_LEFT_NODE_MASK) )
    {
        case QMO_JOIN_LEFT_NODE_NONE :
            sLeftTempScanCost  = 0;
            sLeftMemTempCost   = 0;
            sLeftDiskTempCost  = 0;
            break;

        case QMO_JOIN_LEFT_NODE_STORE :

            if( sTempType != QMO_JOIN_METHOD_LEFT_STORAGE_DISK )
            {
                sLeftMemTempCost = getMemStoreTempCost( aSystemStat,
                                                        sLeftCostInfo );
                sLeftDiskTempCost = 0;
            }
            else
            {
                sLeftMemTempCost = getMemStoreTempCost( aSystemStat,
                                                        sLeftCostInfo );

                sLeftDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                          sLeftCostInfo->outputRecordCnt,
                                                          sLeftCostInfo->recordSize,
                                                          QMO_COST_STORE_SORT_TEMP );
            }
            break;

        case QMO_JOIN_LEFT_NODE_SORTING :

            if( sTempType != QMO_JOIN_METHOD_LEFT_STORAGE_DISK )
            {
                sLeftMemTempCost = getMemSortTempCost( aSystemStat,
                                                       sLeftCostInfo,
                                                       1 ); // nodeCnt
                sLeftDiskTempCost = 0;
            }
            else
            {
                sLeftMemTempCost = getMemSortTempCost( aSystemStat,
                                                       sLeftCostInfo,
                                                       1 ); // nodeCnt

                sLeftDiskTempCost = getDiskSortTempCost( aSystemStat,
                                                         sLeftCostInfo,
                                                         1, // nodeCnt
                                                         sLeftCostInfo->recordSize );
            }
            break;

        default :
            sLeftMemTempCost   = 0;
            sLeftDiskTempCost  = 0;
            IDE_DASSERT ( 0 );
    }


    sTempType = (sFlag & QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    switch ( (sFlag & QMO_JOIN_RIGHT_NODE_MASK) )
    {
        case QMO_JOIN_RIGHT_NODE_NONE :
            sRightTempScanCost  = 0;
            sRightMemTempCost   = 0;
            sRightDiskTempCost  = 0;
            break;

        case QMO_JOIN_RIGHT_NODE_STORE :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                           sRightCostInfo->outputRecordCnt,
                                                           sRightCostInfo->recordSize,
                                                           QMO_COST_STORE_SORT_TEMP );
            }
            break;

        case QMO_JOIN_RIGHT_NODE_SORTING :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt
                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt

                sRightDiskTempCost = getDiskSortTempCost( aSystemStat,
                                                          sRightCostInfo,
                                                          1, // nodeCnt
                                                          sRightCostInfo->recordSize );
            }
            break;

        default :
            sRightMemTempCost   = 0;
            sRightDiskTempCost  = 0;
            IDE_DASSERT ( 0 );
    }

    *aMemCost =  sLeftCostInfo->totalAccessCost +
                 sRightCostInfo->totalAccessCost +
                 sLeftMemTempCost +
                 sRightMemTempCost +
               ( sLeftTempScanCost + ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) ) +
                 aFilterCost;

    *aDiskCost =  sLeftCostInfo->totalDiskCost +
                  sRightCostInfo->totalDiskCost +
                  sLeftDiskTempCost +
                  sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getInverseSortJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                         qmgGraph            * aLeft,
                                         qmgGraph            * aRight,
                                         SDouble               aFilterCost,
                                         qmoSystemStatistics * aSystemStat,
                                         SDouble             * aMemCost,
                                         SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : inverse sort join
 *               left scan cost + right scan cost + right temp build cost +
 *               ( left output * right temp scan cost ) +
 *               right temp scan cost
 *****************************************************************************/

    SDouble          sCost;
    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sRightTempScanCost;
    UInt             sFlag;
    UInt             sTempType;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    if( ( aRight->type == QMG_SELECTION ) ||
        ( aRight->type == QMG_SHARD_SELECT ) || // PROJ-2638
        ( aRight->type == QMG_PARTITION ) )
    {
        sRightTempScanCost = ( sRightCostInfo->outputRecordCnt *
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = sRightCostInfo->outputRecordCnt;
    }

    sFlag     = aJoinMethodCost->flag;
    sTempType = (sFlag & QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    switch ( (sFlag & QMO_JOIN_RIGHT_NODE_MASK) )
    {
        case QMO_JOIN_RIGHT_NODE_NONE :
            sRightTempScanCost  = 0;
            sRightMemTempCost   = 0;
            sRightDiskTempCost  = 0;
            break;

        case QMO_JOIN_RIGHT_NODE_STORE :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                           sRightCostInfo->outputRecordCnt,
                                                           sRightCostInfo->recordSize,
                                                           QMO_COST_STORE_SORT_TEMP );
            }
            break;

        case QMO_JOIN_RIGHT_NODE_SORTING :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt

                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt

                sRightDiskTempCost = getDiskSortTempCost( aSystemStat,
                                                          sRightCostInfo,
                                                          1, // nodeCnt
                                                          sRightCostInfo->recordSize );
            }
            break;

        default :
            sRightMemTempCost   = 0;
            sRightDiskTempCost  = 0;
            IDE_DASSERT ( 0 );
    }

    // PROJ-2385 add sRightTempScanCost
    *aMemCost =  sLeftCostInfo->totalAccessCost +
                 sRightCostInfo->totalAccessCost +
                 sRightMemTempCost +
               ( sLeftCostInfo->outputRecordCnt * sRightTempScanCost ) +
               ( sRightCostInfo->outputRecordCnt * sRightTempScanCost ) +
                 aFilterCost;

    *aDiskCost = sLeftCostInfo->totalDiskCost +
                 sRightCostInfo->totalDiskCost +
                 sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

SDouble qmoCost::getMergeJoinCost( qmoJoinMethodCost   * aJoinMethodCost,
                                   qmgGraph            * aLeft,
                                   qmgGraph            * aRight,
                                   qmoAccessMethod     * aLeftSelMethod,
                                   qmoAccessMethod     * aRightSelMethod,
                                   SDouble               aFilterCost,
                                   qmoSystemStatistics * aSystemStat,
                                   SDouble             * aMemCost,
                                   SDouble             * aDiskCost )
{
/******************************************************************************
 *
 * Description : merge join
 *               left scan cost + right scan cost +
 *               left temp build cost + right temp build cost
 *****************************************************************************/

    SDouble          sCost;

    SDouble          sLeftAccessCost;
    SDouble          sLeftDiskCost;
    SDouble          sLeftMemTempCost;
    SDouble          sLeftDiskTempCost;
    SDouble          sLeftTempScanCost;

    SDouble          sRightAccessCost;
    SDouble          sRightDiskCost;
    SDouble          sRightMemTempCost;
    SDouble          sRightDiskTempCost;
    SDouble          sRightTempScanCost;

    qmgCostInfo    * sLeftCostInfo;
    qmgCostInfo    * sRightCostInfo;
    UInt             sFlag;
    UInt             sTempType;

    sLeftCostInfo  = &(aLeft->costInfo);
    sRightCostInfo = &(aRight->costInfo);

    //--------------------------------------
    // temp table scan Cost
    //--------------------------------------
    if( aLeft->type == QMG_SELECTION ||
        aLeft->type == QMG_SHARD_SELECT || // PROJ-2638
        aLeft->type == QMG_PARTITION )
    {
        sLeftTempScanCost = ( sLeftCostInfo->outputRecordCnt *
                              aLeft->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sLeftTempScanCost = sLeftCostInfo->outputRecordCnt;
    }

    if( aRight->type == QMG_SELECTION ||
        aRight->type == QMG_SHARD_SELECT || // PROJ-2638
        aRight->type == QMG_PARTITION )
    {
        sRightTempScanCost = ( sRightCostInfo->outputRecordCnt *
                               aRight->myFrom->tableRef->statInfo->readRowTime );
    }
    else
    {
        sRightTempScanCost = sRightCostInfo->outputRecordCnt;
    }

    //--------------------------------------
    // temp table build Cost
    //--------------------------------------
    sFlag     = aJoinMethodCost->flag;
    sTempType = (sFlag & QMO_JOIN_METHOD_LEFT_STORAGE_MASK);

    switch ( (sFlag & QMO_JOIN_LEFT_NODE_MASK) )
    {
        case QMO_JOIN_LEFT_NODE_NONE :
            sLeftTempScanCost  = 0;
            sLeftMemTempCost   = 0;
            sLeftDiskTempCost  = 0;
            break;

        case QMO_JOIN_LEFT_NODE_STORE :

            if( sTempType != QMO_JOIN_METHOD_LEFT_STORAGE_DISK )
            {
                sLeftMemTempCost = getMemStoreTempCost( aSystemStat,
                                                        sLeftCostInfo );
                sLeftDiskTempCost = 0;
            }
            else
            {
                sLeftMemTempCost = getMemStoreTempCost( aSystemStat,
                                                        sLeftCostInfo );

                sLeftDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                          sLeftCostInfo->outputRecordCnt,
                                                          sLeftCostInfo->recordSize,
                                                          QMO_COST_STORE_SORT_TEMP );
            }
            break;

        case QMO_JOIN_LEFT_NODE_SORTING :

            if( sTempType != QMO_JOIN_METHOD_LEFT_STORAGE_DISK )
            {
                sLeftMemTempCost = getMemSortTempCost( aSystemStat,
                                                       sLeftCostInfo,
                                                       1 ); // nodeCnt
                sLeftDiskTempCost = 0;
            }
            else
            {
                sLeftMemTempCost = getMemSortTempCost( aSystemStat,
                                                       sLeftCostInfo,
                                                       1 ); // nodeCnt

                sLeftDiskTempCost = getDiskSortTempCost( aSystemStat,
                                                         sLeftCostInfo,
                                                         1, // nodeCnt
                                                         sLeftCostInfo->recordSize );
            }
            break;

        default :
            sLeftMemTempCost   = 0;
            sLeftDiskTempCost  = 0;
            IDE_DASSERT ( 0 );
    }


    sTempType = (sFlag & QMO_JOIN_METHOD_RIGHT_STORAGE_MASK);

    switch ( (sFlag & QMO_JOIN_RIGHT_NODE_MASK) )
    {
        case QMO_JOIN_RIGHT_NODE_NONE :
            sRightTempScanCost = 0;
            sRightMemTempCost  = 0;
            sRightDiskTempCost = 0;
            break;

        case QMO_JOIN_RIGHT_NODE_STORE :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );
                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemStoreTempCost( aSystemStat,
                                                         sRightCostInfo );

                sRightDiskTempCost = getDiskStoreTempCost( aSystemStat,
                                                           sRightCostInfo->outputRecordCnt,
                                                           sRightCostInfo->recordSize,
                                                           QMO_COST_STORE_SORT_TEMP );
            }
            break;

        case QMO_JOIN_RIGHT_NODE_SORTING :

            if( sTempType != QMO_JOIN_METHOD_RIGHT_STORAGE_DISK )
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt
                sRightDiskTempCost = 0;
            }
            else
            {
                sRightMemTempCost = getMemSortTempCost( aSystemStat,
                                                        sRightCostInfo,
                                                        1 ); // nodeCnt

                sRightDiskTempCost = getDiskSortTempCost( aSystemStat,
                                                          sRightCostInfo,
                                                          1, // nodeCnt
                                                          sRightCostInfo->recordSize );
            }
            break;

        default :
            sRightMemTempCost  = 0;
            sRightDiskTempCost = 0;
            IDE_DASSERT ( 0 );
    }

    //--------------------------------------
    // change index
    //--------------------------------------
    if( aJoinMethodCost->leftIdxInfo != NULL )
    {
        sLeftAccessCost = aLeftSelMethod->accessCost;
        sLeftDiskCost   = aLeftSelMethod->diskCost;
    }
    else
    {
        sLeftAccessCost = sLeftCostInfo->totalAccessCost;
        sLeftDiskCost   = sLeftCostInfo->totalDiskCost;
    }

    if( aJoinMethodCost->rightIdxInfo != NULL )
    {
        sRightAccessCost = aRightSelMethod->accessCost;
        sRightDiskCost   = aRightSelMethod->diskCost;
    }
    else
    {
        sRightAccessCost = sRightCostInfo->totalAccessCost;
        sRightDiskCost   = sRightCostInfo->totalDiskCost;
    }

    *aMemCost =  sLeftAccessCost +
                 sRightAccessCost +
                 sLeftMemTempCost +
                 sRightMemTempCost +
               ( sLeftTempScanCost + sRightTempScanCost ) +
                 aFilterCost;

    *aDiskCost = sLeftDiskCost +
                 sRightDiskCost +
                 sLeftDiskTempCost +
                 sRightDiskTempCost;

    sCost       = *aMemCost + *aDiskCost;

    return sCost;
}

/***********************************************************************
* Temp Table Cost
***********************************************************************/
SDouble qmoCost::getMemSortTempCost( qmoSystemStatistics * aSystemStat,
                                     qmgCostInfo         * aCostInfo,
                                     SDouble               aSortNodeCnt )
{
/******************************************************************************
 * Description : mem sort temp
 *               Quick sort 를 사용한다.
 *****************************************************************************/

    SDouble sCost;
    SDouble sRowCnt;

    sRowCnt = aCostInfo->outputRecordCnt;

    //  Quick sort = 2Nlog(N)
    sCost  = 2 * sRowCnt * idlOS::log(sRowCnt) *
             aSystemStat->mCompareTime * aSortNodeCnt;

    sCost += getMemStoreTempCost( aSystemStat,
                                  aCostInfo );

    return sCost;
}

SDouble qmoCost::getMemHashTempCost( qmoSystemStatistics * aSystemStat,
                                     qmgCostInfo         * aCostInfo,
                                     SDouble               aHashNodeCnt )
{
/******************************************************************************
 * Description : hash sort temp
 *****************************************************************************/

    SDouble sCost;

    // BUG-40394 hash join cost가 너무 낮습니다.
    // hash bucket 을 1/2 로 예측하기때문에 2.0 으로 변경한다.
    sCost  = aCostInfo->outputRecordCnt *
             aSystemStat->mHashTime *
             aHashNodeCnt * (2.0);

    sCost += getMemStoreTempCost( aSystemStat,
                                  aCostInfo );

    return sCost;
}

SDouble qmoCost::getMemStoreTempCost( qmoSystemStatistics * aSystemStat,
                                      qmgCostInfo         * aCostInfo )
{
/******************************************************************************
 * Description : mem store temp
 *               output * store time
 *****************************************************************************/

    SDouble sCost;

    sCost = aCostInfo->outputRecordCnt *
            aSystemStat->mStoreTime;

    return sCost;
}

SDouble qmoCost::getDiskSortTempCost( qmoSystemStatistics * aSystemStat,
                                      qmgCostInfo         * aCostInfo,
                                      SDouble               aSortNodeCnt,
                                      SDouble               aSortNodeLen )
{
/******************************************************************************
 * Description : disk sort temp
 *               여러개의 run 으로 분리후 각각의 run을 quick sort 한다.
 *               정렬된 run들을 heap sort 한다.
 *****************************************************************************/

    SDouble  sCost;
    SDouble  sRowCnt;
    SDouble  sSortCnt;
    SDouble  sRunCnt;

    sRowCnt = aCostInfo->outputRecordCnt;

    IDE_DASSERT( aSortNodeCnt > 0 );
    IDE_DASSERT( aSortNodeLen > 0 );

    // 한번에 sort 할수 있는 row의 갯수
    sSortCnt = IDL_MIN( smuProperty::getSortAreaSize()                 *
                       (smuProperty::getTempSortGroupRatio() / 100.0 ) *
                        aSortNodeLen,
                        smuProperty::getTempSortPartitionSize() );

    sRunCnt  = idlOS::ceil(aCostInfo->outputRecordCnt / sSortCnt);

    //  Quick sort = 2Nlog(N)
    sCost  = 2 * sSortCnt * idlOS::log(sSortCnt) *
             aSystemStat->mCompareTime *
             aSortNodeCnt *
             sRunCnt;

    // heap sort
    sCost += sRowCnt * idlOS::log(sRowCnt) *
             aSystemStat->mCompareTime *
             aSortNodeCnt *
             idlOS::log(sSortCnt);

    sCost += getDiskStoreTempCost( aSystemStat,
                                   aCostInfo->outputRecordCnt,
                                   aSortNodeLen,
                                   QMO_COST_STORE_SORT_TEMP );

    return sCost;
}

SDouble qmoCost::getDiskHashTempCost( qmoSystemStatistics * aSystemStat,
                                      qmgCostInfo         * aCostInfo,
                                      SDouble               aHashNodeCnt,
                                      SDouble               aHashNodeLen )
{
/******************************************************************************
 * Description : disk sort temp
 *               disk hash temp 테이블은 2가지 방식으로 동작한다.
 *               현재는 UNIQUE 방식으로 만 동작한다.
 *               향후 CLUSTER 방식을 고려해야 한다.
 *****************************************************************************/

    SDouble sCost;

    IDE_DASSERT( aHashNodeCnt > 0 );
    IDE_DASSERT( aHashNodeLen > 0 );

    sCost  = aCostInfo->outputRecordCnt *
             aSystemStat->mHashTime *
             aHashNodeCnt;

    sCost += getDiskStoreTempCost( aSystemStat,
                                   aCostInfo->outputRecordCnt,
                                   aHashNodeLen,
                                   QMO_COST_STORE_HASH_TEMP );

    return sCost;
}

SDouble qmoCost::getDiskHashGroupCost( qmoSystemStatistics * aSystemStat,
                                       qmgCostInfo         * aCostInfo,
                                       SDouble               aGroupCnt,
                                       SDouble               aHashNodeCnt,
                                       SDouble               aHashNodeLen )
{
/******************************************************************************
 * Description : disk hash temp Group by
 *               hash group by 는 다음과 같이 동작한다.
 *               1. 각각의 group 에 대해서 1개의 레코드만 저장한다.
 *               2. 동일한 group 이 있는지 확인하기 위해 fetch 를 한다.
 *                   2.1 temp 영역이 모자르게 되면 replace 가 일어나게 된다.
 *               cost = hash time +
 *                      replace time +
 *                      hash temp table store time
 *****************************************************************************/

    SDouble sCost;
    SDouble sUseAbleTempArea;
    SDouble sStoreSize;
    SDouble sReplaceCount;
    SDouble sReplaceRatio;

    IDE_DASSERT( aGroupCnt > 0 );
    IDE_DASSERT( aHashNodeCnt > 0 );
    IDE_DASSERT( aHashNodeLen > 0 );

    sCost  = aCostInfo->outputRecordCnt *
             aSystemStat->mHashTime *
             aHashNodeCnt;

    // Hash temp table 의 가용 크기를 구한다.
    sUseAbleTempArea = smuProperty::getHashAreaSize()    *
                     ( 1 - (smuProperty::getTempHashGroupRatio() / 100.0) );

    sStoreSize       = aGroupCnt * (aHashNodeLen + SMI_TR_HEADER_SIZE_FULL);

    ////////////////////////////////
    // 버퍼가 replace 되는 횟수를 구한다.
    ////////////////////////////////

    if(sStoreSize > sUseAbleTempArea)
    {
        // 동일한 group 의 레코드를 연속으로 fetch 하는경우
        // replace 가 안일어나게 된다. 다음과 같이 구한다.
        // replace 확률 =  1- (그룹 갯수 / 전체 레코드 갯수)
        sReplaceRatio = 1 - (aGroupCnt / aCostInfo->outputRecordCnt);

        sReplaceRatio = IDL_MAX( sReplaceRatio, 0 );

        // replace 가 되는 횟수는 temp 에 저장하고 남은 나머지다.
        sReplaceCount = aCostInfo->outputRecordCnt -
                        (sUseAbleTempArea / (aHashNodeLen + 24));

        sReplaceCount = IDL_MAX( sReplaceCount, 0 );

        sReplaceCount = sReplaceCount * sReplaceRatio;
    }
    else
    {
        sReplaceCount = 0;
    }

    // replace 가 발생할때마다 IO 가 일어나게 된다.
    sCost += sReplaceCount *
             aSystemStat->singleIoScanTime;

    sCost += getDiskStoreTempCost( aSystemStat,
                                   aGroupCnt,
                                   aHashNodeLen,
                                   QMO_COST_STORE_HASH_TEMP );

    return sCost;
}

SDouble qmoCost::getDiskStoreTempCost( qmoSystemStatistics * aSystemStat,
                                       SDouble               aStoreRowCnt,
                                       SDouble               aStoreNodeLen,
                                       UShort                aTempTableType )
{
/******************************************************************************
 * Description : disk store temp cost
 *               temp table 종류별로 사이즈를 계산하는 방식이 다르다.
 *****************************************************************************/

    SDouble sCost;
    SDouble sUseAbleTempArea;
    SDouble sStoreSize;
    SDouble sStoreNodeLen;

    IDE_DASSERT( aStoreNodeLen > 0 );

    switch( aTempTableType )
    {
        case QMO_COST_STORE_SORT_TEMP :
            sUseAbleTempArea = smuProperty::getSortAreaSize()    *
                               smuProperty::getTempSortGroupRatio() / 100.0;

            sStoreNodeLen    = aStoreNodeLen + SMI_TR_HEADER_SIZE_FULL;
            break;

        case QMO_COST_STORE_HASH_TEMP :
            sUseAbleTempArea = smuProperty::getHashAreaSize()    *
                               ( 1- (smuProperty::getTempHashGroupRatio() / 100.0) );

            sStoreNodeLen    = aStoreNodeLen + SMI_TR_HEADER_SIZE_FULL;
            break;

        default :
            sUseAbleTempArea = 0;
            sStoreNodeLen    = aStoreNodeLen;
            IDE_DASSERT( 0 );
    }

    sStoreSize = IDL_MAX( ( aStoreRowCnt * sStoreNodeLen) - sUseAbleTempArea, 0 );

    sCost  = aStoreRowCnt * aSystemStat->mStoreTime;

    // DISK IO cost
    // TASK-6699 TPC-H 성능 개선
    // 10 : TPC-H 100SF 의 경험적으로 설정된 값이다.
    sCost += idlOS::ceil((sStoreSize / smiGetPageSize( SMI_DISK_USER_TEMP ))) *
             aSystemStat->singleIoScanTime * 10;

    return sCost;
}

/***********************************************************************
* Filter Cost
***********************************************************************/
SDouble qmoCost::getKeyRangeCost( qmoSystemStatistics * aSystemStat,
                                  qmoStatistics       * aTableStat,
                                  qmoIdxCardInfo      * aIndexStat,
                                  qmoPredWrapper      * aKeyRange,
                                  SDouble               aKeyRangeSelectivity )
{
/******************************************************************************
 * Description : Key range cost
 *               key range 노드의 갯수만큼 cost 증가한다.
 *****************************************************************************/

    SDouble          sCost;
    qmoPredWrapper * sKeyRange;
    UInt             sKeyRangeNodeCnt = 0;
    SDouble          sLeafPageCnt;

    sLeafPageCnt = idlOS::ceil( aTableStat->totalRecordCnt /
                                aIndexStat->avgSlotCount );

    for( sKeyRange = aKeyRange;
         sKeyRange != NULL;
         sKeyRange = sKeyRange->next )
    {
        sKeyRangeNodeCnt++;
    }

    sCost = sLeafPageCnt *
            aKeyRangeSelectivity *
            aSystemStat->mMTCallTime *
            sKeyRangeNodeCnt;

    return sCost;
}

SDouble qmoCost::getKeyFilterCost( qmoSystemStatistics * aSystemStat,
                                   qmoPredWrapper      * aKeyFilter,
                                   SDouble               aLoopCnt )
{
/******************************************************************************
 * Description : Key filter cost
 *               key filter 노드의 갯수만큼 cost 증가한다.
 *****************************************************************************/

    SDouble          sCost;
    qmoPredWrapper * sKeyFilter;
    UInt             sKeyFilterNodeCnt = 0;

    for( sKeyFilter = aKeyFilter;
         sKeyFilter != NULL;
         sKeyFilter = sKeyFilter->next )
    {
        sKeyFilterNodeCnt++;
    }

    sCost = aLoopCnt * aSystemStat->mMTCallTime * sKeyFilterNodeCnt;

    return sCost;
}

UInt countFilterNode( mtcNode * aNode, SDouble * aSubQueryCost )
{
/*******************************************
 * Description : filter counter
 ********************************************/

    mtcNode * sNode  = aNode;
    mtcNode * sConvNode;
    UInt      sCount = 0;

    for( sNode   = aNode;
         sNode  != NULL;
         sNode   = sNode->next )
    {
        // Self Node
        // TPC-H 9 번 질의에서 like 연산을 많이 수행하게되면 성능이 느려지는 경우가 존재한다.
        if( (sNode->module == &mtfLike) ||
            (sNode->module == &mtfNotLike) )
        {
            sCount += 12;
        }
        else
        {
            sCount += 1;
        }

        // TASK-6699 TPC-H 성능 개선
        // QTC_NODE_CONVERSION_TRUE : 1+1 과 같은 상수연산이 연산이 완료된 경우이다.
        if( (((qtcNode*)sNode)->lflag & QTC_NODE_CONVERSION_MASK) == QTC_NODE_CONVERSION_TRUE )
        {
            continue;
        }
        else
        {
            // nothing todo.
        }

        if( (sNode->module == &(qtc::subqueryWrapperModule)) ||
            (sNode->module == &(qtc::subqueryModule)) )
        {
            IDE_DASSERT( ((qtcNode*)sNode)->subquery != NULL );

            if( ((qtcNode*)sNode)->subquery->myPlan->graph != NULL )
            {
                *aSubQueryCost += ((qtcNode*)sNode)->subquery->myPlan->graph->costInfo.totalAllCost;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            *aSubQueryCost += 0;
        }

        // Child Node
        sCount += countFilterNode( sNode->arguments, aSubQueryCost );

        // conversion Node
        for( sConvNode = sNode->conversion;
             sConvNode != NULL;
             sConvNode = sConvNode->conversion )
        {
            sCount++;
        }
    }

    return sCount;
}

SDouble qmoCost::getFilterCost4PredWrapper(
            qmoSystemStatistics * aSystemStat,
            qmoPredWrapper      * aFilter,
            SDouble               aLoopCnt )
{
/******************************************************************************
 * Description : filter cost
 *               filter 노드의 갯수만큼 비용이 증가한다.
 *               qmoPredWrapper 에 filter 가 담겨있을때 비용 계산함수
 *****************************************************************************/

    SDouble          sCost = 0;
    qmoPredWrapper * sFilter;
    qmoPredicate     sTemp;

    for( sFilter = aFilter;
         sFilter != NULL;
         sFilter = sFilter->next )
    {
        // BUG-42480 qmoPredWrapper 의 경우 qmoPredicate 의 next 를 따라가면 안된다.
        idlOS::memcpy( &sTemp, sFilter->pred, ID_SIZEOF( qmoPredicate ) );
        sTemp.next = NULL;

        sCost += getFilterCost( aSystemStat,
                                &sTemp,
                                aLoopCnt );
    }

    return sCost;
}

SDouble qmoCost::getFilterCost( qmoSystemStatistics * aSystemStat,
                                qmoPredicate        * aFilter,
                                SDouble               aLoopCnt )
{
/******************************************************************************
 * Description : filter cost
 *               filter 노드의 갯수만큼 비용이 증가한다.
 *               qmoPredicate 에 filter 가 담겨있을때 비용 계산함수
 *****************************************************************************/

    SDouble         sCost;
    SDouble         sSubQueryCost  = 0;

    qmoPredicate  * sPredicate;
    qmoPredicate  * sMorePredicate;
    mtcNode       * sNode;

    UInt            sFilterNodeCnt = 0;

    // predicate count
    for( sPredicate = aFilter;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        for( sMorePredicate = sPredicate;
             sMorePredicate != NULL;
             sMorePredicate = sMorePredicate->more )
        {
            // BUG-42480 OR, AND 일때 next 를 따라가면 안된다.
            if( ( sMorePredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                  == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                sNode = sMorePredicate->node->node.arguments;
            }
            else
            {
                sNode = (mtcNode*)sMorePredicate->node;
            }

            sFilterNodeCnt += countFilterNode( sNode, &sSubQueryCost );
        }
    }

    sCost  = aLoopCnt * aSystemStat->mMTCallTime * sFilterNodeCnt;

    // SubQuery는 1번만 수행된다고 가정한다.
    // 다양한 경우가 존재하여 제대로 예측하기가 힘들고
    // 잘못 예측한경우 매우 잘못된 plan 이 나올수 있다.
    sCost += sSubQueryCost;

    return sCost;
}

SDouble qmoCost::getTargetCost( qmoSystemStatistics * aSystemStat,
                                qmsTarget           * aTarget,
                                SDouble               aLoopCnt )
{
/******************************************************************************
 * Description : target cost
 *               target 노드의 갯수만큼 비용이 증가한다.
 *****************************************************************************/

    SDouble sCost;
    SDouble sSubQueryCost = 0;
    UInt    sNodeCnt      = 0;

    IDE_DASSERT(aTarget != NULL);

    /* BUG-40662 */
    sNodeCnt = countFilterNode( &(aTarget->targetColumn->node), &sSubQueryCost );

    sCost  = aLoopCnt * aSystemStat->mMTCallTime * sNodeCnt;

    // SubQuery는 1번만 수행된다고 가정한다.
    // 다양한 경우가 존재하여 제대로 예측하기가 힘들고
    // 잘못 예측한경우 매우 잘못된 plan 이 나올수 있다.
    sCost += sSubQueryCost;

    return sCost;
}

SDouble qmoCost::getAggrCost( qmoSystemStatistics * aSystemStat,
                              qmsAggNode          * aAggrNode,
                              SDouble               aLoopCnt )
{
    qmsAggNode * sAggr;
    SDouble      sCost;
    SDouble      sSubQueryCost = 0;
    UInt         sNodeCnt      = 0;

    for( sAggr = aAggrNode; sAggr != NULL; sAggr = sAggr->next )
    {
        sNodeCnt += countFilterNode( &(aAggrNode->aggr->node), &sSubQueryCost );
    }

    sCost = aLoopCnt * sNodeCnt * aSystemStat->mMTCallTime;

    return sCost;
}
