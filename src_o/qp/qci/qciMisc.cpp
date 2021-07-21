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
 

#include <qci.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qdbCommon.h>
#include <qdpRole.h>
#include <qdpPrivilege.h>
#include <qmoPartition.h>
#include <qsf.h>
#include <qcm.h>
#include <qcmUser.h>
#include <qcs.h>
#include <qcuSqlSourceInfo.h>
#include <qcuTemporaryObj.h>
#include <qcsModule.h>
#include <qmsIndexTable.h>
#include <qcmAudit.h>
#include <smiMisc.h>
#include <qcmJob.h>
#include <qdcJob.h>
#include <qcmDictionary.h>
#include <qds.h>
#include <qdnTrigger.h>
#include <qcpUtil.h>
#include <qmn.h>
#include <smiTableSpace.h>
#include <mtl.h>
#include <qdn.h>
#include <qcmProc.h>

smiTransactionalDDLCallback qciMisc::mTransactionalDDLCallback =
{
    qciMisc::backupDDLTargetOldTableInfo,
    qciMisc::backupDDLTargetNewTableInfo,
    qciMisc::removeDDLTargetTableInfo,
    qciMisc::restoreDDLTargetOldTableInfo,
    qciMisc::destroyDDLTargetNewTableInfo
};

UInt qciMisc::getQueryStackSize()
{
    return qcg::getQueryStackSize();
}

// BUG-26017
// [PSM] server restart시 수행되는 psm load과정에서
// 관련프로퍼티를 참조하지 못하는 경우 있음.
UInt qciMisc::getOptimizerMode()
{
    return qcg::getOptimizerMode();
}

UInt qciMisc::getAutoRemoteExec()
{
    return qcg::getAutoRemoteExec();
}

// BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
UInt qciMisc::getOptimizerDefaultTempTbsType()
{
    return qcg::getOptimizerDefaultTempTbsType();
}

