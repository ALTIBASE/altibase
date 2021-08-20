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
 *
 *     ALTIBASE SHARD management function
 *
 * Syntax :
 *    SHARD_SET_REPLICATION( k_safety        in  integer,
 *                           replication_mode in  varchar(10),
 *                           parallel_count   in  integer default 1 )
 *    RETURN 0
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName sdfFunctionName[1] = {
    { NULL, 21, (void*)"SHARD_SET_REPLICATION" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetReplicationModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_SetReplication( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetReplication,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC sdfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[] =
    {
        &mtdInteger, // k-safety
        &mtdVarchar, // replication_mode
        &mtdInteger  // parallel_count
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = sdfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfCalculate_SetReplication( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     Set Replication
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    mtdIntegerType            sKSafety;
    mtdCharType             * sReplicationMode;
    SChar                     sReplicationModeStr[SDI_REPLICATION_MODE_MAX_SIZE + 1];
    mtdIntegerType            sParallelCount;
    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    SChar                     sCommitStr[7] = {0,};

    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

    // BUG-46366
    IDE_TEST_RAISE( ( QC_SMI_STMT(sStatement)->getTrans() == NULL ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DML ) == QCI_STMT_MASK_DML ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DCL ) == QCI_STMT_MASK_DCL ),
                    ERR_INSIDE_QUERY );

    // Check Privilege
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(sStatement) != QCI_SYS_USER_ID,
                    ERR_NO_GRANT );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE ) /* k-safety */
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------
        sKSafety = *(mtdIntegerType*)aStack[1].value;
        IDE_TEST_RAISE( ( sKSafety > SDI_KSAFETY_MAX ) ||  /* 0, 1, 2 */
                        ( sKSafety < 0 ),
                        ERR_KSAFETY_RANGE );

        if ( aStack[2].column->module->isNull( aStack[2].column,
                                               aStack[2].value ) == ID_TRUE ) /* replication_mode */
        {
            if ( sKSafety == 0 )
            {
                idlOS::strncpy( sReplicationModeStr,
                                (SChar*)SDI_REP_MODE_NULL_STR,
                                ID_SIZEOF(SDI_REP_MODE_NULL_STR) );
            }
            else
            {
                idlOS::strncpy( sReplicationModeStr,
                                (SChar*)SDI_REP_MODE_CONSISTENT_STR,
                                ID_SIZEOF(SDI_REP_MODE_CONSISTENT_STR) );
            }
        }
        else
        {
            IDE_TEST_RAISE( sKSafety == 0, ERR_SHARD_REP_MODE );
            sReplicationMode = (mtdCharType*)aStack[2].value;
            IDE_TEST_RAISE( sReplicationMode->length > SDI_REPLICATION_MODE_MAX_SIZE,
                        ERR_SHARD_REP_MODE );
            idlOS::strncpy( sReplicationModeStr,
                        (SChar*)sReplicationMode->value,
                        sReplicationMode->length );
            sReplicationModeStr[sReplicationMode->length] = '\0';
        }

        if ( aStack[3].column->module->isNull( aStack[3].column,
                                               aStack[3].value ) == ID_TRUE ) /* parallel_count */
        {
            if ( sKSafety == 0 )
            {
                sParallelCount = 0;
            }
            else
            {
                sParallelCount = SDU_SHARD_INTERNAL_REPLICATION_PARALLEL_COUNT;
            }
        }
        else
        {
            IDE_TEST_RAISE( sKSafety == 0, ERR_PARALLEL_COUNT );
            sParallelCount = *(mtdIntegerType*)aStack[3].value;
            IDE_TEST_RAISE( ( sParallelCount > SDU_SHARD_REPLICATION_MAX_PARALLEL_COUNT ) ||
                            ( sParallelCount <= 0 ),
                            ERR_PARALLEL_COUNT);
        }
        //---------------------------------
        // Begin Statement for shard meta
        //---------------------------------

        sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
        sOldStmt                = QC_SMI_STMT(sStatement);
        QC_SMI_STMT(sStatement) = &sSmiStmt;
        sState = 1;

        IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                  sOldStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );
        sState = 2;

        /* update local meta info */
        IDE_TEST( sdm::updateLocalMetaInfoForReplication( sStatement,
                                   (UInt) sKSafety,
                                   (SChar*) sReplicationModeStr,
                                   (UInt) sParallelCount,
                                   &sRowCnt )
                  != IDE_SUCCESS );

        //---------------------------------
        // End Statement
        //---------------------------------

        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sState = 1;

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;

        IDE_TEST_RAISE( sRowCnt != 1, ERR_SHARD_META );

        //---------------------------------
        // Commit
        //---------------------------------

        idlOS::snprintf( sCommitStr, idlOS::strlen("COMMIT") + 1, "COMMIT" );

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sCommitStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSIDE_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_PSM_INSIDE_QUERY ) );
    }
    IDE_EXCEPTION( ERR_KSAFETY_RANGE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_REP_ARG_WRONG, "k_safety" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_REP_MODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_REP_ARG_WRONG, "replication_mode" ) );
    }
    IDE_EXCEPTION( ERR_PARALLEL_COUNT );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_REP_ARG_WRONG, "parallel_count" ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_SHARD_META );
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDF_SHARD_LOCAL_META_ERROR));
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(sStatement) = sOldStmt;
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}
