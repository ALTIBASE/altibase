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
 * $Id: qmoPartition.cpp 15403 2006-03-28 00:15:35Z mhjeong $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmmParseTree.h>
#include <qdParseTree.h>
#include <qmnDef.h>
#include <qmoPartition.h>
#include <qcm.h>
#include <qcmPartition.h>
#include <qmnScan.h>
#include <qcsModule.h>
#include <qcg.h>
#include <qcg.h>

extern mtdModule mtdInteger;

// 0 : 비교필요
// 1 or -1 : 1은 첫번째가 큰거, -1은 두번째가 큰거
// 2 : 비교필요없이 서로 같음
SInt qmoPartition::mCompareCondValType
[QMS_PARTKEYCONDVAL_TYPE_COUNT][QMS_PARTKEYCONDVAL_TYPE_COUNT] =
{             /* normal */ /* min */ /* default */
    /* normal  */ {   0,            1,       -1  },
    /* min     */ {  -1,            2,        1  },
    /* default */ {   1,            1,        2  }
};

IDE_RC qmoPartition::optimizeInto( qcStatement  * aStatement,
                                   qmsTableRef  * aTableRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                INTO에 대한 파티션드 테이블 최적화
 *                INTO가 있는 DML : INSERT, MOVE
 *
 *  Implementation :
 * (1) range인 경우
 *  모든 파티션에 대해 파티션 기준값을 구해 놓음
 *  partition key column에 속한 values의 값이 constant가 아닐 때까지 구함.
 *  파티션 기준값에 부합하는 파티션을 선택.
 *   values값이 전부 constant가 아니라면 모든 파티션 선택.
 *   그렇지 않다면 기준값 검색하여 파티션 선택.
 *
 * (2) list인 경우
 *  모든 파티션에 대해 파티션 기준값 구해 놓음
 *  partition key column이 하나이므로 해당 컬럼의 value가 constant여야 함.
 *  constant라면
 *   파티션 기준값에 부합하는 파티션을 선택
 *  constant가 아니라면
 *   모든 파티션 선택
 *
 * (3) hash인 경우
 *  partition key column에 속한 values의 값이 모두 constant여야 한다.
 *  모두 constant라면
 *   values에서 hash값 생성.
 *   hash값을 이용하여 hash키 생성
 *   파티션 개수로 modular연산. = partition order가 나옴.
 *   partition order에 맞는 파티션만 선택.
 *  하나라도 constant가 아니라면
 *   모든 파티션을 선택
 *
 ***********************************************************************/

    /*
    qmsPartCondValList sPartCondVal;
    UInt               sPartOrder;
    */
    qmsPartitionRef   * sPartitionRef;
    mtcTuple          * sMtcTuple    = NULL;
    UInt                sFlag        = 0;
    UShort              sColumnCount = 0;
    UShort              i            = 0;

    IDU_FIT_POINT_FATAL( "qmoPartition::optimizeInto::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );

    switch ( aTableRef->tableInfo->partitionMethod )
    {
        case QCM_PARTITION_METHOD_RANGE:
        case QCM_PARTITION_METHOD_LIST:
        case QCM_PARTITION_METHOD_HASH:
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
            {
                IDE_TEST( makePartitions( aStatement,
                                          aTableRef )
                          != IDE_SUCCESS );

                IDE_TEST( qcmPartition::validateAndLockPartitions( aStatement,
                                                                   aTableRef,
                                                                   SMI_TABLE_LOCK_IS )
                          != IDE_SUCCESS );

                /* PROJ-2464 hybrid partitioned table 지원 */
                IDE_TEST( qcmPartition::makePartitionSummary( aStatement,
                                                              aTableRef )
                          != IDE_SUCCESS );

                // insert시 partition tuple이 필요하다.
                for ( sPartitionRef  = aTableRef->partitionRef;
                      sPartitionRef != NULL;
                      sPartitionRef  = sPartitionRef->next )
                {
                    IDE_TEST( qtc::nextTable( &( sPartitionRef->table ),
                                              aStatement,
                                              sPartitionRef->partitionInfo,
                                              QCM_TABLE_TYPE_IS_DISK( sPartitionRef->partitionInfo->tableFlag ),
                                              MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                              != IDE_SUCCESS );

                    /* PROJ-2464 hybrid partitioned table 지원
                     *  Partitioned Table Tuple에서 수집한 정보를 Partition Tuple에 추가한다.
                     *  대상 : INSERT, MOVE, UPDATE
                     */
                    sColumnCount = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table].columnCount;
                    sMtcTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table]);

                    for ( i = 0; i < sColumnCount; i++ )
                    {
                        sFlag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table].columns[i].flag;

                        sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
                        sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_TARGET_MASK;
                        sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_WHERE_MASK;
                        sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_SET_MASK;

                        sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_COLUMN_MASK;
                        sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_TARGET_MASK;
                        sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_WHERE_MASK;
                        sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_SET_MASK;
                    }
                }
            }
            break;

        case QCM_PARTITION_METHOD_NONE:
            // non-partitioned table.
            // Nothing to do.
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::optimizeIntoForInsert( qcStatement  * aStatement,
                                            qmsTableRef  * aTableRef,
                                            idBool         aIsConst )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                INTO에 대한 파티션드 테이블 최적화
 *                INTO가 있는 DML : INSERT, MOVE
 *
 *  Implementation :
 * (1) range인 경우
 *  모든 파티션에 대해 파티션 기준값을 구해 놓음
 *  partition key column에 속한 values의 값이 constant가 아닐 때까지 구함.
 *  파티션 기준값에 부합하는 파티션을 선택.
 *   values값이 전부 constant가 아니라면 모든 파티션 선택.
 *   그렇지 않다면 기준값 검색하여 파티션 선택.
 *
 * (2) list인 경우
 *  모든 파티션에 대해 파티션 기준값 구해 놓음
 *  partition key column이 하나이므로 해당 컬럼의 value가 constant여야 함.
 *  constant라면
 *   파티션 기준값에 부합하는 파티션을 선택
 *  constant가 아니라면
 *   모든 파티션 선택
 *
 * (3) hash인 경우
 *  partition key column에 속한 values의 값이 모두 constant여야 한다.
 *  모두 constant라면
 *   values에서 hash값 생성.
 *   hash값을 이용하여 hash키 생성
 *   파티션 개수로 modular연산. = partition order가 나옴.
 *   partition order에 맞는 파티션만 선택.
 *  하나라도 constant가 아니라면
 *   모든 파티션을 선택
 *
 ***********************************************************************/

    /*
    qmsPartCondValList sPartCondVal;
    UInt               sPartOrder;
    */
    qmsPartitionRef   * sPartitionRef;
    mtcTuple          * sMtcTuple    = NULL;
    UInt                sFlag        = 0;
    UShort              sColumnCount = 0;
    UShort              i            = 0;

    IDU_FIT_POINT_FATAL( "qmoPartition::optimizeInto::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );

    switch ( aTableRef->tableInfo->partitionMethod )
    {
        case QCM_PARTITION_METHOD_RANGE:
        case QCM_PARTITION_METHOD_LIST:
        case QCM_PARTITION_METHOD_HASH:
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
            {
                IDE_TEST( makePartitions( aStatement,
                                          aTableRef )
                          != IDE_SUCCESS );

                IDE_TEST( qcmPartition::validateAndLockPartitions( aStatement,
                                                                   aTableRef,
                                                                   SMI_TABLE_LOCK_IS )
                          != IDE_SUCCESS );

                /* PROJ-2464 hybrid partitioned table 지원 */
                IDE_TEST( qcmPartition::makePartitionSummary( aStatement,
                                                              aTableRef )
                          != IDE_SUCCESS );

                /* BUG-47614 partition table insert시 optimize 성능 개선
                 * Hybrid partition 이나 foreigneky가 존재할 경우만 tuple을
                 * 생성한다.
                 */
                if ( ( aTableRef->partitionSummary->isHybridPartitionedTable == ID_TRUE ) ||
                     ( aIsConst == ID_TRUE ) )
                {
                    // insert시 partition tuple이 필요하다.
                    for ( sPartitionRef  = aTableRef->partitionRef;
                          sPartitionRef != NULL;
                          sPartitionRef  = sPartitionRef->next )
                    {
                        IDE_TEST( qtc::nextTable( &( sPartitionRef->table ),
                                                  aStatement,
                                                  sPartitionRef->partitionInfo,
                                                  QCM_TABLE_TYPE_IS_DISK( sPartitionRef->partitionInfo->tableFlag ),
                                                  MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
                                  != IDE_SUCCESS );

                        /* PROJ-2464 hybrid partitioned table 지원
                         *  Partitioned Table Tuple에서 수집한 정보를 Partition Tuple에 추가한다.
                         *  대상 : INSERT, MOVE, UPDATE
                         */
                        sColumnCount = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table].columnCount;
                        sMtcTuple = &(QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sPartitionRef->table]);

                        for ( i = 0; i < sColumnCount; i++ )
                        {
                            sFlag = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aTableRef->table].columns[i].flag;

                            sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_COLUMN_MASK;
                            sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_TARGET_MASK;
                            sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_WHERE_MASK;
                            sMtcTuple->columns[i].flag &= ~MTC_COLUMN_USE_SET_MASK;

                            sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_COLUMN_MASK;
                            sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_TARGET_MASK;
                            sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_WHERE_MASK;
                            sMtcTuple->columns[i].flag |= sFlag & MTC_COLUMN_USE_SET_MASK;
                        }
                    }
                }
            }
            break;

        case QCM_PARTITION_METHOD_NONE:
            // non-partitioned table.
            // Nothing to do.
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::makePartitions(
    qcStatement  * aStatement,
    qmsTableRef  * aTableRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *               tableRef에 partitionRef를 구성한다.
 *
 *  Implementation :
 *          (1) 선행 프루닝인 경우
 *              - 이미 만들어져 있으므로, 해당 파티션에 필요한
 *                정보만을 구성한다.
 *          (2) 선행 프루닝이 아닌 경우
 *              - 모든 파티션 정보를 구성한다.
 *
 ***********************************************************************/

    UInt               sPartitionCount = ID_UINT_MAX;
    UInt               sLoopCount = 0;

    smiStatement       sSmiStmt;
    smiStatement     * sSmiStmtOri; /* BUG-45994 - 컴파일러 최적화 회피 */
    UInt               sSmiStmtFlag = 0;
    volatile idBool    sIsBeginStmt;

    /* TASK-7307 DML Data Consistency in Shard */
    qmsPartitionRef * sCurrRef;
    qmsPartitionRef * sPrevRef = NULL;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmoPartition::makePartitions::__FT__" );

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_UNTOUCHABLE;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    qcg::getSmiStmt( aStatement, &sSmiStmtOri );
    qcg::setSmiStmt( aStatement, &sSmiStmt );

    sIsBeginStmt = ID_FALSE; /* BUG-45994 - 컴파일러 최적화 회피 */

    // 이미 파티션이 생성된 적이 있다면
    // partitionRef를 NULL로 세팅하여 새로 만들게 한다.
    // CNF/DNF optimization을 두번 하는 경우가 있기 때문임.
    if ( ( aTableRef->flag & QMS_TABLE_REF_PARTITION_MADE_MASK ) ==
           QMS_TABLE_REF_PARTITION_MADE_TRUE )
    {
        aTableRef->partitionRef = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-42229
    // partition list 순회도중 split이 발생하는 경우를 대비하여
    // 최신 viewSCN으로 한번 더 검사한다.
    while( aTableRef->partitionCount != sPartitionCount )
    {
        sLoopCount++;
        IDE_TEST_RAISE( sLoopCount > 1000, ERR_TABLE_PARTITION_META_COUNT );

        IDE_NOFT_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                       sSmiStmtOri,
                                       sSmiStmtFlag )
                       != IDE_SUCCESS);
        sIsBeginStmt = ID_TRUE;

        /* BUG-45994 */
        IDU_FIT_POINT_FATAL( "qmoPartition::makePartitions::__FT__::STAGE1" );

        IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                                   aTableRef->tableInfo->tableID,
                                                   & aTableRef->partitionCount )
                  != IDE_SUCCESS );

        if ( ( aTableRef->flag & QMS_TABLE_REF_PRE_PRUNING_MASK )
               == QMS_TABLE_REF_PRE_PRUNING_TRUE )
        {
            // 선행 프루닝의 경우 명시한 파티션에 대한
            // 파티션 키 조건 또는 파티션 순서를 구하면 된다.
            IDE_TEST( qcmPartition::getPrePruningPartitionRef( aStatement,
                                                               aTableRef )
                      != IDE_SUCCESS );
        }
        else
        {
            // 선행 프루닝 되어 있지 않은 경우
            // 전부 다 구한다.
            IDE_TEST( qcmPartition::getAllPartitionRef( aStatement,
                                                        aTableRef )
                      != IDE_SUCCESS );
        }

        sIsBeginStmt = ID_FALSE;
        IDE_NOFT_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS);

        IDE_NOFT_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                       sSmiStmtOri,
                                       sSmiStmtFlag )
                       != IDE_SUCCESS);
        sIsBeginStmt = ID_TRUE;

        IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                                   aTableRef->tableInfo->tableID,
                                                   & sPartitionCount )
                  != IDE_SUCCESS );

        if ( aTableRef->partitionCount == sPartitionCount )
        {
            sIsBeginStmt = ID_FALSE;
            IDE_NOFT_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

            break;
        }
        else
        {
            aTableRef->partitionRef = NULL;
        }

        sIsBeginStmt = ID_FALSE;
        IDE_NOFT_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    }/* while */

    // 파티션 생성후 파티션 생성이 되었다고 flag를 세팅.
    aTableRef->flag |= QMS_TABLE_REF_PARTITION_MADE_TRUE;

    qcg::setSmiStmt( aStatement, sSmiStmtOri );

    IDE_FT_END();

    /*
     * TASK-7307 DML Data Consistency in Shard
     *   split method가 range/list/hash 인 경우에만 prunning 하면 된다
     */
    if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) == ID_TRUE ) &&
         ( QCM_TABLE_IS_SHARD_SPLIT( aTableRef->tableInfo->mShardFlag ) == ID_TRUE ) &&
         ( aTableRef->tableInfo->mIsUsable == ID_TRUE ) )
    {
        for ( sCurrRef  = aTableRef->partitionRef;
                sCurrRef != NULL;
                sCurrRef  = sCurrRef->next )
        {
            if ( sCurrRef->partitionInfo->mIsUsable == ID_TRUE )
            {
                sPrevRef = sCurrRef;
            }
            else
            {
                if ( sPrevRef == NULL )
                {
                    aTableRef->partitionRef = sCurrRef->next;
                }
                else
                {
                    sPrevRef->next = sCurrRef->next;
                }
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TABLE_PARTITION_META_COUNT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_INTERNAL_ARG,
                                  "[qmoPartition::makePartitions] partition count wrong" ) );
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    qcg::setSmiStmt( aStatement, sSmiStmtOri );

    if ( sIsBeginStmt == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qmoPartition::getPartCondValues(
    qcStatement        * aStatement,
    qcmColumn          * aPartKeyColumns,
    qcmColumn          * aColumnList,
    qmmValueNode       * aValueList,
    qmsPartCondValList * aPartCondVal )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                values리스트에서 파티션 키 조건값에 해당하는
 *                값을 추출한다.
 *
 *  Implementation : 파티션 키 순서대로
 *                   constant가 아닐 때가지 추출할 수 있는만큼
 *                   추출한다.
 *
 ***********************************************************************/

    qcmColumn         * sCurrPartKeyColumn;
    qcmColumn         * sCurrColumn;
    qmmValueNode      * sCurrValue;
    mtcColumn         * sValueColumn      = NULL;
    void              * sCanonizedValue   = NULL;
    void              * sValue            = NULL;
    mtcNode           * sNode;
    mtcTemplate       * sTemplate;
    mtcColumn           sColumn;
    UInt                sECCSize;

    IDU_FIT_POINT_FATAL( "qmoPartition::getPartCondValues::__FT__" );

    sTemplate = &QC_SHARED_TMPLATE(aStatement)->tmplate;

    aPartCondVal->partCondValCount = 0;
    aPartCondVal->partCondValType = QMS_PARTCONDVAL_NORMAL;

    // 파티션 키 컬럼만큼 루프
    for( sCurrPartKeyColumn = aPartKeyColumns;
         sCurrPartKeyColumn != NULL;
         sCurrPartKeyColumn = sCurrPartKeyColumn->next )
    {
        // 파티션 키 순서대로 검색
        // 모든 컬럼만큼 루프
        for( sCurrColumn = aColumnList,
                 sCurrValue = aValueList;
             (sCurrColumn != NULL) && (sCurrValue != NULL);
             sCurrColumn = sCurrColumn->next,
                 sCurrValue = sCurrValue->next )
        {
            if( sCurrPartKeyColumn->basicInfo->column.id ==
                sCurrColumn->basicInfo->column.id )
            {
                sNode = &sCurrValue->value->node;

                // constant가 아닐 때까지 구한다.
                IDE_TEST_CONT( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                                                  sCurrValue->value ) == ID_FALSE,
                               non_constant );

                // column, value를 바로 뽑아올 수 있는 이유는
                // constant value이기 때문이다.
                sValueColumn = sTemplate->rows[sNode->table].columns + sNode->column;
                sValue = (void*)( (UChar*)sTemplate->rows[sNode->table].row
                                  + sValueColumn->column.offset );

                // 데이터 타입이 반드시 같아야 함.
                IDE_DASSERT( sCurrColumn->basicInfo->type.dataTypeId ==
                             sValueColumn->type.dataTypeId );

                // PROJ-2002 Column Security
                // aPartCondVal은 DML의 optimize시 partition pruning을 위한 것으로
                // compare에만 사용된다. 그러므로 value를 column의 policy에 맞춰
                // 암호화할 필요가 없고, default policy로 canonize한다.
                if ( (sCurrColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK)
                     == MTD_ENCRYPT_TYPE_TRUE )
                {
                    // value는 항상 default policy를 갖는다.
                    IDE_DASSERT( QCS_IS_DEFAULT_POLICY( sValueColumn )
                                 == ID_TRUE );

                    IDE_TEST( qcsModule::getECCSize( sCurrColumn->basicInfo->precision,
                                                     & sECCSize )
                              != IDE_SUCCESS );

                    IDE_TEST( mtc::initializeColumn( & sColumn,
                                                     sCurrColumn->basicInfo->module,
                                                     1,
                                                     sCurrColumn->basicInfo->precision,
                                                     0 )
                              != IDE_SUCCESS );

                    IDE_TEST( mtc::initializeEncryptColumn( & sColumn,
                                                            (const SChar*) "",
                                                            sCurrColumn->basicInfo->precision,
                                                            sECCSize )
                              != IDE_SUCCESS );
                }
                else
                {
                    mtc::copyColumn( & sColumn, sCurrColumn->basicInfo );
                }

                // canonize
                // canonize가 실패하는 경우는 overflow가 나는 경우.
                // 파티션 키 조건 값도 테이블에 저장될 수 있는 값이어야 한다.
                if ( ( sCurrColumn->basicInfo->module->flag & MTD_CANON_MASK )
                     == MTD_CANON_NEED )
                {
                    sCanonizedValue = sValue;

                    IDE_TEST( sCurrColumn->basicInfo->module->canonize(  & sColumn,
                                                                         & sCanonizedValue, // canonized value
                                                                         NULL,
                                                                         sValueColumn,
                                                                         sValue,            // original value
                                                                         NULL,
                                                                         sTemplate )
                              != IDE_SUCCESS );

                    sValue = sCanonizedValue;
                }
                else if ( ( sCurrColumn->basicInfo->module->flag & MTD_CANON_MASK )
                          == MTD_CANON_NEED_WITH_ALLOCATION )
                {
                    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sColumn.column.size,
                                                               (void**)& sCanonizedValue )
                              != IDE_SUCCESS );

                    IDE_TEST( sCurrColumn->basicInfo->module->canonize( & sColumn,
                                                                        & sCanonizedValue, // canonized value
                                                                        NULL,
                                                                        sValueColumn,
                                                                        sValue,            // original value
                                                                        NULL,
                                                                        sTemplate )
                              != IDE_SUCCESS );

                    sValue = sCanonizedValue;
                }
                else
                {
                    // Nothing to do.
                }

                aPartCondVal->partCondValues
                    [ aPartCondVal->partCondValCount++]
                    = sValue;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    IDE_EXCEPTION_CONT( non_constant );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::getPartOrder(
    qcmColumn          * aPartKeyColumns,
    UInt                 aPartCount,
    qmsPartCondValList * aPartCondVal,
    UInt               * aPartOrder )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                values에서 추출한 파티션 키 조건 값을 가지고
 *                hash값을 만들어서 파티션 개수로 modular
 *
 *  Implementation :
 *
 ***********************************************************************/

    qcmColumn * sCurrColumn;
    UInt        i      = 0;
    UInt        sValue = 0;

    IDU_FIT_POINT_FATAL( "qmoPartition::getPartOrder::__FT__" );

    sValue = mtc::hashInitialValue;

    for ( sCurrColumn  = aPartKeyColumns;
          sCurrColumn != NULL;
          sCurrColumn  = sCurrColumn->next )
    {
        sValue = sCurrColumn->basicInfo->module->hash( sValue,
                                                       sCurrColumn->basicInfo,
                                                       aPartCondVal->partCondValues[i++] );
    }

    *aPartOrder = sValue % aPartCount;

    return IDE_SUCCESS;
}

SInt qmoPartition::compareRangePartition(
    qcmColumn          * aKeyColumns,
    qmsPartCondValList * aComp1,
    qmsPartCondValList * aComp2 )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                RANGE PARTITION KEY CONDITION VALUE의 비교
 *
 *  Implementation :
 *
 ***********************************************************************/

    qcmColumn    * sColumn;
    mtdCompareFunc sCompare;

    SInt           sRet;
    SInt           sCompareType;
    SInt           sCompRet;
    UInt           sKeyColIter;
    mtdValueInfo   sValueInfo1;
    mtdValueInfo   sValueInfo2;

    sColumn = aKeyColumns;

    sCompareType = mCompareCondValType[aComp1->partCondValType][aComp2->partCondValType];

    sKeyColIter = 0;

    if( sCompareType == 0 )
    {
        while(1)
        {
            sCompare = sColumn->basicInfo->module->logicalCompare[MTD_COMPARE_ASCENDING];

            sValueInfo1.column = sColumn->basicInfo;
            sValueInfo1.value  = aComp1->partCondValues[sKeyColIter];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = sColumn->basicInfo;
            sValueInfo2.value  = aComp2->partCondValues[sKeyColIter];
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sCompRet = sCompare( &sValueInfo1, &sValueInfo2 );

            if( sCompRet < 0 )
            {
                sRet = -1;
                break;
            }
            else if ( sCompRet == 0 )
            {
                sKeyColIter++;
                sColumn = sColumn->next;

                if( (sKeyColIter < aComp1->partCondValCount) &&
                    (sKeyColIter < aComp2->partCondValCount) )
                {
                    continue;
                }
                else if ( (sKeyColIter == aComp1->partCondValCount) &&
                          (sKeyColIter == aComp2->partCondValCount) )
                {
                    sRet = 0;
                    break;
                }
                else if ( sKeyColIter == aComp1->partCondValCount )
                {
                    sRet = -1;
                    break;
                }
                else
                {
                    sRet = 1;
                    break;
                }
            }
            else
            {
                sRet = 1;
                break;
            }
        }
    }
    else
    {
        // 2는 비교할거없이 서로 같은 경우
        if( sCompareType == 2 )
        {
            sRet = 0;
        }
        else
        {
            // -1 또는 1, 비교할거없이 서로 크거나 작거나 한 경우
            // 그대로 comparetype을 대응시킴.
            sRet = sCompareType;
        }
    }

    return sRet;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::compareRangeUsingHashPartition( qmsPartCondValList * aComp1,
                                                   qmsPartCondValList * aComp2 )
{
    mtdCompareFunc sCompare;
    SInt           sRet;
    SInt           sCompareType;
    SInt           sCompRet;
    UInt           sKeyColIter;
    mtdValueInfo   sValueInfo1;
    mtdValueInfo   sValueInfo2;

    sCompareType = mCompareCondValType[aComp1->partCondValType][aComp2->partCondValType];

    sKeyColIter = 0;

    if ( sCompareType == 0 )
    {
        while ( 1 )
        {
            sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];

            sValueInfo1.column = NULL;
            sValueInfo1.value  = aComp1->partCondValues[sKeyColIter];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = aComp2->partCondValues[sKeyColIter];
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sCompRet = sCompare( &sValueInfo1, &sValueInfo2 );

            if ( sCompRet < 0 )
            {
                sRet = -1;
                break;
            }
            else if ( sCompRet == 0 )
            {
                sKeyColIter++;

                if ( ( sKeyColIter < aComp1->partCondValCount ) &&
                     ( sKeyColIter < aComp2->partCondValCount ) )
                {
                    continue;
                }
                else if ( ( sKeyColIter == aComp1->partCondValCount ) &&
                          ( sKeyColIter == aComp2->partCondValCount ) )
                {
                    sRet = 0;
                    break;
                }
                else if ( sKeyColIter == aComp1->partCondValCount )
                {
                    sRet = -1;
                    break;
                }
                else
                {
                    sRet = 1;
                    break;
                }
            }
            else
            {
                sRet = 1;
                break;
            }
        }
    }
    else
    {
        // 2는 비교할거없이 서로 같은 경우
        if ( sCompareType == 2 )
        {
            sRet = 0;
        }
        else
        {
            // -1 또는 1, 비교할거없이 서로 크거나 작거나 한 경우
            // 그대로 comparetype을 대응시킴.
            sRet = sCompareType;
        }
    }

    return sRet;
}