idBool qciMisc::isStmtDDL( qciStmtType aStmtType )
{
    return ((aStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DDL) ?
        ID_TRUE : ID_FALSE;
}
idBool qciMisc::isStmtDML( qciStmtType aStmtType )
{
    return ((aStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DML ) ?
        ID_TRUE : ID_FALSE;
}

idBool qciMisc::isStmtDCL( qciStmtType aStmtType )
{
    return ((aStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DCL ) ?
        ID_TRUE : ID_FALSE;
}

idBool qciMisc::isStmtSP( qciStmtType aStmtType )
{
    return ((aStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_SP ) ?
        ID_TRUE : ID_FALSE;
}

idBool qciMisc::isStmtDB( qciStmtType aStmtType )
{
    return ((aStmtType & QCI_STMT_MASK_MASK) == QCI_STMT_MASK_DB ) ?
        ID_TRUE : ID_FALSE;
}

UInt qciMisc::getStmtType( qciStmtType aStmtType )
{
    return ((aStmtType) & QCI_STMT_MASK_MASK );
}

// PROJ-1726
// qcmPerformanceViewManager 를 위한 qci interface
IDE_RC qciMisc::initializePerformanceViewManager()
{
    return qcmPerformanceViewManager::initialize();
}

// PROJ-1726
// qcmPerformanceViewManager 를 위한 qci interface
IDE_RC qciMisc::finalizePerformanceViewManager()
{
    return qcmPerformanceViewManager::finalize();
}

// PROJ-1726
// qcmPerformanceViewManager 를 위한 qci interface
// 동적 모듈에서 performance view 를 런타임 중 추가할 때
// <모듈>im.cpp ::initSystemTable() 에서 사용한다.
IDE_RC qciMisc::addPerformanceView( SChar* aPerfViewStr )
{
    return qcmPerformanceViewManager::add( aPerfViewStr );
}

IDE_RC qciMisc::buildPerformanceView( idvSQL * aStatistics )
{
    return qcg::buildPerformanceView( aStatistics );
}

IDE_RC qciMisc::createDB( idvSQL * aStatistics,
                          SChar  * aDBName,
                          SChar  * aOwnerDN,
                          UInt     aOwnerDNLen )
{
    return qcmCreate::createDB( aStatistics, aDBName, aOwnerDN, aOwnerDNLen );
}

IDE_RC qciMisc::getPartitionInfoList( void                  * aQcStatement,
                                      smiStatement          * aSmiStmt,
                                      iduVarMemList         * aMem,
                                      UInt                    aTableID,
                                      qcmPartitionInfoList ** aPartInfoList )
{
    return qcmPartition::getPartitionInfoList( ( qcStatement *)aQcStatement,
                                               aSmiStmt,
                                               aMem,
                                               aTableID,
                                               aPartInfoList,
                                               ID_TRUE );
}

IDE_RC qciMisc::getPartitionInfoList( void                  * aQcStatement,
                                      smiStatement          * aSmiStmt,
                                      iduMemory             * aMem,
                                      UInt                    aTableID,
                                      qcmPartitionInfoList ** aPartInfoList )
{
    return qcmPartition::getPartitionInfoList( ( qcStatement *)aQcStatement,
                                               aSmiStmt,
                                               aMem,
                                               aTableID,
                                               aPartInfoList,
                                               ID_TRUE );
}

IDE_RC qciMisc::getPartMinMaxValue( smiStatement * aSmiStmt,
                                    UInt           aPartitionID,
                                    SChar        * aMinValue,
                                    SChar        * aMaxValue )
{
    return qcmPartition::getPartMinMaxValue( aSmiStmt,
                                             aPartitionID,
                                             aMinValue,
                                             aMaxValue );
}

IDE_RC qciMisc::getPartitionOrder( smiStatement * aSmiStmt,
                                   UInt           aTableID,
                                   UChar        * aPartitionName,
                                   SInt           aPartitionNameSize,
                                   UInt         * aPartOrder )
{
    return qcmPartition::getPartitionOrder( aSmiStmt,
                                            aTableID,
                                            aPartitionName,
                                            aPartitionNameSize,
                                            aPartOrder );
}

IDE_RC qciMisc::comparePartCondValues( void             * aQcStatement,
                                       qcmTableInfo     * aTableInfo,
                                       SChar            * aValue1,
                                       SChar            * aValue2,
                                       SInt             * aResult )
{
    qmsPartCondValList      sPartCondVal1;
    qmsPartCondValList      sPartCondVal2;
    qcCondValueCharBuffer   sBuffer;
    mtdCharType           * sPartKeyCondValueStr = (mtdCharType*) & sBuffer;
    qcStatement           * sStatement = (qcStatement*)aQcStatement;
    UInt                    i = 0;

    qtc::setVarcharValue( sPartKeyCondValueStr,
                          NULL,
                          (SChar*)aValue1,
                          idlOS::strlen( aValue1) );

    IDE_TEST( qcmPartition::getPartCondVal( sStatement,
                                            aTableInfo->partKeyColumns,
                                            aTableInfo->partitionMethod,
                                            &sPartCondVal1,
                                            NULL, /* aPartCondValStmtText */
                                            NULL, /* aPartCondValNode */
                                            sPartKeyCondValueStr )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sPartKeyCondValueStr,
                          NULL,
                          (SChar*)aValue2,
                          idlOS::strlen( aValue2) );

    IDE_TEST( qcmPartition::getPartCondVal( sStatement,
                                            aTableInfo->partKeyColumns,
                                            aTableInfo->partitionMethod,
                                            &sPartCondVal2,
                                            NULL, /* aPartCondValStmtText */
                                            NULL, /* aPartCondValNode */
                                            sPartKeyCondValueStr )
              != IDE_SUCCESS );

    if ( ( sPartCondVal1.partCondValCount == 0 ) ||
         ( sPartCondVal2.partCondValCount == 0 ) )
    {
        /* BUGBUG : partCondValCount로 판단하는 방법은
         *  BUG-45653 Range/List의 Default Partition에 들어가는 갈 수 있는 값이 달라도, Handshake가 성공합니다.
         * 를 해결하지 못합니다. BUG-45653 처리 이후에 수정해야 합니다.
         */
        if ( ( sPartCondVal1.partCondValCount == 0 ) &&
             ( sPartCondVal2.partCondValCount == 0 ) )
        {
            // PartcondValCount가 0인 경우는 값이 Null인 경우, 즉
            // Default일 경우입니다.
            // 따라서 동일합니다.
            *aResult = 0;
        }
        else if ( sPartCondVal1.partCondValCount == 0 )
        {
            *aResult = -1;
        }
        else
        {
            *aResult = 1;
        }
    }
    else
    {
        switch( aTableInfo->partitionMethod )
        {
            case QCM_PARTITION_METHOD_RANGE:
                {
                    *aResult = qmoPartition::compareRangePartition(
                        aTableInfo->partKeyColumns,
                        &sPartCondVal1,
                        &sPartCondVal2 );
                    break;
                }
            /* BUG-46065 support range using hash */
            case QCM_PARTITION_METHOD_RANGE_USING_HASH:
                {
                    *aResult = qmoPartition::compareRangeUsingHashPartition(
                        &sPartCondVal1,
                        &sPartCondVal2 );
                    break;
                }
            case QCM_PARTITION_METHOD_LIST:
                {
                    *aResult = 0;
                    if ( sPartCondVal1.partCondValCount == sPartCondVal2.partCondValCount )
                    {
                        for ( i = 0; i < sPartCondVal2.partCondValCount; i++ )
                        {
                            if ( qmoPartition::compareListPartition(
                                    aTableInfo->partKeyColumns,
                                    &sPartCondVal1,
                                    sPartCondVal2.partCondValues[i] ) == ID_FALSE )
                            {
                                *aResult = 1;
                                break;
                            }
                            else
                            {
                                /* do nothing */
                            }
                        }
                    }
                    else if ( sPartCondVal1.partCondValCount > sPartCondVal2.partCondValCount )
                    {
                        *aResult = 1;
                    }
                    else
                    {
                        *aResult = -1;
                    }
                    break;
                }
            case QCM_PARTITION_METHOD_NONE:
                // non-partitioned table.
                // Nothing to do.
                break;
            case QCM_PARTITION_METHOD_HASH:
            default:
                IDE_DASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::comparePartCondValues( idvSQL                  * aStatistics,
                                       qcmTableInfo            * aTableInfo,
                                       SChar                   * aValue1,
                                       SChar                   * aValue2,
                                       SInt                    * aResult )
{
    qcStatement             sStatement;
    smiStatement          * sDummySmiStmt;
    smiStatement            sSmiStmt;
    UInt                    sStage = 0;
    UInt                    sSmiStmtFlag  = 0;
    smiTrans                sTrans;

    IDE_DASSERT( aResult != NULL );

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            aStatistics,
                            SMI_ISOLATION_CONSISTENT | 
                            SMI_TRANSACTION_NORMAL |
                            SMI_TRANSACTION_REPL_DEFAULT )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sSmiStmt.begin( aStatistics,
                              sDummySmiStmt,
                              sSmiStmtFlag ) != IDE_SUCCESS);
    sStage = 4;

    IDE_TEST( comparePartCondValues( (void*)&sStatement,
                                     aTableInfo,
                                     aValue1,
                                     aValue2,
                                     aResult )
              != IDE_SUCCESS );

    sStage = 3;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage = 2;
    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sTrans.destroy( aStatistics ) != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 4:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 3:
            (void)sTrans.rollback();
        case 2:
            (void)sTrans.destroy( aStatistics );
        case 1:
            (void) qcg::freeStatement( &sStatement );
    }

    return IDE_FAILURE;
}

IDE_RC qciMisc::comparePartCondValues( smiStatement      * aSmiStatement,
                                       SChar             * aTableName,
                                       SChar             * aUserName,
                                       SChar             * aValue1,
                                       SChar             * aValue2,
                                       ULong               aTimeout,
                                       SInt              * aResult )
{
    smSCN                   sTableSCN;
    void                  * sTableHandle = NULL;
    qcmTableInfo          * sTableInfo = NULL;
    qcStatement             sStatement;
    idBool                  sIsAllocStatement = ID_FALSE;
    UInt                    sUserID = 0;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sIsAllocStatement = ID_TRUE;

    qcg::setSmiStmt( &sStatement, aSmiStatement );

    IDE_TEST(qcmUser::getUserID( &sStatement,
                                 aUserName,
                                 idlOS::strlen( aUserName ),
                                 &sUserID )
             != IDE_SUCCESS);

    IDE_TEST( qcm::getTableHandleByName( aSmiStatement,
                                         sUserID,
                                         (UChar *)aTableName,
                                         idlOS::strlen( (SChar*)aTableName ),
                                         &sTableHandle,
                                         &sTableSCN )
              != IDE_SUCCESS );

    IDE_TEST( smiValidateAndLockObjects( aSmiStatement->getTrans(),
                                         sTableHandle,
                                         sTableSCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IS,
                                         aTimeout,
                                         ID_FALSE )
              != IDE_SUCCESS );
    
    /* PROJ-2446 ONE SOURCE statistics를 인자로 받아 넘기지 않고
     * local의 statement를 넘긴다.확인 */
    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( comparePartCondValues( &sStatement,
                                     sTableInfo,
                                     aValue1,
                                     aValue2,
                                     aResult )
              != IDE_SUCCESS );
    
    sIsAllocStatement = ID_FALSE;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAllocStatement == ID_TRUE )
    {
        sIsAllocStatement = ID_FALSE;
        (void)qcg::freeStatement( &sStatement );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC qciMisc::comparePartCondValues( idvSQL            * aStatistics,
                                       SChar             * aTableName,
                                       SChar             * aUserName,
                                       SChar             * aValue1,
                                       SChar             * aValue2,
                                       SInt              * aResult )
{

    smSCN                   sTableSCN;
    void                  * aTableHandle;
    qcmTableInfo          * sTableInfo;
    qcStatement             sStatement;
    smiStatement          * sDummySmiStmt;
    smiStatement            sSmiStmt;
    UInt                    sStage = 0;
    UInt                    sSmiStmtFlag  = 0;
    smiTrans                sTrans;
    UInt                    sUserID;


    IDE_DASSERT( aResult != NULL );

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    sStage = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            aStatistics,
                            SMI_ISOLATION_CONSISTENT | 
                            SMI_TRANSACTION_NORMAL |
                            SMI_TRANSACTION_REPL_DEFAULT )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sSmiStmt.begin( aStatistics,
                              sDummySmiStmt,
                              sSmiStmtFlag ) != IDE_SUCCESS);
    sStage = 4;

    IDE_TEST(qcmUser::getUserID( &sStatement,
                                 aUserName,
                                 idlOS::strlen( aUserName ),
                                 &sUserID )
             != IDE_SUCCESS);

    IDE_TEST( qcm::getTableHandleByName( &sSmiStmt,
                                         sUserID,
                                         (UChar *)aTableName,
                                         idlOS::strlen( (SChar*)aTableName ),
                                         &aTableHandle,
                                         &sTableSCN )
              != IDE_SUCCESS );

    IDE_TEST( smiValidateAndLockObjects( sSmiStmt.getTrans(),
                                         aTableHandle,
                                         sTableSCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IS,
                                         smiGetDDLLockTimeOut(sSmiStmt.getTrans()),
                                         ID_FALSE )
              != IDE_SUCCESS );
    
    /* PROJ-2446 ONE SOURCE statistics를 인자로 받아 넘기지 않고
     * local의 statement를 넘긴다.확인 */
    IDE_TEST( smiGetTableTempInfo( aTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( comparePartCondValues( aStatistics,
                                     sTableInfo,
                                     aValue1,
                                     aValue2,
                                     aResult )
              != IDE_SUCCESS );

    sStage = 3;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage = 2;
    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sTrans.destroy( aStatistics ) != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 4:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 3:
            (void)sTrans.rollback();
        case 2:
            (void)sTrans.destroy( aStatistics );
        case 1:
            (void) qcg::freeStatement( &sStatement );
    }

    return IDE_FAILURE;
}

IDE_RC qciMisc::getUserIdByName( SChar             * aUserName,
                                 UInt              * aUserID )
{
    idvSQL                  sStatistics;
    qcStatement             sStatement;
    smiStatement          * sDummySmiStmt;
    smiStatement            sSmiStmt;
    UInt                    sStage = 0;
    UInt                    sSmiStmtFlag  = 0;
    smiTrans                sTrans;


    IDE_DASSERT( aUserID != NULL );

    /* initialize smiStatement flag */
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    /* In order to avoid a server crash, the sStatistics needs to be initialized
       since smiTrans::initialize() does not initialize this */
    idlOS::memset( &sStatistics, 0, ID_SIZEOF(idvSQL) );

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    sStage = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            &sStatistics,
                            SMI_ISOLATION_NO_PHANTOM | SMI_TRANSACTION_NORMAL |
                            SMI_TRANSACTION_REPL_DEFAULT )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sSmiStmt.begin( &sStatistics,
                              sDummySmiStmt,
                              sSmiStmtFlag ) != IDE_SUCCESS);
    sStage = 4;

    IDE_TEST(qcmUser::getUserID( &sStatement,
                                 aUserName,
                                 idlOS::strlen( aUserName ),
                                 aUserID )
             != IDE_SUCCESS);

    sStage = 3;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage = 2;
    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sTrans.destroy( &sStatistics ) != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 4:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 3:
            (void)sTrans.rollback();
        case 2:
            (void)sTrans.destroy( &sStatistics );
        case 1:
            (void) qcg::freeStatement( &sStatement );
    }

    return IDE_FAILURE;
}

IDE_RC qciMisc::isEquivalentExpressionString( SChar  * aExprString1,
                                              SChar  * aExprString2,
                                              idBool * aIsEquivalent )
{
    qcStatement          sStatement;
    smiTrans             sTrans;
    smiStatement         sSmiStmt;
    smiStatement       * sDummySmiStmt;
    UInt                 sSmiStmtFlag  = 0;
    UInt                 sStage = 0;

    qtcNode            * sNode1[2];
    qtcNode            * sNode2[2];

    IDE_DASSERT( aIsEquivalent != NULL );

    /* initialize smiStatement flag */
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sStage = 1;

    qcg::setSmiStmt( &sStatement, &sSmiStmt );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            NULL,
                            SMI_ISOLATION_NO_PHANTOM | SMI_TRANSACTION_NORMAL | SMI_TRANSACTION_REPL_DEFAULT )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( sSmiStmt.begin( NULL, sDummySmiStmt, sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage = 4;

    /* make qtcNode */
    IDE_TEST( qcpUtil::makeDefaultExpression( &sStatement,
                                              sNode1,
                                              aExprString1,
                                              idlOS::strlen( aExprString1 ) )
              != IDE_SUCCESS );

    /* adjust expression position */
    sNode1[0]->position.offset = 7; /* "RETURN " */
    sNode1[0]->position.size   = idlOS::strlen( aExprString1 );

    IDE_TEST( qcpUtil::makeDefaultExpression( &sStatement,
                                              sNode2,
                                              aExprString2,
                                              idlOS::strlen( aExprString2 ) )
              != IDE_SUCCESS );

    /* adjust expression position */
    sNode2[0]->position.offset = 7; /* "RETURN " */
    sNode2[0]->position.size   = idlOS::strlen( aExprString2 );

    /* check qtcNode */
    IDE_TEST( qtc::isEquivalentExpressionByName( sNode1[0],
                                                 sNode2[0],
                                                 aIsEquivalent )
              != IDE_SUCCESS );

    sStage = 3;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sStage = 2;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 4 :
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 3 :
            (void)sTrans.rollback();
        case 2 :
            (void)sTrans.destroy( NULL );
        case 1 :
            (void)qcg::freeStatement( &sStatement );
        default :
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qciMisc::getLanguage( SChar * aLanguage, mtlModule ** aModule )
{
    IDE_TEST( mtl::moduleByName( (const mtlModule **) aModule,
                                 aLanguage,
                                 idlOS::strlen(aLanguage) ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::lobRead( idvSQL       * aStatistics,
                         smLobLocator   aLocator,
                         UInt           aOffset,
                         UInt           aMount,
                         UChar        * aPiece )
{
    UInt sDummyReadLength;

    return smiLob::read( aStatistics,
                         aLocator,
                         aOffset,
                         aMount,
                         aPiece,
                         &sDummyReadLength );
}

IDE_RC qciMisc::clobRead( idvSQL       * aStatistics,
                          smLobLocator   aLocator,
                          UInt           aByteOffset,
                          UInt           aByteLength,
                          UInt           aCharLength,
                          UChar        * aBuffer,
                          mtlModule    * aLanguage,
                          UInt         * aReadByteLength,
                          UInt         * aReadCharLength )
{
    UInt      sReadCharCount = 0;
    UChar    *sBufPosition;
    UChar    *sPrevBufPosition;
    UInt      sReadLength;
    mtlNCRet  sNCRet;

    IDE_TEST( smiLob::read( aStatistics,
                            aLocator,
                            aByteOffset,
                            aByteLength,
                            aBuffer,
                            &sReadLength )
              != IDE_SUCCESS );

    sBufPosition = aBuffer;
    sPrevBufPosition = sBufPosition;
    while (1)
    {
        sPrevBufPosition = sBufPosition;

        // BUG-45608 JDBC 4Byte char length
        if ( aLanguage->id == MTL_UTF8_ID )
        {
            sNCRet = mtl::nextCharClobForClient(&sBufPosition,
                                                (UChar*)(aBuffer+sReadLength));
        }
        else
        {   
            sNCRet = aLanguage->nextCharPtr(&sBufPosition,
                                            (UChar*)(aBuffer+sReadLength));
        }
        
        sReadCharCount++;

        if ((UInt)(sBufPosition - aBuffer) >= sReadLength)
        {
            if ( (sNCRet == NC_MB_INCOMPLETED) ||
                 (UInt)(sBufPosition - aBuffer) > sReadLength )
            {
                // 읽을 위치가 버퍼를 벗어났다.
                // 마지막에 읽은 문자를 버린다.
                sReadCharCount--;
                sBufPosition = sPrevBufPosition;
            }
            break;
        }

        if (sReadCharCount >= aCharLength)
        {
            break;
        }
    }

    *aReadByteLength = sBufPosition - aBuffer;
    *aReadCharLength = sReadCharCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Removed aOldSize */
IDE_RC qciMisc::lobPrepare4Write( idvSQL     * aStatistics,
                                  smLobLocator aLocator,
                                  UInt         aOffset,
                                  UInt         aSize)
{
    UInt sInfo;

    if ( (aOffset == 0) && (aSize == 0) )
    {
        IDE_TEST( smiLob::getInfo( aLocator,
                                   & sInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (sInfo & MTC_LOB_COLUMN_NOTNULL_MASK)
                        == MTC_LOB_COLUMN_NOTNULL_TRUE,
                        ERR_NOT_ALLOW_NULL );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( smiLob::prepare4Write( aStatistics,
                                     aLocator,
                                     aOffset,
                                     aSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ALLOW_NULL)
    {
        /* BUG-45680 insert 수행시 not null column에 대한 에러메시지 정보에 column 정보 출력. */
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMX_NOT_NULL_CONSTRAINT,
                                  "",
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Removed aOffset */
IDE_RC qciMisc::lobWrite( idvSQL       * aStatistics,
                          smLobLocator   aLocator,
                          UInt           aPieceLen,
                          UChar        * aPiece )
{
    return smiLob::write( aStatistics,
                          aLocator,
                          aPieceLen,
                          aPiece );
}

IDE_RC qciMisc::lobFinishWrite( idvSQL  * aStatistics,
                                smLobLocator aLocator )
{
    return smiLob::finishWrite( aStatistics,
                                aLocator );
}

IDE_RC qciMisc::lobCopy( idvSQL*      aStatistics,
                         smLobLocator aDestLocator,
                         smLobLocator aSrcLocator )
{
    return smiLob::copy( aStatistics,
                         aDestLocator,
                         aSrcLocator );
}

idBool qciMisc::lobIsOpen( smLobLocator aLocator )
{
    return smiLob::isOpen( aLocator );
}

IDE_RC qciMisc::lobGetLength( idvSQL       * aStatistics,
                              smLobLocator   aLocator,
                              UInt         * aLength,
                              idBool       * aIsNullLob )
{
    SLong   sLength;
    idBool  sIsNullLob = ID_FALSE;

    if ( aLocator != MTD_LOCATOR_NULL )
    {
        IDE_TEST( smiLob::getLength( aStatistics,
                                     aLocator,
                                     &sLength,
                                     &sIsNullLob )
                  != IDE_SUCCESS );

        if ( sIsNullLob == ID_TRUE )
        {
            *aLength = 0;
        }
        else
        {
            IDE_TEST_RAISE( (sLength < 0) || (sLength > ID_UINT_MAX),
                            ERR_LOB_SIZE );

            *aLength = (UInt)sLength;
        }
    }
    else
    {
        *aLength = 0;
    }

    /* PROJ-2728 Sharding LOB */
    if ( aIsNullLob != NULL )
    {
        *aIsNullLob = sIsNullLob;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LOB_SIZE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_STORED_DATA_LENGTH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2047 Strengthening LOB - Added Interfaces */
IDE_RC qciMisc::lobTrim(idvSQL       *aStatistics,
                        smLobLocator  aLocator,
                        UInt          aOffset)
{
    return smiLob::trim(aStatistics,
                        aLocator,
                        aOffset);
}

IDE_RC qciMisc::lobFinalize( idvSQL       * aStatistics,
                             smLobLocator   aLocator )
{
    if ( smiLob::isOpen( aLocator ) == ID_TRUE )
    {
        (void) smiLob::closeLobCursor( aStatistics, aLocator );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;
}

// Stored Procedure Function을 MT에 등록
IDE_RC
qciMisc::addExtFuncModule( void )
{
    IDE_TEST( mtc::addExtFuncModule( (mtfModule**)
                                     qsf::extendedFunctionModules )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::addExtRangeFunc( void )
{
    static smiCallBackFunc   sSmiRangeFuncs[3];
    smiRange               * sDefaultRange;

    IDE_TEST( mtc::addExtRangeFunc( qtc::rangeFuncs )
              != IDE_SUCCESS );

    sDefaultRange = smiGetDefaultKeyRange();

    sSmiRangeFuncs[0] = sDefaultRange->minimum.callback;
    sSmiRangeFuncs[1] = sDefaultRange->maximum.callback;
    sSmiRangeFuncs[2] = NULL;

    IDE_TEST( mtc::addExtRangeFunc( sSmiRangeFuncs )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::addExtCompareFunc( void )
{
    IDE_TEST( mtc::addExtCompareFunc( qtc::compareFuncs )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qciMisc::addExternModuleCallback( qcmExternModule * aExternModule )
{
    IDE_ASSERT( aExternModule != NULL );
    IDE_ASSERT( gExternModule.mCnt < QCM_MAX_EXTERN_MODULE_CNT );

    gExternModule.mExternModule[gExternModule.mCnt] = aExternModule;
    gExternModule.mCnt++;

    return IDE_SUCCESS;
}

IDE_RC
qciMisc::touchTable( smiStatement * aSmiStmt,
                     UInt           aTableID )
{
    return qcm::touchTable( aSmiStmt,
                            aTableID,
                            SMI_TBSLV_DDL_DML );
}

IDE_RC
qciMisc::getTableInfoByID( smiStatement    * aSmiStmt,
                           UInt              aTableID,
                           qcmTableInfo   ** aTableInfo,
                           smSCN           * aSCN,
                           void           ** aTableHandle )
{
    IDE_TEST( qcm::getTableHandleByID( aSmiStmt,
                                       aTableID,
                                       aTableHandle,
                                       aSCN )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( *aTableHandle,
                                   (void**)aTableInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qciMisc::validateAndLockTable( void             * aQcStatement,
                               void             * aTableHandle,
                               smSCN              aSCN,
                               smiTableLockMode   aLockMode )
{
    return qcm::validateAndLockTable( ( qcStatement *)aQcStatement,
                                      aTableHandle,
                                      aSCN,
                                      aLockMode );
}

IDE_RC qciMisc::getUserName(void        * aQcStatement,
                            UInt          aUserID,
                            SChar       * aUserName)
{
    return qcmUser::getUserName( ( qcStatement * )aQcStatement,
                                aUserID,
                                aUserName);
}

IDE_RC
qciMisc::makeAndSetQcmTableInfo(
    smiStatement * aSmiStmt,
    UInt           aTableID,
    smOID          aTableOID,
    const void   * aTableRow )
{
    return qcm::makeAndSetQcmTableInfo( aSmiStmt,
                                        aTableID,
                                        aTableOID,
                                        aTableRow );
}

IDE_RC
qciMisc::makeAndSetQcmPartitionInfo(
    smiStatement * aSmiStmt,
    UInt           aTableID,
    smOID          aTableOID,
    qcmTableInfo * aTableInfo )
{
    return qcmPartition::makeAndSetQcmPartitionInfo( aSmiStmt,
                                                     aTableID,
                                                     aTableOID,
                                                     aTableInfo );
}

void
qciMisc::destroyQcmTableInfo(qcmTableInfo * aInfo)
{
    (void)qcm::destroyQcmTableInfo( aInfo );
}

void
qciMisc::destroyQcmPartitionInfo(qcmTableInfo * aInfo)
{
    (void)qcmPartition::destroyQcmPartitionInfo( aInfo );
}

IDE_RC
qciMisc::getDiskRowSize( void * aTableHandle,
                         UInt * aRowSize )
{
    return qdbCommon::getDiskRowSize( aTableHandle,
                                      aRowSize );
}

IDE_RC
qciMisc::copyMtcColumns( void         * aTableHandle,
                         mtcColumn    * aColumns )
{
    qcmTableInfo * sInfo;
    UInt           i;

    IDE_DASSERT( aTableHandle != NULL );
    IDE_DASSERT( aColumns != NULL );

    IDE_TEST( smiGetTableTempInfo( aTableHandle,
                                   (void**)&sInfo )
              != IDE_SUCCESS );

    for( i = 0; i < sInfo->columnCount; i++ )
    {
        idlOS::memcpy( &aColumns[i],
                       sInfo->columns[i].basicInfo,
                       ID_SIZEOF(mtcColumn) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1594 Volatile TBS */
/* volatile table은 휘발성이므로 서버가 구동될 때마다
   테이블을 초기화해야 한다. 테이블을 초기화할 때
   null row를 구성해야 하는데, 이때 사용되는 함수이다.
   이 함수는 SM에서 callback으로 호출된다. */
IDE_RC qciMisc::makeNullRow(idvSQL        * /* aStatistics */, /* PROJ-2446 */
                            smiColumnList *aColumnList,
                            smiValue      *aNullRow,
                            SChar         *aValueBuffer)
{
    smiValue         *sNullValue;
    smiColumnList    *sCurList;
    mtcColumn        *sColumn;
    const mtdModule  *sModule;

    sNullValue = aNullRow;

    for (sCurList = aColumnList; sCurList != NULL; sCurList = sCurList->next)
    {
        /* table header의 column list에 있는 mtcColumn은
           module, language는 사용할 수 없다.
           함수 포인터는 server start이후 올바른 정보가 아니기 때문이다.
           따라서 sColumn->module은 사용해서는 안된다. */
        sColumn = (mtcColumn*)sCurList->column;
        IDE_TEST(mtd::moduleById(&sModule, sColumn->type.dataTypeId)
                 != IDE_SUCCESS);

        /* volatile table을 만들 당시의 컬럼 모듈이 mtd에 없으면
           더이상 서버 구동을 진행시킬 수 없다.
           이때문에 volatile table에는 external column을 사용할 수 없는
           제약이 생긴다. */
        IDE_ASSERT(sModule != NULL);

        if ((sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
            != SMI_COLUMN_TYPE_FIXED)
        {
            sNullValue->value = NULL;
            sNullValue->length = 0;
        }
        else
        {
            // fix BUG-24115
            sNullValue->value = aValueBuffer + sColumn->column.offset;
            sModule->null(sColumn,
                          (void*)sNullValue->value);
            sNullValue->length = sModule->actualSize(sColumn,
                                                     sNullValue->value);
        }

        /* 다음 컬럼의 value로 이동한다. */
        sNullValue++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1723
IDE_RC qciMisc::writeDDLStmtTextLog( qcStatement   * aStatement,
                                     UInt            aUID,
                                     SChar         * aTableName )
{
    smiDDLStmtMeta   sDDLStmtMeta;

    idlOS::memset(&sDDLStmtMeta, 0, ID_SIZEOF(smiDDLStmtMeta));

    IDE_TEST(getUserName(aStatement, aUID, sDDLStmtMeta.mUserName)
             != IDE_SUCCESS);

    idlOS::memcpy((void *)sDDLStmtMeta.mTableName,
                  (const void *)aTableName,
                  idlOS::strlen(aTableName));

    IDE_TEST( smiWriteDDLStmtTextLog( aStatement->mStatistics,
                                      QC_SMI_STMT( aStatement ),
                                      &sDDLStmtMeta,
                                      aStatement->myPlan->stmtText,
                                      aStatement->myPlan->stmtTextLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// FIX BUG-21895
IDE_RC qciMisc::smiCallbackCheckNeedUndoRecord( smiStatement * /* aSmiStmt */,
                                                void         * aTableHandle,
                                                idBool       * aIsNeed )
{
/*--------------------------------------------------------------------
 * SM에서 호출되는 함수이며,
 * foreignKey와 trigger가 존재하지 않는 경우
 * undo record를 기록하지 않기 위해서
 * 해당 테이블에 foreignKey / trigger 존재유무의 정보를 얻는다.
 * 해당 테이블에 대해 이미 lock이 잡혀있는 상태이므로
 * 별도의 lock을 잡지 않아도 된다.
 * (주의)
 * lock이 잡히지 않은 테이블일경우 이 함수를 호출하기전에
 * 반드시 lock을 잡아야 한다.
 ---------------------------------------------------------------------*/
    qcmTableInfo   * sTableInfo;
    idBool           sNeedUndoRecord = ID_TRUE;

    IDE_TEST( smiGetTableTempInfo( aTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    if ( sTableInfo != NULL )
    {
        if ( sTableInfo->foreignKeyCount == 0  )
        {
            sNeedUndoRecord = ID_FALSE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aIsNeed = sNeedUndoRecord;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1705
IDE_RC qciMisc::storingSize( mtcColumn  * aStoringColumn,
                             mtcColumn  * aValueColumn,
                             void       * aValue,
                             UInt       * aOutStoringSize )
{
    IDE_TEST( qdbCommon::storingSize( aStoringColumn,
                                      aValueColumn,
                                      aValue,
                                      aOutStoringSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1705
IDE_RC qciMisc::mtdValue2StoringValue( mtcColumn  * aStoringColumn,
                                       mtcColumn  * aValueColumn,
                                       void       * aValue,
                                       void      ** aOutStoringValue )
{
    IDE_TEST( qdbCommon::mtdValue2StoringValue( aStoringColumn,
                                                aValueColumn,
                                                aValue,
                                                aOutStoringValue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2002 Column Security
IDE_RC qciMisc::startSecurity( idvSQL  * aStatistics )
{
    IDE_TEST( qcs::initialize( aStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::stopSecurity( void )
{
    IDE_TEST( qcs::finalize() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::getECCPolicyName( SChar  * aECCPolicyName )
{
    IDE_TEST( qcsModule::getECCPolicyName( aECCPolicyName )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::getECCPolicyCode( UChar   * aECCPolicyCode,
                                  UShort  * aECCPolicyCodeSize )
{
    IDE_TEST( qcsModule::getECCPolicyCode( aECCPolicyCode,
                                           aECCPolicyCodeSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::verifyECCPolicyCode( UChar   * aECCPolicyCode,
                                     UShort    aECCPolicyCodeSize,
                                     idBool  * aIsValid )
{
    IDE_TEST( qcsModule::verifyECCPolicyCode( aECCPolicyCode,
                                              aECCPolicyCodeSize,
                                              aIsValid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::getPolicyCode( SChar   * aPolicyName,
                               UChar   * aPolicyCode,
                               UShort  * aPolicyCodeLength )
{
    IDE_TEST( qcsModule::getPolicyCode( aPolicyName,
                                        aPolicyCode,
                                        aPolicyCodeLength )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::verifyPolicyCode( SChar  * aPolicyName,
                                  UChar  * aPolicyCode,
                                  UShort   aPolicyCodeLength,
                                  idBool * aIsValid )
{
    IDE_TEST( qcsModule::verifyPolicyCode( aPolicyName,
                                           aPolicyCode,
                                           aPolicyCodeLength,
                                           aIsValid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2240 */
IDE_RC qciMisc::checkDDLReplicationPriv( void   * aQcStatement )
{
    return qdpRole::checkDDLReplicationPriv( ( qcStatement * )aQcStatement);
}

IDE_RC qciMisc::getUserID( void             * aQcStatement,
                           qciNamePosition    aUserName,
                           UInt             * aUserID)
{
    return qcmUser::getUserID(
            ( qcStatement * )aQcStatement,
            ( qcNamePosition )aUserName,
            aUserID);
}
IDE_RC qciMisc::getUserID( void              *aQcStatement,
                           SChar             *aUserName,
                           UInt               aUserNameSize,
                           UInt              *aUserID)
{
    return  qcmUser::getUserID(
            ( qcStatement * )aQcStatement,
            aUserName,
            aUserNameSize,
            aUserID);

}

IDE_RC qciMisc::getUserID( idvSQL       * aStatistics,
                           smiStatement * aSmiStatement,
                           SChar        * aUserName,
                           UInt           aUserNameSize,
                           UInt         * aUserID )
{
    return  qcmUser::getUserID( aStatistics,
                                aSmiStatement,
                                aUserName,
                                aUserNameSize,
                                aUserID );
}

IDE_RC qciMisc::getTableInfo( void             *aQcStatement,
                              UInt              aUserID,
                              qciNamePosition   aTableName,
                              qcmTableInfo    **aTableInfo,
                              smSCN            *aSCN,
                              void            **aTableHandle)
{
    return qcm::getTableInfo( ( qcStatement * )aQcStatement,
                              aUserID,
                              ( qciNamePosition )aTableName,
                              aTableInfo,
                              aSCN,
                              aTableHandle );
}

IDE_RC qciMisc::getTableInfo( void            *aQcStatement,
                              UInt             aUserID,
                              UChar           *aTableName,
                              SInt             aTableNameSize,
                              qcmTableInfo   **aTableInfo,
                              smSCN           *aSCN,
                              void           **aTableHandle)
{
    return qcm::getTableInfo( ( qcStatement * )aQcStatement,
                              aUserID,
                              aTableName,
                              aTableNameSize,
                              aTableInfo,
                              aSCN,
                              aTableHandle );
}

IDE_RC qciMisc::getTableInfoAndLock(void            *aQcStatement,
                                    SChar           *aUserName,
                                    SChar           *aTableName,
                                    smiTableLockMode aLockMode,
                                    ULong            aTimeout,
                                    qcmTableInfo   **aOutTableInfo,
                                    void           **aOutTableHandle)
{
    return qcm::getTableInfoAndLock((qcStatement*)aQcStatement,
                                    aUserName,
                                    aTableName,
                                    aLockMode,
                                    aTimeout,
                                    aOutTableInfo,
                                    aOutTableHandle);
}

IDE_RC qciMisc::lockTableForDDLValidation(void      * aQcStatement,
                                          void      * aTableHandle,
                                          smSCN       aSCN)
{
    return qcm::lockTableForDDLValidation(
            ( qcStatement * )aQcStatement,
            aTableHandle,
            aSCN);
}

IDE_RC qciMisc::isOperatableReplication( void        * aQcStatement,
                                         qriReplItem * aReplItem,
                                         UInt          aOperatableFlag )
{
    qcuSqlSourceInfo  sqlInfo;

    if( QCM_IS_OPERATABLE_QP_REPL( aOperatableFlag ) != ID_TRUE )
    {
        IDE_RAISE( ERR_REPLICATED_OBJECT_TYPE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPLICATED_OBJECT_TYPE )
    {
        sqlInfo.setSourceInfo( ( qcStatement * )aQcStatement,
                              &aReplItem->localTableName );
        ( void )sqlInfo.init( QCI_QME_MEM( aQcStatement ) );
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QRC_REPLICATED_OBJECT_TYPE,
                sqlInfo.getErrMessage( ) ) );
        ( void )sqlInfo.fini( );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

idBool qciMisc::isTemporaryTable( qcmTableInfo * aTableInfo )
{
    return qcuTemporaryObj::isTemporaryTable( aTableInfo );
}

IDE_RC qciMisc::getPolicyInfo( SChar   * aPolicyName,
                               idBool  * aIsExist,
                               idBool  * aIsSalt,
                               idBool  * aIsEncodeECC )
{
    return qcsModule::getPolicyInfo( aPolicyName,
                                     aIsExist,
                                     aIsSalt,
                                     aIsEncodeECC);
}

void qciMisc::setVarcharValue( mtdCharType * aValue,
                               mtcColumn   * ,
                               SChar       * aString,
                               SInt          aLength )
{
    qtc::setVarcharValue( aValue,
                          NULL,
                          aString,
                          aLength );
}

void qciMisc::makeMetaRangeSingleColumn( qtcMetaRangeColumn  * aRangeColumn,
                                         const mtcColumn     * aColumnDesc,
                                         const void          * aValue,
                                         smiRange            * aRange )
{
    qcm::makeMetaRangeSingleColumn( aRangeColumn,
                                    aColumnDesc,
                                    aValue,
                                    aRange);
}

IDE_RC qciMisc::runDMLforDDL( smiStatement * aSmiStmt,
                              SChar        * aSqlStr,
                              vSLong       * aRowCnt )
{
    return qcg::runDMLforDDL( aSmiStmt,
                              aSqlStr,
                              aRowCnt );
}

IDE_RC qciMisc::runDMLforInternal( smiStatement * aSmiStmt,
                                   SChar        * aSqlStr,
                                   vSLong       * aRowCnt )
{
    return qcg::runDMLforInternal( aSmiStmt,
                                   aSqlStr,
                                   aRowCnt );
}

IDE_RC qciMisc::runSQLforShardMeta( smiStatement * aSmiStmt,
                                    SChar        * aSqlStr,
                                    vSLong       * aRowCnt )
{
    return qcg::runSQLforShardMeta( aSmiStmt,
                                    aSqlStr,
                                    aRowCnt,
                                    NULL );
}

IDE_RC qciMisc::runDCLforInternal( qcStatement  * aStatement,
                                   SChar        * aSqlStr,
                                   void         * aSession )
{
    return qcg::runDCLforInternal( aStatement,
                                   aSqlStr,
                                   aSession );
}

IDE_RC qciMisc::runSelectOneRowforDDL( smiStatement * aSmiStmt,
                                       SChar        * aSqlStr,
                                       void         * aResult,
                                       idBool       * aRecordExist,
                                       idBool         aCalledPWVerifyFunc )
{
    return qcg::runSelectOneRowforDDL( aSmiStmt,
                                       aSqlStr,
                                       aResult,
                                       aRecordExist,
                                       aCalledPWVerifyFunc );   
}

IDE_RC qciMisc::selectCount( smiStatement        * aSmiStmt,
                             const void          * aTable,
                             vSLong              * aSelectedRowCount,  /* OUT */
                             smiCallBack         * aCallback ,
                             smiRange            * aRange ,
                             const void          * aIndex  )
{
    return qcm::selectCount( aSmiStmt,
                             aTable,
                             aSelectedRowCount,  /* OUT */
                             aCallback,
                             aRange,
                             aIndex );
}

IDE_RC qciMisc::sysdate( mtdDateType * aDate )
{
    return  qtc::sysdate( aDate );
}

IDE_RC qciMisc::getTableHandleByID( smiStatement  * aSmiStmt,
                                    UInt            aTableID,
                                    void         ** aTableHandle,
                                    smSCN         * aSCN )
{
    return qcm::getTableHandleByID( aSmiStmt,
                                    aTableID,
                                    aTableHandle,
                                    aSCN,
                                    NULL /* aTableRow */,
                                    ID_FALSE/* aTouchTable */ );
}

IDE_RC qciMisc::getTableHandleByName( smiStatement  * aSmiStmt,
                                      UInt            aUserID,
                                      UChar         * aTableName,
                                      SInt            aTableNameSize,
                                      void         ** aTableHandle,
                                      smSCN         * aSCN )
{
    return qcm::getTableHandleByName( aSmiStmt,
                                      aUserID,
                                      aTableName,
                                      aTableNameSize,
                                      aTableHandle,
                                      aSCN );
}

void qciMisc::makeMetaRangeDoubleColumn(
                        qtcMetaRangeColumn  * aFirstRangeColumn,
                        qtcMetaRangeColumn  * aSecondRangeColumn,
                        const mtcColumn     * aFirstColumnDesc,
                        const void          * aFirstColValue,
                        const mtcColumn     * aSecondColumnDesc,
                        const void          * aSecondColValue,
                        smiRange            * aRange)
{
    qcm::makeMetaRangeDoubleColumn( aFirstRangeColumn,
                                    aSecondRangeColumn,
                                    aFirstColumnDesc,
                                    aFirstColValue,
                                    aSecondColumnDesc,
                                    aSecondColValue,
                                    aRange);
}

void qciMisc::makeMetaRangeTripleColumn( qtcMetaRangeColumn  * aFirstRangeColumn,
                                         qtcMetaRangeColumn  * aSecondRangeColumn,
                                         qtcMetaRangeColumn  * aThirdRangeColumn,
                                         const mtcColumn     * aFirstColumnDesc,
                                         const void          * aFirstColValue,
                                         const mtcColumn     * aSecondColumnDesc,
                                         const void          * aSecondColValue,
                                         const mtcColumn     * aThirdColumnDesc,
                                         const void          * aThirdColValue,
                                         smiRange            * aRange)
{
    qcm::makeMetaRangeTripleColumn( aFirstRangeColumn,
                                    aSecondRangeColumn,
                                    aThirdRangeColumn,
                                    aFirstColumnDesc,
                                    aFirstColValue,
                                    aSecondColumnDesc,
                                    aSecondColValue,
                                    aThirdColumnDesc,
                                    aThirdColValue,
                                    aRange );
}

IDE_RC qciMisc::getSequenceHandleByName( smiStatement     * aSmiStmt,
                                         UInt               aUserID,
                                         UChar            * aSequenceName,
                                         SInt               aSequenceNameSize,
                                         void            ** aSequenceHandle )
{
    return qcm::getSequenceHandleByName( aSmiStmt,
                                         aUserID,
                                         aSequenceName,
                                         aSequenceNameSize,
                                         aSequenceHandle );
}

IDE_RC qciMisc::insertIndexTable4OneRow( smiStatement      * aSmiStmt,
                                         qciIndexTableRef  * aIndexTableRef,
                                         smiValue          * aInsertValue,
                                         smOID               aPartOID,
                                         scGRID              aRowGRID )
{
    return qmsIndexTable::insertIndexTable4OneRow(
        aSmiStmt,
        (qmsIndexTableRef*) aIndexTableRef,
        aInsertValue,
        aPartOID,
        aRowGRID );
}

IDE_RC qciMisc::updateIndexTable4OneRow( smiStatement      * aSmiStmt,
                                         qciIndexTableRef  * aIndexTableRef,
                                         UInt                aUpdateColumnCount,
                                         UInt              * aUpdateColumnID,
                                         smiValue          * aUpdateValue,
                                         void              * aRow,
                                         scGRID              aRowGRID )
{
    return qmsIndexTable::updateIndexTable4OneRow(
        aSmiStmt,
        (qmsIndexTableRef*) aIndexTableRef,
        aUpdateColumnCount,
        aUpdateColumnID,
        aUpdateValue,
        aRow,
        aRowGRID );
}

IDE_RC qciMisc::deleteIndexTable4OneRow( smiStatement      * aSmiStmt,
                                         qmsIndexTableRef  * aIndexTableRef,
                                         void              * aRow,
                                         scGRID              aRowGRID )
{
    return qmsIndexTable::deleteIndexTable4OneRow(
        aSmiStmt,
        aIndexTableRef,
        aRow,
        aRowGRID );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getQcStatement( mtcTemplate  * aTemplate,
                                void        ** aQcStatement )
{
    *aQcStatement = ((qcTemplate *)aTemplate)->stmt;

    return IDE_SUCCESS;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getDatabaseLinkSession( void  * aQcStatement,
                                        void ** aSession )
{
    *aSession = QCG_GET_DATABASE_LINK_SESSION( (qcStatement *)aQcStatement );

    return IDE_SUCCESS;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getTempSpaceId( void * aQcStatement, scSpaceID * aSpaceId )
{
    *aSpaceId = QCG_GET_SESSION_TEMPSPACE_ID( (qcStatement *)aQcStatement );

    return IDE_SUCCESS;
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getSmiStatement( void          * aQcStatement,
                                 smiStatement ** aStatement )
{
    *aStatement = QC_SMI_STMT( (qcStatement *)aQcStatement );

    return IDE_SUCCESS;
}

/*
 * PROJ-1832 New database link
 */
UInt qciMisc::getSessionUserID( void * aQcStatement )
{
    return qcg::getSessionUserID( (qcStatement *)aQcStatement );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getNewDatabaseLinkId( void * aQcStatement,
                                      UInt * aDatabaseLinkId )
{
    return qcmGetNewDatabaseLinkId( (qcStatement *)aQcStatement,
                                    aDatabaseLinkId );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::insertDatabaseLinkItem( void                 * aQcStatement,
                                        qciDatabaseLinksItem * aItem,
                                        idBool                 aPublicFlag )
{
    return qcmDatabaseLinksInsertItem( (qcStatement *)aQcStatement,
                                       aItem,
                                       aPublicFlag );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::deleteDatabaseLinkItem( void  * aQcStatement,
                                        smOID   aLinkOID )
{
    return qcmDatabaseLinksDeleteItem( (qcStatement *) aQcStatement,
                                       aLinkOID );
}

/*
 *
 */
IDE_RC qciMisc::deleteDatabaseLinkItemByUserId( void  * aQcStatement,
                                                UInt    aUserId )
{
    return qcmDatabaseLinksDeleteItemByUserId( (qcStatement *) aQcStatement,
                                               aUserId );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getDatabaseLinkFirstItem( idvSQL                * aStatistics,
                                          qciDatabaseLinksItem ** aFirstItem )
{
    return qcmDatabaseLinksGetFirstItem( aStatistics, aFirstItem );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::getDatabaseLinkNextItem( qciDatabaseLinksItem  * aCurrentItem,
                                         qciDatabaseLinksItem ** aNextItem )
{
    return qcmDatabaseLinksNextItem( aCurrentItem, aNextItem );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::freeDatabaseLinkItems( qciDatabaseLinksItem * aFirstItem )
{
    return qcmDatabaseLinksFreeItems( aFirstItem );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::checkCreateDatabaseLinkPrivilege( void   * aQcStatement,
                                                  idBool   aPublic )
{
    return qdpRole::checkDDLCreateDatabaseLinkPriv(
        (qcStatement *)aQcStatement,
        aPublic );
}

/*
 * PROJ-1832 New database link
 */
IDE_RC qciMisc::checkDropDatabaseLinkPrivilege( void   * aQcStatement,
                                                idBool   aPublic )
{
    return qdpRole::checkDDLDropDatabaseLinkPriv(
        (qcStatement *)aQcStatement,
        aPublic );
}

/*
 * PROJ-1832 New database link
 */
idBool qciMisc::isSysdba( void * aQcStatement )
{
    return qcg::getSessionIsSysdbaUser( (qcStatement *)aQcStatement );
}

IDE_RC qciMisc::getAuditMetaInfo( idBool              * aIsStarted,
                                  qciAuditOperation  ** aObjectOptions,
                                  UInt                * aObjectOptionCount,
                                  qciAuditOperation  ** aUserOptions,
                                  UInt                * aUserOptionCount )
{
    smiTrans      sTrans;
    smiStatement  sSmiStmt;
    smiStatement *sDummySmiStmt;
    UInt          sSmiStmtFlag;
    UInt          sStage = 0;

    sSmiStmtFlag = SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR;

    /* PROJ-2446 ONE SOURCE MM 에서 statistics정보를 넘겨 받아야 한다.
     * 추후 작업 */
    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
    sStage++; //1

    IDE_TEST(sTrans.begin( &sDummySmiStmt, NULL )
             != IDE_SUCCESS);
    sStage++; //2

    IDE_TEST(sSmiStmt.begin( NULL,
                             sDummySmiStmt,
                             sSmiStmtFlag )
             != IDE_SUCCESS);
    sStage++; //3

    IDE_TEST( qcmAudit::getStatus( &sSmiStmt,
                                   aIsStarted )
              != IDE_SUCCESS );

    IDE_TEST( qcmAudit::getAllOptions( &sSmiStmt,
                                       aObjectOptions,
                                       aObjectOptionCount,
                                       aUserOptions,
                                       aUserOptionCount )
              != IDE_SUCCESS );

    sStage--; //2
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage--; //1
    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    sStage--; //0
    IDE_TEST(sTrans.destroy( NULL ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 3:
            IDE_ASSERT(sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);
        case 2:
            IDE_ASSERT(sTrans.commit() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(sTrans.destroy( NULL ) == IDE_SUCCESS);
        default:
            break;
    }

    return IDE_FAILURE;
}

// PROJ-2397 Compressed Column Table Replication
IDE_RC qciMisc::makeDictValueForCompress( smiStatement   * aStatement,
                      void           * aTableHandle,
                          void           * aIndexHeader,
                      smiValue       * aInsertedRow,
                      void           * aOIDValue )
{
    return qcmDictionary::makeDictValueForCompress( aStatement,
                                                    aTableHandle,
                                                    aIndexHeader,
                                                    aInsertedRow,
                                                    aOIDValue );
}

/**
 * PROJ-1438 Job Scheduler
 *
 * 현제 시간에 실행할 Job들을 얻는다.
 */
void qciMisc::getExecJobItems( SInt * aIems,
                               UInt * aCount,
                               UInt   aMaxCount )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt;
    UInt           sSmiStmtFlag;
    UInt           sStage = 0;

    sSmiStmtFlag = SMI_STATEMENT_UNTOUCHABLE | SMI_STATEMENT_MEMORY_CURSOR;

    *aCount = 0;

    /* PROJ-2446 ONE SOURCE MM 에서 statistics정보를 넘겨 받아야 한다.
     * 추후 작업 */
    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );

    sStage++; //1
    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS );

    sStage++; //2
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage++; //3
    IDE_TEST( qcmJob::getExecuteJobItems( &sSmiStmt,
                                          aIems,
                                          aCount,
                                          aMaxCount )
              != IDE_SUCCESS );

    sStage--; //2
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sStage--; //1
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    sStage--; //0
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.commit();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }

    ideLog::log( IDE_JOB_0, "[GET JOB ITEMS : FAILURE] ERR-%05X : %s\n",
                 E_ERROR_CODE(ideGetErrorCode()),
                 ideGetErrorMsg(ideGetErrorCode()));
    return;
}

/**
 * PROJ-1438 Job Scheduler
 *
 *   Job ID를 통해서 Job을 실행한다.
 *
 *   1. Job ID를 통해서 Job의 정보를 획득한다.
 *   2. Job State와 Last Exec Time 을 Update 한다.
 *   3. Job 정보를 통해 Procecure 를 실행시킨다.
 *   4. 실행 결과에 따라 Commit or Rallback 명령을 실행한다.
 *   5. 실행후 Job State와, Execute Count, Error Code를 Update 한다.
 */
void qciMisc::executeJobItem( UInt     aJobThreadIndex,
                              SInt     aJob,
                              void   * aSession )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt;
    UInt           sSmiStmtFlag;
    UInt           sStage       = 0;
    SInt           sExecCount   = -1;
    qcmJobState    sJobState    = QCM_JOB_STATE_NONE;
    UInt           sErrorCode   = 0;
    SChar          sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar          sExecQuery[QDC_JOB_EXEC_QUERY_LEN];
    UInt           sExecQueryLen = 0;

    IDE_DASSERT( aSession != NULL );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;

    /* PROJ-2446 ONE SOURCE MM 에서 statistics정보를 넘겨 받아야 한다.
     * 추후 작업 */
    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );

    sStage++; //1
    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS);

    sStage++; //2
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );

    sStage++; //3

    /* 1. Job ID를 통해서 Job의 정보를 획득한다. */
    IDE_TEST( qcmJob::getJobInfo( &sSmiStmt,
                                  aJob,
                                  sJobName,
                                  sExecQuery,
                                  &sExecQueryLen,
                                  &sExecCount,
                                  &sJobState )
              != IDE_SUCCESS );

    if ( ( sJobState != QCM_JOB_STATE_EXEC ) &&
         ( sExecCount > -1 ) )
    {
        /* 2. Job State와 Last Exec Time 을 Update 한다.*/
        IDE_TEST( qcmJob::updateStateAndLastExecTime( &sSmiStmt,
                                                      aJob )
                  != IDE_SUCCESS );

        sStage--; //2
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sStage--; //1
        IDE_TEST( sTrans.commit()
                  != IDE_SUCCESS );

        sStage--; //0
        IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

        /* 3. Job 정보를 통해 Procecure 를 실행시킨다. */
        if ( qcg::runProcforJob( aJobThreadIndex,
                                 aSession,
                                 aJob,
                                 sExecQuery,
                                 sExecQueryLen,
                                 &sErrorCode )
              == IDE_SUCCESS )
        {
            /* 4. 성공시  Commit을 한다 */
            if ( qci::mSessionCallback.mCommit( aSession, ID_FALSE )
                 != IDE_SUCCESS )
            {
                ideLog::log( IDE_JOB_1, "[JOB THREAD %d][JOB %d : COMMIT FAILURE] ERR-%05X : %s",
                             aJobThreadIndex,
                             aJob,
                             E_ERROR_CODE(ideGetErrorCode()),
                             ideGetErrorMsg(ideGetErrorCode()));
                sErrorCode = ideGetErrorCode();
                
                /* 4. 실패시 Rollback 한다 */
                if ( qci::mSessionCallback.mRollback( aSession, NULL, ID_FALSE )
                     != IDE_SUCCESS )
                {
                    ideLog::log( IDE_JOB_1, "[JOB THREAD %d][JOB %d : COMMIT-ROLLBACK FAILURE] ERR-%05X : %s",
                                 aJobThreadIndex,
                                 aJob,
                                 E_ERROR_CODE(ideGetErrorCode()),
                                 ideGetErrorMsg(ideGetErrorCode()));
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* 4. 실패시 Rollback 한다 */
            if ( qci::mSessionCallback.mRollback( aSession, NULL, ID_FALSE )
                 != IDE_SUCCESS )
            {
                ideLog::log( IDE_JOB_1, "[JOB THREAD %d][JOB %d : ROLLBACK FAILURE] ERR-%05X : %s",
                             aJobThreadIndex,
                             aJob,
                             E_ERROR_CODE(ideGetErrorCode()),
                             ideGetErrorMsg(ideGetErrorCode()));
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
        sStage = 1; //1

        IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL ) != IDE_SUCCESS );
        sStage++; //2
        IDE_TEST( sSmiStmt.begin( NULL,
                                  sDummySmiStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );

        sStage++; //3
        /* 5. 실행후 Job State와, Execute Count, Error Code를 Update 한다. */
        sExecCount++;
        IDE_TEST( qcmJob::updateExecuteResult( &sSmiStmt,
                                               aJob,
                                               sExecCount,
                                               sErrorCode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    sStage--; //2
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sStage--; //1
    IDE_TEST( sTrans.commit()
              != IDE_SUCCESS );

    sStage--; //0
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.commit();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }

    return;
}

IDE_RC qciMisc::executeTempSQL( void  * aMmSession, 
                                SChar * aSQL, 
                                idBool  aIsCommit )
{
    smiTrans     * sTrans = NULL;
    idBool         sIsTransBegin  = ID_FALSE;
    UInt           sErrorCode   = 0;

    sTrans     = qci::mSessionCallback.mGetTransWithBegin( aMmSession );
    IDE_TEST_RAISE( sTrans == NULL, ERR_INVALID_CONDITION );
    sIsTransBegin = ID_TRUE;

    IDE_TEST( qcg::runTempSQL( aMmSession,
                               aSQL,
                               idlOS::strlen(aSQL),
                               &sErrorCode )
              != IDE_SUCCESS );

    if ( aIsCommit == ID_TRUE )
    {
        IDE_TEST_RAISE( qci::mSessionCallback.mCommit( aMmSession, ID_FALSE )
                        != IDE_SUCCESS, COMMIT_FAIL );
    }
    else
    {
        IDE_TEST_RAISE( qci::mSessionCallback.mRollback( aMmSession, NULL, ID_FALSE )
                        != IDE_SUCCESS, ROLLBACK_FAIL );
    }
    sIsTransBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qciMisc::executeTempSQL",
                                "invliad condition"));
    }
    IDE_EXCEPTION(COMMIT_FAIL)
    {
        ideLog::log( IDE_QP_0, "[Temporary SQL: COMMIT FAILURE] ERR-%05X : %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));
    }
    IDE_EXCEPTION(ROLLBACK_FAIL)
    {
        ideLog::log( IDE_QP_0, "[Temporary SQL: ROLLBACK FAILURE] ERR-%05X : %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if ( sIsTransBegin == ID_TRUE )
    {
        (void)qci::mSessionCallback.mRollback( aMmSession, NULL, ID_FALSE );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/* BUG-45783 */
void qciMisc::resetInitialJobState( void )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt;
    UInt           sSmiStmtFlag;
    UInt           sStage = 0;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );

    sStage++; //1
    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS );

    sStage++; //2
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sStage++; //3

    IDE_TEST( qcmJob::updateInitialJobState( &sSmiStmt )
              != IDE_SUCCESS );

    sStage--; //2
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sStage--; //1
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    sStage--; //0
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.commit();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }

    ideLog::log( IDE_QP_0, "[resetInitialJobState : FAILURE] ERR-%05X : %s\n",
                 E_ERROR_CODE(ideGetErrorCode()),
                 ideGetErrorMsg(ideGetErrorCode()));
    return;
}

idBool qciMisc::isExecuteForNatc( void )
{
    idBool sIsNatc;

    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        sIsNatc = ID_FALSE;
    }
    else
    {
        sIsNatc = ID_TRUE;
    }

    return sIsNatc;
}

UInt qciMisc::getConcExecDegreeMax( void )
{
    return QCU_CONC_EXEC_DEGREE_MAX;
}

/* PROJ-2446 ONE SOURCE XDB USE */
idvSQL* qciMisc::getStatistics( mtcTemplate * aTemplate )
{
    qcTemplate * sTmplate    = (qcTemplate*) aTemplate;
    idvSQL     * sStatistics = NULL;

    if ( sTmplate->stmt != NULL )
    {
        sStatistics = sTmplate->stmt->mStatistics;
    }
    else
    {
        // Nothing to do.
    }
    return sStatistics;
}

/* PROJ-2446 ONE SOURCE XDB smiGlobalCallBackList에서 사용 된다.
 * partition meta cache, procedure cache, trigger cache */
IDE_RC qciMisc::makeAndSetQcmTblInfo( smiStatement * aSmiStmt,
                                      UInt           aTableID,
                                      smOID          aTableOID )
{
    return qcm::makeAndSetQcmTblInfoCallback( aSmiStmt,
                                              aTableID,
                                              aTableOID );
}

IDE_RC qciMisc::createProcObjAndInfo( smiStatement * aSmiStmt,
                                      smOID          aProcOID )
{
    return qsxProc::createProcObjAndInfoCallback( aSmiStmt,
                                                  aProcOID );
}

/* PROJ-2446 ONE SOURCE FOR SUPPOTING PACKAGE */
IDE_RC qciMisc::createPkgObjAndInfo( smiStatement * aSmiStmt,
                                     smOID          aPkgOID )
{
    return qsxPkg::createPkgObjAndInfoCallback( aSmiStmt,
                                                aPkgOID );
}

IDE_RC qciMisc::createTriggerCache( void    * aTriggerHandle,
                                    void   ** aTriggerCache )
{
    smOID sTriggerOID;

    sTriggerOID = smiGetTableId( aTriggerHandle );

    return qdnTrigger::allocTriggerCache( aTriggerHandle,
                                          sTriggerOID,
                                          (qdnTriggerCache**)aTriggerCache );
}

/*
 *
 */ 
idvSQL * qciMisc::getStatisticsFromQcStatement( void * aQcStatement )
{
    return ( (qcStatement *)aQcStatement )->mStatistics;
}

// BUG-41030 For called by PSM flag setting
void qciMisc::setPSMFlag( void * aQcStmt, idBool aValue )
{
    ((qcStatement*)aQcStmt)->calledByPSMFlag = aValue;
}

void qciMisc::initSimpleInfo( qcSimpleInfo * aInfo )
{
    aInfo->status.mSavedCurr = NULL;
    aInfo->status.mSavedCursor = 0;
    aInfo->results = NULL;
    aInfo->numRows = 0;
    aInfo->count = 0;
}

idBool qciMisc::isSimpleQuery( qcStatement * aStatement )
{
    qciStmtType    sType;
    qmnPlan      * sPlan;
    qmncSCAN     * sSCAN;
    qmncINST     * sINST;
    qmncUPTE     * sUPTE;
    qmncDETE     * sDETE;
    qmncPCRD     * sPCRD;
    qmnChildren  * sChildren;
    idBool         sIsSimple = ID_FALSE;
    SChar          sHostValues[MTC_TUPLE_COLUMN_ID_MAXIMUM] = { 0, };
    UInt           sHostValueCount = 0;
    UInt           sBindParamCount;
    UInt           i;

    sBindParamCount = qcg::getBindCount( aStatement );

    sType = aStatement->myPlan->parseTree->stmtKind;
    sPlan = aStatement->myPlan->plan;

    IDE_TEST_CONT( checkExecFast( aStatement, sPlan ) == ID_FALSE,
                   NORMAL_EXIT );

    switch ( sType )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
        case QCI_STMT_DEQUEUE:
            sIsSimple = isSimpleSelectQuery( sPlan,
                                             sHostValues,
                                             &sHostValueCount);
            break;
        case QCI_STMT_INSERT:
        case QCI_STMT_ENQUEUE:
            IDE_TEST_CONT( sPlan->type != QMN_INST,
                           NORMAL_EXIT );

            sINST = (qmncINST*)sPlan;

            IDE_TEST_CONT( sINST->isSimple == ID_FALSE,
                           NORMAL_EXIT );

            buildSimpleHostValues( sHostValues,
                                   &sHostValueCount,
                                   sINST->simpleValues,
                                   sINST->simpleValueCount );

            sIsSimple = ID_TRUE;
            break;
        case QCI_STMT_UPDATE:
            IDE_TEST_CONT( sPlan->type != QMN_UPTE,
                           NORMAL_EXIT );
            sUPTE = (qmncUPTE*)sPlan;

            IDE_TEST_CONT( sUPTE->isSimple == ID_FALSE, NORMAL_EXIT );

            if ( sPlan->left->type == QMN_SCAN )
            {
                sSCAN = (qmncSCAN*)sPlan->left;

                IDE_TEST_CONT( sSCAN->isSimple == ID_FALSE, NORMAL_EXIT );

                buildSimpleHostValues( sHostValues,
                                       &sHostValueCount,
                                       sUPTE->simpleValues,
                                       sUPTE->updateColumnCount );

                buildSimpleHostValues( sHostValues,
                                       &sHostValueCount,
                                       sSCAN->simpleValues,
                                       sSCAN->simpleValueCount );

                sIsSimple = ID_TRUE;
            }
            else if ( sPlan->left->type == QMN_PCRD )
            {
                sPCRD = (qmncPCRD*)sPlan->left;

                IDE_TEST_CONT( sPCRD->mIsSimple == ID_FALSE, NORMAL_EXIT );

                buildSimpleHostValues( sHostValues,
                                       &sHostValueCount,
                                       sUPTE->simpleValues,
                                       sUPTE->updateColumnCount );
                for ( sChildren = sPCRD->plan.children;
                      sChildren != NULL;
                      sChildren = sChildren->next )
                {
                    sSCAN = (qmncSCAN*)sChildren->childPlan;

                    IDE_TEST_CONT( sSCAN->isSimple == ID_FALSE, NORMAL_EXIT );

                }

                if ( sPCRD->plan.children != NULL )
                {
                    sSCAN = (qmncSCAN *)sPCRD->plan.children->childPlan;
                    buildSimpleHostValues( sHostValues,
                                           &sHostValueCount,
                                           sSCAN->simpleValues,
                                           sSCAN->simpleValueCount );
                }
                else
                {
                    /* Nothing to do */
                }
                sIsSimple = ID_TRUE;
            }
            else
            {
                IDE_CONT( NORMAL_EXIT );
            }
            break;
        case QCI_STMT_DELETE:
            IDE_TEST_CONT( sPlan->type != QMN_DETE,
                           NORMAL_EXIT );

            sDETE = (qmncDETE*)sPlan;

            IDE_TEST_CONT( sDETE->isSimple == ID_FALSE, NORMAL_EXIT );

            if ( sPlan->left->type == QMN_SCAN )
            {
                sSCAN = (qmncSCAN*)sPlan->left;

                IDE_TEST_CONT( sSCAN->isSimple == ID_FALSE, NORMAL_EXIT );

                buildSimpleHostValues( sHostValues,
                                       &sHostValueCount,
                                       sSCAN->simpleValues,
                                       sSCAN->simpleValueCount );
                sIsSimple = ID_TRUE;
            }
            else if ( sPlan->left->type == QMN_PCRD )
            {
                sPCRD = (qmncPCRD*)sPlan->left;

                IDE_TEST_CONT( sPCRD->mIsSimple == ID_FALSE, NORMAL_EXIT );

                for ( sChildren = sPCRD->plan.children;
                      sChildren != NULL;
                      sChildren = sChildren->next )
                {
                    sSCAN = (qmncSCAN*)sChildren->childPlan;
                    IDE_TEST_CONT( sSCAN->isSimple == ID_FALSE, NORMAL_EXIT );
                }

                if ( sPCRD->plan.children != NULL )
                {
                    sSCAN = (qmncSCAN *)sPCRD->plan.children->childPlan;
                    buildSimpleHostValues( sHostValues,
                                           &sHostValueCount,
                                           sSCAN->simpleValues,
                                           sSCAN->simpleValueCount );
                }
                else
                {
                    /* Nothing to do */
                }
                sIsSimple = ID_TRUE;
            }
            else
            {
                IDE_CONT( NORMAL_EXIT );
            }
            break;
        default:
            break;
    }

    IDE_TEST_CONT( sIsSimple == ID_FALSE, NORMAL_EXIT );

    // bind param 갯수가 같고 모두 사용해야 한다.
    if ( sBindParamCount == sHostValueCount )
    {
        for ( i = 0; i < sBindParamCount; i++ )
        {
            if ( sHostValues[i] == 0 )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        sIsSimple = ID_FALSE;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sIsSimple;
}

idBool qciMisc::isSimpleSelectQuery( qmnPlan     * aPlan,
                                     SChar       * aHostValues,
                                     UInt        * aHostValueCount )
{
    qmnPlan     * sPlan = aPlan;
    qmncPROJ    * sPROJ;
    qmncSCAN    * sSCAN;
    qmncJOIN    * sJOIN;
    qmncPCRD    * sPCRD;
    qmnChildren * sChildren;        // multi children node

    idBool       sIsSimple = ID_FALSE;

    IDE_TEST_CONT( sPlan->type != QMN_PROJ,
                   NORMAL_EXIT );
    IDE_TEST_CONT( ( sPlan->left->type != QMN_SCAN ) &&
                   ( sPlan->left->type != QMN_JOIN ) &&
                   ( sPlan->left->type != QMN_PCRD ),
                   NORMAL_EXIT );

    if ( sPlan->left->type == QMN_SCAN )
    {
        sPROJ = (qmncPROJ*)sPlan;
        sSCAN = (qmncSCAN*)sPlan->left;

        IDE_TEST_CONT( ( sPROJ->isSimple == ID_FALSE ) ||
                       ( sSCAN->isSimple == ID_FALSE ),
                       NORMAL_EXIT );

        buildSimpleHostValues( aHostValues,
                               aHostValueCount,
                               sSCAN->simpleValues,
                               sSCAN->simpleValueCount );

        sIsSimple = ID_TRUE;
    }
    else if ( sPlan->left->type == QMN_JOIN )
    {
        /********************************
         *         proj
         *           |
         *         join
         *          / \
         *         /   \
         *       join  scan
         *        /  \
         *       /    \
         *     join  scan
         *      /  \
         *     /    \
         *   scan  scan
         *********************************/

        sPROJ = (qmncPROJ*)sPlan;

        IDE_TEST_CONT( sPROJ->isSimple == ID_FALSE,
                       NORMAL_EXIT );

        sIsSimple = ID_TRUE;

        sPlan = sPlan->left;

        while ( 1 )
        {
            // 오른쪽은 항상 SCAN이어야 한다.
            if ( sPlan->right->type == QMN_SCAN )
            {
                // Nothing to do.
            }
            else
            {
                sIsSimple = ID_FALSE;
                break;
            }

            sJOIN = (qmncJOIN*)sPlan;
            sSCAN = (qmncSCAN*)sPlan->right;

            // simple이어야 한다.
            if ( ( sJOIN->isSimple == ID_FALSE ) ||
                 ( sSCAN->isSimple == ID_FALSE ) )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }

            buildSimpleHostValues( aHostValues,
                                   aHostValueCount,
                                   sSCAN->simpleValues,
                                   sSCAN->simpleValueCount );

            // 왼쪽은 JOIN이거나 SCAN
            if ( sPlan->left->type == QMN_JOIN )
            {
                sPlan = sPlan->left;
            }
            else if ( sPlan->left->type == QMN_SCAN )
            {
                sSCAN = (qmncSCAN*)sPlan->left;

                if ( sSCAN->isSimple == ID_FALSE )
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                buildSimpleHostValues( aHostValues,
                                       aHostValueCount,
                                       sSCAN->simpleValues,
                                       sSCAN->simpleValueCount );

                // 마지막 scan
                break;
            }
            else
            {
                sIsSimple = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        sPROJ = (qmncPROJ*)sPlan;
        sPCRD = (qmncPCRD*)sPlan->left;

        IDE_TEST_CONT( ( sPROJ->isSimple == ID_FALSE ) ||
                       ( sPCRD->mIsSimple == ID_FALSE ),
                       NORMAL_EXIT );

        sIsSimple = ID_TRUE;
        for ( sChildren = sPCRD->plan.children;
              sChildren != NULL;
              sChildren = sChildren->next )
        {
            sSCAN = (qmncSCAN*)sChildren->childPlan;

            if ( sSCAN->isSimple == ID_FALSE )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        
        if ( ( sIsSimple == ID_TRUE ) &&
             ( sPCRD->plan.children != NULL ) )
        {
            sSCAN = (qmncSCAN *)sPCRD->plan.children->childPlan;
            buildSimpleHostValues( aHostValues,
                                   aHostValueCount,
                                   sSCAN->simpleValues,
                                   sSCAN->simpleValueCount );
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    return sIsSimple;
}

idBool qciMisc::isSimpleBind( qcStatement * aStatement )
{
    qciStmtType        sType;
    qmnPlan          * sPlan;
    qmncSCAN         * sSCAN;
    qmncINST         * sINST;
    qmncUPTE         * sUPTE;
    qmncPCRD         * sPCRD;
    qmnChildren      * sChildren;
    idBool             sIsSimple = ID_TRUE;

    sType = aStatement->myPlan->parseTree->stmtKind;
    sPlan = aStatement->myPlan->plan;

    switch ( sType )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
        case QCI_STMT_DEQUEUE:
            sIsSimple = isSimpleSelectBind( aStatement, sPlan );
            break;

        case QCI_STMT_INSERT:
        case QCI_STMT_ENQUEUE:
            sINST = (qmncINST*)sPlan;

            if ( checkSimpleBind( aStatement->pBindParam,
                                  sINST->simpleValues,
                                  sINST->simpleValueCount ) == ID_FALSE )
            {
                sIsSimple = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }

            break;

        case QCI_STMT_UPDATE:
            sUPTE = (qmncUPTE*)sPlan;
            if ( checkSimpleBind( aStatement->pBindParam,
                                  sUPTE->simpleValues,
                                  sUPTE->updateColumnCount ) == ID_FALSE )
            {
                sIsSimple = ID_FALSE;
            }
            else
            {
                if ( sPlan->left->type == QMN_SCAN )
                {
                    sSCAN = (qmncSCAN*)sPlan->left;

                    if ( checkSimpleBind( aStatement->pBindParam,
                                          sSCAN->simpleValues,
                                          sSCAN->simpleValueCount ) == ID_FALSE )
                    {
                        sIsSimple = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else if ( sPlan->left->type == QMN_PCRD )
                {
                    sPCRD = (qmncPCRD *)sPlan->left;

                    for ( sChildren = sPCRD->plan.children;
                          sChildren != NULL;
                          sChildren = sChildren->next )
                    {
                        sSCAN = (qmncSCAN *)sChildren->childPlan;
                        if ( checkSimpleBind( aStatement->pBindParam,
                                              sSCAN->simpleValues,
                                              sSCAN->simpleValueCount ) == ID_FALSE )
                        {
                            sIsSimple = ID_FALSE;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                }
                else
                {
                    sIsSimple = ID_FALSE;
                }
            }
            break;
        case QCI_STMT_DELETE:
            if ( sPlan->left->type == QMN_SCAN )
            {
                sSCAN = (qmncSCAN*)sPlan->left;

                if ( checkSimpleBind( aStatement->pBindParam,
                                      sSCAN->simpleValues,
                                      sSCAN->simpleValueCount ) == ID_FALSE )
                {
                    sIsSimple = ID_FALSE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else if ( sPlan->left->type == QMN_PCRD )
            {
                sPCRD = (qmncPCRD *)sPlan->left;

                for ( sChildren = sPCRD->plan.children;
                      sChildren != NULL;
                      sChildren = sChildren->next )
                {
                    sSCAN = (qmncSCAN *)sChildren->childPlan;

                    if ( checkSimpleBind( aStatement->pBindParam,
                                          sSCAN->simpleValues,
                                          sSCAN->simpleValueCount ) == ID_FALSE )
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            else
            {
                sIsSimple = ID_FALSE;
            }
            break;
        default:
            sIsSimple = ID_FALSE;
            break;
    }

    return sIsSimple;
}

idBool qciMisc::isSimpleSelectBind( qcStatement * aStatement,
                                    qmnPlan     * aPlan )
{
    qmnPlan     * sPlan = aPlan;
    qmncSCAN    * sSCAN;
    qmncPCRD    * sPCRD;
    qmnChildren * sChildren;        // multi children node
    idBool        sIsSimple = ID_TRUE;

    if ( sPlan->left->type == QMN_SCAN )
    {
        sSCAN = (qmncSCAN*)sPlan->left;

        if ( checkSimpleBind( aStatement->pBindParam,
                              sSCAN->simpleValues,
                              sSCAN->simpleValueCount ) == ID_FALSE )
        {
            sIsSimple = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else if ( sPlan->left->type == QMN_JOIN )
    {
        sPlan = sPlan->left;

        while ( 1 )
        {
            // 오른쪽은 항상 SCAN
            IDE_DASSERT( sPlan->right->type == QMN_SCAN );

            sSCAN = (qmncSCAN*)sPlan->right;

            if ( checkSimpleBind( aStatement->pBindParam,
                                  sSCAN->simpleValues,
                                  sSCAN->simpleValueCount ) == ID_FALSE )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }

            // 왼쪽은 JOIN이거나 SCAN
            if ( sPlan->left->type == QMN_JOIN )
            {
                sPlan = sPlan->left;
            }
            else if ( sPlan->left->type == QMN_SCAN )
            {
                sSCAN = (qmncSCAN*)sPlan->left;

                if ( checkSimpleBind( aStatement->pBindParam,
                                      sSCAN->simpleValues,
                                      sSCAN->simpleValueCount ) == ID_FALSE )
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                break;
            }
            else
            {
                break;
            }
        }
    }
    else if ( sPlan->left->type == QMN_PCRD )
    {
        sPCRD = (qmncPCRD *)sPlan->left;

        for ( sChildren = sPCRD->plan.children;
              sChildren != NULL;
              sChildren = sChildren->next )
        {
            sSCAN = (qmncSCAN *)sChildren->childPlan;

            if ( checkSimpleBind( aStatement->pBindParam,
                                  sSCAN->simpleValues,
                                  sSCAN->simpleValueCount ) == ID_FALSE )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        sIsSimple = ID_FALSE;
    }

    return sIsSimple;
}

void qciMisc::setSimpleFlag( qcStatement * aStatement )
{
    if ( qciMisc::isSimpleQuery( aStatement ) == ID_TRUE )
    {
        aStatement->mFlag &= ~QC_STMT_FAST_EXEC_MASK;
        aStatement->mFlag |= QC_STMT_FAST_EXEC_TRUE;
    }
    else
    {
        aStatement->mFlag &= ~QC_STMT_FAST_EXEC_MASK;
        aStatement->mFlag |= QC_STMT_FAST_EXEC_FALSE;
    }
}

void qciMisc::setSimpleBindFlag( qcStatement * aStatement )
{
    if ( ( ( aStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
           == QC_STMT_FAST_EXEC_TRUE ) &&
         /* BUG-45307 Begin Snapshot 이후에는 FAST Execute를 하지 않는다 */
         ( aStatement->mInplaceUpdateDisableFlag == ID_FALSE ) )
    {
        if ( qciMisc::isSimpleBind( aStatement ) == ID_TRUE )
        {
            aStatement->mFlag &= ~QC_STMT_FAST_BIND_MASK;
            aStatement->mFlag |= QC_STMT_FAST_BIND_TRUE;
        }
        else
        {
            aStatement->mFlag &= ~QC_STMT_FAST_BIND_MASK;
            aStatement->mFlag |= QC_STMT_FAST_BIND_FALSE;
        }
    }
    else
    {
        aStatement->mFlag &= ~QC_STMT_FAST_EXEC_MASK;
        aStatement->mFlag |= QC_STMT_FAST_EXEC_FALSE;
    }
}

void qciMisc::buildSimpleHostValues( SChar         * aHostValues,
                                     UInt          * aHostValueCount,
                                     qmnValueInfo  * aSimpleValues,
                                     UInt            aSimpleValueCount )
{
    qmnValueInfo * sValueInfo = aSimpleValues;
    UInt           i;
    
    for ( i = 0; i < aSimpleValueCount; i++ )
    {
        if ( sValueInfo->type == QMN_VALUE_TYPE_HOST_VALUE )
        {
            aHostValues[sValueInfo->value.id] = 1;
            (*aHostValueCount)++;
        }
        else
        {
            // Nothing to do.
        }
        
        sValueInfo++;
    }
}

idBool qciMisc::checkSimpleBind( qciBindParamInfo * aBindParam,
                                 qmnValueInfo     * aSimpleValues,
                                 UInt               aSimpleValueCount )
{
    qciBindParamInfo * sBindParam;
    qmnValueInfo     * sValueInfo = aSimpleValues;
    idBool             sIsSimple = ID_TRUE;
    UInt               i;

    for ( i = 0; i < aSimpleValueCount; i++ )
    {
        if ( sValueInfo->type == QMN_VALUE_TYPE_HOST_VALUE )
        {
            if ( aBindParam == NULL )
            {
                sIsSimple = ID_FALSE;
                break;
            }
            else
            {
                // Nothing to do.
            }
            
            sBindParam = &(aBindParam[sValueInfo->value.id]);
                
            if ( ( sValueInfo->value.id == sBindParam->param.id ) &&
                 ( sBindParam->isParamInfoBound == ID_TRUE ) &&
                 ( sBindParam->isParamDataBound == ID_TRUE ) )
            {
                if ( ( sValueInfo->column.module->id == MTD_SMALLINT_ID ) ||
                     ( sValueInfo->column.module->id == MTD_INTEGER_ID ) ||
                     ( sValueInfo->column.module->id == MTD_BIGINT_ID ) )
                {
                    if ( ( sBindParam->param.type == MTD_SMALLINT_ID ) ||
                         ( sBindParam->param.type == MTD_INTEGER_ID ) ||
                         ( sBindParam->param.type == MTD_BIGINT_ID ) ||
                         ( sBindParam->param.type == MTD_NUMERIC_ID ) ||
                         ( sBindParam->param.type == MTD_FLOAT_ID ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else if ( sValueInfo->column.module->id == MTD_CHAR_ID )
                {
                    // char compare로는 char type만 가능하다.
                    // (물론 compare함수를 varchar compare로 바꾸면 되긴한다.)
                    if ( sBindParam->param.type == MTD_CHAR_ID )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else if ( sValueInfo->column.module->id == MTD_VARCHAR_ID )
                {
                    if ( ( sBindParam->param.type == MTD_CHAR_ID ) ||
                         ( sBindParam->param.type == MTD_VARCHAR_ID ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else if ( ( sValueInfo->column.module->id == MTD_NUMERIC_ID ) ||
                          ( sValueInfo->column.module->id == MTD_FLOAT_ID ) )
                {
                    if ( ( sBindParam->param.type == MTD_SMALLINT_ID ) ||
                         ( sBindParam->param.type == MTD_INTEGER_ID ) ||
                         ( sBindParam->param.type == MTD_BIGINT_ID ) ||
                         ( sBindParam->param.type == MTD_CHAR_ID ) ||
                         ( sBindParam->param.type == MTD_VARCHAR_ID ) ||
                         ( sBindParam->param.type == MTD_NUMERIC_ID ) ||
                         ( sBindParam->param.type == MTD_FLOAT_ID ) ||
                         ( sBindParam->param.type == MTD_DOUBLE_ID ) )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else if ( sValueInfo->column.module->id == MTD_DATE_ID )
                {
                    if ( sBindParam->param.type == MTD_DATE_ID )
                    {
                        // Nothing to do.
                    }
                    else
                    {
                        sIsSimple = ID_FALSE;
                        break;
                    }
                }
                else
                {
                    sIsSimple = ID_FALSE;
                    break;
                }
            }
            else
            {
                sIsSimple = ID_FALSE;
                break;
            }
        }
        else
        {
            // Nothing to do.
        }
        
        sValueInfo++;
    }

    return sIsSimple;
}

/* BUG-41615 simple query hint
 *
 *   property + hint => exec_fast
 *     on       none       on
 *     on       on         on
 *     on       off        off
 *     off      none       off
 *     off      on         on
 *     off      off        off
 */
idBool qciMisc::checkExecFast( qcStatement  * aStatement, qmnPlan * aPlan )
{
    idBool   sExecFast = ID_FALSE;
    UInt     sLevel = 0;

    if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_EXEC_FAST_MASK )
         == QC_TMP_EXEC_FAST_NONE )
    {
        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_EXECUTOR_FAST_SIMPLE_QUERY );

        sLevel = QCU_EXECUTOR_FAST_SIMPLE_QUERY;

        if ( sLevel > 0 )
        {
            sExecFast = ID_TRUE;

            IDE_TEST_CONT( aPlan == NULL, NORMAL_EXIT );
            IDE_TEST_CONT( aPlan->left == NULL, NORMAL_EXIT );

            if ( ( aPlan->left->type == QMN_PCRD ) &&
                 ( sLevel < QCI_SIMPLE_LEVEL_PARTITION ) )
            {
                sExecFast = ID_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            sExecFast = ID_FALSE;
        }
    }
    else
    {
        if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_EXEC_FAST_MASK )
             == QC_TMP_EXEC_FAST_TRUE )
        {
            sExecFast = ID_TRUE;
        }
        else
        {
            sExecFast = ID_FALSE;
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return sExecFast;
}

IDE_RC qciMisc::checkRunningEagerReplicationByTableOID( qcStatement  * aStatement,
                                                        smOID        * aReplicatedTableOIDArray,
                                                        UInt           aReplicatedTableOIDCount )
{
    idBool      sEagerExist = ID_FALSE;

    IDE_TEST( qci::mManageReplicationCallback.mIsRunningEagerSenderByTableOID( QC_SMI_STMT(aStatement),
                                                                               aStatement->mStatistics,
                                                                               aReplicatedTableOIDArray,
                                                                               aReplicatedTableOIDCount,
                                                                               &sEagerExist )
              != IDE_SUCCESS);
    IDE_TEST_RAISE( sEagerExist == ID_TRUE, ERR_REPLICATION_DDL_EAGER_MODE );

    IDE_TEST( qci::mManageReplicationCallback.mIsRunningEagerReceiverByTableOID( QC_SMI_STMT(aStatement),
                                                                                 aStatement->mStatistics,
                                                                                 aReplicatedTableOIDArray,
                                                                                 aReplicatedTableOIDCount,
                                                                                 &sEagerExist )
              != IDE_SUCCESS);
    IDE_TEST_RAISE( sEagerExist == ID_TRUE, ERR_REPLICATION_DDL_EAGER_MODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPLICATION_DDL_EAGER_MODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPLICATION_DDL_EAGER_MODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::writeTableMetaLogForReplication( qcStatement    * aStatement,
                                                 smOID          * aOldTableOID,
                                                 smOID          * aNewTableOID,
                                                 UInt             aTableCount )
{
    UInt                      i = 0;

    for ( i = 0; i < aTableCount; i++ )
    {
        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog( aStatement,
                                                                      aOldTableOID[i],
                                                                      aNewTableOID[i] )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-43605 [mt] random함수의 seed 설정을 session 단위로 변경해야 합니다. */
void qciMisc::initRandomInfo( qcRandomInfo * aRandomInfo )
{
    aRandomInfo->mExistSeed = ID_FALSE;
    aRandomInfo->mIndex     = 0;
}

/* PROJ-2626 Snapshot Export */
void qciMisc::getMemMaxAndUsedSize( ULong * aMaxSize, ULong * aUsedSize )
{
    smiGetMemUsedSize( aUsedSize );
    *aMaxSize = QCU_MEM_MAX_DB_SIZE;
}

IDE_RC qciMisc::getDiskUndoMaxAndUsedSize( ULong * aMaxSize, ULong * aUsedSize )
{
    IDE_TEST( smiGetDiskUndoUsedSize( aUsedSize ) != IDE_SUCCESS );

    *aMaxSize = smiTableSpace::getSysUndoFileMaxSize();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qciMisc::checkRollbackAbleDDLEnable( smiTrans * aTrans, 
                                            smOID      aTableOID,
                                            idBool     aCallByRepl )
{
    const void   * sTable        = NULL;
    qciTableInfo * sTableInfo    = NULL;
    qciTableInfo * sPartInfo     = NULL;
    qcmIndex     * sIndex        = NULL;
    smSCN          sDummySCN;
    smiStatement   sSmiStatement;
    qciStatement   sQciStatement;
    qcStatement  * sQcStatement  = NULL;
    idBool         sExist        = ID_FALSE;
    idBool         sIsInit       = ID_FALSE;
    idBool         sSmiStmtBegin = ID_FALSE;
    qciPartitionInfoList * sPartInfoList     = NULL;
    qciPartitionInfoList * sTempPartInfoList = NULL;

    IDE_TEST_CONT( aTableOID == SM_OID_NULL, NORMAL_EXIT );

    idlOS::memset( &sQciStatement, 0x00, ID_SIZEOF( qciStatement ) );
    IDE_TEST( qci::initializeStatement( &sQciStatement,
                                        NULL,
                                        NULL,
                                        aTrans->getStatistics() )
              != IDE_SUCCESS );
    sIsInit = ID_TRUE;

    IDE_TEST( sSmiStatement.begin( aTrans->getStatistics(),
                                   aTrans->getStatement(),
                                   SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sSmiStmtBegin = ID_TRUE;

    qciMisc::setSmiStmt( &sQciStatement, &sSmiStatement );
    sQcStatement = &(sQciStatement.statement);

    sTable = smiGetTable( aTableOID );
    sDummySCN = smiGetRowSCN( sTable );

    IDE_TEST( smiValidateAndLockObjects( aTrans,
                                         sTable,
                                         sDummySCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IS,
                                         ID_ULONG_MAX,
                                         ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sTable, (void **)&sTableInfo ) != IDE_SUCCESS );

    IDE_TEST_RAISE( qcm::existGlobalNonPartitionedIndex( sTableInfo, &sIndex ) == ID_TRUE,
                    NOT_ALLOWED_GLOBAL_NON_PART_INDEX );

    if ( aCallByRepl != ID_TRUE )
    {
        IDE_TEST( qci::mManageReplicationCallback.mIsDDLAsycReplOption( sQcStatement,
                                                                        sTableInfo,
                                                                        &sExist ) 
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_TRUE, NOT_ALLOWED_DDL_ASYNC );
    }

    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( qciMisc::getPartitionInfoList( sQcStatement,
                                            QCI_SMI_STMT( sQcStatement ),
                                            ( iduMemory * )QCI_QMX_MEM( sQcStatement ),
                                            sTableInfo->tableID,
                                            &sPartInfoList)
             == IDE_SUCCESS )
        {
            for ( sTempPartInfoList = sPartInfoList;
                  sTempPartInfoList != NULL;
                  sTempPartInfoList = sTempPartInfoList->next )
            {
                IDE_TEST( smiValidateAndLockObjects( aTrans,
                                                     sTempPartInfoList->partHandle,
                                                     sTempPartInfoList->partSCN,
                                                     SMI_TBSLV_DDL_DML,
                                                     SMI_TABLE_LOCK_IS,
                                                     smiGetDDLLockTimeOut(aTrans),
                                                     ID_FALSE )
                          != IDE_SUCCESS);
                sPartInfo = sTempPartInfoList->partitionInfo;
                IDE_TEST_RAISE( qcm::existGlobalNonPartitionedIndex( sPartInfo, &sIndex ) == ID_TRUE,
                                NOT_ALLOWED_GLOBAL_NON_PART_INDEX );

                if ( aCallByRepl != ID_TRUE )
                {
                    IDE_TEST( qci::mManageReplicationCallback.mIsDDLAsycReplOption( sQcStatement,
                                                                                    sPartInfo,
                                                                                    &sExist ) 
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( sExist == ID_TRUE, NOT_ALLOWED_DDL_ASYNC );
                }
            }
        }

    }

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sSmiStmtBegin = ID_FALSE;

    sIsInit = ID_FALSE;
    IDE_TEST( qci::finalizeStatement( &sQciStatement ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( NOT_ALLOWED_GLOBAL_NON_PART_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_NON_PART_INDEX,
                                  sIndex->name ) );
    }
    IDE_EXCEPTION( NOT_ALLOWED_DDL_ASYNC );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_DDL_ASYNC ) );
    }
    IDE_EXCEPTION_END;

    if ( sSmiStmtBegin == ID_TRUE )
    {
        (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsInit == ID_TRUE )
    {
        (void)qci::finalizeStatement( &sQciStatement );
    }

    return IDE_FAILURE;
}

void qciMisc::restoreTempInfoForPartition( qciTableInfo * aTableInfo,
                                           qciTableInfo * aPartInfo )
{
    qcmPartition::restoreTempInfoForPartition( aTableInfo,
                                               aPartInfo );
}

void qciMisc::restoreTempInfo( qciTableInfo         * aTableInfo,
                               qciPartitionInfoList * aPartInfoList,
                               qdIndexTableList     * aIndexTableList )
{
    qcmPartition::restoreTempInfo( aTableInfo,
                                   aPartInfoList,
                                   aIndexTableList ); 
}

IDE_RC qciMisc::validateAndLockPartitionInfoList( qciStatement         * aQciStatement,
                                                  qciPartitionInfoList * aPartInfoList,
                                                  smiTBSLockValidType    aTBSLvType,
                                                  smiTableLockMode       aLockMode,
                                                  ULong                  aLockWaitMicroSec )
{
    return qcmPartition::validateAndLockPartitionInfoList( &( aQciStatement->statement ),
                                                           aPartInfoList,
                                                           aTBSLvType,
                                                           aLockMode,
                                                           aLockWaitMicroSec );
}

IDE_RC qciMisc::runDDLforInternal( idvSQL       * aStatistics,
                                   smiStatement * aSmiStmt,
                                   UInt           aUserID,
                                   UInt           aSessionFlag,
                                   SChar        * aSqlStr )
{
    return qcg::runDDLforInternal( aStatistics,
                                   aSmiStmt,
                                   aUserID,
                                   aSessionFlag,
                                   aSqlStr );
}

IDE_RC qciMisc::runDDLforInternalWithMmSession( idvSQL       * aStatistics,
                                                void         * aMmSession,
                                                smiStatement * aSmiStmt,
                                                UInt           aUserID,
                                                UInt           aSessionFlag,
                                                SChar        * aSqlStr )
{
    return qcg::runDDLforInternalWithMmSession( aStatistics,
                                                aMmSession,
                                                aSmiStmt,
                                                aUserID,
                                                aSessionFlag,
                                                aSqlStr );
}

IDE_RC qciMisc::runRollbackableInternalDDL( qcStatement  * aStatement,
                                            smiStatement * aSmiStmt,
                                            UInt           aUserID,
                                            SChar        * aSqlStr )
{
    return qcg::runRollbackableInternalDDL( aStatement,
                                            aSmiStmt,
                                            aUserID,
                                            aSqlStr );
}

void qciMisc::setTransactionalDDLAvailable( void * aQcStatement, idBool aAvailable )
{
    ((qcStatement*)aQcStatement)->mDDLInfo.mTransactionalDDLAvailable = aAvailable;
}

idBool qciMisc::getTransactionalDDLAvailable( qciStatement * aQciStatement )
{
    return aQciStatement->statement.mDDLInfo.mTransactionalDDLAvailable;
}

qciDDLTargetType qciMisc::getDDLTargetType( UInt aSrcPartOIDCount )
{
    qciDDLTargetType sType = QCI_DDL_TARGET_NONE;

    if ( aSrcPartOIDCount == 0 )
    {
        sType = QCI_DDL_TARGET_TABLE;
    }
    else
    {
        sType = QCI_DDL_TARGET_PARTITION;
    }

    return sType;
}

void qciMisc::setDDLSrcInfo( void        * aQcStatement,
                             idBool        aDDLTransAvailable,
                             UInt          aSrcTableOIDCount,
                             smOID       * aSrcTableOIDArray,
                             UInt          aSrcPartOIDCountPerTable,
                             smOID       * aSrcPartOIDArray )
{
    qrc::setDDLSrcInfo( (qcStatement*)aQcStatement,
                        aDDLTransAvailable,
                        aSrcTableOIDCount,
                        aSrcTableOIDArray,
                        aSrcPartOIDCountPerTable,
                        aSrcPartOIDArray );
}

smOID * qciMisc::getDDLSrcTableOIDArray( qciStatement * aQciStatement, UInt * aSrcTableOIDCount )
{
    return getDDLSrcTableOIDArray( (void*)&(aQciStatement->statement), aSrcTableOIDCount );
}

smOID * qciMisc::getDDLSrcTableOIDArray( void * aQcStatement, UInt * aSrcTableOIDCount )
{
    UInt    sCount = ((qcStatement*)aQcStatement)->mDDLInfo.mSrcTableOIDCount;
    smOID * sArray = ((qcStatement*)aQcStatement)->mDDLInfo.mSrcTableOIDArray;

    *aSrcTableOIDCount = sCount;

    return sArray;
}

smOID * qciMisc::getDDLSrcPartTableOIDArray( qciStatement * aQciStatement, UInt * aDestPartOIDCount )
{
    return getDDLSrcPartTableOIDArray( (void*)&(aQciStatement->statement), aDestPartOIDCount );
}

smOID * qciMisc::getDDLSrcPartTableOIDArray( void * aQcStatement, UInt * aSrcPartOIDCount )
{
    UInt    sCount = ((qcStatement*)aQcStatement)->mDDLInfo.mSrcPartOIDCountPerTable;
    smOID * sArray = ((qcStatement*)aQcStatement)->mDDLInfo.mSrcPartOIDArray;

    *aSrcPartOIDCount = sCount;

    return sArray;
}

void qciMisc::setDDLDestInfo( void        * aQcStatement, 
                              UInt          aDestTableOIDCount,
                              smOID       * aDestTableOIDArray,
                              UInt          aDestPartOIDCountPerTable,
                              smOID       * aDestPartOIDArray )
{
    qrc::setDDLDestInfo( (qcStatement*)aQcStatement,
                         aDestTableOIDCount,
                         aDestTableOIDArray,
                         aDestPartOIDCountPerTable,
                         aDestPartOIDArray );
}

smOID * qciMisc::getDDLDestTableOIDArray( qciStatement * aQciStatement, UInt * aDestTableOIDCount )
{
    return getDDLDestTableOIDArray(&(aQciStatement->statement), aDestTableOIDCount);
}

smOID * qciMisc::getDDLDestTableOIDArray( qcStatement * aQcStatement, UInt * aDestTableOIDCount )
{
    UInt    sCount = aQcStatement->mDDLInfo.mDestTableOIDCount;
    smOID * sArray = aQcStatement->mDDLInfo.mDestTableOIDArray;

    *aDestTableOIDCount = sCount;

    return sArray;
}

smOID * qciMisc::getDDLDestPartTableOIDArray( qciStatement * aQciStatement, UInt * aSrcPartOIDCount )
{
    return getDDLDestPartTableOIDArray( (void*)&(aQciStatement->statement), aSrcPartOIDCount );
}

smOID * qciMisc::getDDLDestPartTableOIDArray( void * aQcStatement, UInt * aDestPartOIDCount )
{
    UInt    sCount = ((qcStatement*)aQcStatement)->mDDLInfo.mDestPartOIDCountPerTable;
    smOID * sArray = ((qcStatement*)aQcStatement)->mDDLInfo.mDestPartOIDArray;

    *aDestPartOIDCount = sCount;

    return sArray;
}

idBool qciMisc::isReplicableDDL( qciStatement * aQciStatement )
{
    idBool sIsReplicableDDL = ID_FALSE;
    UInt   sDummyCount      = 0;

    if ( getDDLSrcTableOIDArray( aQciStatement, &sDummyCount ) != NULL )
    {
        sIsReplicableDDL = ID_TRUE;
    }
    else
    {        
        sIsReplicableDDL = ID_FALSE;
    }

    return sIsReplicableDDL;
}

void qciMisc::setSmiStmt( qciStatement *aQciStatement, smiStatement * aSmiStatement )
{
    qcg::setSmiStmt( &( aQciStatement->statement ), aSmiStatement );
}

/* BUG-45948 */
void qciMisc::getSmiStmt( qciStatement *aQciStatement, smiStatement ** aSmiStatement )
{
    qcg::getSmiStmt( &( aQciStatement->statement ), aSmiStatement );
}

idBool qciMisc::isDDLSync( qciStatement * aQciStatement )
{
    return qrc::isDDLSync( &( aQciStatement->statement ) );
}

idBool qciMisc::isDDLSync( void * aQcStatement )
{
    return qrc::isDDLSync( (qcStatement*)aQcStatement );
}

idBool qciMisc::isInternalDDL( void * aQcStatement )
{
    return qrc::isInternalDDL( (qcStatement*)aQcStatement );
}

idBool qciMisc::getIsNeedDDLInfo( void * aQcStatement )
{
    return QCG_GET_SESSION_IS_NEED_DDL_INFO( (qcStatement*)aQcStatement );
}

idBool qciMisc::getTransactionalDDL( void * aQcStatement )
{
    return QCG_GET_SESSION_TRANSACTIONAL_DDL( (qcStatement*)aQcStatement );
}

idBool qciMisc::getIsRollbackableInternalDDL( void * aQcStatement )
{
    return qcg::getIsRollbackableInternalDDL( (qcStatement*)aQcStatement );
}

idBool qciMisc::getGlobalDDL( void * aQcStatement )
{
    return QCG_GET_SESSION_GLOBAL_DDL( (qcStatement*)aQcStatement );
}

IDE_RC qciMisc::backupDDLTargetOldTableInfo( smiTrans               * aTrans, 
                                             smOID                    aTableOID,
                                             UInt                     aPartOIDCount,
                                             smOID                  * aPartOIDArray,
                                             smiDDLTargetTableInfo ** aDDLTargetTableInfo )
{
    const void   * sTable        = NULL;
    qciTableInfo * sTableInfo    = NULL;
    qciTableInfo * sPartInfo     = NULL;
    qciTableInfo * sNewTableInfo = NULL;
    smSCN          sDummySCN;
    qciStatement   sQciStatement;
    smiStatement   sSmiStatement;
    idBool         sIsInit       = ID_FALSE;
    idBool         sSmiStmtBegin = ID_FALSE;
    idBool         sIsTarget     = ID_FALSE;

    UInt           i      = 0;
    iduListNode  * sNode  = NULL;
    iduListNode  * sDummy = NULL;
    const void   * sTempTable = NULL;
    
    smiDDLTargetTableInfo * sTempInfo              = NULL;
    smiDDLTargetTableInfo * sDDLTargetTableInfo    = NULL;
    qciPartitionInfoList  * sTablePartInfoList     = NULL;
    qciPartitionInfoList  * sTempTablePartInfoList = NULL;

    IDE_TEST_CONT( aTableOID == SM_OID_NULL, PASS );

    idlOS::memset( &sQciStatement, 0x00, ID_SIZEOF( qciStatement ) );
    IDE_TEST( qci::initializeStatement( &sQciStatement,
                                        NULL,
                                        NULL,
                                        aTrans->getStatistics() )
              != IDE_SUCCESS );
    sIsInit = ID_TRUE;

    IDE_TEST( sSmiStatement.begin( aTrans->getStatistics(),
                                   aTrans->getStatement(),
                                   SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sSmiStmtBegin = ID_TRUE;

    qciMisc::setSmiStmt( &sQciStatement, &sSmiStatement );

    sTable = smiGetTable( aTableOID );
    sDummySCN = smiGetRowSCN( sTable );

    IDE_TEST( smiValidateAndLockObjects( aTrans,
                                         sTable,
                                         sDummySCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IX,
                                         smiGetDDLLockTimeOut(aTrans),
                                         ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sTable, (void **)&sTableInfo ) != IDE_SUCCESS );

    if ( getDDLTargetType( aPartOIDCount ) == QCI_DDL_TARGET_TABLE )
    {
        IDE_TEST( smiValidateAndLockObjects( aTrans,
                                         sTable,
                                         sDummySCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_X,
                                         smiGetDDLLockTimeOut(aTrans),
                                         ID_FALSE )
              != IDE_SUCCESS );

        IDE_TEST( makeAndSetQcmTableInfo( &sSmiStatement,
                                          sTableInfo->tableID,
                                          sTableInfo->tableOID )
                  != IDE_SUCCESS );

        IDE_TEST( smiTable::touchTable( &sSmiStatement,
                                        sTableInfo->tableHandle,
                                        SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableTempInfo( sTable, (void**)&sNewTableInfo ) != IDE_SUCCESS );
 
        sIsTarget = ID_TRUE;
    }
    else /* QCI_DDL_TARGET_PARTITION */
    {
        sNewTableInfo = sTableInfo;
        sIsTarget = ID_FALSE;
    }

    IDE_TEST( aTrans->allocNSetDDLTargetTableInfo( sTableInfo->tableID,
                                                   (void*)sTableInfo,
                                                   NULL,
                                                   sIsTarget,
                                                   &sDDLTargetTableInfo )
              != IDE_SUCCESS );

    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( getDDLTargetType( aPartOIDCount ) == QCI_DDL_TARGET_TABLE )
        {
            IDE_TEST( getPartitionInfoList( &( sQciStatement.statement ),
                                            &sSmiStatement,
                                            ( iduMemory * )QCI_QMX_MEM( &( sQciStatement.statement ) ),
                                            sTableInfo->tableID,
                                            &sTablePartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( validateAndLockPartitionInfoList( &sQciStatement,
                                                        sTablePartInfoList,
                                                        SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                        SMI_TABLE_LOCK_X,
                                                        smiGetDDLLockTimeOut(aTrans) )
                      != IDE_SUCCESS );

            for ( sTempTablePartInfoList = sTablePartInfoList;
                  sTempTablePartInfoList != NULL;
                  sTempTablePartInfoList = sTempTablePartInfoList->next )
            {
                sPartInfo = sTempTablePartInfoList->partitionInfo;

                IDE_TEST( aTrans->allocNSetDDLTargetPartTableInfo( sDDLTargetTableInfo,
                                                                   sPartInfo->partitionID,
                                                                   ID_TRUE,
                                                                   (void*)sPartInfo,
                                                                   NULL )
                          != IDE_SUCCESS );

                IDE_TEST( makeAndSetQcmPartitionInfo( &sSmiStatement,
                                                      sPartInfo->partitionID,
                                                      sPartInfo->tableOID,
                                                      sNewTableInfo )
                          != IDE_SUCCESS );

                IDE_TEST( smiTable::touchTable( &sSmiStatement,
                                                sPartInfo->tableHandle,
                                                SMI_TBSLV_DDL_DML )
                          != IDE_SUCCESS );
        
                IDE_TEST( smiGetTableTempInfo( sTable, (void**)&sNewTableInfo ) != IDE_SUCCESS );
            }
        }
        else
        {
            for ( i = 0; i < aPartOIDCount; i++ )
            {
                sTable = smiGetTable( aPartOIDArray[i] );
                sDummySCN = smiGetRowSCN( sTable );

                IDE_TEST( smiValidateAndLockObjects( aTrans,
                                                     sTable,
                                                     sDummySCN,
                                                     SMI_TBSLV_DDL_DML,
                                                     SMI_TABLE_LOCK_X,
                                                     smiGetDDLLockTimeOut(aTrans),
                                                     ID_FALSE )
                          != IDE_SUCCESS );

                IDE_TEST( smiGetTableTempInfo( sTable, (void **)&sPartInfo ) != IDE_SUCCESS );

                IDE_TEST( aTrans->allocNSetDDLTargetPartTableInfo( sDDLTargetTableInfo,
                                                                   sPartInfo->partitionID,
                                                                   ID_TRUE,
                                                                   (void*)sPartInfo,
                                                                   NULL )
                          != IDE_SUCCESS );

                IDE_TEST( makeAndSetQcmPartitionInfo( &sSmiStatement,
                                                      sPartInfo->partitionID,
                                                      sPartInfo->tableOID,
                                                      sNewTableInfo )
                          != IDE_SUCCESS );

                IDE_TEST( smiTable::touchTable( &sSmiStatement,
                                                sPartInfo->tableHandle,
                                                SMI_TBSLV_DDL_DML )
                          != IDE_SUCCESS );                        
            }
        }
    }

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sSmiStmtBegin = ID_FALSE;

    sIsInit = ID_FALSE;
    IDE_TEST( qci::finalizeStatement( &sQciStatement ) != IDE_SUCCESS );

    *aDDLTargetTableInfo = sDDLTargetTableInfo;

    IDE_EXCEPTION_CONT( PASS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDDLTargetTableInfo != NULL )
    {
        IDU_LIST_ITERATE_SAFE( &(sDDLTargetTableInfo->mPartInfoList), sNode, sDummy )
        {
            sTempInfo = (smiDDLTargetTableInfo*)sNode->mObj;
            if ( ( sTempInfo->mOldTableInfo != NULL ) &&
                 ( sTempInfo->mIsReCreated == ID_TRUE ) )
            {
                sTempTable = smiGetTable( ((qciTableInfo*)sTempInfo->mOldTableInfo)->tableOID );
                (void)smiGetTableTempInfo( sTempTable, (void **)&sTableInfo );
                if ( sTempInfo->mOldTableInfo != sTableInfo )
                {
                    (void)destroyQcmPartitionInfo( sTableInfo );
                }
            }
        }
        restoreDDLTargetOldTableInfo( sDDLTargetTableInfo );

        aTrans->freeDDLTargetTableInfo( sDDLTargetTableInfo );
        sDDLTargetTableInfo = NULL;
    }

    if ( sDDLTargetTableInfo != NULL )
    {
        if ( ( sNewTableInfo != NULL ) &&
             ( sDDLTargetTableInfo->mIsReCreated == ID_TRUE ) )
        {
            (void)destroyQcmTableInfo( (qcmTableInfo*)sNewTableInfo );
            sNewTableInfo = NULL;
        }
    }
    else
    {
        if ( ( sNewTableInfo != NULL ) && 
             ( sIsTarget == ID_TRUE ) )
        {
            (void)destroyQcmTableInfo( (qcmTableInfo*)sNewTableInfo );
            sNewTableInfo = NULL;
        }
    }

    if ( sSmiStmtBegin == ID_TRUE )
    {
        (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsInit == ID_TRUE )
    {
        (void)qci::finalizeStatement( &sQciStatement );
    }

    return IDE_FAILURE;
}

IDE_RC qciMisc::backupDDLTargetNewTableInfo( smiTrans               * aTrans, 
                                             smOID                    aTableOID,
                                             UInt                     aPartOIDCount,
                                             smOID                  * aPartOIDArray,
                                             smiDDLTargetTableInfo ** aDDLTargetTableInfo )
{
    UInt           i             = 0;
    const void   * sTable        = NULL;
    qciTableInfo * sTableInfo    = NULL;
    qciTableInfo * sPartInfo     = NULL;
    smSCN          sDummySCN;
    qciStatement   sQciStatement;
    smiStatement   sSmiStatement;
    idBool         sIsInit       = ID_FALSE;
    idBool         sSmiStmtBegin = ID_FALSE;
    idBool         sIsReCreated  = ID_FALSE;

    smiDDLTargetTableInfo * sDDLTargetTableInfo    = NULL;
    qciPartitionInfoList  * sTablePartInfoList     = NULL;
    qciPartitionInfoList  * sTempTablePartInfoList = NULL;

    IDE_TEST_CONT( aTableOID == SM_OID_NULL, PASS );

    idlOS::memset( &sQciStatement, 0x00, ID_SIZEOF( qciStatement ) );
    IDE_TEST( qci::initializeStatement( &sQciStatement,
                                        NULL,
                                        NULL,
                                        aTrans->getStatistics() )
              != IDE_SUCCESS );
    sIsInit = ID_TRUE;

    IDE_TEST( sSmiStatement.begin( aTrans->getStatistics(),
                                   aTrans->getStatement(),
                                   SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sSmiStmtBegin = ID_TRUE;

    setSmiStmt( &sQciStatement, &sSmiStatement );

    sTable = smiGetTable( aTableOID );
    sDummySCN = smiGetRowSCN( sTable );

    IDE_TEST( smiValidateAndLockObjects( aTrans,
                                         sTable,
                                         sDummySCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IX,
                                         ID_ULONG_MAX,
                                         ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableTempInfo( sTable, (void **)&sTableInfo ) != IDE_SUCCESS );

    if ( getDDLTargetType( aPartOIDCount ) == QCI_DDL_TARGET_TABLE )
    {
        sIsReCreated = ID_TRUE;
    }
    else
    {
        sIsReCreated = ID_FALSE;
    }

    IDE_TEST( aTrans->allocNSetDDLTargetTableInfo( sTableInfo->tableID,
                                                   NULL,
                                                   (void*)sTableInfo,
                                                   sIsReCreated,
                                                   &sDDLTargetTableInfo )
              != IDE_SUCCESS );

    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( getDDLTargetType( aPartOIDCount ) == QCI_DDL_TARGET_TABLE )
        {
            IDE_TEST( smiTable::touchTable( &sSmiStatement,
                                            sTableInfo->tableHandle,
                                            SMI_TBSLV_DDL_DML )
                      != IDE_SUCCESS );

            IDE_TEST( getPartitionInfoList( &( sQciStatement.statement ),
                                            &sSmiStatement,
                                            ( iduMemory * )QCI_QMX_MEM( &( sQciStatement.statement ) ),
                                            sTableInfo->tableID,
                                            &sTablePartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( validateAndLockPartitionInfoList( &sQciStatement,
                                                        sTablePartInfoList,
                                                        SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                        SMI_TABLE_LOCK_X,
                                                        smiGetDDLLockTimeOut(aTrans) )
                      != IDE_SUCCESS );

            for ( sTempTablePartInfoList = sTablePartInfoList;
                  sTempTablePartInfoList != NULL;
                  sTempTablePartInfoList = sTempTablePartInfoList->next )
            {
                sPartInfo = sTempTablePartInfoList->partitionInfo;

                IDE_TEST( aTrans->allocNSetDDLTargetPartTableInfo( sDDLTargetTableInfo,
                                                                   sPartInfo->tableID,
                                                                   ID_TRUE,
                                                                   NULL,
                                                                   (void*)sPartInfo )
                          != IDE_SUCCESS );

                IDE_TEST( smiTable::touchTable( &sSmiStatement,
                                                sPartInfo->tableHandle,
                                                SMI_TBSLV_DDL_DML )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            for ( i = 0; i < aPartOIDCount; i++ )
            {
                sTable = smiGetTable( aPartOIDArray[i] );
                sDummySCN = smiGetRowSCN( sTable );

                IDE_TEST( smiValidateAndLockObjects( aTrans,
                                                     sTable,
                                                     sDummySCN,
                                                     SMI_TBSLV_DDL_DML,
                                                     SMI_TABLE_LOCK_X,
                                                     smiGetDDLLockTimeOut(aTrans),
                                                     ID_FALSE )
                          != IDE_SUCCESS );
                IDE_TEST( smiGetTableTempInfo( sTable, (void **)&sPartInfo ) != IDE_SUCCESS );

                IDE_TEST( aTrans->allocNSetDDLTargetPartTableInfo( sDDLTargetTableInfo,
                                                                   sPartInfo->tableID,
                                                                   ID_TRUE,
                                                                   NULL,
                                                                   (void*)sPartInfo )
                          != IDE_SUCCESS );

                IDE_TEST( smiTable::touchTable( &sSmiStatement,
                                                sPartInfo->tableHandle,
                                                SMI_TBSLV_DDL_DML )
                          != IDE_SUCCESS );
            }
        }
    }

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sSmiStmtBegin = ID_FALSE;

    sIsInit = ID_FALSE;
    IDE_TEST( qci::finalizeStatement( &sQciStatement ) != IDE_SUCCESS );

    *aDDLTargetTableInfo = sDDLTargetTableInfo;

    IDE_EXCEPTION_CONT( PASS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDDLTargetTableInfo != NULL )
    {
        (void)removeDDLTargetTableInfo( aTrans, sDDLTargetTableInfo );
        sDDLTargetTableInfo = NULL;
    }

    if ( sSmiStmtBegin == ID_TRUE )
    {
        (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsInit == ID_TRUE )
    {
        (void)qci::finalizeStatement( &sQciStatement );
    }

    return IDE_FAILURE;
}

void qciMisc::removeDDLTargetTableInfo( smiTrans * aTrans, smiDDLTargetTableInfo * aDDLTargetTableInfo )
{
    iduListNode      * sNode  = NULL;
    iduListNode      * sDummy = NULL;
    smiDDLTargetTableInfo * sPartInfo  = NULL;

    if ( aDDLTargetTableInfo != NULL )
    {
        IDU_LIST_ITERATE_SAFE( &(aDDLTargetTableInfo->mPartInfoList), sNode, sDummy )
        {
            sPartInfo = (smiDDLTargetTableInfo*)sNode->mObj;

            IDU_LIST_REMOVE( &( sPartInfo->mNode ) );

            if ( ( sPartInfo->mOldTableInfo != NULL ) &&
                 ( sPartInfo->mIsReCreated == ID_TRUE ) )
            {
                (void)destroyQcmPartitionInfo( (qcmTableInfo*)sPartInfo->mOldTableInfo );
            }
            sPartInfo->mOldTableInfo = NULL;

            sPartInfo->mNewTableInfo = NULL;
        }

        if ( ( aDDLTargetTableInfo->mOldTableInfo != NULL ) &&
             ( aDDLTargetTableInfo->mIsReCreated == ID_TRUE ) )
        {
            (void)destroyQcmTableInfo( (qcmTableInfo*)aDDLTargetTableInfo->mOldTableInfo );
        }
        aDDLTargetTableInfo->mOldTableInfo = NULL;
        
        aDDLTargetTableInfo->mNewTableInfo = NULL;

        aTrans->freeDDLTargetTableInfo( aDDLTargetTableInfo );
    }
}

void qciMisc::restoreDDLTargetOldTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo )
{
    iduListNode      * sNode  = NULL;
    iduListNode      * sDummy = NULL;
    smiDDLTargetTableInfo * sPartInfo  = NULL;

    if ( aDDLTargetTableInfo != NULL )
    {
        IDU_LIST_ITERATE_SAFE( &(aDDLTargetTableInfo->mPartInfoList), sNode, sDummy )
        {
            sPartInfo = (smiDDLTargetTableInfo*)sNode->mObj;

            IDU_LIST_REMOVE( &( sPartInfo->mNode ) );

            if ( sPartInfo->mOldTableInfo != NULL )
            {
                restoreTempInfoForPartition( (qciTableInfo*)aDDLTargetTableInfo->mOldTableInfo,
                                             (qciTableInfo*)sPartInfo->mOldTableInfo );
                sPartInfo->mOldTableInfo = NULL;
            }
        }
        
        restoreTempInfo( (qciTableInfo*)aDDLTargetTableInfo->mOldTableInfo,
                         NULL,
                         NULL );
        aDDLTargetTableInfo->mOldTableInfo = NULL;
    }
}

void qciMisc::destroyDDLTargetNewTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo )
{
    iduListNode      * sNode  = NULL;
    iduListNode      * sDummy = NULL;
    smiDDLTargetTableInfo * sPartInfo  = NULL;

    if ( aDDLTargetTableInfo != NULL )
    {
        if ( IDU_LIST_IS_EMPTY( &(aDDLTargetTableInfo->mPartInfoList) ) != ID_TRUE )
        {
            IDU_LIST_ITERATE_SAFE( &(aDDLTargetTableInfo->mPartInfoList), sNode, sDummy )
            {
                sPartInfo = (smiDDLTargetTableInfo*)sNode->mObj;

                IDU_LIST_REMOVE( &( sPartInfo->mNode ) );

                if ( ( sPartInfo->mNewTableInfo != NULL ) &&
                     ( sPartInfo->mIsReCreated == ID_TRUE ) )
                {
                    (void)destroyQcmPartitionInfo( (qcmTableInfo*)sPartInfo->mNewTableInfo );
                }
                sPartInfo->mNewTableInfo = NULL;
            }
        }

        if ( aDDLTargetTableInfo->mIsReCreated == ID_TRUE )
        {
            (void)destroyQcmTableInfo( (qcmTableInfo*)aDDLTargetTableInfo->mNewTableInfo );
        }
        aDDLTargetTableInfo->mNewTableInfo = NULL;
    }
}

IDE_RC qciMisc::rebuildStatement( qciStatement * aQciStatement, 
                                  smiTrans     * aTrans,
                                  UInt           aFlag )
{
    idBool         sIsBegin       = ID_FALSE;
    idBool         sIsSwap        = ID_FALSE;
    smiStatement   sSmiStatement;
    smiStatement * sSmiStatementOrig;
    UInt           sSqlLen        = QCI_STMTTEXTLEN( aQciStatement );
    SChar        * sSql           = NULL;
    qciSQLPlanCacheContext  sPlanCacheContext;

    idlOS::memset( &sPlanCacheContext, 0x00, ID_SIZEOF( qciSQLPlanCacheContext ) );
    sPlanCacheContext.mPlanCacheInMode     = QCI_SQL_PLAN_CACHE_IN_OFF;
    sPlanCacheContext.mSharedPlanMemory    = NULL;
    sPlanCacheContext.mPrepPrivateTemplate = NULL;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QCI,
                                 sSqlLen + 1,
                                 (void**)&sSql,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    idlOS::strncpy( sSql,
                    QCI_STMTTEXT( aQciStatement ),
                    sSqlLen + 1);
    sSql[sSqlLen] = '\0';

    idlOS::memset( &sSmiStatement, 0x00, ID_SIZEOF( smiStatement ) );
    IDE_TEST( sSmiStatement.begin( aTrans->getStatistics(),
                                   aTrans->getStatement(),
                                   aFlag )
              != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    getSmiStmt( aQciStatement, &sSmiStatementOrig );
    setSmiStmt( aQciStatement, &sSmiStatement );
    sIsSwap = ID_TRUE;
   
    IDE_TEST( qci::clearStatement4Reprepare( aQciStatement,
                                             &sSmiStatement )
              != IDE_SUCCESS );

    while ( qci::hardRebuild( aQciStatement,
                              &sSmiStatement,
                              aTrans->getStatement(),
                              &sPlanCacheContext,
                              sSql,
                              sSqlLen )
            != IDE_SUCCESS )
    {
    
        IDE_TEST( ideIsRebuild() != IDE_SUCCESS );

        IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE )
                  != IDE_SUCCESS );
        sIsBegin = ID_FALSE;

        idlOS::memset( &sSmiStatement, 0x00, ID_SIZEOF( smiStatement ) );
        IDE_TEST( sSmiStatement.begin( aTrans->getStatistics(),
                                       aTrans->getStatement(),
                                       aFlag )
                  != IDE_SUCCESS );
        sIsBegin = ID_TRUE;
    }

    IDE_TEST( qci::setBindTuple( aQciStatement ) != IDE_SUCCESS );

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    setSmiStmt( aQciStatement, sSmiStatementOrig );
    sIsSwap = ID_FALSE;

    (void)iduMemMgr::free( sSql );
    sSql = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        sIsBegin = ID_FALSE;
    }

    if ( sIsSwap == ID_TRUE )
    {
        setSmiStmt( aQciStatement, sSmiStatementOrig );
    }

    if ( sSql != NULL )
    {
        (void)iduMemMgr::free( sSql );
        sSql = NULL;
    }

    IDE_POP();

    return IDE_FAILURE;
}

idBool qciMisc::isLockTableUntillNextDDL( qciStatement * aQciStatement )
{
    return qrc::isLockTableUntillNextDDL( &( aQciStatement->statement ) );
}

idBool qciMisc::isLockTableUntillNextDDL( void * aQcStatement )
{
    return qrc::isLockTableUntillNextDDL( (qcStatement*)aQcStatement );
}

idBool qciMisc::intersectColumn( UInt *aColumnIDList1,
                                 UInt aColumnIDCount1,
                                 UInt *aColumnIDList2,
                                 UInt aColumnIDCount2)    
{
    return qdn::intersectColumn( aColumnIDList1,
                                 aColumnIDCount1,
                                 aColumnIDList2,
                                 aColumnIDCount2 );
    
}

// TASK-7244 PSM partial rollback in Sharding
void qciMisc::setBeginSP( qcStatement * aQcStatement )
{
    qsxEnv::setBeginSP( aQcStatement->spxEnv );
}

// TASK-7244 PSM partial rollback in Sharding
void qciMisc::unsetBeginSP( qcStatement * aQcStatement )
{
    qsxEnv::unsetBeginSP( aQcStatement->spxEnv );
}

// TASK-7244 PSM partial rollback in Sharding
idBool qciMisc::isBeginSP( qcStatement * aQcStatement )
{
    return qsxEnv::isBeginSP( aQcStatement->spxEnv );
}

// TASK-7244 Set shard split method to PSM info
IDE_RC qciMisc::makeProcStatusInvalidAndSetShardSplitMethodByName( qcStatement * aQcStatement,
                                                                   qsOID         aProcOID,
                                                                   SChar       * aShardSplitMethodStr )
{
    // 위에서 체크하고 호출해야 한다.
    IDE_DASSERT( aProcOID != SMI_NULL_OID );

    if ( aProcOID != SMI_NULL_OID )
    {
        IDE_TEST( qsxProc::makeStatusInvalidAndSetShardSplitMethod ( aQcStatement,
                                                                     aProcOID,
                                                                     aShardSplitMethodStr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