idBool qmoPartition::compareListPartition(
    qcmColumn          * aKeyColumn,
    qmsPartCondValList * aPartKeyCondVal,
    void               * aValue )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                list partition key 비교.
 *                list partition key condition value와
 *                하나의 value와 비교.
 *
 *  Implementation :
 *             (1) default partition인 경우는 무조건 미포함으로
 *                 결과가 나온다.
 *                 - 만약 모든 파티션이 포함이 안되면
 *                   default partition을 선택하도록 상위코드에서
 *                   처리한다.
 *             (2) 포함이면 ID_TRUE반환, 미포함이면 ID_FALSE반환.
 *
 ***********************************************************************/

    mtdCompareFunc sCompare;
    idBool         sMatch;
    UInt           i;
    mtdValueInfo   sValueInfo1;
    mtdValueInfo   sValueInfo2;    

    if( aPartKeyCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
    {
        sMatch = ID_TRUE;
    }
    else
    {
        sMatch = ID_FALSE;
    }

    sCompare = aKeyColumn->basicInfo->module->logicalCompare[MTD_COMPARE_ASCENDING];

    for( i = 0; i < aPartKeyCondVal->partCondValCount; i++ )
    {
        sValueInfo1.column = aKeyColumn->basicInfo;
        sValueInfo1.value  = aPartKeyCondVal->partCondValues[i];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = aKeyColumn->basicInfo;
        sValueInfo2.value  = aValue;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;
        
        if( sCompare( &sValueInfo1, &sValueInfo2 ) == 0 )
        {
            if( aPartKeyCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
            {
                sMatch = ID_FALSE;
            }
            else
            {
                sMatch = ID_TRUE;
            }
            break;
        }
    }

    return sMatch;
}

IDE_RC qmoPartition::getPartCondValuesFromRow(
    qcmColumn          * aKeyColumns,
    smiValue           * aValues,
    qmsPartCondValList * aPartCondVal )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                insert할때의 smiValue형태로 된 row를 이용하여
 *                partition key condition형태로 변환한다.
 *  Implementation :
 *
 ***********************************************************************/

    qcmColumn * sColumn;
    SInt        sColumnOrder = 0;
    void      * sMtdValue;

    IDU_FIT_POINT_FATAL( "qmoPartition::getPartCondValuesFromRow::__FT__" );

    aPartCondVal->partCondValCount = 0;
    aPartCondVal->partCondValType = QMS_PARTCONDVAL_NORMAL;

    for ( sColumn  = aKeyColumns;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        sColumnOrder = sColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

        if ( ( aValues[sColumnOrder].value == NULL ) &&
             ( aValues[sColumnOrder].length == 0 ) )
        {
            aPartCondVal->partCondValues[aPartCondVal->partCondValCount++]
                = sColumn->basicInfo->module->staticNull;
        }
        else
        {
            // PROJ-1705
            // smiValue->value는
            // 원래 row의 mtdValueType의 value를 sm에 저장할 value로 가리키므로,
            // 원래 row의 mtdValueType의 value 포인터로 CondValue를 얻어야 한다.
            // ( qdbCommon::storingValue2MtdValue() 주석 그림 참조. )

            IDE_TEST( qdbCommon::storingValue2MtdValue( sColumn->basicInfo,
                                                        (void*)( aValues[sColumnOrder].value ),
                                                        & sMtdValue )
                      != IDE_SUCCESS );

            aPartCondVal->partCondValues[aPartCondVal->partCondValCount++] = sMtdValue;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::partitionFilteringWithRow(
    qmsTableRef      * aTableRef,
    smiValue         * aValues,
    qmsPartitionRef ** aSelectedPartitionRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmsPartCondValList sPartCondVal;
    qmsPartCondValList sPartCondVal2;
    UInt               sPartOrder;

    IDU_FIT_POINT_FATAL( "qmoPartition::partitionFilteringWithRow::__FT__" );

    *aSelectedPartitionRef = NULL;

    switch ( aTableRef->tableInfo->partitionMethod )
    {
        case QCM_PARTITION_METHOD_RANGE:
            {
                // smiValue에서 partition key에 해당하는 것만 빼냄
                // range partition filtering
                // filtering의 결과는 오직 하나만 나와야 함.
                // 안나오는 경우는 에러(no match)
                IDE_TEST( getPartCondValuesFromRow( aTableRef->tableInfo->partKeyColumns,
                                                    aValues,
                                                    & sPartCondVal )
                          != IDE_SUCCESS );

                IDE_TEST( rangePartitionFilteringWithValues( aTableRef,
                                                             & sPartCondVal,
                                                             aSelectedPartitionRef )
                          != IDE_SUCCESS );
            }
            break;

        case QCM_PARTITION_METHOD_LIST:
        {
            // smiValue에서 partition key에 해당하는 것만 빼냄
            // list partition filtering
            // filtering의 결과는 오직 하나만 나와야 함.
            // 안나오는 경우는 에러(no match)
            IDE_TEST( getPartCondValuesFromRow( aTableRef->tableInfo->partKeyColumns,
                                                aValues,
                                                & sPartCondVal )
                      != IDE_SUCCESS );

            IDE_DASSERT( sPartCondVal.partCondValCount == 1 );

            IDE_TEST( listPartitionFilteringWithValue( aTableRef,
                                                       sPartCondVal.partCondValues[0],
                                                       aSelectedPartitionRef )
                      != IDE_SUCCESS );
        }
        break;

        case QCM_PARTITION_METHOD_HASH:
        {
            // smiValue에서 partition key에 해당하는 것만 빼냄
            // partition order를 구함.
            // 해당 partitoin order가 partitionRef에 있는지 검사.
            // 있으면 있는거 빼내서 올리고, 없으면 에러(no match)
            IDE_TEST( getPartCondValuesFromRow(  aTableRef->tableInfo->partKeyColumns,
                                                 aValues,
                                                 & sPartCondVal )
                      != IDE_SUCCESS );

            IDE_TEST( getPartOrder( aTableRef->tableInfo->partKeyColumns,
                                    aTableRef->partitionCount,
                                    & sPartCondVal,
                                    & sPartOrder )
                      != IDE_SUCCESS );

            IDE_TEST( hashPartitionFilteringWithPartOrder( aTableRef,
                                                           sPartOrder,
                                                           aSelectedPartitionRef )
                      != IDE_SUCCESS );
        }
        break;
        /* BUG-46065 support range using hash */
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
        {
            IDE_TEST( getPartCondValuesFromRow( aTableRef->tableInfo->partKeyColumns,
                                                aValues,
                                                &sPartCondVal )
                      != IDE_SUCCESS );

            IDE_TEST( getPartOrder( aTableRef->tableInfo->partKeyColumns,
                                    QMO_RANGE_USING_HASH_MAX_VALUE,
                                    & sPartCondVal,
                                    & sPartOrder )
                      != IDE_SUCCESS );
            sPartCondVal2.partCondValCount = 1;
            sPartCondVal2.partCondValType  = QMS_PARTCONDVAL_NORMAL;
            sPartCondVal2.partCondValues[0] = &sPartOrder;

            IDE_TEST( rangeUsingHashPartitionFilteringWithValues( aTableRef,
                                                                  &sPartCondVal2,
                                                                  aSelectedPartitionRef )
                      != IDE_SUCCESS );
        }
        break;
        case QCM_PARTITION_METHOD_NONE:
            // non-partitioned table.
            // Nothing to do.
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_TEST_RAISE( *aSelectedPartitionRef == NULL,
                    ERR_NO_MATCH_PARTITION );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_MATCH_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_PARTITION_KEY_INSERT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::rangePartitionFilteringWithValues(
    qmsTableRef        * aTableRef,
    qmsPartCondValList * aPartCondVal,
    qmsPartitionRef   ** aSelectedPartitionRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                range partition filtering with values
 *                into절에 쓰이는 것에 대한 처리
 *
 *  Implementation : 값이 포함되는 하나의 파티션을 찾아서 리턴.
 *                   못찾으면 에러.
 *
 ***********************************************************************/

    qmsPartitionRef * sCurrRef;
    SInt              sRet;

    IDU_FIT_POINT_FATAL( "qmoPartition::rangePartitionFilteringWithValues::__FT__" );

    *aSelectedPartitionRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sRet = compareRangePartition(
            aTableRef->tableInfo->partKeyColumns,
            &sCurrRef->maxPartCondVal,
            aPartCondVal );

        if ( sRet == 1 )
        {
            // max partition key보다 작은 경우.
            sRet = compareRangePartition(
                aTableRef->tableInfo->partKeyColumns,
                &sCurrRef->minPartCondVal,
                aPartCondVal );

            if ( (sRet == 0) || (sRet == -1) )
            {
                // min partition key와 같거나 큰 경우
                *aSelectedPartitionRef = sCurrRef;

                // TODO1502: 디버깅용 코드임.
//                ideLog::log( IDE_QP_0,
//                             "INSERT RANGE PARTITION FILTERING, tableID: %d,partitionID: %d\n",
//                             aTableRef->tableInfo->tableID,
//                             sCurrRef->partitionID );
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( *aSelectedPartitionRef == NULL,
                    ERR_NO_MATCH_PARTITION );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_MATCH_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_PARTITION_KEY_INSERT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::rangeUsingHashPartitionFilteringWithValues( qmsTableRef        * aTableRef,
                                                                 qmsPartCondValList * aPartCondVal,
                                                                 qmsPartitionRef   ** aSelectedPartitionRef )
{
    qmsPartitionRef * sCurrRef;
    SInt              sRet;

    *aSelectedPartitionRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sRet = compareRangeUsingHashPartition( &sCurrRef->maxPartCondVal,
                                               aPartCondVal );

        if ( sRet == 1 )
        {
            // max partition key보다 작은 경우.
            sRet = compareRangeUsingHashPartition( &sCurrRef->minPartCondVal,
                                                   aPartCondVal );

            if ( (sRet == 0) || (sRet == -1) )
            {
                // min partition key와 같거나 큰 경우
                *aSelectedPartitionRef = sCurrRef;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( *aSelectedPartitionRef == NULL,
                    ERR_NO_MATCH_PARTITION );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_MATCH_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_PARTITION_KEY_INSERT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::listPartitionFilteringWithValue(
    qmsTableRef        * aTableRef,
    void               * aValue,
    qmsPartitionRef   ** aSelectedPartitionRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                하나의 값에 대한 list partition filtering
 *  Implementation :
 *
 ***********************************************************************/

    // TODO1502: default partition에 대해 고려가 필요함.
    // default partition에 포함되는지 안포함되는지 보려면
    // 다른 파티션 키가 존재해야 한다!

    qmsPartitionRef * sCurrRef;
    idBool            sRet;

    IDU_FIT_POINT_FATAL( "qmoPartition::listPartitionFilteringWithValue::__FT__" );

    *aSelectedPartitionRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sRet = compareListPartition(
            aTableRef->tableInfo->partKeyColumns,
            &sCurrRef->maxPartCondVal,
            aValue );

        if ( sRet == ID_TRUE )
        {
            *aSelectedPartitionRef = sCurrRef;
            // TODO1502: 디버깅용 코드임.
//            ideLog::log( IDE_QP_0,
//                         "INSERT LIST PARTITION FILTERING, tableID: %d,partitionID: %d\n",
//                         aTableRef->tableInfo->tableID,
//                         sCurrRef->partitionID );
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    *aSelectedPartitionRef = sCurrRef;

    IDE_TEST_RAISE( *aSelectedPartitionRef == NULL,
                    ERR_NO_MATCH_PARTITION );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_MATCH_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_PARTITION_KEY_INSERT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoPartition::hashPartitionFilteringWithPartOrder(
    qmsTableRef      * aTableRef,
    UInt               aPartOrder,
    qmsPartitionRef ** aSelectedPartitionRef )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                hash partition filtering은
 *                partition order로 해당하는 파티션을 선택
 *
 *  Implementation :
 *
 ***********************************************************************/

    qmsPartitionRef * sCurrRef;

    IDU_FIT_POINT_FATAL( "qmoPartition::hashPartitionFilteringWithPartOrder::__FT__" );

    *aSelectedPartitionRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        if ( sCurrRef->partOrder == aPartOrder )
        {
            *aSelectedPartitionRef = sCurrRef;

//            ideLog::log( IDE_QP_0,
//                         "INSERT HASH PARTITION FILTERING, tableID: %d,partitionID: %d\n",
//                         aTableRef->tableInfo->tableID,
//                         sCurrRef->partitionID );
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    IDE_TEST_RAISE( *aSelectedPartitionRef == NULL,
                    ERR_NO_MATCH_PARTITION );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_MATCH_PARTITION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_PARTITION_KEY_INSERT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool
qmoPartition::isIntersectRangePartition(
    qmsPartCondValList * aMinPartCondVal,
    qmsPartCondValList * aMaxPartCondVal,
    smiRange           * aPartKeyRange )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                partition의 min, max condition과 keyrange와 비교한다.
 *                range와 intersect하면 ID_TRUE, 아니면 ID_FALSE를 리턴.
 *
 *  Implementation :
 *                min condition보다 range의 max가 작으면 not intersect.
 *                max contition보다 range의 min이 크거나 같으면 not intersect.
 *                나머지 경우는 intersect.
 *
 ***********************************************************************/

    mtkRangeCallBack * sData;
    UInt               i;
    SInt               sOrder;
    idBool             sIsIntersect = ID_FALSE;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    // min condition check.QMS_PARTCONDVAL_MIN
    // min condition이 원래 partitioned table의 min인 경우 무조건 포함.
    if( aMinPartCondVal->partCondValType == QMS_PARTCONDVAL_MIN )
    {
        sIsIntersect = ID_TRUE;
    }
    else
    {
        // min partcondval은 default일 수 없음.
        IDE_DASSERT( aMinPartCondVal->partCondValType !=
                     QMS_PARTCONDVAL_DEFAULT );

        // range로 오는 것: LE, LT
        // partcondval의 결과와 range maximum과의 결과는
        // LE인 경우 : range maximum이 partcondval보다 작아야함
        // LT인 경우 : range maximum이 partcondval보다 작거나 같아야함.

        sData = (mtkRangeCallBack*)aPartKeyRange->maximum.data;
        i = 0;

        while(1)
        {
            // key range compare에서는 fixed compare를 호출하므로
            // offset을 0으로 변경한다.
            sData->columnDesc.column.offset = 0;
            
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aMinPartCondVal->partCondValues[i];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1, &sValueInfo2 );

            if (sOrder == 0)
            {
                i++;
                sData = sData->next;

                if( ( i < aMinPartCondVal->partCondValCount ) &&
                    sData != NULL )
                {
                    continue;
                }
                else if ( ( i == aMinPartCondVal->partCondValCount ) &&
                          sData == NULL )
                {
                    break;
                }
                else if ( i == aMinPartCondVal->partCondValCount )
                {
                    // ex)
                    // partition 범위min : 10
                    // i1 = 10 and i2 < 10
                    // i1 = 10 and i2 <= 10
                    // 뒤에 컬럼이 하나 더 있음으로 해서 무조건
                    // keyrange의 max가 크다.

                    sOrder = -1;

                    break;
                }
                else
                {
                    // ex)
                    // partition 범위min : 10, 10
                    // i1 < 10   .... (1)
                    // i1 <= 10  .... (2)
                    // 1의 경우는 partition min이 크다.
                    // 하지만, 2의 경우는 keyrange의 max가 크다.
                    if( ( aPartKeyRange->maximum.callback
                          == mtk::rangeCallBackLT4Mtd )
                        ||
                        ( aPartKeyRange->maximum.callback
                          == mtk::rangeCallBackLT4Stored ) )
                    {
                        sOrder = 1;
                    }
                    else
                    {
                        sOrder = -1;
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if( ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLT4Mtd )
            ||
            ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLT4Stored ))
        {
            if( sOrder >= 0 )
            {
                sIsIntersect = ID_FALSE;
            }
            else
            {
                sIsIntersect = ID_TRUE;
            }
        }
        else
        {
            if( sOrder > 0 )
            {
                sIsIntersect = ID_FALSE;
            }
            else
            {
                sIsIntersect = ID_TRUE;
            }
        }
    }

    if( sIsIntersect == ID_TRUE )
    {
        // 위에서 체크했는데 intersect인 경우
        // max condition하고 range minimum하고 체크해봐야 한다.

        // max condition check.
        // max condition이 원래 partitioned table의 max인 경우 무조건 포함.
        if( aMaxPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
        {
            sIsIntersect = ID_TRUE;
        }
        else
        {
            // max partcondval은 min일 수 없음.
            IDE_DASSERT( aMaxPartCondVal->partCondValType
                         != QMS_PARTCONDVAL_MIN );

            // range로 오는 것 : GE, GT
            // partcondval의 결과와 range minimum과의 결과는
            // GT, GE인 경우 : range minimum이 partcondval보다 같거나 커야 함.

            i = 0;
            sData  = (mtkRangeCallBack*)aPartKeyRange->minimum.data;

            while(1)
            {
                // key range compare에서는 fixed compare를 호출하므로
                // offset을 0으로 변경한다.
                sData->columnDesc.column.offset = 0;
            
                sValueInfo1.column = &(sData->columnDesc);
                sValueInfo1.value  = aMaxPartCondVal->partCondValues[i];
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = &(sData->valueDesc);
                sValueInfo2.value  = sData->value;
                sValueInfo2.flag   = MTD_OFFSET_USE;

                sOrder = sData->compare( &sValueInfo1, &sValueInfo2 );

                if (sOrder == 0)
                {
                    i++;
                    sData = sData->next;

                    if( ( i < aMaxPartCondVal->partCondValCount ) &&
                        sData != NULL )
                    {
                        continue;
                    }
                    else if ( ( i == aMaxPartCondVal->partCondValCount ) &&
                              sData == NULL )
                    {
                        break;
                    }
                    else if ( i == aMaxPartCondVal->partCondValCount )
                    {
                        // ex)
                        // partition 범위min : 10
                        // i1 = 10 and i2 > 10
                        // i1 = 10 and i2 >= 10
                        // 뒤에 컬럼이 하나 더 있음으로 해서 무조건
                        // keyrange의 min이 크다.

                        sOrder = -1;
                        break;
                    }
                    else
                    {
                        // ex)
                        // partition 범위min : 10, 10
                        // i1 > 10   .... (1)
                        // i1 >= 10  .... (2)
                        // 1의 경우는 keyrange의 max가 더 크다.
                        // 하지만, 2의 경우는 keyrange의 min이 크다.
                        if( ( aPartKeyRange->minimum.callback ==
                              mtk::rangeCallBackGT4Mtd )
                            ||
                            ( aPartKeyRange->minimum.callback ==
                              mtk::rangeCallBackGT4Stored ) )
                        {
                            sOrder = -1;
                        }
                        else
                        {
                            sOrder = 1;
                        }
                        break;
                    }
                }
                else
                {
                    break;
                }
            }

            if( sOrder <= 0 )
            {
                sIsIntersect = ID_FALSE;
            }
            else
            {
                sIsIntersect = ID_TRUE;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return sIsIntersect;
}

idBool
qmoPartition::isIntersectListPartition(
    qmsPartCondValList * aPartCondVal,
    smiRange           * aPartKeyRange )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                partition condition과 keyrange와 비교한다.
 *                range와 intersect하면 ID_TRUE, 아니면 ID_FALSE를 리턴.
 *
 *  Implementation :
 *                list partition key column은 하나이고, 값이 여러개임.
 *                partition condition과 keyrange중 하나라도 일치하는 것이
 *                있으면 intersect. 그렇지 않으면 not intersect.
 *                단, default는 다른 파티션 조건의 반대이므로,
 *                not(intersect값)을 반환한다.
 *                equal, is null range만 오기 때문에, GE, LE callback만이
 *                존재한다. 따라서, 둘중 하나만 일치하면 intersect.
 *
 ***********************************************************************/

    mtkRangeCallBack * sMinData;
    UInt               i;
    SInt               sOrder;
    idBool             sIsIntersect = ID_FALSE;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    // 적합성 검사.
    // list partition column은 하나임.
    // keyrange는 composite가 아님.
    // Greater Equal, Less Equal의 조합만 가능.
    IDE_DASSERT( ((mtkRangeCallBack*)aPartKeyRange->minimum.data)->next == NULL );
    IDE_DASSERT( ((mtkRangeCallBack*)aPartKeyRange->maximum.data)->next == NULL );

    sMinData = (mtkRangeCallBack*)aPartKeyRange->minimum.data;

    for( i = 0; i < aPartCondVal->partCondValCount; i++ )
    {
        // key range compare에서는 fixed compare를 호출하므로
        // offset을 0으로 변경한다.
        sMinData->columnDesc.column.offset = 0;
            
        sValueInfo1.column = &(sMinData->columnDesc);
        sValueInfo1.value  = aPartCondVal->partCondValues[i];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = &(sMinData->valueDesc);
        sValueInfo2.value  = sMinData->value;
        sValueInfo2.flag   = MTD_OFFSET_USE;

        sOrder = sMinData->compare( &sValueInfo1, &sValueInfo2 );

        if( sOrder == 0 )
        {
            sIsIntersect = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if( aPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
    {
        // default partition인 경우는 intersect값을 뒤집는다.
        if( sIsIntersect == ID_TRUE )
        {
            sIsIntersect = ID_FALSE;
        }
        else
        {
            sIsIntersect = ID_TRUE;
        }
    }
    else
    {
        // Nothing to do.
    }

    return sIsIntersect;
}

IDE_RC
qmoPartition::makeHashKeyFromPartKeyRange(
    qcStatement   * aStatement,
    iduVarMemList * aMemory,
    smiRange      * aPartKeyRange,
    idBool        * aIsValidHashVal )
{
/***********************************************************************
 *
 *  Description : smiRange로부터 hash key를 얻어낸다.
 *                생성한 hash key는 smiRange의 mHashVal에 세팅한다.
 *
 *  Implementation : is null의 경우 minimum condition에 column정보가 없다.
 *                   따라서, is null은 따로 해당 모듈의 staticNull을
 *                   이용해야 한다.
 *
 ***********************************************************************/

    void               * sSourceValue;
    UInt                 sTargetArgCount;
    UInt                 sHashVal = mtc::hashInitialValue;
    mtkRangeCallBack   * sData;
    mtvConvert         * sConvert;

    mtcColumn          * sDestColumn;
    void               * sDestValue;
    void               * sDestCanonizedValue;
    UInt                 sErrorCode;

    volatile iduVarMemListStatus sMemStatus;
    volatile UInt                sStage;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmoPartition::makeHashKeyFromPartKeyRange::__FT__" );

    sStage = 0; /* BUG-45994 - 컴파일러 최적화 회피 */

    // 적합성 검사.
    // keyrange는 composite가 아님.
    // Greater Equal, Less Equal의 조합만 가능.

    if((( aPartKeyRange->minimum.callback == mtk::rangeCallBackGE4Mtd )
        ||
        ( aPartKeyRange->minimum.callback == mtk::rangeCallBackGE4Stored ) )
       &&
       ( ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLE4Mtd )
         ||
         ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLE4Stored )))
    {
        *aIsValidHashVal = ID_TRUE;

        for( sData  =(mtkRangeCallBack*)aPartKeyRange->minimum.data;
             sData != NULL;
             sData  = sData->next )
        {
            IDE_TEST( aMemory-> getStatus( (iduVarMemListStatus *)&sMemStatus ) != IDE_SUCCESS );
            sStage = 1;

            /* BUG-45994 */
            IDU_FIT_POINT_FATAL( "qmoPartition::makeHashKeyFromPartKeyRange::__FT__::STAGE1" );

            if( ( sData->compare == mtk::compareMaximumLimit4Mtd ) ||
                ( sData->compare == mtk::compareMaximumLimit4Stored ) )
            {
                sDestValue = sData->columnDesc.module->staticNull;
                // is null인 경우임.
            }
            else
            {
                if( sData->columnDesc.type.dataTypeId !=
                    sData->valueDesc.type.dataTypeId )
                {
                    // PROJ-2002 Column Security
                    // echar, evarchar는 동일 group이 아니므로 들어올 수 없다.
                    IDE_DASSERT(
                        (sData->columnDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                        (sData->columnDesc.type.dataTypeId != MTD_EVARCHAR_ID) &&
                        (sData->valueDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                        (sData->valueDesc.type.dataTypeId != MTD_EVARCHAR_ID) );

                    sSourceValue = (void*)mtc::value( &(sData->valueDesc),
                                                      sData->value,
                                                      MTD_OFFSET_USE );

                    sTargetArgCount = sData->columnDesc.flag &
                        MTC_COLUMN_ARGUMENT_COUNT_MASK;

                    IDE_TEST( mtv::estimateConvert4Server( aMemory,
                                                           & sConvert,
                                                           sData->columnDesc.type,
                                                           sData->valueDesc.type,
                                                           sTargetArgCount,
                                                           sData->columnDesc.precision,
                                                           sData->valueDesc.scale,
                                                           & QC_SHARED_TMPLATE( aStatement )->tmplate )
                              != IDE_SUCCESS );

                    // source value pointer
                    sConvert->stack[sConvert->count].value = sSourceValue;

                    sDestColumn = sConvert->stack[0].column;
                    sDestValue  = sConvert->stack[0].value;

                    IDE_TEST( mtv::executeConvert( sConvert,
                                                   & QC_SHARED_TMPLATE( aStatement )->tmplate)
                              != IDE_SUCCESS);

                    if ( ( sData->columnDesc.module->flag & MTD_CANON_MASK )
                         == MTD_CANON_NEED )
                    {
                        sDestCanonizedValue = sDestValue;

                        IDE_TEST( sData->columnDesc.module->canonize( &( sData->columnDesc ),
                                                                      & sDestCanonizedValue,
                                                                      NULL,
                                                                      sDestColumn,
                                                                      sDestValue,
                                                                      NULL,
                                                                      & QC_SHARED_TMPLATE( aStatement )->tmplate )
                                  != IDE_SUCCESS );

                        sDestValue = sDestCanonizedValue;
                    }
                    else if ( ( sData->columnDesc.module->flag & MTD_CANON_MASK )
                              == MTD_CANON_NEED_WITH_ALLOCATION )
                    {
                        IDE_TEST( aMemory->alloc( sData->columnDesc.column.size,
                                                  (void**)& sDestCanonizedValue )
                                  != IDE_SUCCESS );

                        IDE_TEST( sData->columnDesc.module->canonize( &( sData->columnDesc ),
                                                                      & sDestCanonizedValue,
                                                                      NULL,
                                                                      sDestColumn,
                                                                      sDestValue,
                                                                      NULL,
                                                                      & QC_SHARED_TMPLATE( aStatement )->tmplate )
                                  != IDE_SUCCESS );

                        sDestValue = sDestCanonizedValue;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    sDestValue = (void*)mtc::value( &(sData->valueDesc),
                                                    sData->value,
                                                    MTD_OFFSET_USE );
                }
            } // end if

            sHashVal = sData->columnDesc.module->hash( sHashVal,
                                                       &(sData->columnDesc),
                                                       sDestValue );

            sStage = 0;
            IDE_TEST( aMemory-> setStatus( (iduVarMemListStatus *)&sMemStatus ) != IDE_SUCCESS );
        } // end for
    }
    else
    {
        *aIsValidHashVal = ID_FALSE;
    }

    aPartKeyRange->minimum.mHashVal = sHashVal;

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    if ( sStage == 1 )
    {
        (void)aMemory->setStatus( (iduVarMemListStatus *)&sMemStatus );
    }

    *aIsValidHashVal = ID_FALSE;

    /* PROJ-2617 [안정성] QP - PVO에서의 안정성 향상
     *  - 다중 반환구문을 지닌 경우로, 여러 반환구문 상위에서 IDE_FT_EXCEPTION_END를 호출한다.
     */
    IDE_FT_EXCEPTION_END();

    sErrorCode = ideGetErrorCode();

    // overflow 에러에 대해서는 무시한다.
    if( sErrorCode == mtERR_ABORT_VALUE_OVERFLOW )
    {
        IDE_CLEAR();

        return IDE_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC
qmoPartition::makeHashKeyFromPartKeyRange(
    qcStatement * aStatement,
    iduMemory   * aMemory,
    smiRange    * aPartKeyRange,
    idBool      * aIsValidHashVal )
{
/***********************************************************************
 *
 *  Description : smiRange로부터 hash key를 얻어낸다.
 *                생성한 hash key는 smiRange의 mHashVal에 세팅한다.
 *
 *  Implementation : is null의 경우 minimum condition에 column정보가 없다.
 *                   따라서, is null은 따로 해당 모듈의 staticNull을
 *                   이용해야 한다.
 *
 ***********************************************************************/

    void               * sSourceValue;
    UInt                 sTargetArgCount;
    UInt                 sHashVal = mtc::hashInitialValue;
    mtkRangeCallBack   * sData;
    iduMemoryStatus      sMemStatus;
    UInt                 sStage = 0;
    mtvConvert         * sConvert;

    mtcColumn          * sDestColumn;
    void               * sDestValue;
    void               * sDestCanonizedValue;
    UInt                 sErrorCode;

    // 적합성 검사.
    // keyrange는 composite가 아님.
    // Greater Equal, Less Equal의 조합만 가능.

    if((( aPartKeyRange->minimum.callback == mtk::rangeCallBackGE4Mtd )
        ||
        ( aPartKeyRange->minimum.callback == mtk::rangeCallBackGE4Stored ) )
       &&
       ( ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLE4Mtd )
         ||
         ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLE4Stored )))
    {
        *aIsValidHashVal = ID_TRUE;

        for( sData  = (mtkRangeCallBack*)aPartKeyRange->minimum.data;
             sData != NULL;
             sData  = sData->next )
        {
            IDE_TEST( aMemory-> getStatus( & sMemStatus ) != IDE_SUCCESS );
            sStage = 1;

            if( ( sData->compare == mtk::compareMaximumLimit4Mtd ) ||
                ( sData->compare == mtk::compareMaximumLimit4Stored ) )
            {
                sDestValue = sData->columnDesc.module->staticNull;
                // is null인 경우임.
            }
            else
            {
                if( sData->columnDesc.type.dataTypeId !=
                    sData->valueDesc.type.dataTypeId )
                {
                    // PROJ-2002 Column Security
                    // echar, evarchar는 동일 group이 아니므로 들어올 수 없다.
                    IDE_DASSERT(
                        (sData->columnDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                        (sData->columnDesc.type.dataTypeId != MTD_EVARCHAR_ID) &&
                        (sData->valueDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                        (sData->valueDesc.type.dataTypeId != MTD_EVARCHAR_ID) );

                    sSourceValue = (void*)mtc::value( &(sData->valueDesc),
                                                      sData->value,
                                                      MTD_OFFSET_USE );

                    sTargetArgCount = sData->columnDesc.flag &
                        MTC_COLUMN_ARGUMENT_COUNT_MASK;

                    IDE_TEST( mtv::estimateConvert4Server( aMemory,
                                                           & sConvert,
                                                           sData->columnDesc.type,
                                                           sData->valueDesc.type,
                                                           sTargetArgCount,
                                                           sData->columnDesc.precision,
                                                           sData->valueDesc.scale,
                                                           & QC_PRIVATE_TMPLATE( aStatement )->tmplate )
                              != IDE_SUCCESS );

                    // source value pointer
                    sConvert->stack[sConvert->count].value = sSourceValue;

                    sDestColumn = sConvert->stack[0].column;
                    sDestValue  = sConvert->stack[0].value;

                    IDE_TEST( mtv::executeConvert( sConvert,
                                                   & QC_PRIVATE_TMPLATE( aStatement )->tmplate )
                              != IDE_SUCCESS );

                    if ( ( sData->columnDesc.module->flag & MTD_CANON_MASK )
                         == MTD_CANON_NEED )
                    {
                        sDestCanonizedValue = sDestValue;

                        IDE_TEST( sData->columnDesc.module->canonize( &( sData->columnDesc ),
                                                                      & sDestCanonizedValue,
                                                                      NULL,
                                                                      sDestColumn,
                                                                      sDestValue,
                                                                      NULL,
                                                                      & QC_PRIVATE_TMPLATE( aStatement )->tmplate )
                                  != IDE_SUCCESS );

                        sDestValue = sDestCanonizedValue;
                    }
                    else if ( ( sData->columnDesc.module->flag & MTD_CANON_MASK )
                              == MTD_CANON_NEED_WITH_ALLOCATION )
                    {
                        IDE_TEST( aMemory->alloc( sData->columnDesc.column.size,
                                                  (void**)& sDestCanonizedValue )
                                  != IDE_SUCCESS );

                        IDE_TEST( sData->columnDesc.module->canonize( &( sData->columnDesc ),
                                                                      & sDestCanonizedValue,
                                                                      NULL,
                                                                      sDestColumn,
                                                                      sDestValue,
                                                                      NULL,
                                                                      & QC_PRIVATE_TMPLATE( aStatement )->tmplate )
                                  != IDE_SUCCESS );

                        sDestValue = sDestCanonizedValue;
                    }
                }
                else
                {
                    sDestValue = (void*)mtc::value( &(sData->valueDesc),
                                                    sData->value,
                                                    MTD_OFFSET_USE );
                }
            } // end if

            sHashVal = sData->columnDesc.module->hash( sHashVal,
                                                       &(sData->columnDesc),
                                                       sDestValue );

            sStage = 0;
            IDE_TEST( aMemory-> setStatus( & sMemStatus ) != IDE_SUCCESS );
        } // end for
    }
    else
    {
        *aIsValidHashVal = ID_FALSE;
    }

    aPartKeyRange->minimum.mHashVal = sHashVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)aMemory->setStatus( &sMemStatus );
    }

    *aIsValidHashVal = ID_FALSE;

    sErrorCode = ideGetErrorCode();

    // overflow 에러에 대해서는 무시한다.
    if( sErrorCode == mtERR_ABORT_VALUE_OVERFLOW )
    {
        IDE_CLEAR();

        return IDE_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC
qmoPartition::mergePartitionRef( qmsTableRef     * aTableRef,
                                 qmsPartitionRef * aPartitionRef )
{
    qmsPartitionRef * sPartitionRef;
    idBool            isIncludePartition = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoPartition::mergePartitionRef::__FT__" );

    IDE_DASSERT( aPartitionRef != NULL );

    if ( aTableRef->partitionRef == NULL )
    {
        aTableRef->partitionRef = aPartitionRef;
        aTableRef->partitionRef->next = NULL;
    }
    else
    {
        for ( sPartitionRef  = aTableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef  = sPartitionRef->next )
        {
            if ( sPartitionRef->partitionHandle
                 == aPartitionRef->partitionHandle )
            {
                isIncludePartition = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( isIncludePartition == ID_FALSE )
        {
            aPartitionRef->next = aTableRef->partitionRef;
            aTableRef->partitionRef = aPartitionRef;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::partitionPruningWithKeyRange(
    qcStatement * aStatement,
    qmsTableRef * aTableRef,
    smiRange    * aPartKeyRange )
{
    IDU_FIT_POINT_FATAL( "qmoPartition::partitionPruningWithKeyRange::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aPartKeyRange != NULL );

    switch ( aTableRef->tableInfo->partitionMethod )
    {
        case QCM_PARTITION_METHOD_RANGE:
        {
            IDE_TEST( rangePartitionPruningWithKeyRange( aTableRef,
                                                         aPartKeyRange )
                      != IDE_SUCCESS );
        }
        break;

        case QCM_PARTITION_METHOD_LIST:
        {
            IDE_TEST( listPartitionPruningWithKeyRange( aTableRef,
                                                        aPartKeyRange )
                      != IDE_SUCCESS );
        }
        break;

        case QCM_PARTITION_METHOD_HASH:
        {
            IDE_TEST( hashPartitionPruningWithKeyRange( aStatement,
                                                        aTableRef,
                                                        aPartKeyRange )
                      != IDE_SUCCESS );
        }
        break;
        /* BUG-46065 support range using hash */
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
        {
            IDE_TEST( rangeUsinghashPartitionPruningWithKeyRange( aStatement,
                                                                  aTableRef,
                                                                  aPartKeyRange )
                      != IDE_SUCCESS );
        }
        break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPartition::rangePartitionPruningWithKeyRange(
    qmsTableRef * aTableRef,
    smiRange    * aPartKeyRange )
{
    qmsPartitionRef * sCurrRef;
    qmsPartitionRef * sPrevRef;
    smiRange        * sPartKeyRange;
    idBool            sIsIntersect;

    IDU_FIT_POINT_FATAL( "qmoPartition::rangePartitionPruningWithKeyRange::__FT__" );

    sPrevRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sIsIntersect = ID_FALSE;

        for ( sPartKeyRange  = aPartKeyRange;
              sPartKeyRange != NULL;
              sPartKeyRange  = sPartKeyRange->next )
        {
            if ( isIntersectRangePartition(
                     & sCurrRef->minPartCondVal,
                     & sCurrRef->maxPartCondVal,
                     sPartKeyRange )
                 == ID_TRUE )
            {
                sIsIntersect = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsIntersect == ID_TRUE )
        {
            sPrevRef = sCurrRef;
        }
        else
        {
            // intersect되지 않으면 partitionRef 리스트에서 제거.
            if ( sPrevRef == NULL )
            {
                aTableRef->partitionRef = sCurrRef->next;
            }
            else
            {
                sPrevRef->next = sCurrRef->next;
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::listPartitionPruningWithKeyRange(
    qmsTableRef * aTableRef,
    smiRange    * aPartKeyRange )
{
    qmsPartitionRef * sCurrRef;
    qmsPartitionRef * sPrevRef;
    smiRange        * sPartKeyRange;
    idBool            sIsIntersect;

    IDU_FIT_POINT_FATAL( "qmoPartition::listPartitionPruningWithKeyRange::__FT__" );

    sPrevRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sIsIntersect = ID_FALSE;

        for ( sPartKeyRange  = aPartKeyRange;
              sPartKeyRange != NULL;
              sPartKeyRange  = sPartKeyRange->next )
        {
            if ( isIntersectListPartition(
                     & sCurrRef->maxPartCondVal,
                     sPartKeyRange )
                 == ID_TRUE )
            {
                sIsIntersect = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsIntersect == ID_TRUE )
        {
            sPrevRef = sCurrRef;
        }
        else
        {
            // intersect되지 않으면 partitionRef 리스트에서 제거.
            if ( sPrevRef == NULL )
            {
                aTableRef->partitionRef = sCurrRef->next;
            }
            else
            {
                sPrevRef->next = sCurrRef->next;
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::hashPartitionPruningWithKeyRange(
    qcStatement * aStatement,
    qmsTableRef * aTableRef,
    smiRange    * aPartKeyRange )
{
    qmsPartitionRef * sCurrRef;
    qmsPartitionRef * sPrevRef;
    smiRange        * sPartKeyRange;
    idBool            sIsIntersect;
    UInt              sPartOrder;
    idBool            sIsValidHashVal;

    IDU_FIT_POINT_FATAL( "qmoPartition::hashPartitionPruningWithKeyRange::__FT__" );

    for ( sPartKeyRange  = aPartKeyRange;
          sPartKeyRange != NULL;
          sPartKeyRange  = sPartKeyRange->next )
    {
        IDE_TEST( makeHashKeyFromPartKeyRange( aStatement,
                                               QC_QMP_MEM( aStatement ),
                                               sPartKeyRange,
                                               & sIsValidHashVal )
                  != IDE_SUCCESS );

        if ( sIsValidHashVal == ID_FALSE )
        {
            // invalid하다는 표시를 callback을 null로 바꾸어서 확인한다.
            // 일반적으로 callback이 null일 일은 절대로 발생하지 않음.
            sPartKeyRange->minimum.callback = NULL;
        }
        else
        {
            // Nothing to do.
        }
    }

    sPrevRef = NULL;

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sIsIntersect = ID_FALSE;

        for ( sPartKeyRange  = aPartKeyRange;
              sPartKeyRange != NULL;
              sPartKeyRange  = sPartKeyRange->next )
        {
            if ( sPartKeyRange->minimum.callback == NULL )
            {
                // hash key가 invalid한 경우임.
                // Nothing to do.
            }
            else
            {
                sPartOrder = sPartKeyRange->minimum.mHashVal %
                    aTableRef->partitionCount;

                if ( sPartOrder == sCurrRef->partOrder )
                {
                    sIsIntersect = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        if ( sIsIntersect == ID_TRUE )
        {
            sPrevRef = sCurrRef;
        }
        else
        {
            // intersect되지 않으면 partitionRef 리스트에서 제거.
            if ( sPrevRef == NULL )
            {
                aTableRef->partitionRef = sCurrRef->next;
            }
            else
            {
                sPrevRef->next = sCurrRef->next;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPartition::partitionFilteringWithPartitionFilter(
    qcStatement            * aStatement,
    qmnRangeSortedChildren * aRangeSortedChildrenArray,
    UInt                   * aRangeIntersectCountArray,
    UInt                     aSelectedPartitionCount,
    UInt                     aPartitionCount,
    qmnChildren            * aChildren,
    qcmPartitionMethod       aPartitionMethod,
    smiRange               * aPartFilter,
    qmnChildren           ** aSelectedChildrenArea,
    UInt                   * aSelectedChildrenCount )
{

    IDU_FIT_POINT_FATAL( "qmoPartition::partitionFilteringWithPartitionFilter::__FT__" );

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aChildren != NULL );
    IDE_DASSERT( aPartFilter != NULL );
    IDE_DASSERT( aSelectedChildrenArea != NULL );
    IDE_DASSERT( aSelectedChildrenCount != NULL );

    switch ( aPartitionMethod )
    {
        case QCM_PARTITION_METHOD_RANGE:
            {
                IDE_TEST( rangePartitionFilteringWithPartitionFilter( aRangeSortedChildrenArray,
                                                                      aRangeIntersectCountArray,
                                                                      aSelectedPartitionCount,
                                                                      aPartFilter,
                                                                      aSelectedChildrenArea,
                                                                      aSelectedChildrenCount )
                          != IDE_SUCCESS );
            }
            break;

        case QCM_PARTITION_METHOD_LIST:
            {
                IDE_TEST( listPartitionFilteringWithPartitionFilter( aChildren,
                                                                     aPartFilter,
                                                                     aSelectedChildrenArea,
                                                                     aSelectedChildrenCount )
                          != IDE_SUCCESS );
            }
            break;

        case QCM_PARTITION_METHOD_HASH:
            {
                IDE_TEST( hashPartitionFilteringWithPartitionFilter( aStatement,
                                                                     aPartitionCount,
                                                                     aChildren,
                                                                     aPartFilter,
                                                                     aSelectedChildrenArea,
                                                                     aSelectedChildrenCount )
                          != IDE_SUCCESS );
            }
            break;
        /* BUG-46065 support range using hash */
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
            {
                IDE_TEST( rangeUsingHashFilteringWithPartitionFilter( aStatement,
                                                                      aRangeSortedChildrenArray,
                                                                      aRangeIntersectCountArray,
                                                                      aSelectedPartitionCount,
                                                                      aPartFilter,
                                                                      aSelectedChildrenArea,
                                                                      aSelectedChildrenCount )
                          != IDE_SUCCESS );
            }
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPartition::rangePartitionFilteringWithPartitionFilter(
    qmnRangeSortedChildren * aRangeSortedChildrenArray,
    UInt                   * aRangeIntersectCountArray,
    UInt                     aPartCount,
    smiRange               * aPartFilter,
    qmnChildren           ** aSelectedChildrenArea,
    UInt                   * aSelectedChildrenCount )
{
    smiRange * sPartFilter;
    UInt       i;
    UInt       sPlusCount = 0;

    IDU_FIT_POINT_FATAL( "qmoPartition::rangePartitionFilteringWithPartitionFilter::__FT__" );

    *aSelectedChildrenCount = 0;

    for ( sPartFilter  = aPartFilter;
          sPartFilter != NULL;
          sPartFilter  = sPartFilter->next )
    {
        if ( ( sPartFilter->minimum.callback
               == mtk::rangeCallBackGT4Mtd )
             ||
             ( sPartFilter->minimum.callback
               == mtk::rangeCallBackGT4Stored ) )
        {
            partitionFilterGT( aRangeSortedChildrenArray,
                               aRangeIntersectCountArray,
                               sPartFilter,
                               aPartCount );
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sPartFilter->minimum.callback
               == mtk::rangeCallBackGE4Mtd )
             ||
             ( sPartFilter->minimum.callback
               == mtk::rangeCallBackGE4Stored ) )
        {
            partitionFilterGE( aRangeSortedChildrenArray,
                               aRangeIntersectCountArray,
                               sPartFilter,
                               aPartCount );
        }
        else
        {
            // Nothing to do.
        }

        //keyrange max
        if ( ( sPartFilter->maximum.callback
               == mtk::rangeCallBackLT4Mtd )
             ||
             ( sPartFilter->maximum.callback
               == mtk::rangeCallBackLT4Stored ) )
        {
            partitionFilterLT( aRangeSortedChildrenArray,
                               aRangeIntersectCountArray,
                               sPartFilter,
                               aPartCount );
            sPlusCount ++;
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sPartFilter->maximum.callback
               == mtk::rangeCallBackLE4Mtd )
             ||
             ( sPartFilter->maximum.callback
               == mtk::rangeCallBackLE4Stored ) )
        {
            partitionFilterLE( aRangeSortedChildrenArray,
                               aRangeIntersectCountArray,
                               sPartFilter,
                               aPartCount );
            sPlusCount ++;
        }
        else
        {
            // Nothing to do.
        }
    }

    for ( i = 0; i < aPartCount; i++ )
    {
        if ( aRangeIntersectCountArray[i] > sPlusCount )
        {
            aSelectedChildrenArea[(*aSelectedChildrenCount)++]
                = aRangeSortedChildrenArray[i].children;
        }
        else
        {
            //Nothing to do.
        }

        aRangeIntersectCountArray[i] = 0;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::listPartitionFilteringWithPartitionFilter(
    qmnChildren  * aChildren,
    smiRange     * aPartFilter,
    qmnChildren ** aSelectedChildrenArea,
    UInt         * aSelectedChildrenCount )
{
    qmnChildren     * sCurrChild;
    smiRange        * sPartFilter;
    idBool            sIsIntersect;
    qmsPartitionRef * sPartitionRef;

    IDU_FIT_POINT_FATAL( "qmoPartition::listPartitionFilteringWithPartitionFilter::__FT__" );

    *aSelectedChildrenCount = 0;

    for ( sCurrChild  = aChildren;
          sCurrChild != NULL;
          sCurrChild  = sCurrChild->next )
    {
        sIsIntersect = ID_FALSE;

        for ( sPartFilter  = aPartFilter;
              sPartFilter != NULL;
              sPartFilter  = sPartFilter->next )
        {
            sPartitionRef =
                ((qmncSCAN*)(sCurrChild->childPlan))->partitionRef;

            if ( isIntersectListPartition(
                     & sPartitionRef->maxPartCondVal,
                     sPartFilter )
                 == ID_TRUE )
            {
                sIsIntersect = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsIntersect == ID_TRUE )
        {
            aSelectedChildrenArea[(*aSelectedChildrenCount)++]
                = sCurrChild;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::hashPartitionFilteringWithPartitionFilter(
    qcStatement  * aStatement,
    UInt           aPartitionCount,
    qmnChildren  * aChildren,
    smiRange     * aPartFilter,
    qmnChildren ** aSelectedChildrenArea,
    UInt         * aSelectedChildrenCount )
{
    qmnChildren     * sCurrChild;
    smiRange        * sPartFilter;
    idBool            sIsIntersect;
    qmsPartitionRef * sPartitionRef;

    UInt              sPartOrder;
    idBool            sIsValidHashVal;

    IDU_FIT_POINT_FATAL( "qmoPartition::hashPartitionFilteringWithPartitionFilter::__FT__" );

    for ( sPartFilter  = aPartFilter;
          sPartFilter != NULL;
          sPartFilter  = sPartFilter->next )
    {
        IDE_TEST( makeHashKeyFromPartKeyRange( aStatement,
                                               aStatement->qmxMem,
                                               sPartFilter,
                                               & sIsValidHashVal )
                  != IDE_SUCCESS );

        if ( sIsValidHashVal == ID_FALSE )
        {
            // invalid하다는 표시를 callback을 null로 바꾸어서 확인한다.
            // 일반적으로 callback이 null일 일은 절대로 발생하지 않음.
            sPartFilter->minimum.callback = NULL;
        }
        else
        {
            // Nothing to do.
        }
    }

    *aSelectedChildrenCount = 0;

    for ( sCurrChild = aChildren;
          sCurrChild != NULL;
          sCurrChild = sCurrChild->next )
    {
        sIsIntersect = ID_FALSE;

        for ( sPartFilter = aPartFilter;
              sPartFilter != NULL;
              sPartFilter = sPartFilter->next )
        {

            sPartitionRef =
                ((qmncSCAN*)(sCurrChild->childPlan))->partitionRef;

            if ( sPartFilter->minimum.callback == NULL )
            {
                // hash key가 invalid한 경우임.
                // Nothing to do.
            }
            else
            {
                sPartOrder = sPartFilter->minimum.mHashVal %
                    aPartitionCount;

                if ( sPartOrder == sPartitionRef->partOrder )
                {
                    sIsIntersect = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        if ( sIsIntersect == ID_TRUE )
        {
            aSelectedChildrenArea[(*aSelectedChildrenCount)++]
                = sCurrChild;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoPartition::isIntersectPartKeyColumn(
    qcmColumn * aPartKeyColumns,
    qcmColumn * aUptColumns,
    idBool    * aIsIntersect )
{
    qcmColumn * sUptColumn;
    qcmColumn * sPartKeyColumn;

    IDU_FIT_POINT_FATAL( "qmoPartition::isIntersectPartKeyColumn::__FT__" );

    *aIsIntersect = ID_FALSE;

    for ( sUptColumn  = aUptColumns;
          sUptColumn != NULL;
          sUptColumn  = sUptColumn->next )
    {
        for ( sPartKeyColumn  = aPartKeyColumns;
              sPartKeyColumn != NULL;
              sPartKeyColumn  = sPartKeyColumn->next )
        {
            if ( sUptColumn->basicInfo->column.id ==
                 sPartKeyColumn->basicInfo->column.id )
            {
                *aIsIntersect = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::minusPartitionRef( qmsTableRef * aTableRef1,
                                 qmsTableRef * aTableRef2 )
{
    qmsPartitionRef * sCurrRef1;
    qmsPartitionRef * sCurrRef2;
    qmsPartitionRef * sPrevRef;
    idBool            sFound;

    IDU_FIT_POINT_FATAL( "qmoPartition::minusPartitionRef::__FT__" );

    sPrevRef = NULL;

    for ( sCurrRef1  = aTableRef1->partitionRef;
          sCurrRef1 != NULL;
          sCurrRef1  = sCurrRef1->next )
    {
        sFound = ID_FALSE;

        for ( sCurrRef2  = aTableRef2->partitionRef;
              sCurrRef2 != NULL;
              sCurrRef2  = sCurrRef2->next )
        {
            if ( sCurrRef1->partitionID == sCurrRef2->partitionID )
            {
                sFound = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sFound == ID_TRUE )
        {
            if ( sPrevRef == NULL )
            {
                aTableRef1->partitionRef = sCurrRef1->next;
            }
            else
            {
                sPrevRef->next = sCurrRef1->next;
            }
        }
        else
        {
            sPrevRef = sCurrRef1;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoPartition::makePartUpdateColumnList( qcStatement      * aStatement,
                                        qmsPartitionRef  * aPartitionRef,
                                        smiColumnList    * aColumnList,
                                        smiColumnList   ** aPartColumnList )
{
/***********************************************************************
 *
 *  Description :
 *
 *  Implementation :
 *      partition용 update column list를 생성한다.
 *
 ***********************************************************************/

    smiColumnList * sCurrColumn;
    smiColumnList * sFirstColumn;
    smiColumnList * sPrevColumn;
    smiColumnList * sColumn;
    qcmColumn     * sQcmColumn;

    IDU_FIT_POINT_FATAL( "qmoPartition::makePartUpdateColumnList::__FT__" );

    sFirstColumn = NULL;
    sPrevColumn  = NULL;

    for ( sColumn  = aColumnList;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( smiColumnList ),
                                                   (void **) & sCurrColumn )
                  != IDE_SUCCESS );

        // smiColumnList 정보 설정
        sCurrColumn->next = NULL;

        for ( sQcmColumn  = aPartitionRef->partitionInfo->columns;
              sQcmColumn != NULL;
              sQcmColumn  = sQcmColumn->next )
        {
            if ( sQcmColumn->basicInfo->column.id == sColumn->column->id )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        IDE_TEST_RAISE( sQcmColumn == NULL, ERR_COLUMN_NOT_FOUND );

        sCurrColumn->column = & sQcmColumn->basicInfo->column;

        if ( sFirstColumn == NULL )
        {
            sFirstColumn = sCurrColumn;
        }
        else
        {
            sPrevColumn->next = sCurrColumn;
        }

        sPrevColumn = sCurrColumn;
    }

    *aPartColumnList = sFirstColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmo::makePartUpdateColumnList",
                                  "Column not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

extern "C" SInt
compareRangePartChildren( const void* aElem1, const void* aElem2 )
{
    qmnRangeSortedChildren * sComp1;
    qmnRangeSortedChildren * sComp2;

    qmsPartitionRef  * sPartitionRef1;
    qmsPartitionRef  * sPartitionRef2;

    SInt sRet = 0;

    sComp1 = (qmnRangeSortedChildren*)aElem1;
    sComp2 = (qmnRangeSortedChildren*)aElem2;

    IDE_DASSERT( sComp1 != NULL );
    IDE_DASSERT( sComp2 != NULL );

    sPartitionRef1 = ((qmncSCAN*)(sComp1->children->childPlan))->partitionRef;
    sPartitionRef2 = ((qmncSCAN*)(sComp2->children->childPlan))->partitionRef;

    /* PROJ-2446 ONE SOURCE */
    sRet = qmoPartition::compareRangePartition(
        sComp1->partKeyColumns,
        &sPartitionRef1->minPartCondVal,
        &sPartitionRef2->minPartCondVal );

    return sRet;
}

/* BUG-46065 support range using hash */
extern "C" SInt compareRangeHashPartChildren( const void * aElem1, const void * aElem2 )
{
    qmnRangeSortedChildren * sComp1;
    qmnRangeSortedChildren * sComp2;
    qmsPartitionRef        * sPartitionRef1;
    qmsPartitionRef        * sPartitionRef2;
    SInt                     sRet = 0;

    sComp1 = (qmnRangeSortedChildren*)aElem1;
    sComp2 = (qmnRangeSortedChildren*)aElem2;

    IDE_DASSERT( sComp1 != NULL );
    IDE_DASSERT( sComp2 != NULL );

    sPartitionRef1 = ((qmncSCAN*)(sComp1->children->childPlan))->partitionRef;
    sPartitionRef2 = ((qmncSCAN*)(sComp2->children->childPlan))->partitionRef;

    /* PROJ-2446 ONE SOURCE */
    sRet = qmoPartition::compareRangeUsingHashPartition( &sPartitionRef1->minPartCondVal,
                                                         &sPartitionRef2->minPartCondVal );

    return sRet;
}

IDE_RC qmoPartition::sortPartitionRef(
                qmnRangeSortedChildren * aRangeSortedChildrenArray,
                UInt                     aPartitionCount)
{
    IDU_FIT_POINT_FATAL( "qmoPartition::sortPartitionRef::__FT__" );

    /* PROJ-2446 ONE SOURCE */
    idlOS::qsort( aRangeSortedChildrenArray,
                  aPartitionCount,
                  ID_SIZEOF(qmnRangeSortedChildren),
                  compareRangePartChildren );

    return IDE_SUCCESS;
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::sortRangeHashPartitionRef( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                                UInt                     aPartitionCount )
{
    /* PROJ-2446 ONE SOURCE */
    idlOS::qsort( aRangeSortedChildrenArray,
                  aPartitionCount,
                  ID_SIZEOF(qmnRangeSortedChildren),
                  compareRangeHashPartChildren );

    return IDE_SUCCESS;
}

SInt qmoPartition::comparePartMinRangeMax( qmsPartCondValList * aMinPartCondVal,
                                           smiRange           * aPartKeyRange )
{
    mtkRangeCallBack * sData;
    UInt               i;
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    if( aMinPartCondVal->partCondValType == QMS_PARTCONDVAL_MIN )
    {
        sOrder = -1;
    }
    else
    {
        // min partcondval은 default일 수 없음.
        IDE_DASSERT( aMinPartCondVal->partCondValType !=
                     QMS_PARTCONDVAL_DEFAULT );

        sData = (mtkRangeCallBack*)aPartKeyRange->maximum.data;
        i = 0;

        while( 1 )
        {
            // key range compare에서는 fixed compare를 호출하므로
            // offset을 0으로 변경한다.
            sData->columnDesc.column.offset = 0;
            
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aMinPartCondVal->partCondValues[i];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1, &sValueInfo2 );

            if ( sOrder == 0 )
            {
                i++;
                sData = sData->next;

                if( ( i < aMinPartCondVal->partCondValCount ) &&
                    ( sData != NULL ) )
                {
                    continue;
                }
                else if ( ( i == aMinPartCondVal->partCondValCount ) &&
                          ( sData == NULL ) )
                {
                    break;
                }
                else if ( i == aMinPartCondVal->partCondValCount )
                {
                    // ex)
                    // partition 범위min : 10
                    // i1 = 10 and i2 < 10
                    // i1 = 10 and i2 <= 10
                    // 뒤에 컬럼이 하나 더 있음으로 해서 무조건
                    // keyrange의 max가 크다.

                    sOrder = -1;

                    break;
                }
                else
                {
                    // ex)
                    // partition 범위min : 10, 10
                    // i1 < 10   .... (1)
                    // i1 <= 10  .... (2)
                    // 1의 경우는 partition min이 크다.
                    // 하지만, 2의 경우는 keyrange의 max가 크다.
                    if( ( aPartKeyRange->maximum.callback
                          == mtk::rangeCallBackLT4Mtd )
                        ||
                        ( aPartKeyRange->maximum.callback
                          == mtk::rangeCallBackLT4Stored ) )
                    {
                        sOrder = 1;
                    }
                    else
                    {
                        sOrder = -1;
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return sOrder;
}

SInt qmoPartition::comparePartMinRangeMin( qmsPartCondValList * aMinPartCondVal,
                                           smiRange           * aPartKeyRange )
{
    mtkRangeCallBack * sData;
    UInt               i;
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    if( aMinPartCondVal->partCondValType == QMS_PARTCONDVAL_MIN )
    {
        sOrder = -1;
    }
    else
    {
        // min partcondval은 default일 수 없음.
        IDE_DASSERT( aMinPartCondVal->partCondValType !=
                     QMS_PARTCONDVAL_DEFAULT );

        sData = (mtkRangeCallBack*)aPartKeyRange->minimum.data;
        i = 0;

        while( 1 )
        {
            // key range compare에서는 fixed compare를 호출하므로
            // offset을 0으로 변경한다.
            sData->columnDesc.column.offset = 0;

            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aMinPartCondVal->partCondValues[i];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1, &sValueInfo2 );

            if ( sOrder == 0 )
            {
                i++;
                sData = sData->next;

                if ( ( i < aMinPartCondVal->partCondValCount ) &&
                     ( sData != NULL ) )
                {
                    continue;
                }
                else if ( ( i == aMinPartCondVal->partCondValCount ) &&
                          ( sData == NULL ) )
                {
                    break;
                }
                else if ( i == aMinPartCondVal->partCondValCount )
                {
                    // ex)
                    // partition 범위min : 10
                    // i1 = 10 and i2 < 10
                    // i1 = 10 and i2 <= 10
                    // 뒤에 컬럼이 하나 더 있음으로 해서 무조건
                    // keyrange의 max가 크다.

                    sOrder = -1;

                    break;
                }
                else
                {
                    // ex)
                    // partition 범위min : 10, 10
                    // i1 > 10   .... (1)
                    // i1 >= 10  .... (2)
                    // 1의 경우는 keyrange의 max가 더 크다.
                    // 하지만, 2의 경우는 keyrange의 min이 크다.
                    if( ( aPartKeyRange->minimum.callback ==
                          mtk::rangeCallBackGT4Mtd )
                        ||
                        ( aPartKeyRange->minimum.callback ==
                          mtk::rangeCallBackGT4Stored ) )
                    {
                        sOrder = -1;
                    }
                    else
                    {
                        sOrder = 1;
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return sOrder;
}

SInt qmoPartition::comparePartMaxRangeMin( qmsPartCondValList * aMaxPartCondVal,
                                           smiRange           * aPartKeyRange )
{
    mtkRangeCallBack * sData;
    UInt               i;
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    if( aMaxPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
    {
        sOrder = 1;
    }
    else
    {
        // max partcondval은 min일 수 없음.
        IDE_DASSERT( aMaxPartCondVal->partCondValType
                     != QMS_PARTCONDVAL_MIN );

        i = 0;
        sData  = (mtkRangeCallBack*)aPartKeyRange->minimum.data;

        while( 1 )
        {
            // key range compare에서는 fixed compare를 호출하므로
            // offset을 0으로 변경한다.
            sData->columnDesc.column.offset = 0;
            
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aMaxPartCondVal->partCondValues[i];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1, &sValueInfo2 );

            if ( sOrder == 0 )
            {
                i++;
                sData = sData->next;

                if( ( i < aMaxPartCondVal->partCondValCount ) &&
                    ( sData != NULL ) )
                {
                    continue;
                }
                else if ( ( i == aMaxPartCondVal->partCondValCount ) &&
                          ( sData == NULL ) )
                {
                    break;
                }
                else if ( i == aMaxPartCondVal->partCondValCount )
                {
                    // ex)
                    // partition 범위min : 10
                    // i1 = 10 and i2 > 10
                    // i1 = 10 and i2 >= 10
                    // 뒤에 컬럼이 하나 더 있음으로 해서 무조건
                    // keyrange의 min이 크다.

                    sOrder = -1;
                    break;
                }
                else
                {
                    // ex)
                    // partition 범위min : 10, 10
                    // i1 > 10   .... (1)
                    // i1 >= 10  .... (2)
                    // 1의 경우는 keyrange의 max가 더 크다.
                    // 하지만, 2의 경우는 keyrange의 min이 크다.
                    if( ( aPartKeyRange->minimum.callback ==
                          mtk::rangeCallBackGT4Mtd )
                        ||
                        ( aPartKeyRange->minimum.callback ==
                          mtk::rangeCallBackGT4Stored ) )
                    {
                        sOrder = -1;
                    }
                    else
                    {
                        sOrder = 1;
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return sOrder;
}
//
SInt qmoPartition::comparePartMaxRangeMax( qmsPartCondValList * aMaxPartCondVal,
                                           smiRange           * aPartKeyRange )
{
    mtkRangeCallBack * sData;
    UInt               i;
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;

    if( aMaxPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
    {
        sOrder = 1;
    }
    else
    {
        // max partcondval은 min일 수 없음.
        IDE_DASSERT( aMaxPartCondVal->partCondValType
                     != QMS_PARTCONDVAL_MIN );

        i = 0;
        sData  = (mtkRangeCallBack*)aPartKeyRange->maximum.data;

        while( 1 )
        {
            // key range compare에서는 fixed compare를 호출하므로
            // offset을 0으로 변경한다.
            sData->columnDesc.column.offset = 0;
            
            sValueInfo1.column = &(sData->columnDesc);
            sValueInfo1.value  = aMaxPartCondVal->partCondValues[i];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = &(sData->valueDesc);
            sValueInfo2.value  = sData->value;
            sValueInfo2.flag   = MTD_OFFSET_USE;

            sOrder = sData->compare( &sValueInfo1, &sValueInfo2 );

            if ( sOrder == 0 )
            {
                i++;
                sData = sData->next;

                if( ( i < aMaxPartCondVal->partCondValCount ) &&
                    ( sData != NULL ) )
                {
                    continue;
                }
                else if ( ( i == aMaxPartCondVal->partCondValCount ) &&
                          ( sData == NULL ) )
                {
                    break;
                }
                else if ( i == aMaxPartCondVal->partCondValCount )
                {
                    // ex)
                    // partition 범위min : 10
                    // i1 = 10 and i2 < 10
                    // i1 = 10 and i2 <= 10
                    // 뒤에 컬럼이 하나 더 있음으로 해서 무조건
                    // keyrange의 max가 크다.

                    sOrder = -1;

                    break;
                }
                else
                {
                    // ex)
                    // partition 범위min : 10, 10
                    // i1 < 10   .... (1)
                    // i1 <= 10  .... (2)
                    // 1의 경우는 partition max이 크다.
                    // 하지만, 2의 경우는 keyrange의 max가 크다.
                    if( ( aPartKeyRange->maximum.callback
                          == mtk::rangeCallBackLT4Mtd )
                        ||
                        ( aPartKeyRange->maximum.callback
                          == mtk::rangeCallBackLT4Stored ) )
                    {
                        sOrder = 1;
                    }
                    else
                    {
                        sOrder = -1;
                    }
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    return sOrder;
}

/*
 * compare GT
 *
 *    +------+
 * +1 |  0   | -1
 *    |  p2  |
 *    +      o
 *
 *    +------>
 *    |
 *    |
 *    o
 *    0
 *           +------>
 *           |
 *           |
 *           o
 *          -1
 */
SInt qmoPartition::comparePartGT( qmsPartCondValList * aMinPartCondVal,
                                  qmsPartCondValList * aMaxPartCondVal,
                                  smiRange           * aPartKeyRange )
{
    SInt sOrder;
    SInt sRes;

    sOrder = comparePartMinRangeMin( aMinPartCondVal, aPartKeyRange );

    if ( sOrder > 0 )
    {
        sRes = 1;
    }
    else if ( sOrder == 0 )
    {   
        sRes = 0;
    }
    else
    {
        sOrder = comparePartMaxRangeMin( aMaxPartCondVal, aPartKeyRange );

        if ( sOrder > 0 )
        {   
            sRes = 0;
        }
        else if ( sOrder == 0 )
        {
            sRes = -1;
        }
        else
        {   
            sRes = -1;
        }
    }

    return sRes;
}

/*
 * compare GE
 *
 *    +------+
 * +1 |  0   | -1
 *    |  p2  |
 *    +      o
 *
 *    +------>
 *    |
 *    |
 *    +
 *    0
 *           +------>
 *           |
 *           |
 *           +
 *          -1
 */
SInt qmoPartition::comparePartGE( qmsPartCondValList * aMinPartCondVal,
                                  qmsPartCondValList * aMaxPartCondVal,
                                  smiRange           * aPartKeyRange )
{
    SInt sOrder;
    SInt sRes;

    sOrder = comparePartMinRangeMin( aMinPartCondVal, aPartKeyRange );

    if ( sOrder > 0 )
    {
        sRes = 1;
    }
    else if ( sOrder == 0 )
    {
        sRes = 0;
    }
    else 
    {
        sOrder = comparePartMaxRangeMin( aMaxPartCondVal, aPartKeyRange );

        if ( sOrder > 0 )
        {
            sRes = 0;
        }
        else if ( sOrder == 0 )
        {
            sRes = -1;
        }
        else
        {
            sRes = -1;
        }
    }

    return sRes;
}

/*
 * compare LT
 *
 *           +------+
 *        +1 |  0   | -1
 *           |  p2  |
 *           +      o
 *
 *          <-------+
 *                  |
 *                  |
 *                  o
 *                  0
 *   <-------+
 *           |
 *           |
 *           o
 *          +1
 */
SInt qmoPartition::comparePartLT( qmsPartCondValList * aMinPartCondVal,
                                  qmsPartCondValList * aMaxPartCondVal,
                                  smiRange           * aPartKeyRange )
{
    SInt sOrder;
    SInt sRes = 0;

    sOrder = comparePartMaxRangeMax( aMaxPartCondVal, aPartKeyRange );
    
    if ( sOrder < 0 )
    {
        sRes = -1;
    }
    else if ( sOrder == 0 )
    {
        sRes = 0;
    }
    else
    {
        sOrder = comparePartMinRangeMax( aMinPartCondVal, aPartKeyRange );
        if ( sOrder > 0 )
        {
            sRes = 1;
        }
        else if ( sOrder == 0 )
        {
            sRes = 1;
        }
        else
        {
            sRes = 0;
        }
    }

    return sRes;
}

/*
 * compare LE
 *
 *           +------+
 *        +1 |  0   | -1
 *           |  p2  |
 *           +      o
 *
 *          <-------+
 *                  |
 *                  |
 *                  +
 *                 -1
 *   <-------+
 *           |
 *           |
 *           +
 *           0

 */
SInt qmoPartition::comparePartLE( qmsPartCondValList * aMinPartCondVal,
                                  qmsPartCondValList * aMaxPartCondVal,
                                  smiRange           * aPartKeyRange )
{
    SInt sOrder;
    SInt sRes = 0;

    sOrder = comparePartMaxRangeMax( aMaxPartCondVal, aPartKeyRange );
    
    if ( sOrder < 0 )
    {
        sRes = -1;
    }
    else if ( sOrder == 0 )
    {
        sRes = -1;
    }
    else
    {
        sOrder = comparePartMinRangeMax( aMinPartCondVal, aPartKeyRange );

        if ( sOrder > 0 )
        {
            sRes = 1;
        }
        else if ( sOrder == 0 )
        {
            sRes = 0;
        }
        else
        {
            sRes = 0;
        }
    }

    return sRes;
}

void qmoPartition::partitionFilterGT( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                      UInt                   * aRangeIntersectCountArray,
                                      smiRange               * aPartKeyRange,
                                      UInt                     aPartCount )
{
    UInt sHigh;
    UInt sLow;
    UInt sMid;

    SInt sRes;
    qmnRangeSortedChildren * sRangeSortedChildrenArray = NULL;

    sHigh = aPartCount - 1;
    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        sRangeSortedChildrenArray = aRangeSortedChildrenArray + sMid;
        
        sRes = comparePartGT( &(sRangeSortedChildrenArray->partitionRef->minPartCondVal),
                              &(sRangeSortedChildrenArray->partitionRef->maxPartCondVal),
                              aPartKeyRange );

        if ( sRes == 0 )
        {
            break;
        }
        else if ( sRes > 0 )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                sHigh = sMid - 1;
            }
        }
        else
        {
            sLow = sMid + 1;
        }
    }

    IDE_ASSERT( sRangeSortedChildrenArray != NULL );

    for ( ; sMid < aPartCount; sMid++ )
    {
        aRangeIntersectCountArray[sMid]++;    
    }
    
}

void qmoPartition::partitionFilterGE( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                      UInt                   * aRangeIntersectCountArray,
                                      smiRange               * aPartKeyRange,
                                      UInt                     aPartCount )
{
    UInt sHigh;
    UInt sLow;
    UInt sMid;

    SInt sRes;
    qmnRangeSortedChildren * sRangeSortedChildrenArray = NULL;

    sHigh = aPartCount -1;
    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        sRangeSortedChildrenArray = aRangeSortedChildrenArray + sMid;
    
        sRes = comparePartGE( &(sRangeSortedChildrenArray->partitionRef->minPartCondVal),
                              &(sRangeSortedChildrenArray->partitionRef->maxPartCondVal),
                              aPartKeyRange );

        if ( sRes == 0 )
        {
            break;
        }
        else if ( sRes > 0 )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                sHigh = sMid - 1;
            }
        }
        else
        {
            sLow = sMid + 1;
        }
    }
    
    for ( ; sMid < aPartCount; sMid++ )
    {
        aRangeIntersectCountArray[sMid]++;
        
    }
}

void qmoPartition::partitionFilterLT( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                      UInt                   * aRangeIntersectCountArray,
                                      smiRange               * aPartKeyRange,
                                      UInt                     aPartCount )
{
    UInt sHigh;
    UInt sLow;
    UInt sMid;

    UInt i;
    
    SInt sRes;
    qmnRangeSortedChildren * sRangeSortedChildrenArray = NULL;

    sHigh = aPartCount -1;
    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        sRangeSortedChildrenArray = aRangeSortedChildrenArray + sMid;
    
        sRes = comparePartLT( &(sRangeSortedChildrenArray->partitionRef->minPartCondVal),
                              &(sRangeSortedChildrenArray->partitionRef->maxPartCondVal),
                              aPartKeyRange );

        if ( sRes == 0 )
        {
            break;
        }
        else if ( sRes > 0 )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                sHigh = sMid - 1;
            }
        }
        else
        {
            sLow = sMid + 1;
        }
    }
    
    for ( i = 0; i <=sMid; i++ )
    {
        aRangeIntersectCountArray[i]++;        
    }
}

void qmoPartition::partitionFilterLE( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                      UInt                   * aRangeIntersectCountArray,
                                      smiRange               * aPartKeyRange,
                                      UInt                     aPartCount )
{
    UInt sHigh;
    UInt sLow;
    UInt sMid;

    UInt i;

    SInt sRes;
    qmnRangeSortedChildren * sRangeSortedChildrenArray = NULL;

    sHigh = aPartCount -1;
    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        sRangeSortedChildrenArray = aRangeSortedChildrenArray + sMid;
    
        sRes = comparePartLE( &(sRangeSortedChildrenArray->partitionRef->minPartCondVal),
                              &(sRangeSortedChildrenArray->partitionRef->maxPartCondVal),
                              aPartKeyRange );

        if ( sRes == 0 )
        {
            break;
        }
        else if ( sRes > 0 )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                sHigh = sMid - 1;
            }
        }
        else
        {
            sLow = sMid + 1;
        }
    }
    
    for ( i = 0; i <= sMid; i++ )
    {
        aRangeIntersectCountArray[i]++;        
    }
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::makeHashKeyForRangePruning( qcStatement        * aStatement,
                                                 mtkRangeCallBack   * aData,
                                                 UInt               * aHashValue )
{
    iduVarMemList      * sMemory;
    void               * sSourceValue;
    UInt                 sTargetArgCount;
    UInt                 sHashVal = mtc::hashInitialValue;
    iduVarMemListStatus  sMemStatus;
    volatile UInt        sStage;
    mtvConvert         * sConvert;
    mtcColumn          * sDestColumn;
    void               * sDestValue;
    void               * sDestCanonizedValue;

    IDE_FT_BEGIN();

    sStage = 0;
    sMemory = QC_QMP_MEM( aStatement );

    IDE_TEST( sMemory->getStatus( &sMemStatus ) != IDE_SUCCESS );
    sStage = 1;

    if ( ( aData->compare == mtk::compareMaximumLimit4Mtd ) ||
         ( aData->compare == mtk::compareMaximumLimit4Stored ) )
    {
        sDestValue = aData->columnDesc.module->staticNull;
        // is null인 경우임.
    }
    else
    {
        if ( aData->columnDesc.type.dataTypeId !=
             aData->valueDesc.type.dataTypeId )
        {
            // PROJ-2002 Column Security
            // echar, evarchar는 동일 group이 아니므로 들어올 수 없다.
            IDE_DASSERT(
                (aData->columnDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                (aData->columnDesc.type.dataTypeId != MTD_EVARCHAR_ID) &&
                (aData->valueDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                (aData->valueDesc.type.dataTypeId != MTD_EVARCHAR_ID) );

            sSourceValue = (void*)mtc::value( &(aData->valueDesc),
                                              aData->value,
                                              MTD_OFFSET_USE );

            sTargetArgCount = aData->columnDesc.flag &
                MTC_COLUMN_ARGUMENT_COUNT_MASK;

            IDE_TEST( mtv::estimateConvert4Server( sMemory,
                                                   & sConvert,
                                                   aData->columnDesc.type,
                                                   aData->valueDesc.type,
                                                   sTargetArgCount,
                                                   aData->columnDesc.precision,
                                                   aData->valueDesc.scale,
                                                   & QC_SHARED_TMPLATE( aStatement )->tmplate )
                      != IDE_SUCCESS );

            // source value pointer
            sConvert->stack[sConvert->count].value = sSourceValue;

            sDestColumn = sConvert->stack[0].column;
            sDestValue  = sConvert->stack[0].value;

            IDE_TEST( mtv::executeConvert( sConvert,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate)
                      != IDE_SUCCESS);

            if ( ( aData->columnDesc.module->flag & MTD_CANON_MASK )
                 == MTD_CANON_NEED )
            {
                sDestCanonizedValue = sDestValue;

                IDE_TEST( aData->columnDesc.module->canonize( &( aData->columnDesc ),
                                                              & sDestCanonizedValue,
                                                              NULL,
                                                              sDestColumn,
                                                              sDestValue,
                                                              NULL,
                                                              & QC_SHARED_TMPLATE( aStatement )->tmplate )
                          != IDE_SUCCESS );

                sDestValue = sDestCanonizedValue;
            }
            else if ( ( aData->columnDesc.module->flag & MTD_CANON_MASK )
                      == MTD_CANON_NEED_WITH_ALLOCATION )
            {
                IDE_TEST( sMemory->alloc( aData->columnDesc.column.size,
                                          (void**)& sDestCanonizedValue )
                          != IDE_SUCCESS );

                IDE_TEST( aData->columnDesc.module->canonize( &( aData->columnDesc ),
                                                              & sDestCanonizedValue,
                                                              NULL,
                                                              sDestColumn,
                                                              sDestValue,
                                                              NULL,
                                                              & QC_SHARED_TMPLATE( aStatement )->tmplate )
                          != IDE_SUCCESS );

                sDestValue = sDestCanonizedValue;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sDestValue = (void*)mtc::value( &(aData->valueDesc),
                                            aData->value,
                                            MTD_OFFSET_USE );
        }
    } // end if

    sHashVal = aData->columnDesc.module->hash( sHashVal,
                                               &(aData->columnDesc),
                                               sDestValue );

    sStage = 0;
    IDE_TEST( sMemory->setStatus( &sMemStatus ) != IDE_SUCCESS );

    *aHashValue = sHashVal % QMO_RANGE_USING_HASH_MAX_VALUE;

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    if ( sStage == 1 )
    {
        (void)sMemory->setStatus( &sMemStatus );
    }
    return IDE_FAILURE;
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::isIntersectRangeUsingHashPartition( qmsPartCondValList * aMinPartCondVal,
                                                         qmsPartCondValList * aMaxPartCondVal,
                                                         UInt                 aMinHashVal,
                                                         UInt                 aMaxHashVal,
                                                         idBool             * aIsIntersect )
{
    UInt               i            = 0;
    SInt               sOrder       = 0;
    idBool             sIsIntersect = ID_FALSE;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;
    mtdCompareFunc     sCompare;

    sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];
    // min condition check.QMS_PARTCONDVAL_MIN
    // min condition이 원래 partitioned table의 min인 경우 무조건 포함.
    if ( aMinPartCondVal->partCondValType == QMS_PARTCONDVAL_MIN )
    {
        sIsIntersect = ID_TRUE;
    }
    else
    {
        // min partcondval은 default일 수 없음.
        IDE_DASSERT( aMinPartCondVal->partCondValType !=
                     QMS_PARTCONDVAL_DEFAULT );

        // range로 오는 것: LE, LT
        // partcondval의 결과와 range maximum과의 결과는
        // LE인 경우 : range maximum이 partcondval보다 작아야함
        // LT인 경우 : range maximum이 partcondval보다 작거나 같아야함.
        sValueInfo1.column = NULL;
        sValueInfo1.value  = aMinPartCondVal->partCondValues[i];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = (void *)&aMaxHashVal;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sCompare( &sValueInfo1, &sValueInfo2 );

        if ( sOrder > 0 )
        {
            sIsIntersect = ID_FALSE;
        }
        else
        {
            sIsIntersect = ID_TRUE;
        }
    }

    if ( sIsIntersect == ID_TRUE )
    {
        // 위에서 체크했는데 intersect인 경우
        // max condition하고 range minimum하고 체크해봐야 한다.

        // max condition check.
        // max condition이 원래 partitioned table의 max인 경우 무조건 포함.
        if ( aMaxPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
        {
            sIsIntersect = ID_TRUE;
        }
        else
        {
            // max partcondval은 min일 수 없음.
            IDE_DASSERT( aMaxPartCondVal->partCondValType
                         != QMS_PARTCONDVAL_MIN );

            // range로 오는 것 : GE, GT
            // partcondval의 결과와 range minimum과의 결과는
            // GT, GE인 경우 : range minimum이 partcondval보다 같거나 커야 함.

            sValueInfo1.column = NULL;
            sValueInfo1.value  = aMaxPartCondVal->partCondValues[i];
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = (void *)&aMinHashVal;
            sValueInfo2.flag   = MTD_OFFSET_USELESS;

            sOrder = sCompare( &sValueInfo1, &sValueInfo2 );

            if ( sOrder <= 0 )
            {
                sIsIntersect = ID_FALSE;
            }
            else
            {
                sIsIntersect = ID_TRUE;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    *aIsIntersect = sIsIntersect;

    return IDE_SUCCESS;
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::rangeUsinghashPartitionPruningWithKeyRange( qcStatement * aStatement,
                                                                 qmsTableRef * aTableRef,
                                                                 smiRange    * aPartKeyRange )
{
    qmsPartitionRef  * sCurrRef      = NULL;
    qmsPartitionRef  * sPrevRef      = NULL;
    smiRange         * sPartKeyRange = NULL;
    idBool             sIsIntersect  = ID_FALSE;
    UInt               sHashCount    = 0;
    UInt               sMinHashValue[QMO_RANGE_USING_HASH_PRED_MAX];
    UInt               sMaxHashValue[QMO_RANGE_USING_HASH_PRED_MAX];
    UInt               sHashVal     = 0;
    UInt               i            = 0;
    mtkRangeCallBack * sData;

    sPrevRef = NULL;

    if ((( aPartKeyRange->minimum.callback == mtk::rangeCallBackGE4Mtd )
        ||
        ( aPartKeyRange->minimum.callback == mtk::rangeCallBackGE4Stored ) )
       &&
       ( ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLE4Mtd )
        ||
       ( aPartKeyRange->maximum.callback == mtk::rangeCallBackLE4Stored )))
    {
        for ( sPartKeyRange  = aPartKeyRange;
              sPartKeyRange != NULL;
              sPartKeyRange  = sPartKeyRange->next )
        {
            sData = (mtkRangeCallBack*)sPartKeyRange->minimum.data;

            if ( sData->value != NULL )
            {
                IDE_TEST( makeHashKeyForRangePruning( aStatement, sData, &sHashVal )
                          != IDE_SUCCESS );
                sMinHashValue[sHashCount] = sHashVal;
            }
            else
            {
                sMinHashValue[sHashCount] = 0;
            }

            sData = (mtkRangeCallBack*)sPartKeyRange->maximum.data;

            if ( sData->value != NULL )
            {
                IDE_TEST( makeHashKeyForRangePruning( aStatement, sData, &sHashVal )
                          != IDE_SUCCESS );
                sMaxHashValue[sHashCount] = sHashVal;
            }
            else
            {
                sMaxHashValue[sHashCount] = 0;
            }

            sHashCount++;

            IDE_TEST_RAISE( sHashCount >= QMO_RANGE_USING_HASH_PRED_MAX, UNEXPECTED_ERROR1 );
        }
    }
    else
    {
        IDE_RAISE( UNEXPECTED_ERROR2 );
    }

    for ( sCurrRef  = aTableRef->partitionRef;
          sCurrRef != NULL;
          sCurrRef  = sCurrRef->next )
    {
        sIsIntersect = ID_FALSE;

        for ( i = 0; i < sHashCount; i++ )
        {
            IDE_TEST( isIntersectRangeUsingHashPartition( &sCurrRef->minPartCondVal,
                                                          &sCurrRef->maxPartCondVal,
                                                          sMinHashValue[i],
                                                          sMaxHashValue[i],
                                                          &sIsIntersect )
                      != IDE_SUCCESS );

            if ( sIsIntersect == ID_TRUE )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsIntersect == ID_TRUE )
        {
            sPrevRef = sCurrRef;
        }
        else
        {
            // intersect되지 않으면 partitionRef 리스트에서 제거.
            if ( sPrevRef == NULL )
            {
                aTableRef->partitionRef = sCurrRef->next;
            }
            else
            {
                sPrevRef->next = sCurrRef->next;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNEXPECTED_ERROR1 )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qmoPartition::rangeUsinghashPartitionPruningWithKeyRange",
                                "QMO_RANGE_USING_HASH_PRED_MAX over"));
    }
    IDE_EXCEPTION( UNEXPECTED_ERROR2 )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qmoPartition::rangeUsinghashPartitionPruningWithKeyRange",
                                "Callback error"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::rangeUsingHashFilteringWithPartitionFilter( qcStatement            * aStatement,
                                                                 qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                                                 UInt                   * aRangeIntersectCountArray,
                                                                 UInt                     aPartCount,
                                                                 smiRange               * aPartFilter,
                                                                 qmnChildren           ** aSelectedChildrenArea,
                                                                 UInt                   * aSelectedChildrenCount )
{
    mtkRangeCallBack * sData;
    smiRange         * sPartFilter;
    UInt               sPlusCount  = 0;
    UInt               sMinHashVal = 0;
    UInt               sMaxHashVal = 0;
    UInt               i;

    *aSelectedChildrenCount = 0;

    for ( sPartFilter  = aPartFilter;
          sPartFilter != NULL;
          sPartFilter  = sPartFilter->next )
    {
        if ( ( sPartFilter->minimum.callback
               == mtk::rangeCallBackGE4Mtd )
             ||
             ( sPartFilter->minimum.callback
               == mtk::rangeCallBackGE4Stored ) )
        {
            sData = (mtkRangeCallBack*)sPartFilter->minimum.data;

            if ( sData->value != NULL )
            {
                IDE_TEST( makeHashKeyForRangeFilter( aStatement, sData, &sMinHashVal )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            partitionFilterGEUsingHash( aRangeSortedChildrenArray,
                                        aRangeIntersectCountArray,
                                        sMinHashVal,
                                        aPartCount );
        }
        else
        {
            // Nothing to do.
        }

        if ( ( sPartFilter->maximum.callback
               == mtk::rangeCallBackLE4Mtd )
             ||
             ( sPartFilter->maximum.callback
               == mtk::rangeCallBackLE4Stored ) )
        {
            sData = (mtkRangeCallBack*)sPartFilter->maximum.data;

            if ( sData->value != NULL )
            {
                IDE_TEST( makeHashKeyForRangeFilter( aStatement, sData, &sMaxHashVal )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            partitionFilterLEUsingHash( aRangeSortedChildrenArray,
                                        aRangeIntersectCountArray,
                                        sMaxHashVal,
                                        aPartCount );
            sPlusCount ++;
        }
        else
        {
            // Nothing to do.
        }
    }

    for ( i = 0; i < aPartCount; i++ )
    {
        if ( aRangeIntersectCountArray[i] > sPlusCount )
        {
            aSelectedChildrenArea[(*aSelectedChildrenCount)++]
                = aRangeSortedChildrenArray[i].children;
        }
        else
        {
            //Nothing to do.
        }

        aRangeIntersectCountArray[i] = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::comparePartMinRangeMaxUsingHash( qmsPartCondValList * aMinPartCondVal,
                                                    UInt                 aMaxHashVal )
{
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;
    mtdCompareFunc     sCompare;

    if ( aMinPartCondVal->partCondValType == QMS_PARTCONDVAL_MIN )
    {
        sOrder = -1;
    }
    else
    {
        // min partcondval은 default일 수 없음.
        IDE_DASSERT( aMinPartCondVal->partCondValType !=
                     QMS_PARTCONDVAL_DEFAULT );

        sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];

        sValueInfo1.column = NULL;
        sValueInfo1.value  = aMinPartCondVal->partCondValues[0];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = (void *)&aMaxHashVal;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sCompare( &sValueInfo1, &sValueInfo2 );
    }

    return sOrder;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::comparePartMinRangeMinUsingHash( qmsPartCondValList * aMinPartCondVal,
                                                    UInt                 aMinHashVal )
{
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;
    mtdCompareFunc     sCompare;

    if ( aMinPartCondVal->partCondValType == QMS_PARTCONDVAL_MIN )
    {
        sOrder = -1;
    }
    else
    {
        // min partcondval은 default일 수 없음.
        IDE_DASSERT( aMinPartCondVal->partCondValType !=
                     QMS_PARTCONDVAL_DEFAULT );

        sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];

        sValueInfo1.column = NULL;
        sValueInfo1.value  = aMinPartCondVal->partCondValues[0];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = (void *)&aMinHashVal;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sCompare( &sValueInfo1, &sValueInfo2 );
    }

    return sOrder;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::comparePartMaxRangeMinUsingHash( qmsPartCondValList * aMaxPartCondVal,
                                                    UInt                 aMinHashVal )
{
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;
    mtdCompareFunc     sCompare;

    if ( aMaxPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
    {
        sOrder = 1;
    }
    else
    {
        // max partcondval은 min일 수 없음.
        IDE_DASSERT( aMaxPartCondVal->partCondValType
                     != QMS_PARTCONDVAL_MIN );

        sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];

        sValueInfo1.column = NULL;
        sValueInfo1.value  = aMaxPartCondVal->partCondValues[0];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = (void *)&aMinHashVal;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sCompare( &sValueInfo1, &sValueInfo2 );
    }

    return sOrder;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::comparePartMaxRangeMaxUsingHash( qmsPartCondValList * aMaxPartCondVal,
                                                    UInt                 aMaxHashVal )
{
    SInt               sOrder;
    mtdValueInfo       sValueInfo1;
    mtdValueInfo       sValueInfo2;
    mtdCompareFunc     sCompare;

    if ( aMaxPartCondVal->partCondValType == QMS_PARTCONDVAL_DEFAULT )
    {
        sOrder = 1;
    }
    else
    {
        // max partcondval은 min일 수 없음.
        IDE_DASSERT( aMaxPartCondVal->partCondValType
                     != QMS_PARTCONDVAL_MIN );

        sCompare = mtdInteger.logicalCompare[MTD_COMPARE_ASCENDING];

        sValueInfo1.column = NULL;
        sValueInfo1.value  = aMaxPartCondVal->partCondValues[0];
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = NULL;
        sValueInfo2.value  = (void *)&aMaxHashVal;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        sOrder = sCompare( &sValueInfo1, &sValueInfo2 );
    }

    return sOrder;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::comparePartGEUsingHash( qmsPartCondValList * aMinPartCondVal,
                                           qmsPartCondValList * aMaxPartCondVal,
                                           UInt                 aMinHashVal )
{
    SInt sOrder;
    SInt sRes;

    sOrder = comparePartMinRangeMinUsingHash( aMinPartCondVal, aMinHashVal );

    if ( sOrder > 0 )
    {
        sRes = 1;
    }
    else if ( sOrder == 0 )
    {
        sRes = 0;
    }
    else
    {
        sOrder = comparePartMaxRangeMinUsingHash( aMaxPartCondVal, aMinHashVal );

        if ( sOrder > 0 )
        {
            sRes = 0;
        }
        else if ( sOrder == 0 )
        {
            sRes = -1;
        }
        else
        {
            sRes = -1;
        }
    }

    return sRes;
}

/* BUG-46065 support range using hash */
SInt qmoPartition::comparePartLEUsingHash( qmsPartCondValList * aMinPartCondVal,
                                           qmsPartCondValList * aMaxPartCondVal,
                                           UInt                 aMaxHashVal )
{
    SInt sOrder;
    SInt sRes;

    sOrder = comparePartMaxRangeMaxUsingHash( aMaxPartCondVal, aMaxHashVal );

    if ( sOrder < 0 )
    {
        sRes = -1;
    }
    else if ( sOrder == 0 )
    {
        sRes = -1;
    }
    else
    {
        sOrder = comparePartMinRangeMaxUsingHash( aMinPartCondVal, aMaxHashVal );

        if ( sOrder > 0 )
        {
            sRes = 1;
        }
        else if ( sOrder == 0 )
        {
            sRes = 0;
        }
        else
        {
            sRes = 0;
        }
    }

    return sRes;
}

/* BUG-46065 support range using hash */
void qmoPartition::partitionFilterGEUsingHash( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                               UInt                   * aRangeIntersectCountArray,
                                               UInt                     aMinHashVal,
                                               UInt                     aPartCount )
{
    UInt sHigh;
    UInt sLow;
    UInt sMid;
    SInt sRes;

    qmnRangeSortedChildren * sRangeSortedChildrenArray = NULL;

    sHigh = aPartCount -1;
    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        sRangeSortedChildrenArray = aRangeSortedChildrenArray + sMid;

        sRes = comparePartGEUsingHash( &(sRangeSortedChildrenArray->partitionRef->minPartCondVal),
                                       &(sRangeSortedChildrenArray->partitionRef->maxPartCondVal),
                                       aMinHashVal );
        if ( sRes == 0 )
        {
            break;
        }
        else if ( sRes > 0 )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                sHigh = sMid - 1;
            }
        }
        else
        {
            sLow = sMid + 1;
        }
    }

    for ( ; sMid < aPartCount; sMid++ )
    {
        aRangeIntersectCountArray[sMid]++;

    }
}

/* BUG-46065 support range using hash */
void qmoPartition::partitionFilterLEUsingHash( qmnRangeSortedChildren * aRangeSortedChildrenArray,
                                               UInt                   * aRangeIntersectCountArray,
                                               UInt                     aMaxHashVal,
                                               UInt                     aPartCount )
{
    UInt sHigh;
    UInt sLow;
    UInt sMid;
    UInt i;
    SInt sRes;

    qmnRangeSortedChildren * sRangeSortedChildrenArray = NULL;

    sHigh = aPartCount -1;
    for ( sLow = 0, sMid = sHigh >> 1;
          sLow <= sHigh;
          sMid = (sHigh + sLow) >> 1 )
    {
        sRangeSortedChildrenArray = aRangeSortedChildrenArray + sMid;

        sRes = comparePartLEUsingHash( &(sRangeSortedChildrenArray->partitionRef->minPartCondVal),
                                       &(sRangeSortedChildrenArray->partitionRef->maxPartCondVal),
                                       aMaxHashVal );

        if ( sRes == 0 )
        {
            break;
        }
        else if ( sRes > 0 )
        {
            if ( sMid == 0 )
            {
                break;
            }
            else
            {
                sHigh = sMid - 1;
            }
        }
        else
        {
            sLow = sMid + 1;
        }
    }

    for ( i = 0; i <= sMid; i++ )
    {
        aRangeIntersectCountArray[i]++;
    }
}

/* BUG-46065 support range using hash */
IDE_RC qmoPartition::makeHashKeyForRangeFilter( qcStatement        * aStatement,
                                                mtkRangeCallBack   * aData,
                                                UInt               * aHashValue )
{
    iduMemory      * sMemory;
    void           * sSourceValue;
    UInt             sTargetArgCount;
    UInt             sHashVal = mtc::hashInitialValue;
    iduMemoryStatus  sMemStatus;
    mtvConvert     * sConvert;
    mtcColumn      * sDestColumn;
    void           * sDestValue;
    void           * sDestCanonizedValue;
    UInt             sStage = 0;

    sMemory = QC_QMX_MEM( aStatement );

    IDE_TEST( sMemory->getStatus( &sMemStatus ) != IDE_SUCCESS );
    sStage = 1;

    if ( ( aData->compare == mtk::compareMaximumLimit4Mtd ) ||
         ( aData->compare == mtk::compareMaximumLimit4Stored ) )
    {
        sDestValue = aData->columnDesc.module->staticNull;
        // is null인 경우임.
    }
    else
    {
        if ( aData->columnDesc.type.dataTypeId !=
             aData->valueDesc.type.dataTypeId )
        {
            // PROJ-2002 Column Security
            // echar, evarchar는 동일 group이 아니므로 들어올 수 없다.
            IDE_DASSERT(
                (aData->columnDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                (aData->columnDesc.type.dataTypeId != MTD_EVARCHAR_ID) &&
                (aData->valueDesc.type.dataTypeId != MTD_ECHAR_ID) &&
                (aData->valueDesc.type.dataTypeId != MTD_EVARCHAR_ID) );

            sSourceValue = (void*)mtc::value( &(aData->valueDesc),
                                              aData->value,
                                              MTD_OFFSET_USE );

            sTargetArgCount = aData->columnDesc.flag &
                MTC_COLUMN_ARGUMENT_COUNT_MASK;

            IDE_TEST( mtv::estimateConvert4Server( sMemory,
                                                   & sConvert,
                                                   aData->columnDesc.type,
                                                   aData->valueDesc.type,
                                                   sTargetArgCount,
                                                   aData->columnDesc.precision,
                                                   aData->valueDesc.scale,
                                                   & QC_SHARED_TMPLATE( aStatement )->tmplate )
                      != IDE_SUCCESS );

            // source value pointer
            sConvert->stack[sConvert->count].value = sSourceValue;

            sDestColumn = sConvert->stack[0].column;
            sDestValue  = sConvert->stack[0].value;

            IDE_TEST( mtv::executeConvert( sConvert,
                                           & QC_SHARED_TMPLATE( aStatement )->tmplate)
                      != IDE_SUCCESS);

            if ( ( aData->columnDesc.module->flag & MTD_CANON_MASK )
                 == MTD_CANON_NEED )
            {
                sDestCanonizedValue = sDestValue;

                IDE_TEST( aData->columnDesc.module->canonize( &( aData->columnDesc ),
                                                              & sDestCanonizedValue,
                                                              NULL,
                                                              sDestColumn,
                                                              sDestValue,
                                                              NULL,
                                                              & QC_SHARED_TMPLATE( aStatement )->tmplate )
                          != IDE_SUCCESS );

                sDestValue = sDestCanonizedValue;
            }
            else if ( ( aData->columnDesc.module->flag & MTD_CANON_MASK )
                      == MTD_CANON_NEED_WITH_ALLOCATION )
            {
                IDE_TEST( sMemory->alloc( aData->columnDesc.column.size,
                                          (void**)& sDestCanonizedValue )
                          != IDE_SUCCESS );

                IDE_TEST( aData->columnDesc.module->canonize( &( aData->columnDesc ),
                                                              & sDestCanonizedValue,
                                                              NULL,
                                                              sDestColumn,
                                                              sDestValue,
                                                              NULL,
                                                              & QC_SHARED_TMPLATE( aStatement )->tmplate )
                          != IDE_SUCCESS );

                sDestValue = sDestCanonizedValue;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            sDestValue = (void*)mtc::value( &(aData->valueDesc),
                                            aData->value,
                                            MTD_OFFSET_USE );
        }
    } // end if

    sHashVal = aData->columnDesc.module->hash( sHashVal,
                                               &(aData->columnDesc),
                                               sDestValue );

    sStage = 0;
    IDE_TEST( sMemory->setStatus( &sMemStatus ) != IDE_SUCCESS );

    *aHashValue = sHashVal % QMO_RANGE_USING_HASH_MAX_VALUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sMemory->setStatus( &sMemStatus );
    }

    return IDE_FAILURE;
}
