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
 *the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <sda.h>
#include <sduProperty.h>
#include <qmsParseTree.h>
#include <qmv.h>
#include <qmo.h>
#include <qmoNormalForm.h>
#include <qmoConstExpr.h>
#include <qmoCrtPathMgr.h>
#include <qsxRelatedProc.h>
#include <qciStmtType.h>
#include <qmoOuterJoinOper.h>
#include <qmoOuterJoinElimination.h>
#include <qmoInnerJoinPushDown.h>
#include <mtv.h>

extern mtfModule mtfOr;
extern mtfModule mtfEqual;
extern mtfModule mtfEqualAny;
extern mtfModule sdfShardKey;
extern mtfModule mtfList;

IDE_RC sda::checkStmtTypeBeforeAnalysis( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *      샤드 분석 전에 샤드 분석이 가능한지(필요한지) 먼저 확인
 *
 * Implementation :
 *      1. 분석 가능한 Statement type인지 확인한다.
 *      2. 분석이 필요없는 Shard prefix인지 확인한다.
 *
 * Arguments :
 *
 ***********************************************************************/

    qciStmtType      sStmtType  = aStatement->myPlan->parseTree->stmtKind;
    qcShardStmtType  sPrefixType = aStatement->myPlan->parseTree->stmtShard;
    qcuSqlSourceInfo sqlInfo;

    /* TASK-7219 Shard Transformer Refactoring */
    switch( qciMisc::getStmtType( sStmtType ) )
    {
        case QCI_STMT_MASK_DML:
        case QCI_STMT_MASK_SP:
            break;
        default :
            IDE_RAISE( ERR_UNSUPPORTED_STMT );
            break;
    }

    switch( sStmtType )
    {
        case QCI_STMT_SELECT :
        case QCI_STMT_SELECT_FOR_UPDATE :
        case QCI_STMT_INSERT :
        case QCI_STMT_UPDATE :
        case QCI_STMT_DELETE :
        case QCI_STMT_EXEC_PROC :
            break;
        default :
            /* TASK-7219 Shard Transformer Refactoring */
            sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );

            IDE_RAISE( ERR_UNSUPPORTED_STMT_WITH_POS );
            break;
    }

    switch( sPrefixType )
    {
        case QC_STMT_SHARD_NONE :
        case QC_STMT_SHARD_ANALYZE :
            break;
        default :
            IDE_RAISE( ERR_SHARD_PREFIX_EXISTS );
            break;
    }

    return IDE_SUCCESS;


    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION( ERR_UNSUPPORTED_STMT_WITH_POS )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Unsupported statement type",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_STMT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Unsupported statement type",
                                  "" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_PREFIX_EXISTS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  sdi::getNonShardQueryReasonArr( SDI_SHARD_KEYWORD_EXISTS ),
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkDCLStmt( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *     PROJ-2727
 *      수행 가능한 DCL statement type인지 확인한다.
 *
 *     *  수행 가능한 statement type
 *           1. QCI_STMT_SET_SESSION_PROPERTY
 *           2. QCI_STMT_SET_SYSTEM_PROPERTY
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qciStmtType     sType  = aStatement->myPlan->parseTree->stmtKind;
    qcShardStmtType sShard = aStatement->myPlan->parseTree->stmtShard;

    if (( sType == QCI_STMT_SET_SESSION_PROPERTY ) ||
        ( sType == QCI_STMT_SET_SYSTEM_PROPERTY ))
    {
        IDE_TEST_RAISE( sShard == QC_STMT_SHARD_DATA, ERR_UNSUPPORTED_STMT );
    }
    else
    {
        IDE_TEST_RAISE( ( sShard == QC_STMT_SHARD_ANALYZE ) ||
                        ( sShard == QC_STMT_SHARD_META ) ||
                        ( sShard == QC_STMT_SHARD_DATA ),
                        ERR_UNSUPPORTED_STMT );
    }
    
    return IDE_SUCCESS;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION( ERR_UNSUPPORTED_STMT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Unsupported shard SQL",
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyze( qcStatement * aStatement,
                     ULong         aSMN )
{
    qciStmtType sStmtType = aStatement->myPlan->parseTree->stmtKind;

    switch ( sStmtType )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:

            IDE_TEST( analyzeSelect( aStatement,
                                     aSMN )
                      != IDE_SUCCESS );

            break;

        case QCI_STMT_INSERT:

            IDE_TEST( analyzeInsert( aStatement,
                                     aSMN )
                      != IDE_SUCCESS );
            break;

        case QCI_STMT_UPDATE:

            IDE_TEST( analyzeUpdate( aStatement,
                                     aSMN )
                      != IDE_SUCCESS );
            break;

        case QCI_STMT_DELETE:

            IDE_TEST( analyzeDelete( aStatement,
                                     aSMN )
                      != IDE_SUCCESS );
            break;

        case QCI_STMT_EXEC_PROC:

            IDE_TEST( analyzeExecProc( aStatement,
                                       aSMN )
                      != IDE_SUCCESS );
            break;

        default:

            IDE_RAISE(ERR_INVALID_STMT_TYPE);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_STMT_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyze",
                                "Invalid stmt type"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeSelect( qcStatement * aStatement,
                           ULong         aSMN )
{
/***********************************************************************
 *
 * Description : SELECT 구문의 분석 과정
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree * sParseTree = (qmsParseTree*)aStatement->myPlan->parseTree;

    //------------------------------------------
    // Analyze SELECT statement
    //------------------------------------------
    IDE_TEST ( analyzeSelectCore( aStatement,
                                  aSMN,
                                  NULL,
                                  ID_FALSE )
               != IDE_SUCCESS );

    if ( sParseTree->querySet->mShardAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        //------------------------------------------
        // PROJ-2655 Composite shard key
        //
        // Shard key에 대해서 분석 한 결과 를 보고,
        // Sub-shard key를 가진 object가 존재하면,
        // Sub-shard key에 대해서 분석을 수행한다.
        //
        //------------------------------------------
        IDE_TEST ( analyzeSelectCore( aStatement,
                                      aSMN,
                                      NULL,
                                      ID_TRUE )
                   != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeSelectCore( qcStatement * aStatement,
                               ULong         aSMN,
                               sdiKeyInfo  * aOutRefInfo,
                               idBool        aIsSubKey )
{
    /* TASK-7219 Shard Transformer Refactoring */
    qcShardStmtType    sStmtType  = aStatement->myPlan->parseTree->stmtShard;
    qmsParseTree     * sParseTree = (qmsParseTree * ) (aStatement->myPlan->parseTree );
    sdiShardAnalysis * sAnalysis  = NULL;

    /* BUG-45823 */
    increaseAnalyzeCount( aStatement );

    /* TASK-7219 Shard Transformer Refactoring
     *  Shard 여부에 상관없이 수행되는
     *   DATA, META 키워드인 구문은 더이상 분석하지 않는다.
     */
    switch ( sStmtType )
    {
        case QC_STMT_SHARD_DATA:
        case QC_STMT_SHARD_META:
            IDE_TEST( allocAnalysis( aStatement,
                                     &( sAnalysis ) )
                      != IDE_SUCCESS );

            sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_SHARD_KEYWORD_EXISTS ] = ID_TRUE;

            if ( aIsSubKey == ID_FALSE )
            {
                sParseTree->querySet->mShardAnalysis = sAnalysis;
                sParseTree->mShardAnalysis           = sAnalysis;
            }
            else
            {
                sParseTree->querySet->mShardAnalysis->mNext = sAnalysis;
                sParseTree->mShardAnalysis->mNext           = sAnalysis;
            }

            IDE_CONT( NORMAL_EXIT );
            break;

        default:
            break;
    }

    /* BUG-48872 */
    if ( ( aIsSubKey == ID_FALSE )
         &&
         ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                          SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE )
           == ID_TRUE ) )
    {
        //------------------------------------------
        // PROJ-1413
        // Query Transformation 수행
        //------------------------------------------
        IDE_TEST( qmo::doTransform( aStatement ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    //------------------------------------------
    // PROJ-2646 shard analyzer enhancement
    // ParseTree의 분석
    //------------------------------------------
    IDE_TEST( analyzeParseTree( aStatement,
                                aSMN,
                                aOutRefInfo,
                                aIsSubKey )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::makeAndSetAnalyzeInfo( qcStatement      * aStatement,
                                   sdiShardAnalysis * aAnalysis,
                                   sdiAnalyzeInfo  ** aAnalyzeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                sda:setAnalysis 함수 이름, 예외처리 변경
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiAnalyzeInfo * sAnalysisResult  = NULL;
    sdiAnalysisFlag  sAnalysisFlag;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aAnalysis == NULL, ERR_NULL_ANALYSIS );

    SDI_INIT_ANALYSIS_FLAG( sAnalysisFlag );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiAnalyzeInfo ),
                                             (void **)&( sAnalysisResult ) )
              != IDE_SUCCESS );

    SDI_INIT_ANALYZE_INFO( sAnalysisResult );

    IDE_TEST( setAnalysisCommonInfo( aStatement,
                                     aAnalysis,
                                     sAnalysisResult )
              != IDE_SUCCESS );

    IDE_TEST( setAnalysisCombinedInfo( aAnalysis,
                                       sAnalysisResult,
                                       &( sAnalysisFlag ) )
              != IDE_SUCCESS );

    IDE_TEST( setAnalysisIndividualInfo( aStatement,
                                         aAnalysis,
                                         &( sAnalysisResult->mValuePtrCount ),
                                         &( sAnalysisResult->mValuePtrArray ),
                                         &( sAnalysisResult->mSplitMethod ),
                                         &( sAnalysisResult->mKeyDataType ) )
              != IDE_SUCCESS );

    if ( sAnalysisResult->mSubKeyExists == ID_TRUE )
    {
        IDE_TEST( setAnalysisIndividualInfo( aStatement,
                                             aAnalysis->mNext,
                                             &( sAnalysisResult->mSubValuePtrCount ),
                                             &( sAnalysisResult->mSubValuePtrArray ),
                                             &( sAnalysisResult->mSubSplitMethod ),
                                             &( sAnalysisResult->mSubKeyDataType ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    if ( sAnalysisResult->mIsShardQuery == ID_FALSE )
    {
        setNonShardQueryReasonErr( &( sAnalysisFlag ),
                                   &( sAnalysisResult->mNonShardQueryReason ) );
    }
    else
    {
        /* Nothing to do. */
    }

    /* BUG-49088
     * FOR UPDATE 에 대한 식별이 필요하여
     * 분석 결과에 mTopQueryFlag를 저장한다.
     */
    IDE_TEST( mergeFlag( sAnalysisFlag.mTopQueryFlag,
                         sAnalysisResult->mTopQueryFlag,
                         SDI_ANALYSIS_TOP_QUERY_FLAG_MAX )
              != IDE_SUCCESS );

    if ( aAnalyzeInfo != NULL )
    {
        *aAnalyzeInfo = sAnalysisResult;
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::makeAndSetAnalyzeInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::makeAndSetAnalyzeInfo",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisCommonInfo( qcStatement      * aStatement,
                                   sdiShardAnalysis * aAnalysis,
                                   sdiAnalyzeInfo   * aAnalysisResult )
{
    // Default nodeID
    aAnalysisResult->mDefaultNodeId = aAnalysis->mShardInfo.mDefaultNodeId;

    // Range info
    IDE_TEST( sdi::allocAndCopyRanges( aStatement,
                                       &aAnalysisResult->mRangeInfo,
                                       aAnalysis->mShardInfo.mRangeInfo )
              != IDE_SUCCESS );

    // Sub shard key existence
    if ( aAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        IDE_DASSERT( aAnalysis->mNext != NULL );

        // Sub-shard key exists
        aAnalysisResult->mSubKeyExists   = ID_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    // Table reference info
    aAnalysisResult->mTableInfoList = aAnalysis->mTableInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisCombinedInfo( sdiShardAnalysis * aAnalysis,
                                     sdiAnalyzeInfo   * aAnalysisResult,
                                     sdiAnalysisFlag  * aAnalysisFlag )
{
    sdiShardAnalysis * sAnalysis = NULL;

    //---------------------------------------
    // Shard query 여부 설정
    //   Non-shard query 라면
    //     1. Error reason 결정
    //     2. Aggregation Transform 가능여부 결정
    //---------------------------------------

    for ( sAnalysis  = aAnalysis;
          sAnalysis != NULL;
          sAnalysis  = sAnalysis->mNext )
    {
        // 첫번째 키 이거나,
        // 이전 결과가 TRUE인 경우에만 수행
        if ( ( sAnalysis == aAnalysis ) ||
             ( aAnalysisResult->mIsShardQuery == ID_TRUE ) )
        {
            // Set Shard query or not
            IDE_TEST( isShardQuery( &( sAnalysis->mAnalysisFlag ),
                                    &( aAnalysisResult->mIsShardQuery ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( mergeAnalysisFlag( &( sAnalysis->mAnalysisFlag ),
                                     aAnalysisFlag,
                                     SDI_ANALYSIS_FLAG_NON_TRUE |
                                     SDI_ANALYSIS_FLAG_TOP_TRUE |
                                     SDI_ANALYSIS_FLAG_SET_TRUE |
                                     SDI_ANALYSIS_FLAG_CUR_TRUE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisIndividualInfo( qcStatement      * aStatement,
                                       sdiShardAnalysis * aAnalysis,
                                       UShort           * aValuePtrCount,
                                       sdiValueInfo*   ** aValuePtrArray,
                                       sdiSplitMethod   * aSplitMethod,
                                       UInt             * aKeyDataType )
{
    sdiKeyInfo * sKeyInfo = NULL;

    sdiValueInfo * sValueInfo = NULL;

    UShort sKeyInfoCount = 0;
    UShort sValueIdx = 0;

    /* TASK-7219 Shard Transformer Refactoring
     *  Shard 여부에 상관없이 Analyze Info를 생성한다.
     */

    // Set split method
    *aSplitMethod = aAnalysis->mShardInfo.mSplitMethod;

    // Set shrad key data type
    *aKeyDataType = aAnalysis->mShardInfo.mKeyDataType;

    // Set shard key value info
    if ( *aSplitMethod == SDI_SPLIT_CLONE )
    {
        //---------------------------------------
        // Split CLONE
        //---------------------------------------
        if ( ( aStatement->myPlan->parseTree->stmtKind != QCI_STMT_SELECT ) &&
             ( aStatement->myPlan->parseTree->stmtKind != QCI_STMT_SELECT_FOR_UPDATE )  )
        {
            *aValuePtrCount = 1;

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo*),
                                                     (void**) aValuePtrArray )
                      != IDE_SUCCESS );

            IDE_TEST( allocAndSetShardHostValueInfo( aStatement,
                                                     0,
                                                     &sValueInfo )
                      != IDE_SUCCESS );

            (*aValuePtrArray)[0] = sValueInfo;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        /* TASJ-7219
         *   BUG-47903 이후, 전 노드 수행 Non-Shard Query 를 고려해서 수정함
         */
        //---------------------------------------
        // Split HASH, LIST, RANGE, SOLO, COMPOSITE
        //---------------------------------------
        for ( sKeyInfo = aAnalysis->mKeyInfo;
              sKeyInfo != NULL;
              sKeyInfo = sKeyInfo->mNext )
        {
            if ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
            {
                sKeyInfoCount++;
            }
            else
            {
                /* Nothing to do. */
            }
        }

        if ( sKeyInfoCount <= 1 )
        {
            for ( sKeyInfo = aAnalysis->mKeyInfo;
                  sKeyInfo != NULL;
                  sKeyInfo = sKeyInfo->mNext )
            {
                if ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                {
                    *aValuePtrCount = sKeyInfo->mValuePtrCount;

                    if ( *aValuePtrCount > 0 )
                    {
                        IDE_TEST_RAISE( sKeyInfo->mValuePtrCount >= SDI_VALUE_MAX_COUNT, ERR_SHARD_VALUE_MAX );

                        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo*) * sKeyInfo->mValuePtrCount,
                                                                 (void**) aValuePtrArray )
                                  != IDE_SUCCESS );

                        for ( sValueIdx = 0;
                              sValueIdx < sKeyInfo->mValuePtrCount;
                              sValueIdx++ )
                        {
                            if ( sKeyInfo->mValuePtrArray[sValueIdx]->mType == SDI_VALUE_INFO_HOST_VAR )
                            {
                                IDE_TEST( allocAndSetShardHostValueInfo(
                                              aStatement,
                                              sKeyInfo->mValuePtrArray[sValueIdx]->mValue.mBindParamId,
                                              &( sValueInfo ) )
                                          != IDE_SUCCESS );
                            }
                            else
                            {
                                IDE_DASSERT( sKeyInfo->mValuePtrArray[sValueIdx]->mType == SDI_VALUE_INFO_CONST_VAL );

                                /* TASK-7219 Shard Transformer Refactoring
                                 *  Shard 여부에 상관없이, Analyze Info 를 생성하여,
                                 *   Shard Info 구성 실패 시 aKeyDataType 이 기본값인 경우가 발생하고,
                                 *    sKeyInfo 에 mValuePtrArray 가 존재하더라도 sValueInfo 를 생성하지 않고 무시한다.
                                 *
                                 *     이후에 해당 Query Set 을 Shard View 로 변환한다면,
                                 *      qmgShardSelect::setShardKeyValue 에서 Shard Analyze 를 재수행할 때에
                                 *       적절한 sValueInfo 로 생성하기를 기대한다.
                                 */
                                if ( *aKeyDataType != ID_UINT_MAX )
                                {
                                    IDE_TEST( allocAndSetShardConstValueInfo(
                                                  aStatement,
                                                  *aKeyDataType,
                                                  (void *)&( sKeyInfo->mValuePtrArray[sValueIdx]->mValue ),
                                                  &( sValueInfo ) )
                                              != IDE_SUCCESS );
                                }
                                else
                                {
                                    sValueInfo = NULL;
                                }
                            }

                            (*aValuePtrArray)[sValueIdx] = sValueInfo;
                        }
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SHARD_VALUE_MAX)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::setAnalysisIndividualInfo",
                                "The number of shard values exceeds the maximum(1000)."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::subqueryAnalysis4NodeTree( qcStatement          * aStatement,
                                       ULong                  aSMN,
                                       qtcNode              * aNode,
                                       sdiKeyInfo           * aOutRefInfo,
                                       idBool                 aIsSubKey,
                                       sdaSubqueryAnalysis ** aSubqueryAnalysis )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/
    sdaSubqueryAnalysis * sSubqueryAnalysis = NULL;

    if ( aNode != NULL )
    {
        if ( QTC_IS_SUBQUERY( aNode ) == ID_TRUE )
        {
            IDE_TEST( analyzeSelectCore( aNode->subquery,
                                         aSMN,
                                         aOutRefInfo,
                                         aIsSubKey )
                      != IDE_SUCCESS );

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdaSubqueryAnalysis),
                                                     (void**) & sSubqueryAnalysis )
                      != IDE_SUCCESS );

            sSubqueryAnalysis->mShardAnalysis =
                ( (qmsParseTree*)aNode->subquery->myPlan->parseTree )->mShardAnalysis;

            sSubqueryAnalysis->mNext = *aSubqueryAnalysis;

            *aSubqueryAnalysis = sSubqueryAnalysis;
        }
        else
        {
            if ( QTC_HAVE_SUBQUERY( aNode ) == ID_TRUE )
            {
                IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                     aSMN,
                                                     (qtcNode*)aNode->node.arguments,
                                                     aOutRefInfo,
                                                     aIsSubKey,
                                                     aSubqueryAnalysis )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }

            if ( aNode->overClause != NULL )
            {
                IDE_TEST( traverseOverColumn4SubqueryAnalysis( aStatement,
                                                               aSMN,
                                                               aNode->overClause->overColumn,
                                                               aOutRefInfo,
                                                               aIsSubKey,
                                                               aSubqueryAnalysis )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }
        }
      
        if ( ( ( aNode->node.module == &mtfEqual ) ||
               ( aNode->node.module == &mtfEqualAny ) ) &&
             ( QTC_HAVE_SUBQUERY( aNode ) == ID_TRUE ) )
        {
            IDE_TEST( traversePredForCheckingShardKeyJoin( aStatement,
                                                           aNode,
                                                           (qtcNode*)aNode->node.arguments,
                                                           (qtcNode*)aNode->node.arguments->next,
                                                           aOutRefInfo,
                                                           aIsSubKey )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }

        IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                             aSMN,
                                             (qtcNode*)aNode->node.next,
                                             aOutRefInfo,
                                             aIsSubKey,
                                             aSubqueryAnalysis )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::traverseOverColumn4SubqueryAnalysis( qcStatement          * aStatement,
                                                 ULong                  aSMN,
                                                 qtcOverColumn        * aOverColumn,
                                                 sdiKeyInfo           * aOutRefInfo,
                                                 idBool                 aIsSubKey,
                                                 sdaSubqueryAnalysis ** aSubqueryAnalysis )
{
    qtcOverColumn * sOverColumn = NULL;

    for ( sOverColumn  = aOverColumn;
          sOverColumn != NULL;
          sOverColumn  = sOverColumn->next )
    {
        if ( QTC_HAVE_SUBQUERY( sOverColumn->node ) == ID_TRUE )
        {
            IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                 aSMN,
                                                 sOverColumn->node,
                                                 aOutRefInfo,
                                                 aIsSubKey,
                                                 aSubqueryAnalysis )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setShardInfo4Subquery( qcStatement         * aStatement,
                                   sdiShardAnalysis    * aQuerySetAnalysis,
                                   sdaSubqueryAnalysis * aSubqueryAnalysis,
                                   idBool                aIsSelect,
                                   idBool                aIsSubKey,
                                   sdiAnalysisFlag     * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
 *  ------------------|----------------|--------------------------------
 *  1.    H/R/L       |      H/R/L     | 1. SubQ의 out ref join 확인
 *  ------------------|----------------|--------------------------------
 *  2.    H/R/L       |       C/S      | 1. Range가 C/S includes H/R/L 인지 확인
 *  ------------------|----------------|--------------------------------
 *
 *  ------------------|----------------|--------------------------------
 *  3.      C         |      H/R/L     | 1. 불가 판정
 *  ------------------|----------------|--------------------------------
 *  4.      C         |       C/S      | 1. SELECT의 경우
 *                    |                |    Range를 intersect
 *                    |                |
 *                    |                | 2. DML의 경우
 *                    |                |    Range가 C = C/S 인지 확인
 *  ------------------|----------------|--------------------------------
 *
 *  ------------------|----------------|--------------------------------
 *  5.      S         |     H/R/L/S    | 1. Range가 S = H/R/L/S 인지 확인
 *  ------------------|----------------|--------------------------------
 *  6.      S         |       C        | 1. Range가 C include S 인지 확인
 *  ------------------|----------------|--------------------------------
 *  7.             COMMON              | 1. SubQ의 non-shard reason 확인
 *  ------------------|----------------|--------------------------------
 *
 * Arguments :
 *
 ***********************************************************************/

    sdaSubqueryAnalysis * sSubqueryAnalysis  = NULL;
    sdiShardInfo        * sSubqueryShardInfo = NULL;
    sdiShardInfo        * sOuterQueryShardInfo = NULL;
    sdiAnalysisFlag     * sSubqueryFlag = NULL;
    sdiRangeInfo        * sOutRangeInfo = NULL; /* TASK-7219 Shard Transformer Refactoring */
    idBool                sOutRefReasonBack  = ID_FALSE;
    idBool                sIsSameShardInfo = ID_FALSE;

    //---------------------------------------
    // Check arguments
    //---------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySetAnalysis != NULL );
    IDE_DASSERT( aSubqueryAnalysis != NULL );
    IDE_DASSERT( aAnalysisFlag != NULL );

    sOuterQueryShardInfo = &aQuerySetAnalysis->mShardInfo;

    for ( sSubqueryAnalysis  = aSubqueryAnalysis;
          sSubqueryAnalysis != NULL;
          sSubqueryAnalysis  = sSubqueryAnalysis->mNext )
    {
        if ( aIsSubKey == ID_FALSE )
        {
            sSubqueryShardInfo =
                &(sSubqueryAnalysis->mShardAnalysis->mShardInfo);

            sSubqueryFlag =
                &( sSubqueryAnalysis->mShardAnalysis->mAnalysisFlag );

            if ( ( sdi::getSplitType( aQuerySetAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) &&
                 ( sdi::getSplitType( sSubqueryShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) &&
                 ( aQuerySetAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] !=
                   sSubqueryFlag->mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] ) )
            {
                aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            IDE_DASSERT( sSubqueryAnalysis->mShardAnalysis->mNext != NULL );

            sSubqueryShardInfo =
                &(sSubqueryAnalysis->mShardAnalysis->mNext->mShardInfo);

            sSubqueryFlag =
                &( sSubqueryAnalysis->mShardAnalysis->mNext->mAnalysisFlag );
        }

        if ( aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] == ID_FALSE )
        {
            if ( sdi::getSplitType( sOuterQueryShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
            {
                if ( sOuterQueryShardInfo->mSplitMethod == sSubqueryShardInfo->mSplitMethod )
                {
                    sIsSameShardInfo = ID_FALSE;

                    IDE_TEST( isSameShardInfo( sOuterQueryShardInfo,
                                               sSubqueryShardInfo,
                                               aIsSubKey,
                                               &sIsSameShardInfo )
                              != IDE_SUCCESS );

                    if ( sIsSameShardInfo == ID_TRUE )
                    {
                        /*  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
                         *  ------------------|----------------|--------------------------------
                         *  1.    H/R/L       |      H/R/L     | 1. SubQ의 out ref join 확인
                         *  ------------------|----------------|--------------------------------
                         */
                        if ( sSubqueryFlag->mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ] == ID_TRUE )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                    else
                    {
                        aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                    }
                }
                else if ( sdi::getSplitType( sSubqueryShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST )
                {
                    /*  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
                     *  ------------------|----------------|--------------------------------
                     *  2.    H/R/L       |       C/S      |  1. Range가 C/S include H/R/L 인지 확인
                     *  ------------------|----------------|--------------------------------
                     */
                    if ( isSubRangeInfo( sSubqueryShardInfo->mRangeInfo,
                                         sOuterQueryShardInfo->mRangeInfo,
                                         sOuterQueryShardInfo->mDefaultNodeId ) == ID_FALSE )
                    {
                        aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                    aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                }
            }
            else if ( sOuterQueryShardInfo->mSplitMethod == SDI_SPLIT_CLONE )
            {
                if ( sdi::getSplitType( sSubqueryShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                {
                    /*  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
                     *  ------------------|----------------|--------------------------------
                     *  3.      C         |      H/R/L     | 1. 불가 판정
                     *  ------------------|----------------|--------------------------------
                     */
                    aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                }
                else if ( sdi::getSplitType( sSubqueryShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST )
                {
                    /*  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
                     *  ------------------|----------------|--------------------------------
                     *  4.      C         |       C/S      | 1. SELECT의 경우
                     *                    |                |    Range를 intersect
                     *                    |                |
                     *                    |                | 2. DML의 경우
                     *                    |                |    Range가 C = C/S 인지 확인
                     *  ------------------|----------------|--------------------------------
                     */
                    if ( aIsSelect == ID_TRUE )
                    {
                        /* TASK-7219 Shard Transformer Refactoring */
                        IDE_TEST( getRangeIntersection( aStatement,
                                                        sOuterQueryShardInfo->mRangeInfo,
                                                        sSubqueryShardInfo->mRangeInfo,
                                                        &( sOutRangeInfo ) )
                                  != IDE_SUCCESS );

                        if ( sOutRangeInfo->mCount == 0 )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                            aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                        }
                        else
                        {
                            sOuterQueryShardInfo->mRangeInfo = sOutRangeInfo;
                        }
                    }
                    else
                    {
                        if ( isSameRangesForCloneAndSolo( sOuterQueryShardInfo->mRangeInfo,
                                                          sSubqueryShardInfo->mRangeInfo ) == ID_FALSE )
                        {
                            /* DML의 경우 range가 C = C/S 인지 확인 */
                            aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                            aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                }
                else
                {
                    IDE_DASSERT( sSubqueryShardInfo->mSplitMethod == SDI_SPLIT_NONE );
                }
            
            }
            else if ( sOuterQueryShardInfo->mSplitMethod == SDI_SPLIT_SOLO )
            {
                if ( ( sdi::getSplitType( sSubqueryShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) ||
                     ( sSubqueryShardInfo->mSplitMethod == SDI_SPLIT_SOLO ) )
                {
                    /*  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
                     *  ------------------|----------------|--------------------------------
                     *  5.      S         |     H/R/L/S    | 1. Range가 S = H/R/L/S 인지 확인
                     *  ------------------|----------------|--------------------------------
                     */
                    if ( isSubRangeInfo( sOuterQueryShardInfo->mRangeInfo, // SOLO
                                         sSubqueryShardInfo->mRangeInfo,
                                         sSubqueryShardInfo->mDefaultNodeId ) == ID_FALSE ) // H/R/L/S
                    {
                        /* Range가 S = H/R/L/S 인지 확인 */
                        aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else if ( sSubqueryShardInfo->mSplitMethod == SDI_SPLIT_CLONE )
                {
                    /*  OUTER QUERY BLOCK | SUBQUERY BLOCK |           ANALYSIS
                     *  ------------------|----------------|--------------------------------
                     *  6.      S         |       C        | 1. Range가 C include S 인지 확인
                     *  ------------------|----------------|--------------------------------
                     */
                    if ( isSubRangeInfo( sSubqueryShardInfo->mRangeInfo, // SOLO
                                         sOuterQueryShardInfo->mRangeInfo,
                                         sOuterQueryShardInfo->mDefaultNodeId ) == ID_FALSE ) // CLONE
                    {
                        /* Range가 C include S 인지 확인 */
                        aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    IDE_DASSERT( sSubqueryShardInfo->mSplitMethod == SDI_SPLIT_NONE );
                }
            }
            else
            {
                IDE_DASSERT( sOuterQueryShardInfo->mSplitMethod == SDI_SPLIT_NONE );
                aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
            }
        }
        else
        {
            /* Nothing to do. */
        }

        /* 7. SubQ의 non-shard reason 확인 */
        sOutRefReasonBack = aAnalysisFlag->mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ];

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( mergeAnalysisFlag( sSubqueryFlag,
                                     aAnalysisFlag,
                                     SDI_ANALYSIS_FLAG_NON_TRUE |
                                     SDI_ANALYSIS_FLAG_TOP_TRUE )
                  != IDE_SUCCESS );

        aAnalysisFlag->mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ] = sOutRefReasonBack;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::traversePredForCheckingShardKeyJoin( qcStatement * aStatement,
                                                 qtcNode     * aCompareNode,
                                                 qtcNode     * aLeftNode,
                                                 qtcNode     * aRightNode,
                                                 sdiKeyInfo  * aOutRefInfo,
                                                 idBool        aIsSubKey )
{
    qtcNode * sLeftArgs  = NULL;
    qtcNode * sRightArgs = NULL;

    if ( aLeftNode->node.module == &mtfList )
    {
        if ( aRightNode->node.module == &mtfList )
        {
            /* QTC_IS_LIST 는 mtfList와 return이 두 개 이상인 qtc::subqueryModule을 포함한다. */
            if ( QTC_IS_LIST( (qtcNode*)aRightNode->node.arguments ) == ID_TRUE )
            {
                //---------------------------------------
                // ( I1, I2 ) IN ( ( ( SELECT I1 FROM T1 ), I2 ), ( I2, I3 ) )
                // ^             ^ 
                //
                // ( I1, I2 ) IN ( ( SELECT I1, I1 FROM T1 ), ( I2, I3 ) )
                // ^             ^
                //---------------------------------------
                for ( sRightArgs  = (qtcNode*)aRightNode->node.arguments;
                      sRightArgs != NULL;
                      sRightArgs  = (qtcNode*)sRightArgs->node.next )
                {
                    // Recursive call
                    IDE_TEST( traversePredForCheckingShardKeyJoin( aStatement,
                                                                   aCompareNode,
                                                                   aLeftNode, // aLeftArgs
                                                                   sRightArgs,
                                                                   aOutRefInfo,
                                                                   aIsSubKey )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                //---------------------------------------
                // ( I1, I2 ) = ( ( SELECT I1 FROM T1 ), I1 )
                // ^            ^
                //
                // ( I1, I2 ) IN ( ( ( SELECT I1 FROM T1 ), I2 ), ( I2, I3 ) )
                // ^               ^                              ^
                //---------------------------------------
                for ( sLeftArgs  = (qtcNode*)aLeftNode->node.arguments,
                          sRightArgs = (qtcNode*)aRightNode->node.arguments;
                      ( ( sLeftArgs != NULL ) ||
                        ( sRightArgs != NULL ) );
                      sLeftArgs  = (qtcNode*)sLeftArgs->node.next,
                          sRightArgs  = (qtcNode*)sRightArgs->node.next )
                {
                    // Conversion error
                    IDE_TEST_CONT( sLeftArgs->node.module == &mtfList, NORMAL_EXIT );

                    // Recursive call
                    IDE_TEST( traversePredForCheckingShardKeyJoin( aStatement,
                                                                   aCompareNode,
                                                                   sLeftArgs, // sLeftArgs ( an argument of aLeftNode)
                                                                   sRightArgs,
                                                                   aOutRefInfo,
                                                                   aIsSubKey )
                              != IDE_SUCCESS );
                }
            }
        }
        else if ( QTC_IS_SUBQUERY( aRightNode ) == ID_TRUE )
        {
            //---------------------------------------
            // ( I1, I2 ) IN ( SELECT I1, I2 FROM T1 )
            // ^             ^
            //
            // ( I1, I2 ) IN ( ( SELECT I1, I2 FROM T1 ), ( I1, I2 ) )
            // ^               ^
            //---------------------------------------
            IDE_TEST( checkSubqueryAndTheOtherSideNode( aStatement,
                                                        aRightNode,
                                                        aLeftNode,
                                                        aOutRefInfo,
                                                        aIsSubKey )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else if ( QTC_IS_SUBQUERY( aLeftNode ) )
    {
        if ( aCompareNode->node.module == &mtfEqual )
        {
            //---------------------------------------
            // ( SELECT I1 FROM T1 ) = I1
            // ^                       ^
            //
            // ( SELECT I1,I2 FROM T1 ) = ( I1, I2 )
            // ^                          ^
            //
            // ( ( SELECT I1 FROM T1 ), I2 ) = ( I1, I2 )
            //   ^                               ^
            //---------------------------------------
            if ( ( aRightNode->node.module == &mtfList ) ||
                 QTC_IS_COLUMN( aStatement, aRightNode ) )
            {
                IDE_TEST( checkSubqueryAndTheOtherSideNode( aStatement,
                                                            aLeftNode,
                                                            aRightNode,
                                                            aOutRefInfo,
                                                            aIsSubKey )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            //---------------------------------------
            // ( ( SELECT I1 FROM T1 ), I2 ) IN ( ( I1, I2 ), ( I3, I4 ) )
            //   ^                              ^
            //---------------------------------------
            /* Nothing to do. */
        }
    }
    else if ( QTC_IS_COLUMN( aStatement, aLeftNode ) == ID_TRUE )
    {
        if ( aRightNode->node.module == &mtfList )
        {
            //---------------------------------------
            // I1 IN ( ( SELECT I1 FROM T1), I2 )
            // ^     ^
            //---------------------------------------
            for ( sRightArgs  = (qtcNode*)aRightNode->node.arguments;
                  sRightArgs != NULL;
                  sRightArgs  = (qtcNode*)sRightArgs->node.next )
            {
                // Recursive call
                IDE_TEST( traversePredForCheckingShardKeyJoin( aStatement,
                                                               aCompareNode,
                                                               aLeftNode,
                                                               sRightArgs,
                                                               aOutRefInfo,
                                                               aIsSubKey )
                          != IDE_SUCCESS );
            }
        }
        else if ( QTC_IS_SUBQUERY( aRightNode ) == ID_TRUE )
        {
            //---------------------------------------
            // I1 IN ( SELECT I1 FROM T1 )
            // ^     ^
            //
            // ( I1, I2 ) IN ( ( ( SELECT I1 FROM T1 ), I2 ), ( I2, I3 ) )
            //   ^               ^                           
            //---------------------------------------
            IDE_TEST( checkSubqueryAndTheOtherSideNode( aStatement,
                                                        aRightNode,
                                                        aLeftNode,
                                                        aOutRefInfo,
                                                        aIsSubKey )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkSubqueryAndTheOtherSideNode( qcStatement * aStatement,
                                              qtcNode     * aSubquery,
                                              qtcNode     * aOuterColumn,
                                              sdiKeyInfo  * aOutRefInfo,
                                              idBool        aIsSubKey )
{
    sdiShardAnalysis *sSubQAnalysis = NULL;
    qtcNode          *sListArgs     = NULL;

    sdaQtcNodeType  sQtcNodeType        = SDA_NONE;
    sdiKeyInfo     *sOuterColumnKeyInfo = NULL;
    UShort          sShardKeyColumnIdx  = 0;

    IDE_DASSERT( ( QTC_IS_COLUMN( aStatement, aOuterColumn ) == ID_TRUE ||
                   aOuterColumn->node.module == &mtfList ) );

    IDE_DASSERT( QTC_IS_SUBQUERY( aSubquery ) == ID_TRUE );

    sSubQAnalysis =
        ((qmsParseTree*)aSubquery->subquery->myPlan->parseTree)->mShardAnalysis;

    if ( aOuterColumn->node.module == &mtfList )
    {
        //---------------------------------------
        // ( I1, I2 ) IN ( SELECT I1, I2 FROM T1 )
        // ^             ^
        //---------------------------------------
        for ( sListArgs  = (qtcNode*)aOuterColumn->node.arguments, sShardKeyColumnIdx = 0;
              sListArgs != NULL;
              sListArgs  = (qtcNode*)sListArgs->node.next, sShardKeyColumnIdx++ )
        {
            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aOutRefInfo,
                                                 sListArgs,
                                                 &sQtcNodeType,
                                                 &sOuterColumnKeyInfo )
                      != IDE_SUCCESS );

            if ( sQtcNodeType == SDA_KEY_COLUMN )
            {
                IDE_TEST( resetSubqueryAnalysisReasonForTargetLink( sSubQAnalysis,
                                                                    &sOuterColumnKeyInfo->mShardInfo,
                                                                    sShardKeyColumnIdx,
                                                                    aIsSubKey,
                                                                    NULL )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        //---------------------------------------
        // I1 IN ( SELECT I1 FROM T1 )
        // ^     ^
        //
        // ( I1, I2 ) IN ( ( SELECT I1 FROM T1 ), I1 )
        //   ^             ^
        //---------------------------------------
        IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                             aOutRefInfo,
                                             aOuterColumn,
                                             &sQtcNodeType,
                                             &sOuterColumnKeyInfo )
                  != IDE_SUCCESS );

        if ( sQtcNodeType == SDA_KEY_COLUMN )
        {
            // The subquery target column count is the only one
            IDE_TEST( resetSubqueryAnalysisReasonForTargetLink( sSubQAnalysis,
                                                                &sOuterColumnKeyInfo->mShardInfo,
                                                                sShardKeyColumnIdx, // Initial value 0 
                                                                aIsSubKey,
                                                                NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::resetSubqueryAnalysisReasonForTargetLink( sdiShardAnalysis * aSubQAnalysis,
                                                      sdiShardInfo     * aShardInfo,
                                                      UShort             aShardKeyColumnIdx,
                                                      idBool             aIsSubKey,
                                                      sdiKeyInfo      ** aKeyInfoRet  )
{
    sdiShardAnalysis * sSubQAnalysis = NULL;
    sdiKeyInfo       * sSubQKeyInfo  = NULL;
    UShort             sKeyTargetIdx = 0;
    idBool             sIsSameShardInfo = ID_FALSE;
    idBool             sIsFound = ID_FALSE;

    sSubQAnalysis = ( aIsSubKey == ID_FALSE )?
        (aSubQAnalysis):
        (aSubQAnalysis->mNext);

    sSubQKeyInfo = sSubQAnalysis->mKeyInfo;

    for (;
         sSubQKeyInfo != NULL;
         sSubQKeyInfo  = sSubQKeyInfo->mNext )
    {
        for ( sKeyTargetIdx = 0;              
              sKeyTargetIdx < sSubQKeyInfo->mKeyTargetCount;
              sKeyTargetIdx++ )
        {
            if ( sSubQKeyInfo->mKeyTarget[sKeyTargetIdx] == aShardKeyColumnIdx )
            {
                IDE_TEST( isSameShardInfo( &sSubQKeyInfo->mShardInfo,
                                           aShardInfo,
                                           aIsSubKey,
                                           &sIsSameShardInfo )
                          != IDE_SUCCESS );

                if ( sIsSameShardInfo == ID_TRUE )
                {
                    sSubQAnalysis->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ] = ID_FALSE;

                    if ( aKeyInfoRet != NULL )
                    {
                        *aKeyInfoRet = sSubQKeyInfo;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    /* Nothing to do. */
                }

                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do. */
            }
        }

        if ( sIsFound == ID_TRUE )
        {
            break;
        }
        else
        {
            /* Nothing to do. */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sda::isSameRangesForCloneAndSolo( sdiRangeInfo * aRangeInfo1,
                                         sdiRangeInfo * aRangeInfo2 )
{
    idBool sIsSameRanges = ID_FALSE;
    UShort idx1 = 0;
    UShort idx2 = 0;
    UShort sFoundCount = 0;

    if ( aRangeInfo1->mCount == aRangeInfo2->mCount )
    {
        for ( ;
              idx1 < aRangeInfo1->mCount;
              idx1++ )
        {
            for( idx2 = 0;
                 idx2 < aRangeInfo2->mCount;
                 idx2++ )
            {
                if ( aRangeInfo1->mRanges[idx1].mNodeId == aRangeInfo2->mRanges[idx2].mNodeId )
                {
                    sFoundCount++;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }

        if ( sFoundCount == aRangeInfo1->mCount )
        {
            sIsSameRanges = ID_TRUE;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return sIsSameRanges;
}

IDE_RC sda::subqueryAnalysis4ParseTree( qcStatement             * aStatement,
                                        ULong                     aSMN,
                                        qmsParseTree            * aParseTree,
                                        idBool                    aIsSubKey )
{
    sdiShardAnalysis    * sAnalysis = NULL;
    qmsSortColumns      * sSortColumn = NULL;
    sdaSubqueryAnalysis * sSubqueryAnalysis = NULL;

    IDE_DASSERT( aParseTree != NULL );

    // SET OUT REF INFO
    sAnalysis = aParseTree->mShardAnalysis;

    // ORDER BY
    for ( sSortColumn = aParseTree->orderBy;
          sSortColumn != NULL;
          sSortColumn = sSortColumn->next )
    {
        IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                             aSMN,
                                             sSortColumn->sortColumn,
                                             sAnalysis->mKeyInfo,
                                             aIsSubKey,
                                             &sSubqueryAnalysis )
                  != IDE_SUCCESS );
    }

    if ( sSubqueryAnalysis != NULL )
    {
        IDE_TEST( setShardInfo4Subquery( aStatement,
                                         sAnalysis,
                                         sSubqueryAnalysis,
                                         ID_TRUE, // aIsSelect
                                         aIsSubKey,
                                         &( sAnalysis->mAnalysisFlag ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::subqueryAnalysis4SFWGH( qcStatement              * aStatement,
                                    ULong                      aSMN,
                                    qmsSFWGH                 * aSFWGH,
                                    sdiShardAnalysis         * aQuerySetAnalysis,
                                    idBool                     aIsSubKey )
{
    sdaSubqueryAnalysis * sSubqueryAnalysis = NULL;

    qmsSortColumns * sSiblings = NULL;
    qmsFrom        * sFrom     = NULL;

    sdiAnalysisFlag * sAnalysisFlag = NULL;

    // SET OUT REF INFO
    sAnalysisFlag = &( aQuerySetAnalysis->mAnalysisFlag );

    // SELECT
    IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                         aSMN,
                                         aSFWGH->target->targetColumn,
                                         aQuerySetAnalysis->mKeyInfo,
                                         aIsSubKey,
                                         &sSubqueryAnalysis )
              != IDE_SUCCESS );

    // FROM
    for ( sFrom = aSFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        IDE_TEST( subqueryAnalysis4FromTree( aStatement,
                                             aSMN,
                                             sFrom,
                                             aQuerySetAnalysis->mKeyInfo,
                                             aIsSubKey,
                                             &sSubqueryAnalysis )
                  != IDE_SUCCESS );
    }

    // WHERE
    IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                         aSMN,
                                         aSFWGH->where,
                                         aQuerySetAnalysis->mKeyInfo,
                                         aIsSubKey,
                                         &sSubqueryAnalysis )
              != IDE_SUCCESS );

    // HIERARCHY
    if ( aSFWGH->hierarchy != NULL )
    {
        IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                             aSMN,
                                             aSFWGH->hierarchy->startWith,
                                             aQuerySetAnalysis->mKeyInfo,
                                             aIsSubKey,
                                             &sSubqueryAnalysis )
                  != IDE_SUCCESS );

        IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                             aSMN,
                                             aSFWGH->hierarchy->connectBy,
                                             aQuerySetAnalysis->mKeyInfo,
                                             aIsSubKey,
                                             &sSubqueryAnalysis )
                  != IDE_SUCCESS );

        for ( sSiblings  = aSFWGH->hierarchy->siblings;
              sSiblings != NULL;
              sSiblings  = sSiblings->next )
        {
            IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                 aSMN,
                                                 sSiblings->sortColumn,
                                                 aQuerySetAnalysis->mKeyInfo,
                                                 aIsSubKey,
                                                 &sSubqueryAnalysis )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do. */
    }

    // HAVING
    IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                         aSMN,
                                         aSFWGH->having,
                                         aQuerySetAnalysis->mKeyInfo,
                                         aIsSubKey,
                                         &sSubqueryAnalysis )
              != IDE_SUCCESS );

    if ( sSubqueryAnalysis != NULL )
    {
        IDE_TEST( setShardInfo4Subquery( aStatement,
                                         aQuerySetAnalysis,
                                         sSubqueryAnalysis,
                                         ID_TRUE, // aIsSelect
                                         aIsSubKey,
                                         sAnalysisFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::subqueryAnalysis4FromTree( qcStatement          * aStatement,
                                       ULong                  aSMN,
                                       qmsFrom              * aFrom,
                                       sdiKeyInfo           * aOutRefInfo,
                                       idBool                 aIsSubKey,
                                       sdaSubqueryAnalysis ** aSubqueryAnalysis )
{
    if ( aFrom != NULL )
    {
        if ( aFrom->joinType != QMS_NO_JOIN )
        {
            IDE_TEST( subqueryAnalysis4FromTree( aStatement,
                                                 aSMN,
                                                 aFrom->left,
                                                 aOutRefInfo,
                                                 aIsSubKey,
                                                 aSubqueryAnalysis )
                      != IDE_SUCCESS );

            IDE_TEST( subqueryAnalysis4FromTree( aStatement,
                                                 aSMN,
                                                 aFrom->right,
                                                 aOutRefInfo,
                                                 aIsSubKey,
                                                 aSubqueryAnalysis )
                      != IDE_SUCCESS );

            IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                 aSMN,
                                                 aFrom->onCondition,
                                                 aOutRefInfo,
                                                 aIsSubKey,
                                                 aSubqueryAnalysis )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeParseTree( qcStatement * aStatement,
                              ULong         aSMN,
                              sdiKeyInfo  * aOutRefInfo,
                              idBool        aIsSubKey )
{
    qmsParseTree * sParseTree = (qmsParseTree*)aStatement->myPlan->parseTree;
    qmsQuerySet  * sQuerySet  = sParseTree->querySet;

    //------------------------------------------
    // QuerySet의 분석
    //------------------------------------------
    IDE_TEST( analyzeQuerySet( aStatement,
                               aSMN,
                               sQuerySet,
                               aOutRefInfo,
                               aIsSubKey ) != IDE_SUCCESS );

    if ( aIsSubKey == ID_FALSE )
    {
        IDE_DASSERT( sQuerySet->mShardAnalysis != NULL );
        sParseTree->mShardAnalysis = sQuerySet->mShardAnalysis;
    }
    else
    {
        /* Nothing to do. */
    }

    //------------------------------------------
    // Analyze SUB-QUERY
    //------------------------------------------
    IDE_TEST( subqueryAnalysis4ParseTree( aStatement,
                                          aSMN,
                                          sParseTree,
                                          aIsSubKey )
              != IDE_SUCCESS );

    //------------------------------------------
    // Check nonShardReason for parseTree
    //------------------------------------------
    IDE_TEST( setAnalysisFlag4ParseTree( sParseTree,
                                         aSMN,
                                         aIsSubKey )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeQuerySet( qcStatement * aStatement,
                             ULong         aSMN,
                             qmsQuerySet * aQuerySet,
                             sdiKeyInfo  * aOutRefInfo,
                             idBool        aIsSubKey )
{
    if ( aQuerySet->setOp != QMS_NONE )
    {
        //------------------------------------------
        // SET operator일 경우
        // Left, right query set을 각각 analyze 한 후
        // 두 개의 analysis info로 SET operator의 analyze 정보를 생성한다.
        //------------------------------------------
        IDE_TEST( analyzeQuerySet( aStatement,
                                   aSMN,
                                   aQuerySet->left,
                                   aOutRefInfo,
                                   aIsSubKey )
                  != IDE_SUCCESS );

        IDE_TEST( analyzeQuerySet( aStatement,
                                   aSMN,
                                   aQuerySet->right,
                                   aOutRefInfo,
                                   aIsSubKey )
                  != IDE_SUCCESS );

        IDE_TEST( analyzeQuerySetAnalysis( aStatement,
                                           aQuerySet,
                                           aQuerySet->left,
                                           aQuerySet->right,
                                           aIsSubKey )
                  != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------------
        // SELECT ~ HAVING의 분석
        //------------------------------------------
        IDE_TEST( analyzeSFWGH( aStatement,
                                aSMN,
                                aQuerySet,
                                aOutRefInfo,
                                aIsSubKey )
                  != IDE_SUCCESS );
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( setAnalysisFlag( aQuerySet,
                               aIsSubKey )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeSFWGH( qcStatement * aStatement,
                          ULong         aSMN,
                          qmsQuerySet * aQuerySet,
                          sdiKeyInfo  * aOutRefInfo,
                          idBool        aIsSubKey )
{
    sdiShardAnalysis * sQuerySetAnalysis = NULL;
    qtcNode          * sShardPredicate   = NULL;

    //------------------------------------------
    // 정합성
    //------------------------------------------

    IDE_TEST_RAISE( aQuerySet->SFWGH == NULL, ERR_NULL_SFWGH );

    //------------------------------------------
    // 할당
    //------------------------------------------

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( allocAndSetAnalysis( aStatement,
                                   aStatement->myPlan->parseTree,
                                   aQuerySet,
                                   aIsSubKey,
                                   &( sQuerySetAnalysis ) )
              != IDE_SUCCESS );

    IDE_TEST( getShardPredicate( aStatement,
                                 aQuerySet,
                                 ID_TRUE,
                                 aIsSubKey,
                                 &( sShardPredicate ) )
              != IDE_SUCCESS );

    IDE_TEST ( analyzeSFWGHCore( aStatement,
                                 aSMN,
                                 aQuerySet,
                                 sQuerySetAnalysis,
                                 sShardPredicate,
                                 aOutRefInfo,
                                 aIsSubKey )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_SFWGH)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeSFWGH",
                                "The SFWGH is NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeSFWGHCore( qcStatement      * aStatement,
                              ULong              aSMN,
                              qmsQuerySet      * aQuerySet,
                              sdiShardAnalysis * aQuerySetAnalysis,
                              qtcNode          * aShardPredicate,
                              sdiKeyInfo       * aOutRefInfo,
                              idBool             aIsSubKey )
{
    sdaFrom    * sShardFromInfo    = NULL;
    idBool       sIsShardQueryAble = ID_TRUE;
    sdaCNFList * sCNFList          = NULL;
    qmsFrom    * sFrom             = NULL;
    idBool       sIsOneNodeSQL     = ID_TRUE;
    UInt         sDistKeyCount     = 0;

    //------------------------------------------
    // Analyze FROM
    //------------------------------------------

    for ( sFrom = aQuerySet->SFWGH->from; sFrom != NULL; sFrom = sFrom->next )
    {
        IDE_TEST( analyzeFrom( aStatement,
                               aSMN,
                               sFrom,
                               aQuerySetAnalysis,
                               &sShardFromInfo,
                               aIsSubKey )
                  != IDE_SUCCESS );
    }

    if ( sShardFromInfo != NULL )
    {
        /*
         * Query에 사용 된 shard table들의 분산 속성이
         * 모두 clone이거나, solo가 포함 되어 있으면
         * Shard analysis의 결과는 수행 노드가 단일 노드로 판별되거나,
         * 수행될 수 없는 것 으로 판별 되기 때문에
         * Semi/anti-join과 outer-join의 일부 제약사항의 영향을 받지 않는다.
         */
        IDE_TEST( isOneNodeSQL( sShardFromInfo,
                                NULL,
                                &sIsOneNodeSQL,
                                &sDistKeyCount )
                  != IDE_SUCCESS );

        if ( sIsOneNodeSQL == ID_FALSE )
        {
            //------------------------------------------
            // Check ANSI-style outer join
            //------------------------------------------
            for ( sFrom = aQuerySet->SFWGH->from; sFrom != NULL; sFrom = sFrom->next )
            {
                IDE_TEST( analyzeAnsiJoin( aStatement,
                                           aQuerySet,
                                           sFrom,
                                           sShardFromInfo,
                                           &( sCNFList ),
                                           &( aQuerySetAnalysis->mAnalysisFlag ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // Nothing to do.
        }

        //------------------------------------------
        // Analyze WHERE
        //------------------------------------------

        IDE_TEST( analyzePredicate( aStatement,
                                    aShardPredicate,
                                    sCNFList,
                                    sShardFromInfo,
                                    sIsOneNodeSQL,
                                    aOutRefInfo,
                                    &( aQuerySetAnalysis->mKeyInfo ),
                                    &( aQuerySetAnalysis->mAnalysisFlag ),
                                    aIsSubKey )
                  != IDE_SUCCESS );

        //------------------------------------------
        // Make Shard Info regarding for clone
        //------------------------------------------

        IDE_TEST( setShardInfoWithKeyInfo( aStatement,
                                           aQuerySetAnalysis->mKeyInfo,
                                           &aQuerySetAnalysis->mShardInfo,
                                           aIsSubKey )
                  != IDE_SUCCESS );

        if ( aQuerySetAnalysis->mShardInfo.mSplitMethod != SDI_SPLIT_NONE )
        {
            /* TASJ-7219
             *   BUG-47903 이후, 전 노드 수행 Non-Shard Query 를 고려해서 수정함 
             */
            //------------------------------------------
            // Check candidate shard query
            //------------------------------------------
            IDE_TEST( isShardQuery( &( aQuerySetAnalysis->mAnalysisFlag ),
                                    &( sIsShardQueryAble ) )
                      != IDE_SUCCESS );

            if ( sIsShardQueryAble == ID_TRUE )
            {
                //------------------------------------------
                // Set Non-shard query reason
                //------------------------------------------
                IDE_TEST( setAnalysisFlag4SFWGH( aStatement,
                                                 aQuerySet->SFWGH,
                                                 aQuerySetAnalysis->mKeyInfo,
                                                 &( aQuerySetAnalysis->mAnalysisFlag ) )
                          != IDE_SUCCESS );

                //---------------------------------------
                // BUG-47642 Check window function
                //---------------------------------------
                IDE_TEST( setAnalysisFlag4WindowFunc( aQuerySet->analyticFuncList,
                                                      aQuerySetAnalysis->mKeyInfo,
                                                      &( aQuerySetAnalysis->mAnalysisFlag ) )
                          != IDE_SUCCESS );

                /* TASJ-7219
                 *   BUG-47903 이후, 전 노드 수행 Non-Shard Query 를 고려해서 수정함 
                 */
                //------------------------------------------
                // Check candidate shard query
                //------------------------------------------
                IDE_TEST( isShardQuery( &( aQuerySetAnalysis->mAnalysisFlag ),
                                        &( sIsShardQueryAble ) )
                          != IDE_SUCCESS );

                if ( sIsShardQueryAble == ID_TRUE )
                {
                    if ( sdi::getSplitType( aQuerySetAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                    {
                        //------------------------------------------
                        // Analyze TARGET
                        //------------------------------------------
                        IDE_TEST( analyzeTarget( aStatement,
                                                 aQuerySet,
                                                 aQuerySetAnalysis->mKeyInfo )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    /* Non-shard reason was aleady set. */
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Non-shard reason was aleady set. */
                /* Nothing to do. */
            }
        }
        else
        {
            aQuerySetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
        }
    }
    else
    {
        //------------------------------------------
        // Shard table이 존재하지 않는 SFWGH
        //------------------------------------------
        aQuerySetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NON_SHARD_OBJECT_EXISTS ] = ID_TRUE;
    }

    /* TASK-7219 Shard Transformer Refactoring */
    //------------------------------------------
    // Analyze SUB-QUERY
    //------------------------------------------
    IDE_TEST( subqueryAnalysis4SFWGH( aStatement,
                                      aSMN,
                                      aQuerySet->SFWGH,
                                      aQuerySetAnalysis,
                                      aIsSubKey )
              != IDE_SUCCESS );

    //------------------------------------------
    // Make Query set analysis
    //------------------------------------------

    IDE_TEST( isShardQuery( &( aQuerySetAnalysis->mAnalysisFlag ),
                            &( aQuerySetAnalysis->mIsShardQuery ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeFrom( qcStatement       * aStatement,
                         ULong               aSMN,
                         qmsFrom           * aFrom,
                         sdiShardAnalysis  * aQuerySetAnalysis,
                         sdaFrom          ** aShardFromInfo,
                         idBool              aIsSubKey )
{
    qmsParseTree     * sViewParseTree = NULL;
    sdiShardAnalysis * sViewAnalysis  = NULL;

    if ( aFrom != NULL )
    {
        if ( aFrom->joinType == QMS_NO_JOIN )
        {
            //---------------------------------------
            // Check unsupported from operation
            //---------------------------------------
            IDE_TEST( checkTableRef( aStatement,
                                     aFrom->tableRef,
                                     aQuerySetAnalysis )
                      != IDE_SUCCESS );

            if ( aFrom->tableRef->view != NULL )
            {
                sViewParseTree = (qmsParseTree*)aFrom->tableRef->view->myPlan->parseTree;

                if ( sViewParseTree->isShardView == ID_TRUE )
                {
                    aQuerySetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_SHARD_KEYWORD_EXISTS ] = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }

                //---------------------------------------
                // View statement의 analyze 수행
                //---------------------------------------
                IDE_TEST( analyzeSelectCore( aFrom->tableRef->view,
                                             aSMN,
                                             NULL,
                                             aIsSubKey ) != IDE_SUCCESS );

                sViewAnalysis = sViewParseTree->mShardAnalysis;

                /* TASK-7219 Shard Transformer Refactoring */
                IDE_TEST( mergeAnalysisFlag( &( sViewAnalysis->mAnalysisFlag ),
                                             &( aQuerySetAnalysis->mAnalysisFlag ),
                                             SDI_ANALYSIS_FLAG_NON_TRUE |
                                             SDI_ANALYSIS_FLAG_TOP_TRUE |
                                             SDI_ANALYSIS_FLAG_SET_TRUE )
                          != IDE_SUCCESS );

                //---------------------------------------
                // View에 대한 shard from info를 생성하고, list에 연결한다.
                //---------------------------------------
                IDE_TEST( makeShardFromInfo4View( aStatement,
                                                  aFrom,
                                                  aQuerySetAnalysis,
                                                  sViewAnalysis,
                                                  aShardFromInfo,
                                                  aIsSubKey )
                          != IDE_SUCCESS );
            }
            else
            {
                /* TASK-7219 Shard Transformer Refactoring */
                //---------------------------------------
                // Table에 대한 shard from info를 생성하고, list에 연결한다.
                //---------------------------------------
                if ( aStatement->myPlan->parseTree->stmtShard != QC_STMT_SHARD_META )
                {
                    IDE_TEST( makeShardFromInfo4Table( aStatement,
                                                       aSMN,
                                                       aFrom,
                                                       aQuerySetAnalysis,
                                                       aShardFromInfo,
                                                       aIsSubKey )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }
        else
        {
            //---------------------------------------
            // ANSI STANDARD STYLE JOIN
            //---------------------------------------

            IDE_TEST( analyzeFrom( aStatement,
                                   aSMN,
                                   aFrom->left,
                                   aQuerySetAnalysis,
                                   aShardFromInfo,
                                   aIsSubKey )
                      != IDE_SUCCESS );

            IDE_TEST( analyzeFrom( aStatement,
                                   aSMN,
                                   aFrom->right,
                                   aQuerySetAnalysis,
                                   aShardFromInfo,
                                   aIsSubKey )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkTableRef( qcStatement      * aStatement,
                           qmsTableRef      * aTableRef,
                           sdiShardAnalysis * aQuerySetAnalysis )
{
    qcuSqlSourceInfo sqlInfo;

    if ( ( aTableRef->flag & QMS_TABLE_REF_LATERAL_VIEW_MASK ) ==
         QMS_TABLE_REF_LATERAL_VIEW_TRUE )
    {
        aQuerySetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_LATERAL_VIEW_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    if ( ( aTableRef->pivot != NULL ) ||
         ( aTableRef->unpivot != NULL ) )
    {
        aQuerySetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_PIVOT_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nohting to do. */
    }

    if ( aTableRef->recursiveView != NULL )
    {
        /* TASK-7219 Shard Transformer Refactoring */
        if ( QC_IS_NULL_NAME( aTableRef->recursiveView->myPlan->parseTree->stmtPos ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, &( aTableRef->recursiveView->myPlan->parseTree->stmtPos ) );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );
        }

        IDE_RAISE( ERR_RECURSIVE_VIEW_EXISTS );

        aQuerySetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_RECURSIVE_VIEW_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION( ERR_RECURSIVE_VIEW_EXISTS )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  sdi::getNonShardQueryReasonArr( SDI_RECURSIVE_VIEW_EXISTS ),
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC sda::makeShardFromInfo4Table( qcStatement       * aStatement,
                                     ULong               aSMN,
                                     qmsFrom           * aFrom,
                                     sdiShardAnalysis  * aQuerySetAnalysis,
                                     sdaFrom          ** aShardFromInfo,
                                     idBool              aIsSubKey )
{
    sdaFrom    * sShardFrom      = NULL;
    UInt         sColumnOrder    = 0;
    UShort     * sKeyColumnOrder = NULL;

    sdiObjectInfo * sShardObjInfo = NULL;

    /* TASK-7219 Shard Transformer Refactoring */
    //---------------------------------------
    // Shard object의 획득
    //---------------------------------------
    IDE_TEST( checkAndGetShardObjInfo( aSMN,
                                       &( aQuerySetAnalysis->mAnalysisFlag ),
                                       aFrom->tableRef->mShardObjInfo,
                                       ID_TRUE, /* aIsSelect */
                                       aIsSubKey,
                                       &( sShardObjInfo ) )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sShardObjInfo == NULL, NORMAL_EXIT );

    //---------------------------------------
    // Shard from의 할당
    //---------------------------------------
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            sdaFrom,
                            (void*) &sShardFrom )
              != IDE_SUCCESS );

    SDA_INIT_SHARD_FROM( sShardFrom );

    //---------------------------------------
    // Shard from의 설정
    //---------------------------------------
    sShardFrom->mTupleId  = aFrom->tableRef->table;

    // Table info의 shard key로 shardFromInfo의 shard key를 세팅한다.
    for ( sColumnOrder = 0;
          sColumnOrder < aFrom->tableRef->tableInfo->columnCount;
          sColumnOrder++ )
    {
        if ( ( aIsSubKey == ID_FALSE ) &&
             ( sShardObjInfo->mKeyFlags[sColumnOrder] == 1 ) )
        {
            /*
             * 첫 번 째 shard key를 analyze 대상으로 설정한다.
             */

            sShardFrom->mKeyCount++;

            // Shard key가 두 개인 table은 존재 할 수 없다. ( break 하지 않고 검사 )
            IDE_TEST_RAISE( sShardFrom->mKeyCount != 1, ERR_MULTI_SHARD_KEY_ON_TABLE);

            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                    UShort,
                                    (void*) &sKeyColumnOrder )
                      != IDE_SUCCESS );

            *sKeyColumnOrder = sColumnOrder;
        }
        else if ( ( aIsSubKey == ID_FALSE ) &&
                  ( sShardObjInfo->mKeyFlags[sColumnOrder] == 2 ) )
        {
            /*
             * 첫 번 째 shard key에 대해서 analyze 할 때, 참조되는 table들에 sub-shard key가 있는지 표시한다.
             */

            sShardFrom->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] = ID_TRUE;

            /* TASK-7219 Shard Transformer Refactoring */
            aQuerySetAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] = ID_TRUE;
        }
        else if ( ( aIsSubKey == ID_TRUE ) &&
                  ( sShardObjInfo->mKeyFlags[sColumnOrder] == 2 ) )
        {
            /*
             * 두 번 째 shard key를 analyze 대상으로 설정한다.
             */

            sShardFrom->mKeyCount++;

            // Shard key가 두 개인 table은 존재 할 수 없다. ( break 하지 않고 검사 )
            IDE_TEST_RAISE( sShardFrom->mKeyCount != 1, ERR_MULTI_SUB_SHARD_KEY_ON_TABLE);

            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                    UShort,
                                    (void*) &sKeyColumnOrder )
                      != IDE_SUCCESS );

            *sKeyColumnOrder = sColumnOrder;
        }
        else
        {
            // Nothing to do.
        }
    }

    sShardFrom->mKey = sKeyColumnOrder;

    // 나머지 정보는 tableRef로 부터 그대로 assign
    if ( sdi::getSplitType( sShardObjInfo->mTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST )
    {
        copyShardInfoFromObjectInfo( &sShardFrom->mShardInfo,
                                     sShardObjInfo,
                                     ID_FALSE );
        
    }
    else
    {
        copyShardInfoFromObjectInfo( &sShardFrom->mShardInfo,
                                     sShardObjInfo,
                                     aIsSubKey );
    }

    //---------------------------------------
    // Shard from info에 연결 ( tail to head )
    //---------------------------------------
    sShardFrom->mNext = *aShardFromInfo;
    *aShardFromInfo   = sShardFrom;

    //---------------------------------------
    // PROJ-2685 online rebuild
    // shard table을 querySetAnalysis에 등록
    //---------------------------------------
    if ( aIsSubKey == ID_FALSE )
    {
        IDE_TEST( addTableInfo( aStatement,
                                aQuerySetAnalysis,
                                &(sShardObjInfo->mTableInfo) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MULTI_SHARD_KEY_ON_TABLE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::makeShardFromInfo4Table",
                                "Multiple shard keys exist on the table"));
    }
    IDE_EXCEPTION(ERR_MULTI_SUB_SHARD_KEY_ON_TABLE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::makeShardFromInfo4Table",
                                "Multiple sub-shard keys exist on the table"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::makeShardFromInfo4View( qcStatement       * aStatement,
                                    qmsFrom           * aFrom,
                                    sdiShardAnalysis  * aQuerySetAnalysis,
                                    sdiShardAnalysis  * aViewAnalysis,
                                    sdaFrom          ** aShardFromInfo,
                                    idBool              aIsSubKey )
{
    sdiKeyInfo   * sKeyInfo = NULL;
    sdaFrom      * sShardFrom = NULL;

    IDE_DASSERT( aViewAnalysis != NULL );

    if ( aIsSubKey == ID_FALSE )
    {
        sKeyInfo = aViewAnalysis->mKeyInfo;
    }
    else
    {
        sKeyInfo = aViewAnalysis->mNext->mKeyInfo;
    }

    //---------------------------------------
    // Individual한 shard key 의 갯수만큼 from list를 생성한다.
    //---------------------------------------
    for ( ;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        //---------------------------------------
        // Shard from의 할당
        //---------------------------------------
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                sdaFrom,
                                (void*) &sShardFrom )
                  != IDE_SUCCESS );

        SDA_INIT_SHARD_FROM( sShardFrom );

        //---------------------------------------
        // Shard from의 설정
        //---------------------------------------
        sShardFrom->mTupleId = aFrom->tableRef->table;

        // View의 target상의 shard key로 shardFromInfo의 shard key를 세팅한다.
        sShardFrom->mKeyCount = sKeyInfo->mKeyTargetCount;

        if ( sShardFrom->mKeyCount > 0 )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                          ID_SIZEOF(UShort) * sShardFrom->mKeyCount,
                          (void**) & sShardFrom->mKey )
                      != IDE_SUCCESS );

            idlOS::memcpy( (void*) sShardFrom->mKey,
                           (void*) sKeyInfo->mKeyTarget,
                           ID_SIZEOF(UShort) * sShardFrom->mKeyCount );
        }
        else
        {
            // CLONE
            // Nothing to do.
        }

        // View의 value info를 취한다.
        sShardFrom->mValuePtrCount = sKeyInfo->mValuePtrCount;
        sShardFrom->mValuePtrArray = sKeyInfo->mValuePtrArray;

        // View의 non-shard query reason을 shardFromInfo가 이어받는다.
        IDE_DASSERT( aViewAnalysis != NULL );
        sShardFrom->mIsShardQuery = aViewAnalysis->mIsShardQuery;

        idlOS::memcpy( (void*) sShardFrom->mAnalysisFlag.mNonShardFlag,
                       (void*) aViewAnalysis->mAnalysisFlag.mNonShardFlag,
                       ID_SIZEOF(idBool) * SDI_NON_SHARD_QUERY_REASON_MAX );

        // 나머지 정보는 view의 analysis info로 부터 그대로 assign
        copyShardInfoFromShardInfo( &sShardFrom->mShardInfo,
                                    &sKeyInfo->mShardInfo );

        //---------------------------------------
        // Shard from info에 연결 ( tail to head )
        //---------------------------------------
        sShardFrom->mNext = *aShardFromInfo;
        *aShardFromInfo   = sShardFrom;
    }

    //---------------------------------------
    // PROJ-2685 online rebuild
    // shard table을 querySetAnalysis에 등록
    //---------------------------------------
    if ( aIsSubKey == ID_FALSE )
    {
        IDE_TEST( addTableInfoList( aStatement,
                                    aQuerySetAnalysis,
                                    aViewAnalysis->mTableInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzePredicate( qcStatement     * aStatement,
                              qtcNode         * aCNF,
                              sdaCNFList      * aCNFOnCondition,
                              sdaFrom         * aShardFromInfo,
                              idBool            aIsOneNodeSQL,
                              sdiKeyInfo      * aOutRefInfo,
                              sdiKeyInfo     ** aKeyInfo,
                              sdiAnalysisFlag * aAnalysisFlag,
                              idBool            aIsSubKey )
{
    sdaCNFList * sCNFOnCondition = NULL;

    if ( aCNF != NULL )
    {
        //---------------------------------------
        // Make equivalent key list from shard key equi-join
        //---------------------------------------

        if ( aIsOneNodeSQL == ID_FALSE )
        {
            /*
             * CNF로 normalize된 where predicate을 순회하며
             * Outer-join과 semi/anti-join의 제약을 검사한다.
             *
             */
            IDE_TEST( analyzePredJoin( aStatement,
                                       aCNF,
                                       aShardFromInfo,
                                       aAnalysisFlag,
                                       aIsSubKey )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( makeKeyInfoFromJoin( aStatement,
                                       aCNF,
                                       aShardFromInfo,
                                       aKeyInfo,
                                       aIsSubKey )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //---------------------------------------
    // Make equivalent key list from shard key equi-join with on condition
    //---------------------------------------

    for ( sCNFOnCondition  = aCNFOnCondition;
          sCNFOnCondition != NULL;
          sCNFOnCondition  = sCNFOnCondition->mNext )
    {
        IDE_TEST( makeKeyInfoFromJoin( aStatement,
                                       sCNFOnCondition->mCNF,
                                       aShardFromInfo,
                                       aKeyInfo,
                                       aIsSubKey )
                  != IDE_SUCCESS );
    }

    //---------------------------------------
    // Add equivalent key list for no shard join
    //---------------------------------------

    IDE_TEST( makeKeyInfoFromNoJoin( aStatement,
                                     aShardFromInfo,
                                     aIsSubKey,
                                     aKeyInfo )
              != IDE_SUCCESS );

    //---------------------------------------
    // Check Out reference shard join pred
    //---------------------------------------
    IDE_TEST( checkOutRefShardJoin( aStatement,
                                    aCNF,
                                    sCNFOnCondition,
                                    *aKeyInfo,
                                    aOutRefInfo,
                                    aAnalysisFlag,
                                    aIsSubKey )
              != IDE_SUCCESS );

    //---------------------------------------
    // Set equivalent value list from shard key predicate
    //---------------------------------------

    IDE_TEST( makeValueInfo( aStatement,
                             aCNF,
                             *aKeyInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::makeKeyInfoFromJoin( qcStatement              * aStatement,
                                 qtcNode                  * aCNF,
                                 sdaFrom                  * aShardFromInfo,
                                 sdiKeyInfo              ** aKeyInfo,
                                 idBool                     aIsSubKey )
{
    qtcNode * sCNFOr          = NULL;

    sdaFrom * sLeftShardFrom  = NULL;
    sdaFrom * sRightShardFrom = NULL;

    UShort    sLeftTuple      = ID_USHORT_MAX;
    UShort    sLeftColumn     = ID_USHORT_MAX;

    UShort    sRightTuple     = ID_USHORT_MAX;
    UShort    sRightColumn    = ID_USHORT_MAX;

    idBool    sIsFound        = ID_FALSE;
    idBool    sIsSame         = ID_FALSE;

    //---------------------------------------
    // CNF tree를 순회하며 shard join condition을 찾는다.
    //---------------------------------------
    for ( sCNFOr  = (qtcNode*)aCNF->node.arguments;
          sCNFOr != NULL;
          sCNFOr  = (qtcNode*)sCNFOr->node.next )
    {
        sIsFound = ID_FALSE;
        sIsSame  = ID_FALSE;

        IDE_TEST( getShardJoinTupleColumn( aStatement,
                                           aShardFromInfo,
                                           (qtcNode*)sCNFOr->node.arguments,
                                           &sIsFound,
                                           &sLeftTuple,
                                           &sLeftColumn,
                                           &sRightTuple,
                                           &sRightColumn )
                  != IDE_SUCCESS );

        if ( sIsFound == ID_TRUE )
        {
            IDE_TEST( getShardFromInfo( aShardFromInfo,
                                        sLeftTuple,
                                        sLeftColumn,
                                        &sLeftShardFrom )
                      != IDE_SUCCESS );

            IDE_TEST( getShardFromInfo( aShardFromInfo,
                                        sRightTuple,
                                        sRightColumn,
                                        &sRightShardFrom )
                      != IDE_SUCCESS );

            if ( ( sLeftShardFrom != NULL ) && ( sRightShardFrom != NULL ) )
            {
                if ( ( sLeftShardFrom->mShardInfo.mSplitMethod  != SDI_SPLIT_NONE ) &&
                     ( sRightShardFrom->mShardInfo.mSplitMethod != SDI_SPLIT_NONE ) )
                {
                    //---------------------------------------
                    // Join 된 left shard from과 right shard from의
                    // 분산정의가 동일한지 확인한다.
                    //---------------------------------------
                    IDE_TEST( isSameShardInfo( &sLeftShardFrom->mShardInfo,
                                               &sRightShardFrom->mShardInfo,
                                               aIsSubKey,
                                               &sIsSame )
                              != IDE_SUCCESS );

                    if ( sIsSame == ID_TRUE )
                    {
                        IDE_TEST( makeKeyInfo( aStatement,
                                               sLeftShardFrom,
                                               sRightShardFrom,
                                               aIsSubKey,
                                               aKeyInfo )
                                  != IDE_SUCCESS );

                        IDE_TEST( makeKeyInfo( aStatement,
                                               sRightShardFrom,
                                               sLeftShardFrom,
                                               aIsSubKey,
                                               aKeyInfo )
                                  != IDE_SUCCESS );

                        // 두 shard from이 shard key equi-join 되었음이 확정
                        // Shard join으로 부터 이미 key info가 생성 된 shard from임을 표시
                        sLeftShardFrom->mIsJoined  = ID_TRUE;
                        sRightShardFrom->mIsJoined = ID_TRUE;
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getShardJoinTupleColumn( qcStatement * aStatement,
                                     sdaFrom     * aShardFromInfo,
                                     qtcNode     * aCNF,
                                     idBool      * aIsFound,
                                     UShort      * aLeftTuple,
                                     UShort      * aLeftColumn,
                                     UShort      * aRightTuple,
                                     UShort      * aRightColumn )
{
    sdaQtcNodeType   sQtcNodeType       = SDA_NONE;

    qtcNode        * sLeftQtcNode       = NULL;
    qtcNode        * sRightQtcNode      = NULL;

    qtcNode        * sOrList            = NULL;

    idBool           sIsShardEquivalent = ID_TRUE;

    // 초기화
    *aIsFound = ID_FALSE;

    //---------------------------------------
    // Shard join condition을 찾는다.
    //---------------------------------------
    IDE_TEST( getQtcNodeTypeWithShardFrom( aStatement,
                                           aShardFromInfo,
                                           aCNF,
                                           &sQtcNodeType )
              != IDE_SUCCESS );

    if ( sQtcNodeType == SDA_EQUAL )
    {
        sLeftQtcNode = (qtcNode*)aCNF->node.arguments;

        IDE_TEST( getQtcNodeTypeWithShardFrom( aStatement,
                                               aShardFromInfo,
                                               sLeftQtcNode,
                                               &sQtcNodeType )
                  != IDE_SUCCESS );

        if ( sQtcNodeType == SDA_KEY_COLUMN )
        {
            sRightQtcNode = (qtcNode*)sLeftQtcNode->node.next;

            IDE_TEST( getQtcNodeTypeWithShardFrom( aStatement,
                                                   aShardFromInfo,
                                                   sRightQtcNode,
                                                   &sQtcNodeType )
                      != IDE_SUCCESS );

            //---------------------------------------
            // Shard join condition을 찾음
            //---------------------------------------
            if ( sQtcNodeType == SDA_KEY_COLUMN )
            {
                //---------------------------------------
                // Shard join condition에 OR가 있으면(equal->next != NULL),
                // shard equivalent expression이어야 한다.
                //---------------------------------------
                if ( aCNF->node.next != NULL )
                {
                    for ( sOrList  = (qtcNode*)aCNF->node.next;
                          sOrList != NULL;
                          sOrList  = (qtcNode*)sOrList->node.next )
                    {
                        IDE_TEST( isShardEquivalentExpression( aStatement,
                                                               aCNF,
                                                               sOrList,
                                                               &sIsShardEquivalent )
                                  != IDE_SUCCESS );

                        if ( sIsShardEquivalent == ID_FALSE )
                        {
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
                    // Nothing to do.
                }

                if ( sIsShardEquivalent == ID_TRUE )
                {
                    *aIsFound     = ID_TRUE;
                    *aLeftTuple   = sLeftQtcNode->node.table;
                    *aLeftColumn  = sLeftQtcNode->node.column;
                    *aRightTuple  = sRightQtcNode->node.table;
                    *aRightColumn = sRightQtcNode->node.column;
                }
                else
                {
                    // shard join predicate으로 취급하지 않음.
                    // Nothing to do.
                }
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
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::isShardEquivalentExpression( qcStatement * aStatement,
                                         qtcNode     * aNode1,
                                         qtcNode     * aNode2,
                                         idBool      * aIsTrue )
{
    IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                           aNode1,
                                           aNode2,
                                           aIsTrue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getShardFromInfo( sdaFrom     * aShardFromInfo,
                              UShort        aTuple,
                              UShort        aColumn,
                              sdaFrom    ** aRetShardFrom )
{
    sdaFrom * sShardFrom = NULL;
    UInt      sKeyCount  = ID_UINT_MAX;

    // 초기화
    *aRetShardFrom = NULL;

    for ( sShardFrom  = aShardFromInfo;
          sShardFrom != NULL;
          sShardFrom  = sShardFrom->mNext )
    {
        if ( sShardFrom->mTupleId == aTuple )
        {
            for ( sKeyCount = 0;
                  sKeyCount < sShardFrom->mKeyCount;
                  sKeyCount++ )
            {
                if ( sShardFrom->mKey[sKeyCount] == aColumn )
                {
                    *aRetShardFrom = sShardFrom;
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
            // Nothing to do.
        }

        if ( *aRetShardFrom != NULL )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::makeKeyInfo( qcStatement * aStatement,
                         sdaFrom     * aMyShardFrom,
                         sdaFrom     * aLinkShardFrom,
                         idBool        aIsSubKey,
                         sdiKeyInfo ** aKeyInfo )
{
    sdiKeyInfo * sKeyInfoForAddingKeyList = NULL;

    if ( aLinkShardFrom != NULL )
    {
        //---------------------------------------
        // keyList를 추가할 대상 keyInfo를 얻어온다.
        //---------------------------------------

        // Link shard from의 key list가 들어있는 key info를 찾는다.
        IDE_TEST( getKeyInfoForAddingKeyList( aLinkShardFrom,
                                              *aKeyInfo,
                                              aIsSubKey,
                                              &sKeyInfoForAddingKeyList )
                  != IDE_SUCCESS );

        // 못 찾았으면, my shard from의 key list가 들어있는 key info를 찾는다.
        if ( sKeyInfoForAddingKeyList == NULL )
        {
            IDE_TEST( getKeyInfoForAddingKeyList( aMyShardFrom,
                                                  *aKeyInfo,
                                                  aIsSubKey,
                                                  &sKeyInfoForAddingKeyList )
                      != IDE_SUCCESS );
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

    //---------------------------------------
    // KeyInfo에 key list를 추가한다.
    //---------------------------------------
    IDE_TEST( addKeyList( aStatement,
                          aMyShardFrom,
                          aKeyInfo,
                          sKeyInfoForAddingKeyList )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getKeyInfoForAddingKeyList( sdaFrom     * aShardFrom,
                                        sdiKeyInfo  * aKeyInfo,
                                        idBool        aIsSubKey,
                                        sdiKeyInfo ** aRetKeyInfo )
{
    sdiKeyInfo * sKeyInfo                 = NULL;

    UInt         sKeyCount                = 0;
    UInt         sKeyInfoKeyCount         = ID_UINT_MAX;

    sdiKeyInfo * sKeyInfoForAddingKeyList = NULL;

    idBool       sIsSame                  = ID_FALSE;

    IDE_DASSERT( aRetKeyInfo != NULL );

    // 초기화
    *aRetKeyInfo = NULL;

    //---------------------------------------
    // shard from의 key list가 들어있는 key info를 찾는다.
    //---------------------------------------
    for ( sKeyCount = 0;
          sKeyCount < aShardFrom->mKeyCount;
          sKeyCount++ )
    {
        for ( sKeyInfo  = aKeyInfo;
              sKeyInfo != NULL;
              sKeyInfo  = sKeyInfo->mNext )
        {
            //---------------------------------------
            // shard from의 분산정의와 keyInfo의 분산정의가
            // join 될 수 있는 것 인지 확인한다.
            //---------------------------------------
            IDE_TEST( isSameShardInfo( &aShardFrom->mShardInfo,
                                       &sKeyInfo->mShardInfo,
                                       aIsSubKey,
                                       &sIsSame )
                      != IDE_SUCCESS );

            if ( sIsSame == ID_TRUE )
            {
                for ( sKeyInfoKeyCount = 0;
                      sKeyInfoKeyCount < sKeyInfo->mKeyCount;
                      sKeyInfoKeyCount++ )
                {
                    if ( ( aShardFrom->mTupleId ==
                           sKeyInfo->mKey[sKeyInfoKeyCount].mTupleId ) &&
                         ( aShardFrom->mKey[sKeyCount] ==
                           sKeyInfo->mKey[sKeyInfoKeyCount].mColumn ) )
                    {
                        //---------------------------------------
                        // 찾았으면, key list는 이 곳 에 추가되어야 한다.
                        //---------------------------------------
                        sKeyInfoForAddingKeyList = sKeyInfo;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                if ( sKeyInfoForAddingKeyList != NULL )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                /* Noting to do. */
            }
        }

        if ( sKeyInfoForAddingKeyList != NULL )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    *aRetKeyInfo = sKeyInfoForAddingKeyList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::isSameShardHashInfo( sdiShardInfo * aShardInfo1,
                                 sdiShardInfo * aShardInfo2,
                                 idBool         aIsSubKey,
                                 idBool       * aOutResult )
{
    idBool     sIsSame = ID_TRUE;
    sdiRange * sRange1;
    sdiRange * sRange2;
    UShort     i;

    IDE_DASSERT( aShardInfo1 != NULL );
    IDE_DASSERT( aShardInfo2 != NULL );

    if ( aShardInfo1->mRangeInfo->mCount == aShardInfo2->mRangeInfo->mCount )
    {
        for ( i = 0; i < aShardInfo1->mRangeInfo->mCount; i++ )
        {
            sRange1 = &(aShardInfo1->mRangeInfo->mRanges[i]);
            sRange2 = &(aShardInfo2->mRangeInfo->mRanges[i]);

            if ( sRange1->mNodeId == sRange2->mNodeId )
            {
                if ( ( ( aIsSubKey == ID_FALSE ) && ( sRange1->mValue.mHashMax == sRange2->mValue.mHashMax ) ) ||
                     ( ( aIsSubKey == ID_TRUE ) && ( sRange1->mSubValue.mHashMax == sRange2->mSubValue.mHashMax ) ) )
                {
                    /* TASJ-7219
                     *   BUG-47879 이후, mPartitionName 검증 코드 제거
                     */
                }
                else
                {
                    sIsSame = ID_FALSE;
                    break;
                }
            }
            else
            {
                sIsSame = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        sIsSame = ID_FALSE;
    }

    *aOutResult = sIsSame;

    return IDE_SUCCESS;

    /* IDE_EXCEPTION_END; */

    return IDE_FAILURE;
}

IDE_RC sda::isSameShardRangeInfo( sdiShardInfo * aShardInfo1,
                                  sdiShardInfo * aShardInfo2,
                                  idBool         aIsSubKey,
                                  idBool       * aOutResult )
{
    idBool     sIsSame = ID_TRUE;
    sdiRange * sRange1;
    sdiRange * sRange2;
    UShort     i;

    IDE_DASSERT( aShardInfo1 != NULL );
    IDE_DASSERT( aShardInfo2 != NULL );

    if ( aShardInfo1->mRangeInfo->mCount == aShardInfo2->mRangeInfo->mCount )
    {
        for ( i = 0; i < aShardInfo1->mRangeInfo->mCount; i++ )
        {
            sRange1 = &(aShardInfo1->mRangeInfo->mRanges[i]);
            sRange2 = &(aShardInfo2->mRangeInfo->mRanges[i]);

            if ( sRange1->mNodeId == sRange2->mNodeId )
            {
                if ( aShardInfo1->mKeyDataType == MTD_SMALLINT_ID )
                {
                    if ( ( ( aIsSubKey == ID_FALSE ) &&
                           ( sRange1->mValue.mSmallintMax == sRange2->mValue.mSmallintMax ) ) ||
                         ( ( aIsSubKey == ID_TRUE ) &&
                           ( sRange1->mSubValue.mSmallintMax == sRange2->mSubValue.mSmallintMax ) ) )
                    {
                        /* TASJ-7219
                         *   BUG-47879 이후, mPartitionName 검증 코드 제거
                         */
                    }
                    else
                    {
                        sIsSame = ID_FALSE;
                        break;
                    }
                }
                else if ( aShardInfo1->mKeyDataType == MTD_INTEGER_ID )
                {
                    if ( ( ( aIsSubKey == ID_FALSE ) &&
                           ( sRange1->mValue.mIntegerMax == sRange2->mValue.mIntegerMax ) ) ||
                         ( ( aIsSubKey == ID_TRUE ) &&
                           ( sRange1->mSubValue.mIntegerMax == sRange2->mSubValue.mIntegerMax ) ) )
                    {
                        /* TASJ-7219
                         *   BUG-47879 이후, mPartitionName 검증 코드 제거
                         */
                    }
                    else
                    {
                        sIsSame = ID_FALSE;
                        break;
                    }
                }
                else if ( aShardInfo1->mKeyDataType == MTD_BIGINT_ID )
                {
                    if ( ( ( aIsSubKey == ID_FALSE ) &&
                           ( sRange1->mValue.mBigintMax == sRange2->mValue.mBigintMax ) ) ||
                         ( ( aIsSubKey == ID_TRUE ) &&
                           ( sRange1->mSubValue.mBigintMax == sRange2->mSubValue.mBigintMax ) ) )
                    {
                        /* TASJ-7219
                         *   BUG-47879 이후, mPartitionName 검증 코드 제거
                         */
                    }
                    else
                    {
                        sIsSame = ID_FALSE;
                        break;
                    }
                }
                else if ( ( aShardInfo1->mKeyDataType == MTD_CHAR_ID ) ||
                          ( aShardInfo1->mKeyDataType == MTD_VARCHAR_ID ) )
                {
                    if ( aIsSubKey == ID_FALSE )
                    {   
                        if ( sRange1->mValue.mCharMax.length ==
                             sRange2->mValue.mCharMax.length )
                        {
                            if ( idlOS::memcmp(
                                     (void*) &(sRange1->mValue.mCharMax.value),
                                     (void*) &(sRange2->mValue.mCharMax.value),
                                     sRange1->mValue.mCharMax.length ) == 0 )
                            {
                                /* TASJ-7219
                                 *   BUG-47879 이후, mPartitionName 검증 코드 제거
                                 */
                            }
                            else
                            {
                                sIsSame = ID_FALSE;
                                break;
                            }
                        }
                        else
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                    }
                    else
                    {
                        if ( sRange1->mSubValue.mCharMax.length ==
                             sRange2->mSubValue.mCharMax.length )
                        {
                            if ( idlOS::memcmp(
                                     (void*) &(sRange1->mSubValue.mCharMax.value),
                                     (void*) &(sRange2->mSubValue.mCharMax.value),
                                     sRange1->mSubValue.mCharMax.length ) == 0 )
                            {
                                sIsSame = ID_TRUE;
                            }
                            else
                            {
                                sIsSame = ID_FALSE;
                            }
                        }
                        else
                        {
                            sIsSame = ID_FALSE;
                        }
                    }
                }
                else
                {
                    sIsSame = ID_FALSE;
                    break;
                }
            }
            else
            {
                sIsSame = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        sIsSame = ID_FALSE;
    }

    *aOutResult = sIsSame;

    return IDE_SUCCESS;

    /* IDE_EXCEPTION_END; */

    return IDE_FAILURE;
}

IDE_RC sda::isSameShardInfo( sdiShardInfo * aShardInfo1,
                             sdiShardInfo * aShardInfo2,
                             idBool         aIsSubKey,
                             idBool       * aIsSame )
{
    IDE_DASSERT( aShardInfo1 != NULL );
    IDE_DASSERT( aShardInfo2 != NULL );
    IDE_DASSERT( aIsSame != NULL );

    // 초기화
    *aIsSame = ID_FALSE;

    if ( ( aShardInfo1->mSplitMethod   == aShardInfo2->mSplitMethod ) &&
         ( aShardInfo1->mKeyDataType   == aShardInfo2->mKeyDataType ) &&
         ( aShardInfo1->mDefaultNodeId == aShardInfo2->mDefaultNodeId ) )
    {
        switch ( aShardInfo1->mSplitMethod )
        {
            case SDI_SPLIT_HASH:
            {
                IDE_TEST( isSameShardHashInfo( aShardInfo1,
                                               aShardInfo2,
                                               aIsSubKey,
                                               aIsSame ) != IDE_SUCCESS);
                break;
            }

            case SDI_SPLIT_RANGE:
            case SDI_SPLIT_LIST:
            {
                IDE_TEST( isSameShardRangeInfo( aShardInfo1,
                                                aShardInfo2,
                                                aIsSubKey,
                                                aIsSame ) != IDE_SUCCESS);
                break;
            }

            /* TASK-7219 Shard Transformer Refactoring
             *  기존에 Shard Analyze 실패 후, Query Set 를 좁혀서 재수행하기 때문에
             *   확인하지 못하였던 제외된 조건을 발견하여 추가하였다.
             */
            case SDI_SPLIT_CLONE:
            case SDI_SPLIT_SOLO:
            {
                if ( isSameRangesForCloneAndSolo( aShardInfo1->mRangeInfo,
                                                  aShardInfo2->mRangeInfo )
                     == ID_TRUE )
                {
                    *aIsSame = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }

                break;
            }

            case SDI_SPLIT_NONE:
            {
                break;
            }

            default:
            {
                IDE_RAISE(ERR_INVALID_SPLIT_METHOD);
                break;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SPLIT_METHOD)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::isSameShardInfo",
                                "Invalid split method"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::addKeyList( qcStatement * aStatement,
                        sdaFrom     * aFrom,
                        sdiKeyInfo ** aKeyInfo,
                        sdiKeyInfo  * aKeyInfoForAdding )
{
    UInt                sMyKeyCount      = ID_UINT_MAX;
    UInt                sCurrKeyIdx      = ID_UINT_MAX;

    UInt                sKeyInfoKeyCount = ID_UINT_MAX;

    sdiKeyTupleColumn * sNewKeyList      = NULL;

    sdiKeyInfo        * sNewKeyInfo      = NULL;

    idBool              sIsFound         = ID_FALSE;

    if ( aKeyInfoForAdding != NULL )
    {
        //---------------------------------------
        // Key list를 추가할 key info가 지정 되어있는 경우
        // 해당 key info에 중복없이 추가한다.
        //---------------------------------------

        // 중복검사.
        // Shard from의 key list를 통째로 등록하기 때문에
        // Key list 중 하나라도 발견되면, 전부 중복인 것 으로 판단하고 넘어간다.

        for ( sKeyInfoKeyCount = 0;
              sKeyInfoKeyCount < aKeyInfoForAdding->mKeyCount;
              sKeyInfoKeyCount++ )
        {
            if ( ( aFrom->mTupleId == aKeyInfoForAdding->mKey[sKeyInfoKeyCount].mTupleId ) &&
                 ( aFrom->mKey[0]  == aKeyInfoForAdding->mKey[sKeyInfoKeyCount].mColumn ) )
            {
                sIsFound = ID_TRUE;

                break;
            }
        }

        if ( sIsFound == ID_FALSE )
        {
            // 중복없음, 

            //---------------------------------------
            // key list를 새로 추가
            //---------------------------------------
            sKeyInfoKeyCount = aKeyInfoForAdding->mKeyCount;
            sMyKeyCount      = aFrom->mKeyCount;

            // KeyInfo의 key list갯수와 shard from의 key list 갯수를
            // 더한 크기 만큼 key list 를 할당한다.
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiKeyTupleColumn) *
                                                     ( sKeyInfoKeyCount + sMyKeyCount ),
                                                     (void**) & sNewKeyList )
                      != IDE_SUCCESS );

            // 기존 keyInfo에 있던 key list들을 복사
            idlOS::memcpy( (void*) sNewKeyList,
                           (void*) aKeyInfoForAdding->mKey,
                           ID_SIZEOF(sdiKeyTupleColumn) * sKeyInfoKeyCount );

            // 이어서 shard from의 key list들을 추가
            for ( sCurrKeyIdx = sKeyInfoKeyCount;
                  sCurrKeyIdx < ( sKeyInfoKeyCount + sMyKeyCount );
                  sCurrKeyIdx++ )
            {
                sNewKeyList[sCurrKeyIdx].mTupleId = aFrom->mTupleId;
                sNewKeyList[sCurrKeyIdx].mColumn  = aFrom->mKey[sCurrKeyIdx - sKeyInfoKeyCount];
                sNewKeyList[sCurrKeyIdx].mIsNullPadding = aFrom->mIsNullPadding;
                sNewKeyList[sCurrKeyIdx].mIsAntiJoinInner = aFrom->mIsAntiJoinInner;
            }

            // 새로만든 key list를 key info에 연결
            aKeyInfoForAdding->mKeyCount = (sKeyInfoKeyCount + sMyKeyCount);
            aKeyInfoForAdding->mKey      = sNewKeyList;

            //---------------------------------------
            // Value list를 갱신
            //---------------------------------------
            if ( aFrom->mValuePtrCount > 0 )
            {
                //---------------------------------------
                // CNF tree의 AND's ORs 중 먼저 수행되어 keyInfo에 구성 된 value info와
                // 새로운 CNF tree의 AND's ORs상의 value info와 AND의 개념이다.
                // 더 적은 것으로 덮어씌운다.
                //
                // 덮어쓰지 않고, 기존 value info를 유지 해도된다.
                //---------------------------------------
                if ( ( aFrom->mValuePtrCount < aKeyInfoForAdding->mValuePtrCount ) ||
                     ( aKeyInfoForAdding->mValuePtrCount == 0 ) )
                {
                    aKeyInfoForAdding->mValuePtrCount = aFrom->mValuePtrCount;
                    aKeyInfoForAdding->mValuePtrArray = aFrom->mValuePtrArray;
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
        else
        {
            // 이미 있는 key list
            // Nothing to do.
        }
    }
    else
    {
        //---------------------------------------
        // Key list를 추가할 key info가 지정 되지 않은 경우
        // 새로운 Key info를 생성하여 key list를 추가한다.
        //---------------------------------------

        // 새로운 key info 구조체 할당
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                sdiKeyInfo,
                                (void*) &sNewKeyInfo )
                  != IDE_SUCCESS );

        // 새로운 key info의 정보 초기화
        SDI_INIT_KEY_INFO( sNewKeyInfo );

        // 새로운 key info에 shard from의 key list를 추가
        sNewKeyInfo->mKeyCount = aFrom->mKeyCount;

        if ( sNewKeyInfo->mKeyCount > 0 )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                          ID_SIZEOF(sdiKeyTupleColumn) * sNewKeyInfo->mKeyCount,
                          (void**) & sNewKeyList )
                      != IDE_SUCCESS );

            for ( sCurrKeyIdx = 0;
                  sCurrKeyIdx < sNewKeyInfo->mKeyCount;
                  sCurrKeyIdx++ )
            {
                sNewKeyList[sCurrKeyIdx].mTupleId = aFrom->mTupleId;
                sNewKeyList[sCurrKeyIdx].mColumn  = aFrom->mKey[sCurrKeyIdx];
                sNewKeyList[sCurrKeyIdx].mIsNullPadding = aFrom->mIsNullPadding;
                sNewKeyList[sCurrKeyIdx].mIsAntiJoinInner = aFrom->mIsAntiJoinInner;
            }
        }
        else
        {
            // Nothing to do.
        }

        sNewKeyInfo->mKey = sNewKeyList;

        // Shard From에서 올라온 valueInfo를 전달.
        sNewKeyInfo->mValuePtrCount = aFrom->mValuePtrCount;
        sNewKeyInfo->mValuePtrArray = aFrom->mValuePtrArray;

        copyShardInfoFromShardInfo( &sNewKeyInfo->mShardInfo,
                                    &aFrom->mShardInfo );

        // 연결 ( tail to head )
        sNewKeyInfo->mNext      = *aKeyInfo;
        *aKeyInfo               = sNewKeyInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::makeKeyInfoFromNoJoin( qcStatement * aStatement,
                                   sdaFrom     * aShardFromInfo,
                                   idBool        aIsSubKey,
                                   sdiKeyInfo ** aKeyInfo )
{
    sdaFrom * sShardFrom = NULL;

    for ( sShardFrom  = aShardFromInfo;
          sShardFrom != NULL;
          sShardFrom  = sShardFrom->mNext )
    {
        //---------------------------------------
        // Shard join predicate에 의해 이미 등록 되지 않은
        // Non-shard joined shard from( split clone 포함 )에 대해서 key info를 생성한다.
        //---------------------------------------
        if ( sShardFrom->mIsJoined == ID_FALSE )
        {
            IDE_TEST( makeKeyInfo( aStatement,
                                   sShardFrom,
                                   NULL,
                                   aIsSubKey,
                                   aKeyInfo )
                      != IDE_SUCCESS );
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

IDE_RC sda::checkOutRefShardJoin( qcStatement     * aStatement,
                                  qtcNode         * aCNF,
                                  sdaCNFList      * aCNFOnCondition,
                                  sdiKeyInfo      * aKeyInfo,
                                  sdiKeyInfo      * aOutRefInfo,
                                  sdiAnalysisFlag * aAnalysisFlag,
                                  idBool            aIsSubKey )
{
    sdaCNFList     * sCNFOnCondition = NULL;
    idBool           sIsOutRefJoinFound = ID_FALSE;

    if ( aOutRefInfo != NULL )
    {
        //---------------------------------------
        // CHECK ON PREDICATES
        //---------------------------------------
        for ( sCNFOnCondition  = aCNFOnCondition;
              sCNFOnCondition != NULL;
              sCNFOnCondition  = sCNFOnCondition->mNext )
        {
            IDE_TEST( findShardJoinWithOutRefKey( aStatement,
                                                  aKeyInfo,
                                                  (qtcNode*)sCNFOnCondition->mCNF,
                                                  aOutRefInfo,
                                                  aAnalysisFlag,
                                                  aIsSubKey,
                                                  &( sIsOutRefJoinFound ) )
                      != IDE_SUCCESS );

            if ( sIsOutRefJoinFound == ID_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do. */
            }
        }

        if ( sIsOutRefJoinFound == ID_FALSE )
        {
            //---------------------------------------
            // CHECK WHERE PREDICATES
            //---------------------------------------
            IDE_TEST( findShardJoinWithOutRefKey( aStatement,
                                                  aKeyInfo,
                                                  (qtcNode*)aCNF,
                                                  aOutRefInfo,
                                                  aAnalysisFlag,
                                                  aIsSubKey,
                                                  &( sIsOutRefJoinFound ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }

        if ( sIsOutRefJoinFound == ID_FALSE )
        {
            aAnalysisFlag->mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ] = ID_TRUE;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::findShardJoinWithOutRefKey( qcStatement     * aStatement,
                                        sdiKeyInfo      * aKeyInfo,
                                        qtcNode         * aCNFOr,
                                        sdiKeyInfo      * aOutRefInfo,
                                        sdiAnalysisFlag * aAnalysisFlag,
                                        idBool            aIsSubKey,
                                        idBool          * aIsOutRefJoinFound )
{
    qtcNode        * sCNFOr = NULL;
    sdaQtcNodeType   sQtcNodeType = SDA_NONE;

    qtcNode        * sLeftQtcNode = NULL;
    qtcNode        * sRightQtcNode = NULL;

    idBool           sIsOutRefKeyExists = ID_FALSE;

    qtcNode        * sOrList = NULL;
    idBool           sIsShardEquivalent = ID_TRUE;

    sdiKeyInfo     * sKeyInfoRef = NULL;
    sdiKeyInfo     * sOutKeyInfoRef = NULL;

    idBool           sIsSameShardInfo = ID_FALSE;

    // Initialization for return value
    *aIsOutRefJoinFound = ID_FALSE;

    if ( aCNFOr != NULL )
    {
        for ( sCNFOr  = (qtcNode*)aCNFOr->node.arguments; // AND's OR list
              sCNFOr != NULL;
              sCNFOr  = (qtcNode*)sCNFOr->node.next )
        {
            sIsOutRefKeyExists = ID_FALSE;

            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 (qtcNode*)sCNFOr->node.arguments,
                                                 &sQtcNodeType,
                                                 &sKeyInfoRef )
                      != IDE_SUCCESS );

            if ( sQtcNodeType == SDA_EQUAL )
            {
                sLeftQtcNode = (qtcNode*)sCNFOr->node.arguments->arguments;

                IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                     aKeyInfo,
                                                     sLeftQtcNode,
                                                     &sQtcNodeType,
                                                     &sKeyInfoRef )
                          != IDE_SUCCESS );

                if ( sQtcNodeType != SDA_KEY_COLUMN )
                {
                    IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                         aOutRefInfo,
                                                         sLeftQtcNode,
                                                         &sQtcNodeType,
                                                         &sOutKeyInfoRef )
                              != IDE_SUCCESS );

                    if ( sQtcNodeType == SDA_KEY_COLUMN )
                    {
                        sIsOutRefKeyExists = ID_TRUE;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    /* Nothing to do. */
                }

                if ( sQtcNodeType == SDA_KEY_COLUMN )
                {
                    sRightQtcNode = (qtcNode*)sLeftQtcNode->node.next;

                    if ( sIsOutRefKeyExists == ID_TRUE )
                    {
                        IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                             aKeyInfo,
                                                             sRightQtcNode,
                                                             &sQtcNodeType,
                                                             &sKeyInfoRef )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                             aOutRefInfo,
                                                             sRightQtcNode,
                                                             &sQtcNodeType,
                                                             &sOutKeyInfoRef )
                                  != IDE_SUCCESS );

                        if ( sQtcNodeType == SDA_KEY_COLUMN )
                        {
                            sIsOutRefKeyExists = ID_TRUE;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }   
                    }
                }
                else
                {
                    /* Nothing to do. */
                }

                //---------------------------------------
                // Shard out ref join cond를 찾음
                //---------------------------------------
                if ( ( sQtcNodeType == SDA_KEY_COLUMN ) &&
                     ( sIsOutRefKeyExists == ID_TRUE ) )
                {
                    //---------------------------------------
                    // Shard join condition에 OR가 있으면(equal->next != NULL),
                    // shard equivalent expression이어야 한다.
                    //---------------------------------------
                    if ( sCNFOr->node.arguments->next != NULL )
                    {
                        for( sOrList  = (qtcNode*)sCNFOr->node.arguments->next;
                             sOrList != NULL;
                             sOrList  = (qtcNode*)sOrList->node.next )
                        {
                            IDE_TEST( isShardEquivalentExpression( aStatement,
                                                                   sCNFOr,
                                                                   sOrList,
                                                                   &sIsShardEquivalent )
                                      != IDE_SUCCESS );

                            if ( sIsShardEquivalent == ID_FALSE )
                            {
                                break;
                            }
                            else
                            {
                                /* Nothing to do. */
                            }
                        }
                    }
                    else
                    {
                        /* Nothing to do. */
                    }

                    if ( sIsShardEquivalent == ID_TRUE )
                    {
                        IDE_TEST ( isSameShardInfo( &sKeyInfoRef->mShardInfo,
                                                    &sOutKeyInfoRef->mShardInfo,
                                                    aIsSubKey,
                                                    &sIsSameShardInfo )
                                   != IDE_SUCCESS );

                        if ( sIsSameShardInfo == ID_TRUE )
                        {
                            *aIsOutRefJoinFound = ID_TRUE;

                            if ( ( sOutKeyInfoRef->mValuePtrCount > 0 ) &&
                                 ( ( sKeyInfoRef->mValuePtrCount > sOutKeyInfoRef->mValuePtrCount ) ||
                                   ( sKeyInfoRef->mValuePtrCount == 0 ) ) )
                            {
                                sKeyInfoRef->mValuePtrCount = sOutKeyInfoRef->mValuePtrCount;
                                sKeyInfoRef->mValuePtrArray = sOutKeyInfoRef->mValuePtrArray;
                            }
                            else
                            {
                                /* Nothing to do. */
                            }
                    
                            break;
                        }
                        else
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                        }
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::makeValueInfo( qcStatement * aStatement,
                           qtcNode     * aCNF,
                           sdiKeyInfo  * aKeyInfo )
{
    qtcNode          * sCNFOr     = NULL;
    sdaValueList     * sValueList = NULL;

    //---------------------------------------
    // CNF tree를 순회하며 shard predicate을 찾고,
    // Shard value list를 해당하는 key info에 세팅한다.
    //---------------------------------------
    if ( aCNF != NULL )
    {
        for ( sCNFOr  = (qtcNode*)aCNF->node.arguments;
              sCNFOr != NULL;
              sCNFOr  = (qtcNode*)sCNFOr->node.next )
        {
            sValueList = NULL;

            IDE_TEST( getShardValue( aStatement,
                                     (qtcNode*)sCNFOr->node.arguments,
                                     aKeyInfo,
                                     &sValueList )
                      != IDE_SUCCESS );

            if ( sValueList != NULL )
            {
                IDE_TEST( addValueListOnKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 sValueList )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getShardValue( qcStatement       * aStatement,
                           qtcNode           * aCNF,
                           sdiKeyInfo        * aKeyInfo,
                           sdaValueList     ** aValueList )
{
    sdaQtcNodeType    sQtcNodeType = SDA_NONE;

    qtcNode         * sKeyColumn   = NULL;
    qtcNode         * sValue       = NULL;

    qtcNode         * sOrList      = NULL;

    idBool            sIsShardCond = ID_FALSE;

    sdaValueList    * sValueList   = NULL;
    sdiValueInfo    * sValueInfo   = NULL;

    sdiShardInfo    * sShardInfo   = NULL;
    
    const mtdModule * sKeyModule   = NULL;
    const void      * sKeyValue    = NULL;

    sdiKeyInfo      * sKeyInfoRef  = NULL;

    //초기화
    *aValueList = NULL;

    //---------------------------------------
    // OR list 전체가 shard condition인지 확인한다.
    //---------------------------------------
    for ( sOrList  = aCNF;
          sOrList != NULL;
          sOrList  = (qtcNode*)sOrList->node.next )
    {
        IDE_TEST( isShardCondition( aStatement,
                                    sOrList,
                                    aKeyInfo,
                                    &sIsShardCond )
                  != IDE_SUCCESS );

        if ( sIsShardCond == ID_FALSE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    //---------------------------------------
    // OR list 전체가 shard condition이어야만,
    // 해당 predicate이 shard predicate이다.
    //---------------------------------------
    if ( sIsShardCond == ID_TRUE )
    {
        for ( sOrList  = aCNF;
              sOrList != NULL;
              sOrList  = (qtcNode*)sOrList->node.next )
        {
            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 (qtcNode*)sOrList->node.arguments,
                                                 &sQtcNodeType,
                                                 &sKeyInfoRef )
                      != IDE_SUCCESS );

            if ( sQtcNodeType == SDA_KEY_COLUMN )
            {
                sKeyColumn = (qtcNode*)sOrList->node.arguments;
                sValue     = (qtcNode*)sOrList->node.arguments->next;
            }
            else if ( ( sQtcNodeType == SDA_HOST_VAR ) ||
                      ( sQtcNodeType == SDA_CONSTANT_VALUE ) )
            {
                sValue     = (qtcNode*)sOrList->node.arguments;
                sKeyColumn = (qtcNode*)sOrList->node.arguments->next;
            }
            else
            {
                IDE_RAISE( ERR_INVALID_SHARD_ARGUMENTS_TYPE );
            }

            //---------------------------------------
            // Value list의 할당
            //---------------------------------------
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                    sdaValueList,
                                    (void*) &sValueList )
                      != IDE_SUCCESS );

            //---------------------------------------
            // Value info의 설정
            //---------------------------------------
            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 sValue,
                                                 &sQtcNodeType,
                                                 &sKeyInfoRef )
                      != IDE_SUCCESS );

            switch ( sQtcNodeType )
            {
                case SDA_HOST_VAR:
                    IDE_TEST( allocAndSetShardHostValueInfo( aStatement,
                                                             sValue->node.column,
                                                             &sValueInfo )
                              != IDE_SUCCESS );
                    break;

                case SDA_CONSTANT_VALUE:
                    IDE_TEST( getShardInfo( sKeyColumn,
                                            aKeyInfo,
                                            &sShardInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( mtd::moduleById( &sKeyModule,
                                               sShardInfo->mKeyDataType )
                              != IDE_SUCCESS );

                    IDE_TEST( getKeyConstValue( aStatement,
                                                sValue,
                                                sKeyModule,
                                                &sKeyValue )
                              != IDE_SUCCESS );

                    IDE_TEST( allocAndSetShardConstValueInfo( aStatement,
                                                              sShardInfo->mKeyDataType,
                                                              sKeyValue,
                                                              &sValueInfo )
                              != IDE_SUCCESS );
                    break;

                default:
                    IDE_RAISE( ERR_INVALID_SHARD_VALUE_TYPE );
                    break;
            }

            //---------------------------------------
            // Value list의 설정
            //---------------------------------------

            sValueList->mValue     = sValueInfo;
            sValueList->mKeyColumn = sKeyColumn;

            //---------------------------------------
            // 연결
            //---------------------------------------

            sValueList->mNext = *aValueList;
            *aValueList = sValueList;
        }
    }
    else
    {
        // Shard condition이 아니다.
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_ARGUMENTS_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getShardValue",
                                  "Invalid shard arguments type" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_VALUE_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getShardValue",
                                  "Invalid shard value type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getShardInfo( qtcNode       * aNode,
                          sdiKeyInfo    * aKeyInfo,
                          sdiShardInfo ** aShardInfo )
{
    sdiKeyInfo * sKeyInfo = NULL;
    UInt         sKeyIdx  = 0;

    // 초기화
    *aShardInfo = NULL;

    for ( sKeyInfo  = aKeyInfo;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        for ( sKeyIdx = 0;
              sKeyIdx < sKeyInfo->mKeyCount;
              sKeyIdx++ )
        {
            if ( ( aNode->node.table  == sKeyInfo->mKey[sKeyIdx].mTupleId ) &&
                 ( aNode->node.column == sKeyInfo->mKey[sKeyIdx].mColumn ) )
            {
                *aShardInfo = &sKeyInfo->mShardInfo;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    IDE_TEST_RAISE( *aShardInfo == NULL, ERR_NULL_SHARD_INFO )

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_SHARD_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getShardInfo",
                                "The shard info is null."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sda::isShardCondition( qcStatement  * aStatement,
                              qtcNode      * aCNF,
                              sdiKeyInfo   * aKeyInfo,
                              idBool       * aIsShardCond )
{
    sdaQtcNodeType sQtcNodeType = SDA_NONE;

    qtcNode       * sCurrNode = NULL;

    idBool          sIsAntiJoinInner = ID_FALSE;
    sdiKeyInfo    * sKeyInfoRef = NULL;

    // 초기화
    *aIsShardCond = ID_FALSE;

    IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                         aKeyInfo,
                                         aCNF,
                                         &sQtcNodeType,
                                         &sKeyInfoRef )
              != IDE_SUCCESS );

    if ( ( sQtcNodeType == SDA_EQUAL ) ||
         ( sQtcNodeType == SDA_SHARD_KEY_FUNC ) )
    {
        IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                             aKeyInfo,
                                             (qtcNode*)aCNF->node.arguments,
                                             &sQtcNodeType,
                                             &sKeyInfoRef )
                  != IDE_SUCCESS );

        if ( sQtcNodeType == SDA_KEY_COLUMN )
        {
            sCurrNode = (qtcNode*)aCNF->node.arguments;

            /*
             * BUG-45391
             *
             * Anti-join의 inner table에 대한 shard condition( shard_key = value )은
             * 수행 노드를 한정 할 수 있는 shard condition으로 취급하지 않는다.
             *
             */
            IDE_TEST( checkAntiJoinInner( aKeyInfo,
                                          sCurrNode->node.table,
                                          sCurrNode->node.column,
                                          &sIsAntiJoinInner )
                      != IDE_SUCCESS );

            if ( sIsAntiJoinInner == ID_FALSE )
            {
                IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                     aKeyInfo,
                                                     (qtcNode*)sCurrNode->node.next,
                                                     &sQtcNodeType,
                                                     &sKeyInfoRef )
                          != IDE_SUCCESS );

                if ( ( sQtcNodeType == SDA_HOST_VAR ) ||
                     ( sQtcNodeType == SDA_CONSTANT_VALUE ) )
                {
                    *aIsShardCond = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                *aIsShardCond = ID_FALSE;
            }
        }
        else if ( ( sQtcNodeType == SDA_HOST_VAR ) ||
                  ( sQtcNodeType == SDA_CONSTANT_VALUE ) )
        {
            sCurrNode = (qtcNode*)aCNF->node.arguments;

            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 (qtcNode*)sCurrNode->node.next,
                                                 &sQtcNodeType,
                                                 &sKeyInfoRef )
                      != IDE_SUCCESS );

            if ( sQtcNodeType == SDA_KEY_COLUMN )
            {
                sCurrNode = (qtcNode*)sCurrNode->node.next;

                IDE_TEST( checkAntiJoinInner( aKeyInfo,
                                              sCurrNode->node.table,
                                              sCurrNode->node.column,
                                              &sIsAntiJoinInner )
                          != IDE_SUCCESS );
                if ( sIsAntiJoinInner == ID_FALSE )
                {
                    *aIsShardCond = ID_TRUE;
                }
                else
                {
                    *aIsShardCond = ID_FALSE;
                }
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
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::addValueListOnKeyInfo( qcStatement  * aStatement,
                                   sdiKeyInfo   * aKeyInfo,
                                   sdaValueList * aValueList )
{
    sdiKeyInfo        * sKeyInfo          = NULL;
    sdiKeyInfo        * sKeyInfoForAdding = NULL;

    UInt                sKeyIdx           = 0;

    sdaValueList      * sValueList        = NULL;

    UInt                sValuePtrCount    = 0;
    UInt                sValueIdx         = 0;

    for ( sValueList  = aValueList;
          sValueList != NULL;
          sValueList  = sValueList->mNext )
    {
        sValuePtrCount++;

        for ( sKeyInfo  = aKeyInfo;
              sKeyInfo != NULL;
              sKeyInfo  = sKeyInfo->mNext )
        {
            for ( sKeyIdx = 0;
                  sKeyIdx < sKeyInfo->mKeyCount;
                  sKeyIdx++ )
            {
                if ( ( sValueList->mKeyColumn->node.table ==
                       sKeyInfo->mKey[sKeyIdx].mTupleId ) &&
                     ( sValueList->mKeyColumn->node.column ==
                       sKeyInfo->mKey[sKeyIdx].mColumn ) )
                {
                    if ( sKeyInfoForAdding == NULL )
                    {
                        sKeyInfoForAdding = sKeyInfo;
                    }
                    else
                    {
                        if ( sKeyInfoForAdding != sKeyInfo )
                        {
                            /*
                             * valueList의 shard key column이 같은 keyInfo에 있을 수 도 있고,
                             * 다른 keyInfo에 있을 수 도 있다.
                             *
                             * valueList는 shard value의 OR list이기 때문에,
                             * shard join되지 않아서 별개로 구성된 keyInfo에 value로 추가될 수 없다.
                             *
                             * SELECT *
                             *   FROM T1, T2
                             *  WHERE T1.I1 = a OR T2.I1 = c;
                             *
                             * 위와 같이 shard join되지 않은 두 테이블의 shard key condition이
                             * or로 쓰였을 경우는 Shard value로 취급하지 않는 것 이 맞다.
                             */
                            sKeyInfoForAdding = NULL;
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
                    // Nothing to do.
                }
            }
        }
    }

    if ( sKeyInfoForAdding != NULL )
    {
        //---------------------------------------
        // CNF tree의 AND's ORs 중 먼저 수행되어 keyInfo에 구성 된 value info와
        // View analysis를 통해 올라온 value info는
        // 새로운 CNF tree의 AND's ORs와 AND의 개념이다.
        // 더 적은 것으로 덮어씌운다.
        //
        // 덮어쓰지 않고, 기존 value info를 유지 해도된다.
        //
        // SELECT *
        //   FROM T1
        //  WHERE ( I1 = ? OR I1 = ? OR I1 = ? ) AND ( I1 = ? OR I1 = ? ) AND ( I1 = ? )
        //
        // 위와 같은 경우 처음에 ?,?,?이 key info에 value로 등록되었다가,
        // 다음 CNF tree를 돌면서 들어온 ?,?가 갯 수가 더 적기 때문에 ?,? 로 갱신되고,
        // 그 다음 들어오는 ?가 ?,?보다 value의 갯수가 더 적으므로
        // 최종적으로 맨 뒤의 ? 하나만 shard value로서 유지된다.
        //---------------------------------------
        if ( ( sValuePtrCount < sKeyInfoForAdding->mValuePtrCount ) ||
             ( sKeyInfoForAdding->mValuePtrCount == 0 ) )
        {
            sKeyInfoForAdding->mValuePtrCount = sValuePtrCount;

            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo*) * sValuePtrCount,
                                                     (void**) & sKeyInfoForAdding->mValuePtrArray )
                      != IDE_SUCCESS );

            for ( sValueList  = aValueList, sValueIdx = 0;
                  sValueList != NULL;
                  sValueList  = sValueList->mNext, sValueIdx++ )
            {
                sKeyInfoForAdding->mValuePtrArray[sValueIdx] = sValueList->mValue;
            }
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeTarget( qcStatement * aStatement,
                           qmsQuerySet * aQuerySet,
                           sdiKeyInfo  * aKeyInfo )
{
    qmsTarget  * sQmsTarget        = NULL;
    UShort       sTargetOrder      = 0;

    sdiKeyInfo * sKeyInfoForAdding = NULL;

    sdaQtcNodeType sQtcNodeType    = SDA_NONE;

    qtcNode * sTargetColumn = NULL;

    sdiKeyInfo * sKeyInfoRef = NULL;

    for ( sQmsTarget = aQuerySet->target, sTargetOrder = 0;
          sQmsTarget != NULL;
          sQmsTarget  = sQmsTarget->next, sTargetOrder++ )
    {
        sTargetColumn = sQmsTarget->targetColumn;

        while ( sTargetColumn->node.module == &qtc::passModule )
        {
            sTargetColumn = (qtcNode*)sTargetColumn->node.arguments;
        }

        IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                             aKeyInfo,
                                             sTargetColumn,
                                             &sQtcNodeType,
                                             &sKeyInfoRef )
                  != IDE_SUCCESS );

        if ( sQtcNodeType == SDA_KEY_COLUMN )
        {
            sKeyInfoForAdding = NULL;

            IDE_TEST( getKeyInfoForAddingKeyTarget( sTargetColumn,
                                                    aKeyInfo,
                                                    &sKeyInfoForAdding )
                      != IDE_SUCCESS );

            if ( sKeyInfoForAdding != NULL )
            {
                IDE_TEST( setKeyTarget( aStatement,
                                        sKeyInfoForAdding,
                                        sTargetOrder )
                          != IDE_SUCCESS );
            }
            else
            {
                // Null-padding shard key
                // Nothing to do.
            }
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

IDE_RC sda::getKeyInfoForAddingKeyTarget( qtcNode     * aTargetColumn,
                                          sdiKeyInfo  * aKeyInfo,
                                          sdiKeyInfo ** aKeyInfoForAdding )
{
    sdiKeyInfo        * sKeyInfo        = NULL;
    UInt                sKeyColumnCount = 0;

    // 초기화
    *aKeyInfoForAdding = NULL;

    for ( sKeyInfo  = aKeyInfo;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        for ( sKeyColumnCount = 0;
              sKeyColumnCount < sKeyInfo->mKeyCount;
              sKeyColumnCount++ )
        {
            if ( ( aTargetColumn->node.table  ==
                   sKeyInfo->mKey[sKeyColumnCount].mTupleId ) &&
                 ( aTargetColumn->node.column ==
                   sKeyInfo->mKey[sKeyColumnCount].mColumn ) )
            {
                if ( sKeyInfo->mKey[sKeyColumnCount].mIsNullPadding == ID_FALSE )
                {
                    *aKeyInfoForAdding = sKeyInfo;
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

        if ( *aKeyInfoForAdding != NULL )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::setKeyTarget( qcStatement * aStatement,
                          sdiKeyInfo  * aKeyInfoForAdding,
                          UShort        aTargetOrder )
{
    UShort * sNewKeyTarget = NULL;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  ID_SIZEOF(UShort) * ( aKeyInfoForAdding->mKeyTargetCount + 1 ),
                  (void**) & sNewKeyTarget )
              != IDE_SUCCESS );

    idlOS::memcpy( (void*) sNewKeyTarget,
                   (void*) aKeyInfoForAdding->mKeyTarget,
                   ID_SIZEOF(UShort) * aKeyInfoForAdding->mKeyTargetCount );

    sNewKeyTarget[aKeyInfoForAdding->mKeyTargetCount] = aTargetOrder;

    aKeyInfoForAdding->mKeyTarget = sNewKeyTarget;
    aKeyInfoForAdding->mKeyTargetCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setShardInfoWithKeyInfo( qcStatement  * aStatement,
                                     sdiKeyInfo   * aKeyInfo,
                                     sdiShardInfo * aShardInfo,
                                     idBool         aIsSubKey )
{
    sdiKeyInfo   * sKeyInfo = NULL;
    sdiShardInfo * sShardInfo = NULL;
    sdiRangeInfo * sOutRangeInfo = NULL; /* TASK-7219 Shard Transformer Refactoring */
    idBool         sIsValid = ID_FALSE;

    /*
     * H/R/L + SOLO  = H/R/L
     * H/R/L + CLONE = H/R/L
     * CLONE + SOLO  = SOLO
     */
    for ( sKeyInfo  = aKeyInfo;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        if ( sKeyInfo == aKeyInfo )
        {
            sIsValid = ID_TRUE;
            sShardInfo = &aKeyInfo->mShardInfo;
        }
        else
        {
            /* Nothing to do. */
        }       
        
        if ( ( sdi::getSplitType( sShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) &&
             ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) )
        {
            IDE_TEST( isSameShardInfo( sShardInfo,
                                       &sKeyInfo->mShardInfo,
                                       aIsSubKey,
                                       &sIsValid )
                      != IDE_SUCCESS );
        }
        else if ( ( sdi::getSplitType( sShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) &&
                  ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST ) )
        {
            if ( isSubRangeInfo( sKeyInfo->mShardInfo.mRangeInfo,
                                 sShardInfo->mRangeInfo,
                                 sShardInfo->mDefaultNodeId ) == ID_FALSE )
            {
                sIsValid = ID_FALSE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else if ( ( sdi::getSplitType( sShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST ) &&
                  ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST ) )
        {
            if ( isSubRangeInfo( sShardInfo->mRangeInfo,
                                 sKeyInfo->mShardInfo.mRangeInfo,
                                 sKeyInfo->mShardInfo.mDefaultNodeId ) == ID_FALSE )
            {
                sIsValid = ID_FALSE;
            }
            else
            {
                // CLONE with HASH = HASH
                sShardInfo = &sKeyInfo->mShardInfo;
            }
        }
        else if ( ( sdi::getSplitType( sShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST ) &&
                  ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST ) )
        {
            if ( isSameRangesForCloneAndSolo( sShardInfo->mRangeInfo,
                                              sKeyInfo->mShardInfo.mRangeInfo ) == ID_FALSE)
            {
                /* TASK-7219 Shard Transformer Refactoring */
                IDE_TEST( getRangeIntersection( aStatement,
                                                sShardInfo->mRangeInfo,
                                                sKeyInfo->mShardInfo.mRangeInfo,
                                                &( sOutRangeInfo ) )
                          != IDE_SUCCESS );

                if ( sOutRangeInfo->mCount == 0 )
                {
                    sIsValid = ID_FALSE;
                }
                else
                {
                    sShardInfo->mRangeInfo = sOutRangeInfo;
                }
            }
            else
            {
                /* Nothing to do. */
            }

            if ( sKeyInfo->mShardInfo.mSplitMethod == SDI_SPLIT_SOLO )
            {
                sShardInfo->mSplitMethod = SDI_SPLIT_SOLO;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            sIsValid = ID_FALSE;
            break;
        }

        if ( sIsValid == ID_FALSE )
        {
            break;
        }
        else
        {
            /* Nothing to do. */
        }
    }

    if ( sIsValid == ID_TRUE )
    {
        aShardInfo->mSplitMethod   = sShardInfo->mSplitMethod;
        aShardInfo->mDefaultNodeId = sShardInfo->mDefaultNodeId;
        aShardInfo->mKeyDataType   = sShardInfo->mKeyDataType;
        aShardInfo->mRangeInfo     = sShardInfo->mRangeInfo;
    }
    else
    {
        aShardInfo->mSplitMethod   = SDI_SPLIT_NONE;
        aShardInfo->mDefaultNodeId = SDI_NODE_NULL_ID;
        aShardInfo->mKeyDataType   = ID_UINT_MAX;
        aShardInfo->mRangeInfo     = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getRangeIntersection( qcStatement   * aStatement,
                                  sdiRangeInfo  * aRangeInfo1,
                                  sdiRangeInfo  * aRangeInfo2,
                                  sdiRangeInfo ** aOutRangeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                getRangeIntersection 함수의 입출력 Argument 변형
 *
 * Implementation : 기존 getRangeIntersection 를 입력, 출력 Argument 로 나누어 처리
 *
 ***********************************************************************/

    sdiRangeInfo * sOutRangeInfo = NULL;

    if ( isSubRangeInfo( aRangeInfo1,
                         aRangeInfo2,
                         ID_UINT_MAX )
         == ID_TRUE )
    {
        sOutRangeInfo = aRangeInfo2;
    }
    else if ( isSubRangeInfo( aRangeInfo2,
                              aRangeInfo1,
                              ID_UINT_MAX )
         == ID_TRUE )
    {
        sOutRangeInfo = aRangeInfo1;
    }
    else
    {
        IDE_TEST( andRangeInfo( aStatement,
                                aRangeInfo1,
                                aRangeInfo2,
                                &( sOutRangeInfo ) )
                  != IDE_SUCCESS );
    }

    if ( aOutRangeInfo != NULL )
    {
        *aOutRangeInfo = sOutRangeInfo;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sda::copyShardInfoFromShardInfo( sdiShardInfo * aTo,
                                      sdiShardInfo * aFrom )
{
    IDE_DASSERT( aTo   != NULL);
    IDE_DASSERT( aFrom != NULL);

    aTo->mKeyDataType   = aFrom->mKeyDataType;
    aTo->mDefaultNodeId = aFrom->mDefaultNodeId;
    aTo->mSplitMethod   = aFrom->mSplitMethod;
    aTo->mRangeInfo     = aFrom->mRangeInfo;
}

void sda::copyShardInfoFromObjectInfo( sdiShardInfo  * aTo,
                                       sdiObjectInfo * aFrom,
                                       idBool          aIsSubKey )
{
    IDE_DASSERT( aTo   != NULL);
    IDE_DASSERT( aFrom != NULL);

    if ( aIsSubKey == ID_FALSE )
    {
        aTo->mKeyDataType   = aFrom->mTableInfo.mKeyDataType;
        aTo->mDefaultNodeId = aFrom->mTableInfo.mDefaultNodeId;
        aTo->mSplitMethod   = aFrom->mTableInfo.mSplitMethod;
    }
    else
    {
        aTo->mKeyDataType   = aFrom->mTableInfo.mSubKeyDataType;
        aTo->mDefaultNodeId = aFrom->mTableInfo.mDefaultNodeId;
        aTo->mSplitMethod   = aFrom->mTableInfo.mSubSplitMethod;
    }

    aTo->mRangeInfo = &aFrom->mRangeInfo;
}

IDE_RC sda::allocAndCopyValue( qcStatement      * aStatement,
                               sdiValueInfo     * aFromValueInfo,
                               sdiValueInfo    ** aToValueInfo )
{
    switch( aFromValueInfo->mType )
    {
        case SDI_VALUE_INFO_HOST_VAR:
             IDE_TEST( allocAndSetShardHostValueInfo( aStatement,
                                                      aFromValueInfo->mValue.mBindParamId,
                                                      aToValueInfo )
                       != IDE_SUCCESS );
            break;

        case SDI_VALUE_INFO_CONST_VAL:
            IDE_TEST( allocAndSetShardConstValueInfo( aStatement,
                                                      aFromValueInfo->mDataValueType,
                                                      &aFromValueInfo->mValue,
                                                      aToValueInfo )
              != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_INVALID_SHARD_VALUE_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_VALUE_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::allocAndCopyValue",
                                  "Invalid shard value type" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::allocAndMergeValues( qcStatement         * aStatement,
                                 sdiValueInfo       ** aSrcValueInfoPtrArray1,
                                 UShort                aSrcValueInfoPtrCount1,
                                 sdiValueInfo       ** aSrcValueInfoPtrArray2,
                                 UShort                aSrcValueInfoPtrCount2,
                                 sdiValueInfo *     ** aNewValueInfoPtrArray,
                                 UShort              * aNewValueInfoPtrCount )
{
    UInt               i = 0;
    sdiValueInfo    ** sNewValueInfoPtrArray = NULL;
    UInt               sNewValueInfoPtrIndex = 0;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiValueInfo*) * 
                                             ( aSrcValueInfoPtrCount1 + aSrcValueInfoPtrCount2 ),
                                             (void**) &sNewValueInfoPtrArray )
              != IDE_SUCCESS );

    for ( i = 0; i < aSrcValueInfoPtrCount1; i++, sNewValueInfoPtrIndex++ )
    {
        IDE_TEST( allocAndCopyValue( aStatement,
                                     aSrcValueInfoPtrArray1[i],
                                     &sNewValueInfoPtrArray[sNewValueInfoPtrIndex] )
                  != IDE_SUCCESS );
    }

    for ( i = 0; i < aSrcValueInfoPtrCount2; i++, sNewValueInfoPtrIndex++ )
    {
        IDE_TEST( allocAndCopyValue( aStatement,
                                     aSrcValueInfoPtrArray2[i],
                                     &sNewValueInfoPtrArray[sNewValueInfoPtrIndex] )
                  != IDE_SUCCESS );
    }

    *aNewValueInfoPtrArray = sNewValueInfoPtrArray;
    *aNewValueInfoPtrCount = sNewValueInfoPtrIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::andRangeInfo( qcStatement   * aStatement,
                          sdiRangeInfo  * aRangeInfo1,
                          sdiRangeInfo  * aRangeInfo2,
                          sdiRangeInfo ** aAndRangeInfo )
{
    UShort sMinRangeCount = 0;
    UShort i = 0;
    UShort j = 0;

    IDE_DASSERT( aRangeInfo1 != NULL);
    IDE_DASSERT( aRangeInfo2 != NULL);

    sMinRangeCount = (aRangeInfo1->mCount < aRangeInfo2->mCount)?
        aRangeInfo1->mCount:aRangeInfo2->mCount ;
    
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiRangeInfo),
                                             (void**) aAndRangeInfo )
              != IDE_SUCCESS );

    (*aAndRangeInfo)->mCount = 0;
    (*aAndRangeInfo)->mRanges = NULL;

    if ( sMinRangeCount > 0  )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiRange) *
                                                 sMinRangeCount,
                                                 (void**) & (*aAndRangeInfo)->mRanges )
                  != IDE_SUCCESS );

        for ( i = 0; i < aRangeInfo1->mCount; i++ )
        {
            for ( j = 0; j < aRangeInfo2->mCount; j++ )
            {
                if ( aRangeInfo1->mRanges[i].mNodeId ==
                     aRangeInfo2->mRanges[j].mNodeId )
                {
                    (*aAndRangeInfo)->mRanges[(*aAndRangeInfo)->mCount].mNodeId =
                        aRangeInfo1->mRanges[i].mNodeId;
                    (*aAndRangeInfo)->mCount++;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sda::findRangeInfo( sdiRangeInfo * aRangeInfo,
                           UInt           aNodeId )
{
    idBool sIsFound = ID_FALSE;
    UShort i;

    IDE_DASSERT( aRangeInfo != NULL);

    for ( i = 0; i < aRangeInfo->mCount; i++ )
    {
        if ( aRangeInfo->mRanges[i].mNodeId == aNodeId )
        {
            sIsFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return sIsFound;
}

idBool sda::isSubRangeInfo( sdiRangeInfo * aRangeInfo,
                            sdiRangeInfo * aSubRangeInfo,
                            UInt           aSubRangeDefaultNode )
{
    idBool sIsSubset = ID_TRUE;
    UShort i;

    IDE_DASSERT( aSubRangeInfo != NULL);

    for ( i = 0; i < aSubRangeInfo->mCount; i++ )
    {
        if ( findRangeInfo( aRangeInfo,
                            aSubRangeInfo->mRanges[i].mNodeId ) == ID_FALSE )
        {
            sIsSubset = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    // Default node checking
    if ( ( sIsSubset == ID_TRUE ) &&
         ( aSubRangeDefaultNode != ID_UINT_MAX ) )
    {
        if ( findRangeInfo( aRangeInfo,
                            aSubRangeDefaultNode ) == ID_FALSE )
        {
            sIsSubset = ID_FALSE;
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

    return sIsSubset;
}

/* TASJ-7219
 *   BUG-47903 이후, 전 노드 수행 Non-Shard Query 를 고려해서 수정함 
 */
IDE_RC sda::isShardQuery( sdiAnalysisFlag * aAnalysisFlag,
                          idBool          * aIsShardQuery )
{
    UShort sIdx = 0;

    // 초기화
    *aIsShardQuery = ID_TRUE;

    for ( sIdx = 0;
          sIdx < SDI_NON_SHARD_QUERY_REASON_MAX;
          sIdx++ )
    {
        if ( aAnalysisFlag->mNonShardFlag[ sIdx ] == ID_TRUE)
        {
            *aIsShardQuery = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::mergeAnalysisFlag( sdiAnalysisFlag * aFlag1,
                               sdiAnalysisFlag * aFlag2,
                               UShort            aType )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aFlag1 == NULL, ERR_NULL_FLAG_1 );
    IDE_TEST_RAISE( aFlag2 == NULL, ERR_NULL_FLAG_2 );

    if ( ( aType & SDI_ANALYSIS_FLAG_NON_MASK )
         == SDI_ANALYSIS_FLAG_NON_TRUE )
    {
        IDE_TEST( mergeFlag( aFlag1->mNonShardFlag,
                             aFlag2->mNonShardFlag,
                             SDI_NON_SHARD_QUERY_REASON_MAX )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( aType & SDI_ANALYSIS_FLAG_TOP_MASK )
         == SDI_ANALYSIS_FLAG_TOP_TRUE )
    {
        IDE_TEST_RAISE( ( ( aType & SDI_ANALYSIS_FLAG_NON_MASK ) == SDI_ANALYSIS_FLAG_NON_FALSE ),
                        ERR_NULL_FLAG );

        IDE_TEST( mergeFlag( aFlag1->mTopQueryFlag,
                             aFlag2->mTopQueryFlag,
                             SDI_ANALYSIS_TOP_QUERY_FLAG_MAX )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( aType & SDI_ANALYSIS_FLAG_SET_MASK )
         == SDI_ANALYSIS_FLAG_SET_TRUE )
    {
        IDE_TEST_RAISE( ( ( aType & SDI_ANALYSIS_FLAG_NON_MASK ) == SDI_ANALYSIS_FLAG_NON_FALSE )
                        ||
                        ( ( aType & SDI_ANALYSIS_FLAG_TOP_MASK ) == SDI_ANALYSIS_FLAG_TOP_FALSE ),
                        ERR_NULL_FLAG );

        IDE_TEST( mergeFlag( aFlag1->mSetQueryFlag,
                             aFlag2->mSetQueryFlag,
                             SDI_ANALYSIS_SET_QUERY_FLAG_MAX )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( ( aType & SDI_ANALYSIS_FLAG_CUR_MASK )
         == SDI_ANALYSIS_FLAG_CUR_TRUE )
    {
        IDE_TEST_RAISE( ( ( aType & SDI_ANALYSIS_FLAG_CUR_MASK ) == SDI_ANALYSIS_FLAG_NON_FALSE )
                        ||
                        ( ( aType & SDI_ANALYSIS_FLAG_CUR_MASK ) == SDI_ANALYSIS_FLAG_TOP_FALSE )
                        ||
                        ( ( aType & SDI_ANALYSIS_FLAG_CUR_MASK ) == SDI_ANALYSIS_FLAG_SET_FALSE ),
                        ERR_NULL_FLAG );

        IDE_TEST( mergeFlag( aFlag1->mCurQueryFlag,
                             aFlag2->mCurQueryFlag,
                             SDI_ANALYSIS_CUR_QUERY_FLAG_MAX )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FLAG_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::mergeAnalysisFlag",
                                  "AnalysisfFlag 1 is NULL." ) );
    }
    IDE_EXCEPTION( ERR_NULL_FLAG_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::mergeAnalysisFlag",
                                  "Analysis flag 2 is NULL." ) );
    }
    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::mergeAnalysisFlag",
                                  "flag is invalid." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::mergeFlag( idBool * aFlag1,
                       idBool * aFlag2,
                       UInt     aIdx )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt sIdx = 0;

    IDE_TEST_RAISE( aFlag1 == NULL, ERR_NULL_FLAG_1 );
    IDE_TEST_RAISE( aFlag2 == NULL, ERR_NULL_FLAG_2 );

    for ( sIdx = 0;
          sIdx < aIdx;
          sIdx++ )
    {
        if ( ( aFlag1[ sIdx ] == ID_TRUE )
             ||
             ( aFlag2[ sIdx ] == ID_TRUE ) )
        {
            aFlag2[ sIdx ] = ID_TRUE;
        }
        else
        {
            /*Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FLAG_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::mergeFlag",
                                  "AnalysisfFlag 1 is NULL." ) );
    }
    IDE_EXCEPTION( ERR_NULL_FLAG_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::mergeFlag",
                                  "Analysis flag 2 is NULL." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4SFWGH( qcStatement     * aStatement,
                                   qmsSFWGH        * aSFWGH,
                                   sdiKeyInfo      * aKeyInfo,
                                   sdiAnalysisFlag * aAnalysisFlag )
{
    idBool sIsOneNodeSQL = ID_FALSE;
    UInt   sDistKeyCount = 0;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( setAnalysisFlag4Target( aStatement,
                                      aSFWGH->target,
                                      aAnalysisFlag )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( setAnalysisFlag4GroupPsmBind( aSFWGH->group,
                                            aAnalysisFlag )
              != IDE_SUCCESS );

    //---------------------------------------
    // Check join needed multi-nodes
    //---------------------------------------
    IDE_TEST( isOneNodeSQL( NULL,
                            aKeyInfo,
                            &sIsOneNodeSQL,
                            &sDistKeyCount )
              != IDE_SUCCESS );

    if ( ( sDistKeyCount > 1 ) &&
         ( aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_JOIN_EXISTS ] == ID_FALSE ) )
    {
        aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_JOIN_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    if ( sIsOneNodeSQL == ID_FALSE )
    {
        //---------------------------------------
        // Check distinct
        //---------------------------------------
        if ( aSFWGH->selectType == QMS_DISTINCT )
        {
            IDE_TEST( setAnalysisFlag4Distinct( aStatement,
                                                aSFWGH,
                                                aKeyInfo,
                                                aAnalysisFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. */
        }

        //---------------------------------------
        // Check group-aggregation
        //---------------------------------------
        if ( aSFWGH->aggsDepth2 == NULL )
        { 
            if ( aSFWGH->group != NULL )
            {
                IDE_TEST( setAnalysisFlag4GroupBy( aStatement,
                                                   aSFWGH,
                                                   aKeyInfo,
                                                   aAnalysisFlag )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( aSFWGH->aggsDepth1 != NULL )
                {
                    aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS ] = ID_TRUE;

                    /* TASK-7219 Shard Transformer Refactoring
                     *  TransformAble 속성은 Query Set 분석이 완료되어 있을 때에 확정할 수 있으며,
                     *   여기서는 변형 가능한 Group By 형태인지만 검사하여 남긴다.
                     */
                    aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }
        else
        {
            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_NESTED_AGGREGATION_EXISTS ] = ID_TRUE;
        }
    }
    else
    {
        /* Nothing to do. */
    }

    //---------------------------------------
    // Check hierarchy
    //---------------------------------------
    if ( aSFWGH->hierarchy != NULL )
    {
        aAnalysisFlag->mNonShardFlag[ SDI_HIERARCHY_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4Distinct( qcStatement     * aStatement,
                                      qmsSFWGH        * aSFWGH,
                                      sdiKeyInfo      * aKeyInfo,
                                      sdiAnalysisFlag * aAnalysisFlag )
{
    sdiKeyInfo * sKeyInfo  = NULL;
    idBool       sIsFound  = ID_FALSE;
    UInt         sKeyCount = 0;

    sdaQtcNodeType     sQtcNodeType = SDA_NONE;
    qmsTarget        * sTarget = NULL;
    qtcNode          * sTargetColumn = NULL;
    sdiKeyInfo       * sKeyInfoRef = NULL;

    for ( sKeyInfo  = aKeyInfo;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        if ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
        {
            sIsFound = ID_FALSE;

            for ( sKeyCount = 0;
                  sKeyCount < sKeyInfo->mKeyCount;
                  sKeyCount++ )
            {
                IDE_DASSERT( aSFWGH != NULL);

                for ( sTarget = aSFWGH->target;
                      sTarget != NULL;
                      sTarget = sTarget->next )
                {
                    sTargetColumn = sTarget->targetColumn;

                    while ( sTargetColumn->node.module == &qtc::passModule )
                    {
                        sTargetColumn = (qtcNode*)sTargetColumn->node.arguments;
                    }

                    IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                         aKeyInfo,
                                                         sTargetColumn,
                                                         &sQtcNodeType,
                                                         &sKeyInfoRef )
                              != IDE_SUCCESS );

                    if ( sQtcNodeType == SDA_KEY_COLUMN )
                    {
                        if ( ( sKeyInfo->mKey[sKeyCount].mTupleId ==
                               sTargetColumn->node.table ) &&
                             ( sKeyInfo->mKey[sKeyCount].mColumn  ==
                               sTargetColumn->node.column ) &&
                             ( sKeyInfo->mKey[sKeyCount].mIsNullPadding ==
                               ID_FALSE ) )
                        {
                            sIsFound = ID_TRUE;
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

                if ( sIsFound == ID_TRUE )
                {
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( sIsFound == ID_FALSE )
            {
                // Target 에 올라오지 않은 shard key가 있다면,
                // Distinct 가 분산수행 불가능 한 것으로 판단한다.
                IDE_DASSERT( aAnalysisFlag != NULL );
                aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_DISTINCT_EXISTS ] = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Split clone
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4GroupBy( qcStatement     * aStatement,
                                     qmsSFWGH        * aSFWGH,
                                     sdiKeyInfo      * aKeyInfo,
                                     sdiAnalysisFlag * aAnalysisFlag )
{
    sdiKeyInfo * sKeyInfo  = NULL;
    idBool       sIsFound  = ID_FALSE;
    idBool       sNotNormalGroupExists = ID_FALSE;
    UInt         sKeyCount = 0;

    sdaQtcNodeType     sQtcNodeType = SDA_NONE;
    qmsConcatElement * sGroupKey = NULL;

    sdiKeyInfo       * sKeyInfoRef = NULL;

    if ( ( aSFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK )
         == QMV_SFWGH_GBGS_TRANSFORM_NONE )
    {
        for ( sKeyInfo  = aKeyInfo;
              sKeyInfo != NULL;
              sKeyInfo  = sKeyInfo->mNext )
        {
            if ( sdi::getSplitType( sKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
            {
                sIsFound = ID_FALSE;

                IDE_DASSERT( aAnalysisFlag != NULL );

                for ( sGroupKey  = aSFWGH->group;
                      sGroupKey != NULL;
                      sGroupKey  = sGroupKey->next )
                {
                    if ( sGroupKey->type == QMS_GROUPBY_NORMAL )
                    {
                        for ( sKeyCount = 0;
                              sKeyCount < sKeyInfo->mKeyCount;
                              sKeyCount++ )
                        {
                            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                                 aKeyInfo,
                                                                 sGroupKey->arithmeticOrList,
                                                                 &sQtcNodeType,
                                                                 &sKeyInfoRef )
                                      != IDE_SUCCESS );

                            if ( sQtcNodeType == SDA_KEY_COLUMN )
                            {
                                if ( ( sKeyInfo->mKey[sKeyCount].mTupleId ==
                                       sGroupKey->arithmeticOrList->node.table ) &&
                                     ( sKeyInfo->mKey[sKeyCount].mColumn ==
                                       sGroupKey->arithmeticOrList->node.column ) &&
                                     ( sKeyInfo->mKey[sKeyCount].mIsNullPadding ==
                                       ID_FALSE ) )
                                {
                                    sIsFound = ID_TRUE;
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
                    }
                    else
                    {
                        // Group by extension( ROLLUP, CUBE )
                        sNotNormalGroupExists = ID_TRUE;
                        break;
                    }
                }

                if ( sNotNormalGroupExists == ID_TRUE )
                {
                    aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_GROUP_BY_EXTENSION_EXISTS ] = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                if ( sIsFound == ID_FALSE )
                {
                    aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS ] = ID_TRUE;

                    /* TASK-7219 Shard Transformer Refactoring
                     *  TransformAble 속성은 Query Set 분석이 완료되어 있을 때에 확정할 수 있으며,
                     *   여기서는 변형 가능한 Group By 형태인지만 검사하여 남긴다.
                     */
                    aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_TRUE;

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
    }
    else
    {
        // Group by extension( GROUPING SETS )
        aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_GROUP_BY_EXTENSION_EXISTS ] = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4WindowFunc( qmsAnalyticFunc * aAnalyticFuncList,
                                        sdiKeyInfo      * aKeyInfo,
                                        sdiAnalysisFlag * aAnalysisFlag )
{
    qmsAnalyticFunc * sAnalyticFunc = NULL;
    qtcNode         * sAnalyticFuncNode = NULL;
    qtcOverColumn   * sOrderCol = NULL;
    qtcOverColumn   * sPartitionCol = NULL;
    qtcNode         * sTarget = NULL;
    sdiKeyInfo      * sKeyInfo  = NULL;
    UInt              sKeyCount = 0;
    idBool            sIsFound = ID_FALSE;

    IDE_DASSERT( aAnalysisFlag != NULL );

    for ( sAnalyticFunc = aAnalyticFuncList; sAnalyticFunc != NULL; sAnalyticFunc = sAnalyticFunc->next )
    {
        IDE_DASSERT( sAnalyticFunc->analyticFuncNode != NULL );
        sAnalyticFuncNode = sAnalyticFunc->analyticFuncNode;

        if ( sAnalyticFuncNode->overClause != NULL )
        {
            if ( sAnalyticFuncNode->overClause->partitionByColumn != NULL )
            {
                sOrderCol = sAnalyticFuncNode->overClause->orderByColumn;

                for ( sKeyInfo = aKeyInfo; sKeyInfo != NULL; sKeyInfo = sKeyInfo->mNext )
                {
                    if ( ( sKeyInfo->mShardInfo.mSplitMethod != SDI_SPLIT_CLONE ) &&
                         ( sKeyInfo->mShardInfo.mSplitMethod != SDI_SPLIT_SOLO ) )
                    {
                        sIsFound = ID_FALSE;

                        for ( sPartitionCol = sAnalyticFuncNode->overClause->partitionByColumn;
                              ( sPartitionCol != NULL ) && ( sPartitionCol != sOrderCol );
                              sPartitionCol = sPartitionCol->next )
                        {
                            sTarget = (qtcNode*)(sPartitionCol->node);
                            while ( sTarget->node.module == &qtc::passModule )
                            {
                                sTarget = (qtcNode*)(sTarget->node.arguments);
                            }

                            if ( sTarget->node.module == &qtc::columnModule )
                            {
                                for ( sKeyCount = 0;
                                      sKeyCount < sKeyInfo->mKeyCount;
                                      sKeyCount++ )
                                {
                                    if ( ( sTarget->node.table == sKeyInfo->mKey[sKeyCount].mTupleId ) &&
                                         ( sTarget->node.column == sKeyInfo->mKey[sKeyCount].mColumn ) &&
                                         ( sKeyInfo->mKey[sKeyCount].mIsNullPadding == ID_FALSE ) )
                                    {
                                        sIsFound = ID_TRUE;
                                        break;
                                    }
                                }
                            }
                        }

                        if ( sIsFound == ID_FALSE )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_WINDOW_FUNCTION_EXISTS ] = ID_TRUE;
                            break;
                        }
                    }
                }
            }
            else
            {
                aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_WINDOW_FUNCTION_EXISTS ] = ID_TRUE;
            }
        }
        else
        {
            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_WINDOW_FUNCTION_EXISTS ] = ID_TRUE;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::analyzeQuerySetAnalysis( qcStatement * aStatement,
                                     qmsQuerySet * aMyQuerySet,
                                     qmsQuerySet * aLeftQuerySet,
                                     qmsQuerySet * aRightQuerySet,
                                     idBool        aIsSubKey )
{
    sdiShardAnalysis * sQuerySetAnalysis      = NULL;
    sdiShardAnalysis * sLeftQuerySetAnalysis  = NULL;
    sdiShardAnalysis * sRightQuerySetAnalysis = NULL;

    //------------------------------------------
    // 할당
    //------------------------------------------
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiShardAnalysis),
                                             (void**) & sQuerySetAnalysis )
              != IDE_SUCCESS );

    SDI_INIT_SHARD_ANALYSIS( sQuerySetAnalysis );

    //------------------------------------------
    // 초기화
    //------------------------------------------
    if ( aIsSubKey == ID_FALSE )
    {
        aMyQuerySet->mShardAnalysis = sQuerySetAnalysis;
        sLeftQuerySetAnalysis = aLeftQuerySet->mShardAnalysis;
        sRightQuerySetAnalysis = aRightQuerySet->mShardAnalysis;
    }
    else
    {
        IDE_TEST_RAISE( aMyQuerySet->mShardAnalysis == NULL, ERR_NULL_SHARD_ANALYSIS );

        aMyQuerySet->mShardAnalysis->mNext = sQuerySetAnalysis;
        sLeftQuerySetAnalysis = aLeftQuerySet->mShardAnalysis->mNext;
        sRightQuerySetAnalysis = aRightQuerySet->mShardAnalysis->mNext;
    }

     /* TASK-7219 Shard Transformer Refactoring */
    //------------------------------------------
    // Get NON-SHARD QUERY REASON
    //------------------------------------------
    IDE_TEST( mergeAnalysisFlag( &( sLeftQuerySetAnalysis->mAnalysisFlag ),
                                 &( sQuerySetAnalysis->mAnalysisFlag ),
                                 SDI_ANALYSIS_FLAG_NON_TRUE |
                                 SDI_ANALYSIS_FLAG_TOP_TRUE |
                                 SDI_ANALYSIS_FLAG_SET_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( mergeAnalysisFlag( &( sRightQuerySetAnalysis->mAnalysisFlag ),
                                 &( sQuerySetAnalysis->mAnalysisFlag ),
                                 SDI_ANALYSIS_FLAG_NON_TRUE |
                                 SDI_ANALYSIS_FLAG_TOP_TRUE |
                                 SDI_ANALYSIS_FLAG_SET_TRUE )
              != IDE_SUCCESS );

    //------------------------------------------
    // Analyze left, right of SET
    //------------------------------------------
    IDE_TEST( analyzeSetLeftRight( aStatement,
                                   sLeftQuerySetAnalysis,
                                   sRightQuerySetAnalysis,
                                   &sQuerySetAnalysis->mKeyInfo,
                                   aIsSubKey )
              != IDE_SUCCESS );

    //------------------------------------------
    // Make Shard Info regarding for clone
    //------------------------------------------
    IDE_TEST( setShardInfoWithKeyInfo( aStatement,
                                       sQuerySetAnalysis->mKeyInfo,
                                       &sQuerySetAnalysis->mShardInfo,
                                       aIsSubKey )
              != IDE_SUCCESS );

    IDE_TEST( setQuerySetAnalysis( aMyQuerySet,
                                   sLeftQuerySetAnalysis->mShardInfo.mSplitMethod,
                                   sRightQuerySetAnalysis->mShardInfo.mSplitMethod,
                                   sQuerySetAnalysis )
              != IDE_SUCCESS );  

    //------------------------------------------
    // Make Query set analysis
    //------------------------------------------

    IDE_TEST( isShardQuery( &( sQuerySetAnalysis->mAnalysisFlag ),
                            &( sQuerySetAnalysis->mIsShardQuery ) )
              != IDE_SUCCESS );

    //------------------------------------------
    // Make & Link table info list
    //------------------------------------------

    if ( aIsSubKey == ID_FALSE )
    {
        IDE_TEST( mergeTableInfoList( aStatement,
                                      sQuerySetAnalysis,
                                      sLeftQuerySetAnalysis,
                                      sRightQuerySetAnalysis )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_SHARD_ANALYSIS)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeQuerySetAnalysis",
                                "The shard analysis is null."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setQuerySetAnalysis( qmsQuerySet      * aQuerySet,
                                 sdiSplitMethod     aLeftQuerySetSplit,
                                 sdiSplitMethod     aRightQuerySetSplit,
                                 sdiShardAnalysis * aSetAnalysis )
{
    idBool sIsOneNodeSQL = ID_TRUE;
    sdiKeyInfo * sKeyInfoCheck = NULL;
    UInt sDistKeyCount = 0;

    if ( aSetAnalysis->mShardInfo.mSplitMethod != SDI_SPLIT_NONE )
    {
        IDE_TEST( isOneNodeSQL( NULL,
                                aSetAnalysis->mKeyInfo,
                                &sIsOneNodeSQL,
                                &sDistKeyCount )
                  != IDE_SUCCESS );

        if ( sIsOneNodeSQL == ID_FALSE )
        {
            IDE_TEST_RAISE( ( sdi::getSplitType( aLeftQuerySetSplit ) == SDI_SPLIT_TYPE_NO_DIST ) &&
                            ( sdi::getSplitType( aRightQuerySetSplit ) == SDI_SPLIT_TYPE_NO_DIST ),
                            ERR_SET_ANALYSIS_WAS_FAILED );
            
            if ( aQuerySet->setOp == QMS_UNION_ALL )
            {
                /*
                 * SHARD QUERY     : H/R/L UNION ALL H/R/L
                 *                   C     UNION ALL C ( sIsOneNodeSQL == ID_TRUE )
                 *
                 * NON-SHARD QUERY : H/R/L UNION ALL C
                 *                   C     UNION ALL H/R/L
                 *
                 */

                if ( ( sdi::getSplitType( aLeftQuerySetSplit ) == SDI_SPLIT_TYPE_DIST ) &&
                     ( sdi::getSplitType( aRightQuerySetSplit ) == SDI_SPLIT_TYPE_DIST ) )
                {
                    for ( sKeyInfoCheck  = aSetAnalysis->mKeyInfo;
                          sKeyInfoCheck != NULL;
                          sKeyInfoCheck  = sKeyInfoCheck->mNext )
                    {
                        // H/R/L UNION ALL H/R/L
                        if ( ( sKeyInfoCheck->mLeft == NULL ) || ( sKeyInfoCheck->mRight == NULL ) )
                        {
                            /*
                             * SELECT SKEY_I1     << this is not a shard key, must be unset as a non-shard key.
                             *   FROM HASH_T1
                             *  WHERE SKEY_I1 = 1 << this is not a shard value, must be removed.
                             * UNION ALL
                             * SELECT I2
                             *   FROM HASH_T1
                             */

                            sKeyInfoCheck->mKeyTargetCount = 0;
                            sKeyInfoCheck->mKeyTarget = NULL;
                            sKeyInfoCheck->mValuePtrCount = 0;
                            sKeyInfoCheck->mValuePtrArray = NULL;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                }
                else
                {
                    aSetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_SET_OP_EXISTS ] = ID_TRUE;
                }
                
            }
            else
            {
                /*
                 * SHARD QUERY     : H/R/L UNION H/R/L ( The shard keies were joined. )
                 *                   H/R/L INTERSECT C
                 *                   H/R/L MINUS C
                 *                   C     UNION C ( sIsOneNodeSQL == ID_TRUE )
                 *
                 * NON-SHARD QUERY : H/R/L UNION H/R/L ( The shard keies were not joined. )
                 *                   C     UNION H/R/L
                 *                   H/R/L UNION C
                 *
                 */

                if ( ( sdi::getSplitType( aLeftQuerySetSplit ) == SDI_SPLIT_TYPE_DIST ) &&
                     ( sdi::getSplitType( aRightQuerySetSplit ) == SDI_SPLIT_TYPE_DIST ) )
                {
                    for ( sKeyInfoCheck  = aSetAnalysis->mKeyInfo;
                          sKeyInfoCheck != NULL;
                          sKeyInfoCheck  = sKeyInfoCheck->mNext )
                    {
                        if ( ( sKeyInfoCheck->mLeft == NULL ) || ( sKeyInfoCheck->mRight == NULL ) )
                        {
                            aSetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_SET_OP_EXISTS ] = ID_TRUE;
                        }
                        else
                        {
                            // Target에 모든 shard key가 shard join되면 union all과 동일하게 취급 될 수 있다.
                            /* Nothing to do */
                        }
                    }
                }
                else if ( ( sdi::getSplitType( aLeftQuerySetSplit ) == SDI_SPLIT_TYPE_DIST ) &&
                          ( sdi::getSplitType( aRightQuerySetSplit ) == SDI_SPLIT_TYPE_NO_DIST ) &&
                          ( ( aQuerySet->setOp == QMS_INTERSECT ) ||
                            ( aQuerySet->setOp == QMS_MINUS ) ) )
                {
                    // SET-DIFFERENCE(MINUS), SET-INTERSECT의 경우
                    // SET operator의 right query set에 clone이 오는 것 은 허용 할 수 있다. ( UNION, UNION ALL은 안됨 )
                    /* Nothing to do. */
                }
                else
                {
                    aSetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_SET_OP_EXISTS ] = ID_TRUE;
                }
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        aSetAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SET_ANALYSIS_WAS_FAILED)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::setQuerySetAnalysis",
                                "SET analysis was failed."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeSetLeftRight( qcStatement       * aStatement,
                                 sdiShardAnalysis  * aLeftQuerySetAnalysis,
                                 sdiShardAnalysis  * aRightQuerySetAnalysis,
                                 sdiKeyInfo       ** aKeyInfo,
                                 idBool              aIsSubKey )
{
    sdiKeyInfo * sLeftKeyInfo  = NULL;
    sdiKeyInfo * sRightKeyInfo = NULL;

    sdiKeyInfo * sLeftDevidedKeyInfo  = NULL;
    sdiKeyInfo * sRightDevidedKeyInfo = NULL;

    sdiKeyInfo * sKeyInfoTmp = NULL;

    //---------------------------------------
    // Left의 key info로 부터 SET의 key info를 생성한다.
    //---------------------------------------

    /*
     * SELECT T1.I1 A, T1.I1 B, T2.I1 C, T2.I1 D
     *   FROM T1, T2
     *  UNION
     * SELECT T1.I1  , T2.I1  , T2.I1  , T1.I1
     *   FROM T1, T2;
     *
     * LEFT KEY GROUP  : { T1.I1, T1.I1 }, { T2.I1, T2.I1 }
     *                      |         \        |     |
     *                      |            \     |    (T1.I1*)
     *                      |               \  |
     * RIGHT KEY GROUP : { T1.I1, T1.I1*}, { T2.I1, T2.I1 }
     *
     * UNION :     A      B      C      D
     * LEFT  :   T1.I1   T1.I1  T2.I1  T2.I1
     * RIGHT :   T1.I1   T2.I1  T2.I1  T1.I1
     *
     *
     * 이렇게 서로다른 key group끼리 SET-target merging 될 수 있다
     *
     * 때문에 일단 key group을 분할해서 SET의 key info를 구성하고,
     * (debideKeyInfo + mergeKeyInfo4Set)
     *
     * 다시 합칠 수 있는 key info들을 합치는 작업을 수행한다.
     * (mergeSameKeyInfo)
     */

    for ( sLeftKeyInfo  = aLeftQuerySetAnalysis->mKeyInfo;
          sLeftKeyInfo != NULL;
          sLeftKeyInfo  = sLeftKeyInfo->mNext )
    {
        // Left query analysis에서 올라온 key info 를 각 key info의 target 별로 분할한다.
        IDE_TEST( devideKeyInfo( aStatement,
                                 sLeftKeyInfo,
                                 &sLeftDevidedKeyInfo )
                  != IDE_SUCCESS );

    }

    for ( sRightKeyInfo  = aRightQuerySetAnalysis->mKeyInfo;
          sRightKeyInfo != NULL;
          sRightKeyInfo  = sRightKeyInfo->mNext )
    {
        // Right query analysis 올라온 key info 를 각 key info의 target 별로 분할한다.
        IDE_TEST( devideKeyInfo( aStatement,
                                 sRightKeyInfo,
                                 &sRightDevidedKeyInfo )
                  != IDE_SUCCESS );
    }

    // Left와 right의 keyInfo를 병합한다.
    IDE_TEST( mergeKeyInfo4Set( aStatement,
                                sLeftDevidedKeyInfo,
                                sRightDevidedKeyInfo,
                                aIsSubKey,
                                &sKeyInfoTmp )
              != IDE_SUCCESS );

    /*
     * Key info의 left 끼리, 또 right 끼리 같은 key group으로부터 생성된 key info들은
     * 다시 묶어준다.
     */
    IDE_TEST( mergeSameKeyInfo( aStatement,
                                sKeyInfoTmp,
                                aKeyInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::devideKeyInfo( qcStatement * aStatement,
                           sdiKeyInfo  * aKeyInfo,
                           sdiKeyInfo ** aDevidedKeyInfo )
{
    UInt sKeyTargetCount = 0;

    if ( aKeyInfo->mKeyTargetCount > 0 )
    {
        /*
         * SELECT I1,I1,I1...
         *
         * 위와같이 동일한 shard key가 여러번 사용 되었을 때
         * 한 개 의 KeyInfo 에 keyTargetCount가 3개로 구성되어 올라오지만,
         *
         * SET을 처리하기 위해 동일한 key group을 다시 keyTarget별로 분할한다.
         *
         */
        for ( sKeyTargetCount = 0;
              sKeyTargetCount < aKeyInfo->mKeyTargetCount;
              sKeyTargetCount++ )
        {
            IDE_TEST( makeKeyInfo4SetTarget( aStatement,
                                             aKeyInfo,
                                             aKeyInfo->mKeyTarget[sKeyTargetCount],
                                             aDevidedKeyInfo )
                      != IDE_SUCCESS );

            (*aDevidedKeyInfo)->mOrgKeyInfo = aKeyInfo;
        }
    }
    else
    {
        /* Target에 없는 keyInfo에 대해서도 생성해준다. */
        IDE_TEST( makeKeyInfo4SetTarget( aStatement,
                                         aKeyInfo,
                                         ID_UINT_MAX,
                                         aDevidedKeyInfo )
                  != IDE_SUCCESS );

        (*aDevidedKeyInfo)->mOrgKeyInfo = aKeyInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::makeKeyInfo4SetTarget( qcStatement * aStatement,
                                   sdiKeyInfo  * aKeyInfo,
                                   UInt          aTargetPos,
                                   sdiKeyInfo ** aDevidedKeyInfo )
{
    sdiKeyInfo * sNewKeyInfo = NULL;

    IDE_DASSERT( aKeyInfo != NULL );

    // 새로운 key info 구조체 할당
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            sdiKeyInfo,
                            (void*) &sNewKeyInfo )
              != IDE_SUCCESS );
    
    // 새로운 key info의 정보 초기화
    SDI_INIT_KEY_INFO( sNewKeyInfo );

    // Set key target
    if ( aTargetPos != ID_UINT_MAX )
    {
        sNewKeyInfo->mKeyTargetCount = 1;

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(UShort),
                      (void**) & sNewKeyInfo->mKeyTarget )
                  != IDE_SUCCESS );

        sNewKeyInfo->mKeyTarget[0] = aTargetPos;
    }
    else
    {
        // Nothing to do.
    }

    // Set key
    sNewKeyInfo->mKeyCount = aKeyInfo->mKeyCount;

    if ( sNewKeyInfo->mKeyCount > 0 )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(sdiKeyTupleColumn) * sNewKeyInfo->mKeyCount,
                      (void**) & sNewKeyInfo->mKey )
                  != IDE_SUCCESS );

        idlOS::memcpy( (void*) sNewKeyInfo->mKey,
                       (void*) aKeyInfo->mKey,
                       ID_SIZEOF(sdiKeyTupleColumn) * sNewKeyInfo->mKeyCount );
    }
    else
    {
        // Nothing to do.
    }

    // Set value
    sNewKeyInfo->mValuePtrCount = aKeyInfo->mValuePtrCount;
    sNewKeyInfo->mValuePtrArray = aKeyInfo->mValuePtrArray;

    // Set shard info
    copyShardInfoFromShardInfo( &sNewKeyInfo->mShardInfo,
                                &aKeyInfo->mShardInfo );

    // 연결
    sNewKeyInfo->mNext = *aDevidedKeyInfo;
    *aDevidedKeyInfo = sNewKeyInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::mergeKeyInfo4Set( qcStatement * aStatement,
                              sdiKeyInfo  * aLeftKeyInfo,
                              sdiKeyInfo  * aRightKeyInfo,
                              idBool        aIsSubKey,
                              sdiKeyInfo ** aKeyInfo )
{
    sdiKeyInfo * sLeftKeyInfo  = NULL;
    sdiKeyInfo * sRightKeyInfo = NULL;

    idBool       sSameShardInfo = ID_FALSE;

    UInt         sKeyTarget = 0;

    for ( sLeftKeyInfo  = aLeftKeyInfo;
          sLeftKeyInfo != NULL;
          sLeftKeyInfo  = sLeftKeyInfo->mNext )
    {
        // Devided 된 keyInfo의 keyTargetCount는 1이거나 0 일 수 있다.
        if ( sLeftKeyInfo->mKeyTargetCount == 1 )
        {
            for ( sRightKeyInfo  = aRightKeyInfo;
                  sRightKeyInfo != NULL;
                  sRightKeyInfo  = sRightKeyInfo->mNext )
            {
                // Devided 된 keyInfo의 keyTargetCount는 1이거나 0 일 수 있다.
                if ( sRightKeyInfo->mKeyTargetCount == 1 )
                {
                    if ( sLeftKeyInfo->mKeyTarget[0] == sRightKeyInfo->mKeyTarget[0] )
                    {
                        IDE_TEST( isSameShardInfo( &sLeftKeyInfo->mShardInfo,
                                                   &sRightKeyInfo->mShardInfo,
                                                   aIsSubKey,
                                                   &sSameShardInfo )
                                  != IDE_SUCCESS );

                        if ( sSameShardInfo == ID_TRUE )
                        {
                            IDE_TEST( mergeKeyInfo( aStatement,
                                                    sLeftKeyInfo,
                                                    sRightKeyInfo,
                                                    aKeyInfo )
                                      != IDE_SUCCESS );
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
                else if ( sRightKeyInfo->mKeyTargetCount > 1 )
                {
                    IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else if ( sLeftKeyInfo->mKeyTargetCount > 1 )
        {
            IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
        }
        else
        {
            // Nothing to do.
        }
    }

    for ( sLeftKeyInfo  = aLeftKeyInfo;
          sLeftKeyInfo != NULL;
          sLeftKeyInfo  = sLeftKeyInfo->mNext )
    {
        if ( sLeftKeyInfo->mIsJoined == ID_FALSE )
        {
            if ( sLeftKeyInfo->mKeyTargetCount == 1 )
            {
                sKeyTarget = sLeftKeyInfo->mKeyTarget[0];
            }
            else
            {
                sKeyTarget = ID_UINT_MAX;
            }

            IDE_TEST( makeKeyInfo4SetTarget( aStatement,
                                             sLeftKeyInfo,
                                             sKeyTarget,
                                             aKeyInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    for ( sRightKeyInfo  = aRightKeyInfo;
          sRightKeyInfo != NULL;
          sRightKeyInfo  = sRightKeyInfo->mNext )
    {
        if ( sRightKeyInfo->mIsJoined == ID_FALSE )
        {
            if ( sRightKeyInfo->mKeyTargetCount == 1 )
            {
                sKeyTarget = sRightKeyInfo->mKeyTarget[0];
            }
            else
            {
                sKeyTarget = ID_UINT_MAX;
            }

            IDE_TEST( makeKeyInfo4SetTarget( aStatement,
                                             sRightKeyInfo,
                                             sKeyTarget,
                                             aKeyInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SHARD_KEY_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::mergeKeyInfo4Set",
                                "Invalid shard key info"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::mergeKeyInfo( qcStatement * aStatement,
                          sdiKeyInfo  * aLeftKeyInfo,
                          sdiKeyInfo  * aRightKeyInfo,
                          sdiKeyInfo ** aKeyInfo )
{
    sdiKeyInfo * sNewKeyInfo = NULL;

    IDE_DASSERT( aLeftKeyInfo  != NULL );
    IDE_DASSERT( aRightKeyInfo != NULL );

    // 새로운 key info 구조체 할당
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            sdiKeyInfo,
                            (void*) &sNewKeyInfo )
              != IDE_SUCCESS );

    // 새로운 key info의 정보 초기화
    SDI_INIT_KEY_INFO( sNewKeyInfo );

    // Set key target
    sNewKeyInfo->mKeyTargetCount = aLeftKeyInfo->mKeyTargetCount;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(UShort),
                                             (void**) & sNewKeyInfo->mKeyTarget )
              != IDE_SUCCESS );

    sNewKeyInfo->mKeyTarget[0] = aLeftKeyInfo->mKeyTarget[0];

    // Set key
    sNewKeyInfo->mKeyCount = ( aLeftKeyInfo->mKeyCount + aRightKeyInfo->mKeyCount );

    if ( sNewKeyInfo->mKeyCount > 0 )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(sdiKeyTupleColumn) * sNewKeyInfo->mKeyCount,
                      (void**) & sNewKeyInfo->mKey )
                  != IDE_SUCCESS );

        if ( aLeftKeyInfo->mKeyCount > 0 )
        {
            idlOS::memcpy( (void*) sNewKeyInfo->mKey,
                           (void*) aLeftKeyInfo->mKey,
                           ID_SIZEOF(sdiKeyTupleColumn) * aLeftKeyInfo->mKeyCount );
        }
        else
        {
            // Nothing to do.
        }

        if ( aRightKeyInfo->mKeyCount > 0 )
        {
            idlOS::memcpy( (void*) ( sNewKeyInfo->mKey + aLeftKeyInfo->mKeyCount ),
                           (void*) aRightKeyInfo->mKey,
                           ID_SIZEOF(sdiKeyTupleColumn) * aRightKeyInfo->mKeyCount );
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

    // Set value
    if ( ( aLeftKeyInfo->mValuePtrCount > 0 ) &&
         ( aRightKeyInfo->mValuePtrCount > 0 ) )
    {
        IDE_DASSERT( aLeftKeyInfo->mShardInfo.mKeyDataType == 
                     aRightKeyInfo->mShardInfo.mKeyDataType );

        IDE_TEST( allocAndMergeValues( aStatement,
                                       aLeftKeyInfo->mValuePtrArray,
                                       aLeftKeyInfo->mValuePtrCount,
                                       aRightKeyInfo->mValuePtrArray,
                                       aRightKeyInfo->mValuePtrCount,
                                       &(sNewKeyInfo->mValuePtrArray),
                                       &(sNewKeyInfo->mValuePtrCount) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // Set shard info
    copyShardInfoFromShardInfo( &sNewKeyInfo->mShardInfo,
                                &aLeftKeyInfo->mShardInfo );

    // Set left,right
    sNewKeyInfo->mLeft       = aLeftKeyInfo;
    sNewKeyInfo->mRight      = aRightKeyInfo;
    aLeftKeyInfo->mIsJoined  = ID_TRUE;
    aRightKeyInfo->mIsJoined = ID_TRUE;

    // 연결
    sNewKeyInfo->mNext = *aKeyInfo;
    *aKeyInfo = sNewKeyInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::appendKeyInfo( qcStatement * aStatement,
                           sdiKeyInfo  * aKeyInfo,
                           sdiKeyInfo  * aArgument )
{
    UShort             * sNewTarget;
    sdiKeyTupleColumn  * sNewKey;

    IDE_DASSERT( aKeyInfo  != NULL );
    IDE_DASSERT( aArgument != NULL );

    //-------------------------
    // append key target
    //-------------------------

    if ( aKeyInfo->mKeyTargetCount + aArgument->mKeyTargetCount > 0 )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(UShort) *
                      ( aKeyInfo->mKeyTargetCount + aArgument->mKeyTargetCount ),
                      (void**) & sNewTarget )
                  != IDE_SUCCESS );

        // Set key target
        if ( aKeyInfo->mKeyTargetCount > 0 )
        {
            idlOS::memcpy( (void*) sNewTarget,
                           (void*) aKeyInfo->mKeyTarget,
                           ID_SIZEOF(UShort) * aKeyInfo->mKeyTargetCount );
        }
        else
        {
            IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
        }

        if ( aArgument->mKeyTargetCount > 0 )
        {
            idlOS::memcpy( (void*) ( sNewTarget + aKeyInfo->mKeyTargetCount ),
                           (void*) aArgument->mKeyTarget,
                           ID_SIZEOF(UShort) * aArgument->mKeyTargetCount );
        }
        else
        {
            IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
        }
    }
    else
    {
        IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
    }

    aKeyInfo->mKeyTargetCount = aKeyInfo->mKeyTargetCount + aArgument->mKeyTargetCount;
    aKeyInfo->mKeyTarget      = sNewTarget;

    //-------------------------
    // append key
    //-------------------------

    if ( aKeyInfo->mKeyCount + aArgument->mKeyCount > 0 )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                      ID_SIZEOF(sdiKeyTupleColumn) *
                      ( aKeyInfo->mKeyCount + aArgument->mKeyCount ),
                      (void**) & sNewKey )
                  != IDE_SUCCESS );

        if ( aKeyInfo->mKeyCount > 0 )
        {
            idlOS::memcpy( (void*) sNewKey,
                           (void*) aKeyInfo->mKey,
                           ID_SIZEOF(sdiKeyTupleColumn) * aKeyInfo->mKeyCount );
        }
        else
        {
            IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
        }

        if ( aArgument->mKeyCount > 0 )
        {
            idlOS::memcpy( (void*) ( sNewKey + aKeyInfo->mKeyCount ),
                           (void*) aArgument->mKey,
                           ID_SIZEOF(sdiKeyTupleColumn) * aArgument->mKeyCount );
        }
        else
        {
            IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
        }
    }
    else
    {
        IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
    }

    aKeyInfo->mKeyCount = aKeyInfo->mKeyCount + aArgument->mKeyCount;
    aKeyInfo->mKey      = sNewKey;

    //-------------------------
    // append value
    //-------------------------

    if ( aKeyInfo->mValuePtrCount + aArgument->mValuePtrCount > 0 )
    {
        IDE_TEST( allocAndMergeValues( aStatement,
                                       aKeyInfo->mValuePtrArray,
                                       aKeyInfo->mValuePtrCount,
                                       aArgument->mValuePtrArray,
                                       aArgument->mValuePtrCount,
                                       &(aKeyInfo->mValuePtrArray),
                                       &(aKeyInfo->mValuePtrCount) )
                  != IDE_SUCCESS );
    }
    else
    {
        aKeyInfo->mValuePtrCount = 0;
        aKeyInfo->mValuePtrArray = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SHARD_KEY_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::appendKeyInfo",
                                "Invalid shard key info"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::mergeSameKeyInfo( qcStatement * aStatement,
                              sdiKeyInfo  * aKeyInfoTmp,
                              sdiKeyInfo ** aKeyInfo )
{
    sdiKeyInfo        * sKeyInfoTmp       = NULL;
    sdiKeyInfo        * sKeyInfo          = NULL;
    sdiKeyInfo        * sKeyInfoForAdding = NULL;
    sdiKeyInfo        * sSameKeyInfo      = NULL;
    UInt                sKeyTarget        = 0;

    for ( sKeyInfoTmp  = aKeyInfoTmp;
          sKeyInfoTmp != NULL;
          sKeyInfoTmp  = sKeyInfoTmp->mNext )
    {
        sKeyInfoForAdding = NULL;

        for ( sKeyInfo  = *aKeyInfo;
              sKeyInfo != NULL;
              sKeyInfo  = sKeyInfo->mNext )
        {
            if ( ( sKeyInfoTmp->mLeft  != NULL ) &&
                 ( sKeyInfoTmp->mRight != NULL ) &&
                 ( sKeyInfo->mLeft     != NULL ) &&
                 ( sKeyInfo->mRight    != NULL ) )
            {
                if ( ( sKeyInfoTmp->mLeft->mOrgKeyInfo  == sKeyInfo->mLeft->mOrgKeyInfo ) &&
                     ( sKeyInfoTmp->mRight->mOrgKeyInfo == sKeyInfo->mRight->mOrgKeyInfo ) )
                {
                    /*
                     * SELECT T1.I1 A, T1.I1 B, T2.I1 C, T2.I1 D, T2.I1 E
                     *   FROM T1, T2
                     * UNION
                     * SELECT T1.I1  , T1.I1  , T2.I1  , T2.I1  , T1.I1
                     *   FROM T1, T2;
                     *
                     *
                     *     left key info1             left key info2
                     *     {T1.I1, T1.I1}             {T2.I1, T2.I1, T2.I1}
                     *
                     *
                     *     right key info1            right key info2
                     *     {T1.I1, T1.I1, T1.I1}      {T2.I1, T2.I1}
                     *
                     *
                     *     keyInfo1 keyInfo2 keyInfo3 keyInfo4 keyInfo5
                     *         A        B        C       D        E
                     *       T1.I1    T1.I1    T2.I1   T2.I1    T2.I1
                     *       T1.I1    T1.I1    T2.I1   T2.I1    T1.I1
                     *
                     *       left1    left1    left2   left2    left2
                     *       right1   right1   right2  right2   right1
                     *
                     *
                     *      A,B,C,D는 같은 left끼리 또, right 끼리 같은 key group으로부터
                     *      생성되었다.
                     *      때문에, 이 A,B,C,D key info들은 다시 하나의 key로 merge 될 수 있다.
                     *
                     *      keyInfo1   keyInfo2
                     *
                     *      {A,B,C,D}    {E}
                     *
                     */
                    sKeyInfoForAdding = sKeyInfo;
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

        if ( sKeyInfoForAdding != NULL )
        {
            // Set key target, key, value
            IDE_TEST( appendKeyInfo( aStatement,
                                     sKeyInfoForAdding,
                                     sKeyInfoTmp )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( ( sKeyInfoTmp->mLeft != NULL ) &&
                 ( sKeyInfoTmp->mRight != NULL ) &&
                 ( sKeyInfoTmp->mKeyTargetCount == 1 ) )
            {
                IDE_TEST( makeKeyInfo4SetTarget( aStatement,
                                                 sKeyInfoTmp,
                                                 sKeyInfoTmp->mKeyTarget[0],
                                                 aKeyInfo )
                          != IDE_SUCCESS );
                (*aKeyInfo)->mLeft  = sKeyInfoTmp->mLeft;
                (*aKeyInfo)->mRight = sKeyInfoTmp->mRight;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    /*
     * SELECT I1 A,I1 B
     *   FROM T1
     *  UNION
     * SELECT I1  ,'C'
     *   FROM T1;
     *
     * 에서 B와 같이 홀로 쓰인 Shard key column 에 대해서
     * Set merge 된 shard key중 동일한 key가 있으면 합쳐주고,
     * 없으면, 별도의 key로 생성한다.
     *
     */
    for ( sKeyInfoTmp  = aKeyInfoTmp;
          sKeyInfoTmp != NULL;
          sKeyInfoTmp  = sKeyInfoTmp->mNext )
    {
        sKeyInfoForAdding = NULL;

        if ( ( sKeyInfoTmp->mLeft  == NULL ) &&
             ( sKeyInfoTmp->mRight == NULL ) )
        {
            IDE_TEST( getSameKey4Set( *aKeyInfo,
                                      sKeyInfoTmp,
                                      &sSameKeyInfo )
                      != IDE_SUCCESS );
                
            if ( sSameKeyInfo != NULL )
            {
                sKeyInfoForAdding = sSameKeyInfo;
            }
            else
            {
                // Nothing to do.
            }

            if ( sKeyInfoForAdding != NULL )
            {
                // Set key target, key, value
                IDE_TEST( appendKeyInfo( aStatement,
                                         sKeyInfoForAdding,
                                         sKeyInfoTmp )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( sKeyInfoTmp->mKeyTargetCount == 1 )
                {
                    sKeyTarget = sKeyInfoTmp->mKeyTarget[0];
                }
                else if ( sKeyInfoTmp->mKeyTargetCount == 0 )
                {
                    sKeyTarget = ID_UINT_MAX;
                }
                else
                {
                    IDE_RAISE(ERR_INVALID_SHARD_KEY_INFO);
                }

                IDE_TEST( makeKeyInfo4SetTarget( aStatement,
                                                 sKeyInfoTmp,
                                                 sKeyTarget,
                                                 aKeyInfo )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SHARD_KEY_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::mergeSameKeyInfo",
                                "Invalid shard key info"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getSameKey4Set( sdiKeyInfo  * aKeyInfo,
                            sdiKeyInfo  * aCurrKeyInfo,
                            sdiKeyInfo ** aRetKeyInfo )
{
    sdiKeyInfo * sKeyInfo = NULL;
    UShort       sKeyCount = 0;
    UShort       sCurrKeyCount = 0;

    *aRetKeyInfo = NULL;

    for ( sKeyInfo  = aKeyInfo;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        if ( ( sKeyInfo->mLeft  != NULL ) &&
             ( sKeyInfo->mRight != NULL ) )
        {
            for ( sKeyCount = 0;
                  sKeyCount < sKeyInfo->mKeyCount;
                  sKeyCount++ )
            {
                for ( sCurrKeyCount = 0;
                      sCurrKeyCount < aCurrKeyInfo->mKeyCount;
                      sCurrKeyCount++ )
                {   
                    if ( ( sKeyInfo->mKey[sKeyCount].mTupleId == aCurrKeyInfo->mKey[sCurrKeyCount].mTupleId ) &&
                         ( sKeyInfo->mKey[sKeyCount].mColumn  == aCurrKeyInfo->mKey[sCurrKeyCount].mColumn ) &&
                         ( sKeyInfo->mKey[sKeyCount].mIsNullPadding == ID_FALSE ) )
                    {
                        *aRetKeyInfo = sKeyInfo;
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;
}

IDE_RC sda::setAnalysisFlag4ParseTree( qmsParseTree * aParseTree,
                                       ULong          aSMN,
                                       idBool         aIsSubKey )
{
    sdiShardAnalysis * sShardAnalysis  = NULL;
    idBool             sSkipLimitCheck = ID_FALSE;
    idBool             sIsOneNodeQuery = ID_FALSE;
    UInt               sDistKeyCount   = 0;
    sdiObjectInfo    * sShardObjInfo   = NULL;

    if ( aIsSubKey == ID_FALSE )
    {
        sShardAnalysis = aParseTree->mShardAnalysis;
    }
    else
    {
        sShardAnalysis = aParseTree->mShardAnalysis->mNext;
    }

    IDE_TEST( isOneNodeSQL( NULL,
                            sShardAnalysis->mKeyInfo,
                            &sIsOneNodeQuery,
                            &sDistKeyCount )
              != IDE_SUCCESS );

    if ( sIsOneNodeQuery == ID_FALSE )
    {
        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( setAnalysisFlag4OrderBy( aParseTree->orderBy,
                                           &( sShardAnalysis->mAnalysisFlag ) )
                  != IDE_SUCCESS );

        // Checking limit
        if ( aParseTree->limit != NULL )
        {
            if ( aParseTree->limit->mIsShard == ID_TRUE )
            {
                /* TASK-7219
                 *  Shard Transform 가 Push 한 Limit 는 허용한다.
                 */
                sSkipLimitCheck = ID_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }

            if ( aParseTree->forUpdate != NULL )
            {
                if ( aParseTree->forUpdate->isMoveAndDelete == ID_TRUE )
                {
                    // select for move and delete는 limit을 허용한다.
                    sSkipLimitCheck = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }

            if ( sSkipLimitCheck == ID_FALSE )
            {
                sShardAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_LIMIT_EXISTS ] = ID_TRUE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }

        // Checking loop
        if ( aParseTree->loopNode != NULL )
        {
            sShardAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_LOOP_EXISTS ] = ID_TRUE;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }


    /* BUG-49088
     * TOP QUERY FLAG 에 SELECT FOR UPDATE 사용 여부를 설정한다.
     * Local table(non-shard table) records 에 대한 SELECT FOR UPDATE 의 경우에는 설정하지 않고,
     * Shard table 에 대한 SELECT FOR UPDATE 에 대해서만 설정한다.
     *
     *     아래 사항을 고려하여 최소한의 검사로 판단한다.
     *         1. View 가 simple view merging 되지 않고 남아 있는 경우, for update 가 동작하지 않는다. (BUGBUG)
     *         2. [ERR-31066 : DISTINCT, UNION, UNION ALL, INTERSECT and MINUS are not allowed in SELECT FOR UPDATE statements.]
     *         3. [ERR-31235 : JOIN is not allowed in SELECT FOR UPDATE statements.]
     */
    if ( aParseTree->forUpdate != NULL )
    {
        if ( aParseTree->querySet->SFWGH != NULL )
        {
            // TABLE 하나만 확인하면 된다.
            //   [ERR-31235 : JOIN is not allowed in SELECT FOR UPDATE statements.]
            //   View 가 simple view merging 되지 않고 남아 있는 경우, for update 가 동작하지 않는다. (BUGBUG)
            if ( aParseTree->querySet->SFWGH->from->tableRef->mShardObjInfo != NULL )
            {
                sdi::getShardObjInfoForSMN( aSMN,
                                            aParseTree->querySet->SFWGH->from->tableRef->mShardObjInfo,
                                            &( sShardObjInfo ) );

                if ( sShardObjInfo != NULL )
                {
                    sShardAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_FOR_UPDATE_EXISTS ] = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    // Shard query 여부를 세팅한다.
    IDE_TEST( isShardQuery( &( sShardAnalysis->mAnalysisFlag ),
                            &( sShardAnalysis->mIsShardQuery ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sda::setNonShardQueryReasonErr( sdiAnalysisFlag * aAnalysisFlag,
                                     UShort          * aErrIdx )
{
    // BUG-45899
    UShort sArrIdx = 0;

    for ( sArrIdx = 0; sArrIdx < SDI_NON_SHARD_QUERY_REASON_MAX; sArrIdx++ )
    {
        // Sub-key exists는 non-shard query의 이유로 보지 않는다.
        if ( aAnalysisFlag->mNonShardFlag[ sArrIdx ] == ID_TRUE )
        {
            *aErrIdx = sArrIdx;
            IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                      sdi::getNonShardQueryReasonArr( sArrIdx ),
                                      "" ) );
            break;
        }
    }

    if ( sArrIdx == SDI_NON_SHARD_QUERY_REASON_MAX )
    {
        *aErrIdx = SDI_UNKNOWN_REASON;

        // Avoid null error message
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  sdi::getNonShardQueryReasonArr( SDI_UNKNOWN_REASON ),
                                  "" ) );
    }
}

IDE_RC sda::analyzeInsert( qcStatement * aStatement,
                           ULong         aSMN )
{
/***********************************************************************
 *
 * Description : INSERT 구문의 분석
 *
 * Implementation :
 *    (1) CASE 1 : INSERT...VALUE(...(subquery)...)
 *        qmoSubquery::optimizeExpr()의 수행
 *    (2) CASE 2 : INSERT...SELECT...
 *        qmoSubquery::optimizeSubquery()의 수행
 *
 ***********************************************************************/

    qmmInsParseTree  * sInsParseTree = NULL;
    sdiShardAnalysis * sAnalysis = NULL;
    sdiObjectInfo    * sShardObjInfo = NULL;
   
    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    //------------------------------------------
    // Parse Tree 획득
    //------------------------------------------

    sInsParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;
    /* TASK-7219 Shard Transformer Refactoring */
    sShardObjInfo = sInsParseTree->tableRef->mShardObjInfo;

    /* BUG-45823 */
    increaseAnalyzeCount( aStatement );

    //------------------------------------------
    // 검사
    //------------------------------------------

    IDE_TEST( checkInsert( aStatement, sInsParseTree ) != IDE_SUCCESS );

    //------------------------------------------
    // 분석
    //------------------------------------------

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( analyzeInsertCore( aStatement,
                                 aSMN,
                                 sShardObjInfo,
                                 ID_FALSE )
              != IDE_SUCCESS );

    sAnalysis = sInsParseTree->mShardAnalysis;

    if ( sAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        IDE_TEST( analyzeInsertCore( aStatement,
                                     aSMN,
                                     sShardObjInfo,
                                     ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_STATEMENT)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeInsert",
                                "The statement is NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeUpdate( qcStatement * aStatement,
                           ULong         aSMN )
{
/***********************************************************************
 *
 * Description : UPDATE 구문의 분석
 *
 * Implementation :
 *
 ***********************************************************************/

    qmmUptParseTree  * sUptParseTree  = NULL;
    sdiShardAnalysis * sAnalysis = NULL;
    sdiObjectInfo    * sShardObjInfo = NULL;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    //------------------------------------------
    // Parse Tree 획득
    //------------------------------------------

    IDE_DASSERT( aStatement->myPlan != NULL );
    sUptParseTree = (qmmUptParseTree *)aStatement->myPlan->parseTree;

    /* BUG-45823 */
    increaseAnalyzeCount( aStatement );

    //------------------------------------------
    // PROJ-1718 Where절에 대하여 subquery predicate을 transform한다.
    //------------------------------------------

    IDE_TEST( qmo::doTransformSubqueries( aStatement,
                                          sUptParseTree->querySet->SFWGH->where )
              != IDE_SUCCESS );

    //------------------------------------------
    // 검사
    //------------------------------------------

    IDE_TEST( checkUpdate( aStatement, sUptParseTree, aSMN ) != IDE_SUCCESS );

    //------------------------------------------
    // 분석
    //------------------------------------------

    /* TASK-7219 Shard Transformer Refactoring */
    sShardObjInfo = sUptParseTree->updateTableRef->mShardObjInfo;

    IDE_TEST( analyzeUpdateCore( aStatement,
                                 aSMN,
                                 sShardObjInfo,
                                 ID_FALSE )
              != IDE_SUCCESS );

    sAnalysis = sUptParseTree->mShardAnalysis;

    if ( sAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        IDE_TEST( analyzeUpdateCore( aStatement,
                                     aSMN,
                                     sShardObjInfo,
                                     ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_STATEMENT)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeUpdate",
                                "The statement is NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeDelete( qcStatement * aStatement,
                           ULong         aSMN )
{
/***********************************************************************
 *
 * Description : DELETE 구문의 분석
 *
 * Implementation :
 *
 ***********************************************************************/

    qmmDelParseTree  * sDelParseTree = NULL;
    sdiShardAnalysis * sAnalysis = NULL;
    sdiObjectInfo    * sShardObjInfo = NULL;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    //------------------------------------------
    // Parse Tree 획득
    //------------------------------------------

    IDE_DASSERT( aStatement->myPlan != NULL );
    sDelParseTree = (qmmDelParseTree *) aStatement->myPlan->parseTree;

    /* BUG-45823 */
    increaseAnalyzeCount( aStatement );

    //------------------------------------------
    // 검사
    //------------------------------------------

    IDE_TEST( checkDelete( aStatement, sDelParseTree ) != IDE_SUCCESS );

    //------------------------------------------
    // 분석
    //------------------------------------------

    /* TASK-7219 Shard Transformer Refactoring */
    sShardObjInfo = sDelParseTree->deleteTableRef->mShardObjInfo;

    IDE_TEST( analyzeDeleteCore( aStatement,
                                 aSMN,
                                 sShardObjInfo,
                                 ID_FALSE )
              != IDE_SUCCESS );

    sAnalysis = sDelParseTree->mShardAnalysis;

    if ( sAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        IDE_TEST( analyzeDeleteCore( aStatement,
                                     aSMN,
                                     sShardObjInfo,
                                     ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_STATEMENT)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeDelete",
                                "The statement is NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeExecProc( qcStatement * aStatement,
                             ULong         aSMN )
{
/***********************************************************************
 *
 * Description : EXEC 구문의 분석
 *
 * Implementation :
 *
 ***********************************************************************/

    qsExecParseTree  * sExecParseTree = NULL;
    sdiShardAnalysis * sAnalysis = NULL;
    sdiObjectInfo * sShardObjInfo = NULL;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    //------------------------------------------
    // Parse Tree 획득
    //------------------------------------------

    sExecParseTree = (qsExecParseTree*) aStatement->myPlan->parseTree;
    /* TASK-7219 Shard Transformer Refactoring */
    sShardObjInfo  = sExecParseTree->mShardObjInfo;

    /* BUG-45823 */
    increaseAnalyzeCount( aStatement );

    //------------------------------------------
    // 검사
    //------------------------------------------

    IDE_TEST( checkExecProc( aStatement, sExecParseTree ) != IDE_SUCCESS );

    //------------------------------------------
    // 분석
    //------------------------------------------

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( analyzeExecProcCore( aStatement,
                                   aSMN,
                                   sShardObjInfo,
                                   ID_FALSE )
              != IDE_SUCCESS );

    sAnalysis = sExecParseTree->mShardAnalysis;

    if ( sAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        IDE_TEST( analyzeExecProcCore( aStatement,
                                       aSMN,
                                       sShardObjInfo,
                                       ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_STATEMENT)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeExecProc",
                                "The statement is NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkUpdate( qcStatement * aStatement, qmmUptParseTree * aUptParseTree, ULong aSMN )
{
/***********************************************************************
 *
 * Description :
 *      수행 가능한 SQL인지 판단한다.
 *
 *     *  수행 가능한 SQL의 조건
 *
 *         < UPDATE >
 *
 *             X. DML target 테이블은 non-shard table 이 아니어야 한다.
 *              - TASK-7219 Shard Transformer Refactoring / sda::checkAndGetShardObjInfo 처리
 *             1. Multiple Update 미지원
 *             2. DML target 테이블이 view 가 아니어야 한다. ( Updatable view 미지원 )
 *             3. key column은 update 될 수 없다.
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qcmColumn        * sUpdateColumn = NULL;
    UInt               sColOrder;
    qcuSqlSourceInfo   sqlInfo;
    sdiObjectInfo    * sShardObjInfo = NULL;

    /* TASK-7219 Shard Transformer Refactoring
     *  1. Multiple Update 미지원
     */
    if ( aUptParseTree->mTableList != NULL )
    {
        if ( aUptParseTree->mTableList->mNext == NULL )
        {
            sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );

            IDE_RAISE( ERR_MULTIPLE_UPDATE );
        }
        else
        {
            if ( aUptParseTree->mTableList->mNext->mTableRef == NULL )
            {
                sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );

                IDE_RAISE( ERR_MULTIPLE_UPDATE );
            }
            else
            {
                if ( QC_IS_NULL_NAME( aUptParseTree->mTableList->mNext->mTableRef->position ) == ID_FALSE )
                {
                    sqlInfo.setSourceInfo( aStatement, &( aUptParseTree->mTableList->mNext->mTableRef->position ) );

                    IDE_RAISE( ERR_MULTIPLE_UPDATE );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. DML target 테이블이 view 가 아니어야 한다. ( Updatable view 미지원 ) */
    if ( aUptParseTree->querySet->SFWGH->from->tableRef->view != NULL )
    {
        if ( QC_IS_NULL_NAME( aUptParseTree->querySet->SFWGH->from->tableRef->position ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, &( aUptParseTree->querySet->SFWGH->from->tableRef->position ) );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );
        }

        IDE_RAISE( ERR_VIEW_EXIST );
    }
    else
    {
        /* Nothing to do */
    }

    /* TASK-7219 Shard Transformer Refactoring
     *  3. Key column은 update 될 수 없다.
     */
    if ( aUptParseTree->updateTableRef->mShardObjInfo != NULL )
    {
        sdi::getShardObjInfoForSMN( aSMN,
                                    aUptParseTree->updateTableRef->mShardObjInfo,
                                    &( sShardObjInfo ) );

        IDE_TEST_RAISE( sShardObjInfo == NULL, ERR_INVALID_SHARD_OBJECT_INFO );

        for ( sUpdateColumn  = aUptParseTree->updateColumns;
              sUpdateColumn != NULL;
              sUpdateColumn  = sUpdateColumn->next )
        {
            sColOrder = sUpdateColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

            if ( ( sShardObjInfo->mKeyFlags[sColOrder] == 1 ) ||
                 ( sShardObjInfo->mKeyFlags[sColOrder] == 2 ) )
            {
                if ( ( QC_IS_NULL_NAME( sUpdateColumn->namePos ) == ID_FALSE )
                     &&
                     ( sUpdateColumn->namePos.stmtText == aUptParseTree->common.stmtPos.stmtText ) )
                {
                    sqlInfo.setSourceInfo( aStatement, &( sUpdateColumn->namePos ) );
                }
                else
                {
                    sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );
                }

                IDE_RAISE( ERR_SHARD_KEY_CAN_NOT_BE_UPDATED );
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VIEW_EXIST )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "DML for view",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_SHARD_KEY_CAN_NOT_BE_UPDATED )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Update for the shard key column",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MULTIPLE_UPDATE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Multiple Update",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_SHARD_OBJECT_INFO)
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                 "sda::checkUpdate",
                                 "Invalid shard object information" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkDelete( qcStatement * aStatement, qmmDelParseTree * aDelParseTree )
{
/***********************************************************************
 *
 * Description :
 *      수행 가능한 SQL인지 판단한다.
 *
 *     *  수행 가능한 SQL의 조건
 *
 *         < DELETE  >
 *             X. DML target 테이블은 non-shard table 이 아니어야 한다.
 *              - TASK-7219 Shard Transformer Refactoring / sda::checkAndGetShardObjInfo 처리
 *             1. Multiple Delete 미지원
 *             2. DML target 테이블이 view 가 아니어야 한다. ( Updatable view 미지원 )
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qcuSqlSourceInfo sqlInfo;

    /*  TASK-7219 Shard Transformer Refactoring
     *   1. Multiple Delete 미지원
     */
    if ( aDelParseTree->mTableList != NULL )
    {
        if ( aDelParseTree->mTableList->mNext == NULL )
        {
            sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );

            IDE_RAISE( ERR_MULTIPLE_DELETE );
        }
        else
        {
            if ( aDelParseTree->mTableList->mNext->mTableRef == NULL )
            {
                sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );

                IDE_RAISE( ERR_MULTIPLE_DELETE );
            }
            else
            {
                if ( QC_IS_NULL_NAME( aDelParseTree->mTableList->mNext->mTableRef->position ) == ID_FALSE )
                {
                    sqlInfo.setSourceInfo( aStatement, &( aDelParseTree->mTableList->mNext->mTableRef->position ) );

                    IDE_RAISE( ERR_MULTIPLE_DELETE );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. DML target 테이블이 view 가 아니어야 한다. ( Updatable view 미지원 )
     *  - IDE_TEST_RAISE( aDelParseTree->deleteTableRef->view != NULL, ERR_VIEW_EXIST );
     */
    if ( aDelParseTree->querySet->SFWGH->from->tableRef->view != NULL )
    {
        if ( QC_IS_NULL_NAME( aDelParseTree->querySet->SFWGH->from->tableRef->position ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, &( aDelParseTree->querySet->SFWGH->from->tableRef->position ) );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );
        }

        IDE_RAISE( ERR_VIEW_EXIST );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VIEW_EXIST )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "DML for view",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MULTIPLE_DELETE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Multiple Delete",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkInsert( qcStatement * aStatement, qmmInsParseTree * aInsParseTree )
{
/***********************************************************************
 *
 * Description :
 *      수행 가능한 SQL인지 판단한다.
 *
 *     *  수행 가능한 SQL의 조건
 *
 *         < INSERT  >
 *
 *             1.1. Multi-tables insert 미지원
 *             1.2. 
 *             2.   Insert Values와 Insert Select, Insert Default 지원
 *                   - 미지원 : Multi insert select
 *             X.   Multi-rows insert 미지원
 *                   -Transform 으로 지원중
 *             X.   DML target 테이블은 non-shard table 이 아니어야 한다.
 *                   - TASK-7219 Shard Transformer Refactoring / sda::checkAndGetShardObjInfo 처리
 *             3.   DML target 테이블이 view 가 아니어야 한다. ( Updatable view 미지원 )
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qcuSqlSourceInfo sqlInfo;

    // BUG-42764
    IDE_TEST_RAISE( aInsParseTree == NULL, ERR_NULL_PARSE_TREE );
    IDE_TEST_RAISE( aInsParseTree->rows == NULL, ERR_NULL_ROWS );

    /* TASK-7219 Shard Transformer Refactoring
     *  1.1. Multi-tables insert 미지원
     */
    if ( aInsParseTree->next != NULL )
    {
        if ( aInsParseTree->next->tableRef != NULL )
        {
            sqlInfo.setSourceInfo( aStatement, &( aInsParseTree->next->tableRef->position ) );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &( aInsParseTree->common.stmtPos ) );
        }

        IDE_RAISE( ERR_UNSUPPORTED_INSERT_TYPE );
    }
    else
    {
        /* Nothing to do */
    }

    /* 1.2. 조건 검사 추가로 오작동 방지 */
    if ( ( aInsParseTree->flag & QMM_MULTI_INSERT_MASK ) == QMM_MULTI_INSERT_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &( aInsParseTree->common.stmtPos ) );

        IDE_RAISE( ERR_UNSUPPORTED_INSERT_TYPE );
    }
    else
    {
        /* Nothing to do */
    }

    /* 2.   Insert Values와 Insert Select, Insert Default 지원 */
    if ( ( aInsParseTree->common.parse != qmv::parseInsertValues )
         &&
         ( aInsParseTree->common.parse != qmv::parseInsertSelect )
         &&
         ( aInsParseTree->common.parse != qmv::parseInsertAllDefault ) )
    {
        sqlInfo.setSourceInfo( aStatement, &( aInsParseTree->common.stmtPos ) );

        IDE_RAISE( ERR_UNSUPPORTED_INSERT_TYPE );
    }
    else
    {
        /* Nothing to do */
    }

    /* 3.   DML target 테이블이 view 가 아니어야 한다. */
    if ( aInsParseTree->tableRef->view != NULL )
    {
        if ( QC_IS_NULL_NAME( aInsParseTree->tableRef->position  ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement, &( aInsParseTree->tableRef->position ) );
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement, &( aInsParseTree->common.stmtPos ) );
        }

        IDE_RAISE( ERR_VIEW_EXIST );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSUPPORTED_INSERT_TYPE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Multiple INSERT or INSERT DEFAULT clause",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_VIEW_EXIST )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "DML for view",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NULL_PARSE_TREE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::checkInsert",
                                "The insParseTree is NULL."));
    }
    IDE_EXCEPTION(ERR_NULL_ROWS)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::checkInsert",
                                "The insert rows are NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkExecProc( qcStatement * /* aStatement */, qsExecParseTree * aExecParseTree )
{
/***********************************************************************
 *
 * Description :
 *      수행 가능한 SQL인지 판단한다.
 *
 *     *  수행 가능한 SQL의 조건
 *
 *         < EXEC >
 *
 *             X. Shard procedure 여야 한다.
 *              - TASK-7219 Shard Transformer Refactoring / sda::checkAndGetShardObjInfo 처리
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    // BUG-42764
    IDE_TEST_RAISE( aExecParseTree == NULL, ERR_NULL_PARSE_TREE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_PARSE_TREE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::checkExecProc",
                                "The procParseTree is NULL."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeUpdateCore( qcStatement       * aStatement,
                               ULong               aSMN,
                               sdiObjectInfo     * aShardObjInfo,
                               idBool              aIsSubKey )
{
    qmmUptParseTree  * sParseTree = (qmmUptParseTree*)aStatement->myPlan->parseTree;

    /* TASK-7219 Shard Transformer Refactoring */
    sdiShardAnalysis * sAnalysis       = NULL;
    qtcNode          * sShardPredicate = NULL;

    IDE_TEST( allocAndSetAnalysis( aStatement,
                                   aStatement->myPlan->parseTree,
                                   NULL,
                                   aIsSubKey,
                                   &( sAnalysis ) )
              != IDE_SUCCESS );

    IDE_TEST( getShardPredicate( aStatement,
                                 sParseTree->querySet,
                                 ID_FALSE,
                                 aIsSubKey,
                                 &( sShardPredicate ) )
              != IDE_SUCCESS );

    //------------------------------------------
    // Set common information (add keyInfo & tableInfo)
    //------------------------------------------
    IDE_TEST( setDMLCommonAnalysis( aStatement,
                                    aSMN,
                                    aShardObjInfo,
                                    sParseTree->updateTableRef,
                                    sAnalysis,
                                    aIsSubKey )
              != IDE_SUCCESS );

    //------------------------------------------
    // Analyze predicate
    //------------------------------------------
    if ( sdi::getSplitType( sAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
    {
        IDE_TEST( makeValueInfo( aStatement,
                                 sShardPredicate,
                                 sAnalysis->mKeyInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Analyze sub-query
    //------------------------------------------
    IDE_TEST( subqueryAnalysis4DML( aStatement,
                                    aSMN,
                                    sAnalysis,
                                    aIsSubKey ) /* aIsSubKey */
              != IDE_SUCCESS );

    //------------------------------------------
    // Check Limit
    //------------------------------------------
    // 단일 노드 수행으로 확정되지 않은 상태에서
    // LIMIT을 수행하게 되면 분산 정의에 맞지 않아
    // Non-shard query 이므로 flag checking

    // BUG-47196
    if ( sParseTree->limit != NULL )
    {
        if ( sdi::getSplitType( sAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
        {
            if ( sAnalysis->mKeyInfo->mValuePtrCount != 1 )
            {
                sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_LIMIT_EXISTS ] = ID_TRUE;
                IDE_TEST( isShardQuery( &( sAnalysis->mAnalysisFlag ),
                                        &( sAnalysis->mIsShardQuery ) )
                          != IDE_SUCCESS );
            }
        }
        else if ( sAnalysis->mShardInfo.mSplitMethod == SDI_SPLIT_CLONE )
        {
            sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_LIMIT_EXISTS ] = ID_TRUE;
            IDE_TEST( isShardQuery( &( sAnalysis->mAnalysisFlag ),
                                    &( sAnalysis->mIsShardQuery ) )
                      != IDE_SUCCESS );
        }
        else if ( sAnalysis->mShardInfo.mSplitMethod == SDI_SPLIT_SOLO )
        {
            // Nothing to do for solo.
        }
        else
        {
            IDE_RAISE( ERR_UNSUPPORTED_SPLIT_METHOD );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSUPPORTED_SPLIT_METHOD )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::analyzeUpdateCore",
                                  "Unsupported split method." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeDeleteCore( qcStatement       * aStatement,
                               ULong               aSMN,
                               sdiObjectInfo     * aShardObjInfo,
                               idBool              aIsSubKey )
{
    qmmDelParseTree * sParseTree = (qmmDelParseTree*)aStatement->myPlan->parseTree;

    /* TASK-7219 Shard Transformer Refactoring */
    sdiShardAnalysis * sAnalysis       = NULL;
    qtcNode          * sShardPredicate = NULL;

    IDE_TEST( allocAndSetAnalysis( aStatement,
                                   aStatement->myPlan->parseTree,
                                   NULL,
                                   aIsSubKey,
                                   &( sAnalysis ) )
              != IDE_SUCCESS );

    IDE_TEST( getShardPredicate( aStatement,
                                 sParseTree->querySet,
                                 ID_FALSE,
                                 aIsSubKey,
                                 &( sShardPredicate ) )
              != IDE_SUCCESS );

    //------------------------------------------
    // Set common information (add keyInfo & tableInfo)
    //------------------------------------------
    IDE_TEST( setDMLCommonAnalysis( aStatement,
                                    aSMN,
                                    aShardObjInfo,
                                    sParseTree->deleteTableRef,
                                    sAnalysis,
                                    aIsSubKey )
              != IDE_SUCCESS );

    //------------------------------------------
    // Analyze predicate
    //------------------------------------------
    if ( sdi::getSplitType( sAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
    {
        IDE_TEST( makeValueInfo( aStatement,
                                 sShardPredicate,
                                 sAnalysis->mKeyInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Analyze sub-query
    //------------------------------------------
    IDE_TEST ( subqueryAnalysis4DML( aStatement,
                                     aSMN,
                                     sAnalysis,
                                     aIsSubKey )
               != IDE_SUCCESS );

    //------------------------------------------
    // Check Limit
    //------------------------------------------
    // 단일 노드 수행으로 확정되지 않은 상태에서
    // LIMIT을 수행하게 되면 분산 정의에 맞지 않아
    // Non-shard query이므로 flag checking

    // BUG-47196
    if ( sParseTree->limit != NULL )
    {
        if ( sdi::getSplitType( sAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
        {
            if ( sAnalysis->mKeyInfo->mValuePtrCount != 1 )
            {
                sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_LIMIT_EXISTS ] = ID_TRUE;
                IDE_TEST( isShardQuery( &( sAnalysis->mAnalysisFlag ),
                                        &( sAnalysis->mIsShardQuery ) )
                          != IDE_SUCCESS );
            }
        }
        else if ( sAnalysis->mShardInfo.mSplitMethod == SDI_SPLIT_CLONE )
        {
            sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NODE_TO_NODE_LIMIT_EXISTS ] = ID_TRUE;
            IDE_TEST( isShardQuery( &( sAnalysis->mAnalysisFlag ),
                                    &( sAnalysis->mIsShardQuery ) )
                      != IDE_SUCCESS );
        }
        else if ( sAnalysis->mShardInfo.mSplitMethod == SDI_SPLIT_SOLO )
        {
            // Nothing to do for solo.
        }
        else
        {
            IDE_RAISE( ERR_UNSUPPORTED_SPLIT_METHOD );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSUPPORTED_SPLIT_METHOD )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::analyzeDeleteCore",
                                  "Unsupported split method." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeInsertCore( qcStatement       * aStatement,
                               ULong               aSMN,
                               sdiObjectInfo     * aShardObjInfo,
                               idBool              aIsSubKey )
{
    qmmInsParseTree * sParseTree = (qmmInsParseTree*)aStatement->myPlan->parseTree;

    /* TASK-7219 Shard Transformer Refactoring */
    sdiShardAnalysis * sAnalysis = NULL;

    IDE_TEST( allocAndSetAnalysis( aStatement,
                                   aStatement->myPlan->parseTree,
                                   NULL,
                                   aIsSubKey,
                                   &( sAnalysis ) )
              != IDE_SUCCESS );

    //------------------------------------------
    // Set common information (add keyInfo & tableInfo)
    //------------------------------------------
    IDE_TEST( setDMLCommonAnalysis( aStatement,
                                    aSMN,
                                    aShardObjInfo,
                                    sParseTree->tableRef,
                                    sAnalysis,
                                    aIsSubKey )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring
     *  Transform 으로 지원하고 있으며,
     *   Non-Shard 판단을 위해서 Reason 를 설정한다.
     */
    if ( sParseTree->rows->next != NULL )
    {
        sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_UNKNOWN_REASON ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->common.parse == qmv::parseInsertValues )
    {
        IDE_TEST( setInsertValueAnalysis( aStatement,
                                          aSMN,
                                          sParseTree,
                                          sAnalysis,
                                          aIsSubKey )
                  != IDE_SUCCESS );
    }
    else if ( sParseTree->common.parse == qmv::parseInsertSelect )
    {
        IDE_TEST( setInsertSelectAnalysis( aStatement,
                                           aSMN,
                                           sParseTree,
                                           sAnalysis,
                                           aIsSubKey )
                  != IDE_SUCCESS );
    }
    else if ( sParseTree->common.parse == qmv::parseInsertAllDefault )
    {
        sAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_UNSPECIFIED_SHARD_KEY_VALUE ] = ID_TRUE;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_INSERTION_STMT_TYPE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_INSERTION_STMT_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::analyzeInsertCore",
                                "Invalid insertion statement type."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setInsertValueAnalysis( qcStatement      * aStatement,
                                    UShort             aSMN,
                                    qmmInsParseTree  * aParseTree,
                                    sdiShardAnalysis * aAnalysis,
                                    idBool             aIsSubKey )
{
    IDE_DASSERT( aParseTree->common.parse == qmv::parseInsertValues );

    //------------------------------------------
    // Analyze shard key value
    //------------------------------------------
    if ( sdi::getSplitType( aAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
    {
        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( getKeyValueID4InsertValue( aStatement,
                                             aSMN,
                                             aParseTree->tableRef,
                                             aParseTree->insertColumns,
                                             aParseTree->rows,   // BUG-42764
                                             aAnalysis->mKeyInfo,
                                             &aAnalysis->mIsShardQuery,
                                             &( aAnalysis->mAnalysisFlag ),
                                             aIsSubKey )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Analyze sub-query
    //------------------------------------------
    IDE_TEST( subqueryAnalysis4DML( aStatement,
                                    aSMN,
                                    aAnalysis,
                                    aIsSubKey )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setInsertSelectAnalysis( qcStatement      * aStatement,
                                     UShort             aSMN,
                                     qmmInsParseTree  * aParseTree,
                                     sdiShardAnalysis * aAnalysis,
                                     idBool             aIsSubKey )
{
    sdaSubqueryAnalysis   sSubqueryAnalysis;
    sdiShardAnalysis    * sSubqueryMainKeyAnalysis = NULL;
    idBool                sIsSameShardInfo = ID_FALSE;
    qcmColumn           * sInsertColumn = NULL;
    UInt                  sColOrderOnTable;
    UShort                sColOrderOnInsert;
    sdiKeyInfo          * sKeyInfoRef = NULL;
    idBool                sKeyAppears = ID_FALSE;
    idBool                sOutRefReasonBack = ID_FALSE;

    qmsTarget           * sSelectTarget = NULL;
    UInt                  sColOrder = 0;
    idBool                sPartialTransformable = ID_FALSE;

    mtcColumn           * sMtcColumn = NULL;
    const mtvTable      * sTable;
    ULong                 sCost = ID_ULONG(0);

    sdiObjectInfo       * sShardObjInfo = NULL;

    IDE_DASSERT( aParseTree->common.parse == qmv::parseInsertSelect );

    //------------------------------------------
    // Analyze sub-query
    //------------------------------------------

    IDE_TEST_RAISE( analyzeSelectCore( aParseTree->select,
                                       aSMN,
                                       aAnalysis->mKeyInfo,
                                       aIsSubKey )
                    != IDE_SUCCESS, ERR_PARTIAL_SELECT_ANALYSIS_FAIL );

    sSubqueryMainKeyAnalysis = ((qmsParseTree*)aParseTree->select->myPlan->parseTree)->mShardAnalysis;

    if ( aIsSubKey == ID_FALSE )
    {
        sSubqueryAnalysis.mShardAnalysis = sSubqueryMainKeyAnalysis;
    }
    else
    {
        sSubqueryAnalysis.mShardAnalysis = sSubqueryMainKeyAnalysis->mNext;
    }

    sSubqueryAnalysis.mNext = NULL;

    sOutRefReasonBack = aAnalysis->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ];

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( mergeAnalysisFlag( &( sSubqueryAnalysis.mShardAnalysis->mAnalysisFlag ),
                                 &( aAnalysis->mAnalysisFlag ),
                                 SDI_ANALYSIS_FLAG_NON_TRUE |
                                 SDI_ANALYSIS_FLAG_TOP_TRUE )
              != IDE_SUCCESS );

    aAnalysis->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ] = sOutRefReasonBack;

    if ( aAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] ==
         sSubqueryAnalysis.mShardAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] )
    {
        IDE_TEST( isSameShardInfo( &aAnalysis->mShardInfo,
                                   &sSubqueryAnalysis.mShardAnalysis->mShardInfo,
                                   aIsSubKey,
                                   &sIsSameShardInfo )
                  != IDE_SUCCESS );

        if ( sIsSameShardInfo == ID_TRUE )
        {
            if ( sdi::getSplitType( aAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
            {
                sdi::getShardObjInfoForSMN( aSMN,
                                            aParseTree->tableRef->mShardObjInfo,
                                            &( sShardObjInfo ) );

                IDE_TEST_RAISE( sShardObjInfo == NULL, ERR_NULL_SHARD_OBJECT_INFO );

                //------------------------------------------
                // Analyze shard key link
                //------------------------------------------
                for ( sInsertColumn  = aParseTree->insertColumns, sColOrderOnInsert = 0;
                      sInsertColumn != NULL;
                      sInsertColumn  = sInsertColumn->next, sColOrderOnInsert++ )
                {
                    sColOrderOnTable = sInsertColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

                    if ( ( ( aIsSubKey == ID_FALSE ) && ( sShardObjInfo->mKeyFlags[sColOrderOnTable] == 1 ) ) ||
                         ( ( aIsSubKey == ID_TRUE ) && ( sShardObjInfo->mKeyFlags[sColOrderOnTable] == 2 ) ) )
                    {
                        IDE_TEST( resetSubqueryAnalysisReasonForTargetLink( sSubqueryMainKeyAnalysis,
                                                                            &aAnalysis->mShardInfo,
                                                                            sColOrderOnInsert,
                                                                            aIsSubKey,
                                                                            &sKeyInfoRef ) 
                                  != IDE_SUCCESS );

                        if ( sSubqueryAnalysis.mShardAnalysis->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_OUT_REF_NOT_EXISTS ] == ID_TRUE )
                        {
                            aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                        }
                        else
                        {
                            //------------------------------------------
                            // Make shard key value
                            //------------------------------------------
                            aAnalysis->mKeyInfo->mValuePtrCount = sKeyInfoRef->mValuePtrCount;
                            aAnalysis->mKeyInfo->mValuePtrArray = sKeyInfoRef->mValuePtrArray;
                        }

                        sKeyAppears = ID_TRUE;
                        break;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }

                if ( sKeyAppears == ID_FALSE )
                {
                    aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
                    aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_UNSPECIFIED_SHARD_KEY_VALUE ] = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
            aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
        }
    }
    else
    {
        aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
        aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_NON_SHARD_SUBQUERY_EXISTS ] = ID_TRUE;
    }

    //------------------------------------------
    // Analyze partial non-shard DML executable
    //------------------------------------------

    if ( aParseTree->tableRef->mShardObjInfo != NULL )
    {
        for ( sSelectTarget  = ((qmsParseTree*)aParseTree->select->myPlan->parseTree)->querySet->target,
                  sInsertColumn = aParseTree->insertColumns;
              sSelectTarget != NULL;
              sSelectTarget  = sSelectTarget->next,
                  sInsertColumn = sInsertColumn->next )
        {
            sColOrder = sInsertColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

            if ( ( ( aIsSubKey == ID_FALSE ) &&
                   ( aParseTree->tableRef->mShardObjInfo->mKeyFlags[sColOrder] == 1 ) ) ||
                 ( ( aIsSubKey == ID_TRUE ) &&
                   ( aParseTree->tableRef->mShardObjInfo->mKeyFlags[sColOrder] == 2 ) ) )
            {
                sPartialTransformable = ID_FALSE;

                /* insert... select 에서 insert target 테이블의 거주노드들이
                   select를 중복으로 수행한 후, 자신의 노드에 해당하는 데이터만을 insert할 것이기 때문에
                   select의 중복적인 수행으로 인해 source data가 multiple 될 수 있는 상황을 방지해야 한다.
                   예를들어 rand(), sequence같은 경우가 그럴수 있다.  */
                if ( ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_PROC_FUNCTION_MASK )
                       == QTC_NODE_PROC_FUNCTION_TRUE )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_SEQUENCE_MASK )
                       == QTC_NODE_SEQUENCE_EXIST )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_VAR_FUNCTION_MASK )
                       == QTC_NODE_VAR_FUNCTION_EXIST )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_PRIOR_MASK )
                       == QTC_NODE_PRIOR_EXIST )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_LEVEL_MASK )
                       == QTC_NODE_LEVEL_EXIST )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_ROWNUM_MASK )
                       == QTC_NODE_ROWNUM_EXIST )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_ISLEAF_MASK )
                       == QTC_NODE_ISLEAF_EXIST )
                     ||
                     ( ( sSelectTarget->targetColumn->lflag & QTC_NODE_COLUMN_RID_MASK )
                       == QTC_NODE_COLUMN_RID_EXIST ) )
                {
                    sPartialTransformable = ID_FALSE;
                }
                else
                {
                    sPartialTransformable = ID_TRUE;
                }

                /* insert target table의 shard key column 과
                   select의 해당 위치의 target이 conversion가능한 mtdType이어야 한다.
                */
                if ( sPartialTransformable == ID_TRUE )
                {
                    sMtcColumn = QTC_STMT_COLUMN( aStatement, sSelectTarget->targetColumn );

                    IDE_TEST( mtv::tableByNo( &sTable,
                                              sInsertColumn->basicInfo->module->no,
                                              sMtcColumn->module->no )
                              != IDE_SUCCESS );

                    sCost += sTable->cost;

                    if ( sCost >=  MTV_COST_INFINITE )
                    {
                        sPartialTransformable = ID_FALSE;
                    }
                    else
                    {
                        sPartialTransformable = ID_TRUE;
                    }
                }
                else
                {
                    /* already sPartialTransformable = ID_FALSE; */
                    /* Nothing to do. */
                }

                if ( sPartialTransformable == ID_FALSE )
                {
                    aAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_UNSPECIFIED_SHARD_KEY_VALUE ] = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PARTIAL_SELECT_ANALYSIS_FAIL )
    {
        if ( ( aStatement->mShardPrintInfo.mNonShardQueryReason == SDI_NON_SHARD_QUERY_REASON_MAX ) &&
             ( aParseTree->select != NULL ) )
        {
            aStatement->mShardPrintInfo.mNonShardQueryReason = aParseTree->select->mShardPrintInfo.mNonShardQueryReason;
        }
        else
        {
            /* Nothing to do. */
        }
    }
    IDE_EXCEPTION(ERR_NULL_SHARD_OBJECT_INFO)
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                 "sda::setInsertSelectAnalysis",
                                 "Invalid shard object information" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeExecProcCore( qcStatement       * aStatement,
                                 ULong               aSMN,
                                 sdiObjectInfo     * aShardObjInfo,
                                 idBool              aIsSubKey )
{
    qsExecParseTree     * sExecParseTree    = (qsExecParseTree*)aStatement->myPlan->parseTree;
    qsProcParseTree     * sProcPlanTree     = NULL;
    qsxProcPlanList     * sFoundProcPlan    = NULL;

    /* TASK-7219 Shard Transformer Refactoring */
    sdiShardAnalysis * sAnalysis = NULL;

    IDE_TEST( allocAndSetAnalysis( aStatement,
                                   aStatement->myPlan->parseTree,
                                   NULL,
                                   aIsSubKey,
                                   &( sAnalysis ) )
              != IDE_SUCCESS );

    //------------------------------------------
    // get procPlanTree
    //------------------------------------------
    IDE_TEST( qsxRelatedProc::findPlanTree( aStatement,
                                            sExecParseTree->procOID,
                                            &sFoundProcPlan )
              != IDE_SUCCESS );

    sProcPlanTree = (qsProcParseTree *)(sFoundProcPlan->planTree);

    //------------------------------------------
    // Set common information (add keyInfo & tableInfo)
    //------------------------------------------
    IDE_TEST( setDMLCommonAnalysis( aStatement,
                                    aSMN,
                                    aShardObjInfo,
                                    NULL,
                                    sAnalysis,
                                    aIsSubKey )
              != IDE_SUCCESS );

    //------------------------------------------
    // Analyze shard key value
    //------------------------------------------
    if ( sdi::getSplitType( sAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
    {
        IDE_TEST( getKeyValueID4ProcArguments( aStatement,
                                               aSMN,
                                               sProcPlanTree,
                                               sExecParseTree,
                                               sAnalysis->mKeyInfo,
                                               &( sAnalysis->mAnalysisFlag ),
                                               aIsSubKey )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setDMLCommonAnalysis( qcStatement      * aStatement,
                                  ULong              aSMN,
                                  sdiObjectInfo    * aShardObjInfo,
                                  qmsTableRef      * aTableRef,
                                  sdiShardAnalysis * aAnalysis,
                                  idBool             aIsSubKey )
{
    sdiKeyInfo        * sKeyInfo = NULL;
    UInt                sColumnOrder = 0;
    sdiKeyTupleColumn * sKeyColumn = NULL;

    UInt                sCandidateKeyCnt = 0;
    UShort              sTableId = 0;

    /* TASK-7219 Shard Transformer Refactoring */
    sdiObjectInfo     * sShardObjInfo = NULL;

    /* TASK-7219 Shard Transformer Refactoring */
    //---------------------------------------
    // Shard object의 획득
    //---------------------------------------
    IDE_TEST( checkAndGetShardObjInfo( aSMN,
                                       &( aAnalysis->mAnalysisFlag ),
                                       aShardObjInfo,
                                       ID_FALSE, /* aIsSelect */
                                       aIsSubKey,
                                       &( sShardObjInfo ) )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sShardObjInfo == NULL, NORMAL_EXIT );

    copyShardInfoFromObjectInfo( &( aAnalysis->mShardInfo ),
                                 sShardObjInfo,
                                 aIsSubKey );

    //------------------------------------------
    // Make shard key information
    //------------------------------------------
    if ( sdi::getSplitType( aAnalysis->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
    {
        //------------------------------------------
        // Set variables
        //------------------------------------------
        if ( aTableRef != NULL )
        {
            // tableRef가 있으면
            // 해당 table의 column 들 중에서 shard key를 찾고, 해당 column의 정보로 keyInfo를 설정한다.
            sCandidateKeyCnt = aTableRef->tableInfo->columnCount;
            sTableId = aTableRef->table;
        }
        else
        {
            // tableRef가 없으면( exec proc )
            // dummy keyInfo를 생성하고, 초기 값(dummy value)으로 설정한다.
            sCandidateKeyCnt = 1;
            sTableId = 0;
        }

        for ( sColumnOrder = 0;
              sColumnOrder < sCandidateKeyCnt;
              sColumnOrder++ )
        {
            if ( ( ( sShardObjInfo->mKeyFlags[sColumnOrder] == 1 ) && ( aIsSubKey == ID_FALSE ) )
                 ||
                 ( ( sShardObjInfo->mKeyFlags[sColumnOrder] == 2 ) && ( aIsSubKey == ID_TRUE ) )
                 ||
                 ( aTableRef == NULL ) )
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiKeyInfo),
                                                         (void**) & sKeyInfo )
                          != IDE_SUCCESS );

                SDI_INIT_KEY_INFO( sKeyInfo );

                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                        sdiKeyTupleColumn,
                                        (void*) &sKeyColumn )
                          != IDE_SUCCESS );

                sKeyColumn->mTupleId         = sTableId;
                sKeyColumn->mColumn          = ( aTableRef != NULL )?sColumnOrder:0;
                sKeyColumn->mIsNullPadding   = ID_FALSE;
                sKeyColumn->mIsAntiJoinInner = ID_FALSE;

                sKeyInfo->mKeyCount = 1;
                sKeyInfo->mKey = sKeyColumn;

                // Set shard info for shard key
                copyShardInfoFromShardInfo( &( sKeyInfo->mShardInfo ),
                                            &( aAnalysis->mShardInfo ) );

                aAnalysis->mKeyInfo = sKeyInfo;

                if ( ( sShardObjInfo->mTableInfo.mSubKeyExists == ID_FALSE )
                     ||
                     ( aIsSubKey == ID_TRUE ) )
                {
                    break;
                }
                else
                {
                    /* Nothing to do. */
                }

            }
            else
            {
                if ( ( sShardObjInfo->mKeyFlags[sColumnOrder] == 2 ) && ( aIsSubKey == ID_FALSE ) )
                {
                    aAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] = ID_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
        }
    }
    else
    {
        // CLONE and SOLO split 은 shard key가 없다.
        // Nothing to do.
    }

    //------------------------------------------
    // Make shard table information
    //------------------------------------------
    //------------------------------------------
    // PROJ-2685 online rebuild
    // shard table을 analysis에 등록
    //------------------------------------------
    IDE_TEST( addTableInfo( aStatement,
                            aAnalysis,
                            &( sShardObjInfo->mTableInfo ) )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::subqueryAnalysis4DML( qcStatement      * aStatement,
                                  UShort             aSMN,
                                  sdiShardAnalysis * aAnalysis,
                                  idBool             aIsSubKey )
{
    qciStmtType               sStmtType = aStatement->myPlan->parseTree->stmtKind;
    qtcNode                 * sNode = NULL;
    sdaSubqueryAnalysis     * sSubqueryAnalysis = NULL;
    sdiAnalysisFlag         * sAnalysisFlag = NULL;
    qmmValueNode            * sValues = NULL;
    qmmSubqueries           * sSetSubqueries = NULL;

    sAnalysisFlag = &( aAnalysis->mAnalysisFlag );

    switch ( sStmtType )
    {
        case QCI_STMT_INSERT :

            if ( ((qmmInsParseTree*)aStatement->myPlan->parseTree)->common.parse == qmv::parseInsertValues )
            {
                sValues = ((qmmInsParseTree*)aStatement->myPlan->parseTree)->rows->values;

                for ( ;
                      sValues != NULL;
                      sValues  = sValues->next )
                {
                    if ( ( sValues->value->lflag & QTC_NODE_SUBQUERY_MASK ) ==
                         QTC_NODE_SUBQUERY_EXIST )
                    {
                        IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                             aSMN,
                                                             sValues->value,
                                                             aAnalysis->mKeyInfo,
                                                             aIsSubKey,
                                                             &sSubqueryAnalysis )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
            }
            else
            {
                IDE_RAISE(ERR_INVALID_STMT_TYPE);
            }

            break;
        case QCI_STMT_UPDATE :

            sNode = ((qmmUptParseTree*)aStatement->myPlan->parseTree)->querySet->SFWGH->where;
            sValues = ((qmmUptParseTree*)aStatement->myPlan->parseTree)->values;
            sSetSubqueries = ((qmmUptParseTree*)aStatement->myPlan->parseTree)->subqueries;

            if ( sNode != NULL )
            {
                if ( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK ) ==
                     QTC_NODE_SUBQUERY_EXIST )
                {
                    IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                         aSMN,
                                                         sNode,
                                                         aAnalysis->mKeyInfo,
                                                         aIsSubKey,
                                                         &sSubqueryAnalysis )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }

            for ( ;
                  sValues != NULL;
                  sValues  = sValues->next )
            {
                if ( ( sValues->value->lflag & QTC_NODE_SUBQUERY_MASK ) ==
                     QTC_NODE_SUBQUERY_EXIST )
                {
                    IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                         aSMN,
                                                         sValues->value,
                                                         aAnalysis->mKeyInfo,
                                                         aIsSubKey,
                                                         &sSubqueryAnalysis )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do. */
                }
            }

            for ( ;
                  sSetSubqueries != NULL;
                  sSetSubqueries = sSetSubqueries->next )
            {
                IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                     aSMN,
                                                     sSetSubqueries->subquery,
                                                     aAnalysis->mKeyInfo,
                                                     aIsSubKey,
                                                     &sSubqueryAnalysis )
                          != IDE_SUCCESS );
            }

            break;
        case QCI_STMT_DELETE :

            sNode = ((qmmDelParseTree*)aStatement->myPlan->parseTree)->querySet->SFWGH->where;

            if ( sNode != NULL )
            {
                if ( ( sNode->lflag & QTC_NODE_SUBQUERY_MASK ) ==
                     QTC_NODE_SUBQUERY_EXIST )
                {
                    IDE_TEST( subqueryAnalysis4NodeTree( aStatement,
                                                         aSMN,
                                                         sNode,
                                                         aAnalysis->mKeyInfo,
                                                         aIsSubKey,
                                                         &sSubqueryAnalysis )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
            
            break;
        case QCI_STMT_EXEC_PROC :
            break;
        default :
            IDE_RAISE(ERR_INVALID_STMT_TYPE);
            break;
    }

    if ( sSubqueryAnalysis != NULL )
    {
        IDE_TEST( setShardInfo4Subquery( aStatement,
                                         aAnalysis,
                                         sSubqueryAnalysis,
                                         ID_FALSE, // aIsSelect
                                         aIsSubKey,
                                         sAnalysisFlag )
                  != IDE_SUCCESS );      
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_STMT_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::subqueryAnalysis4DML",
                                "Invalid stmt type"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}                                 

IDE_RC sda::getKeyValueID4InsertValue( qcStatement     * aStatement,
                                       ULong             aSMN,
                                       qmsTableRef     * aInsertTableRef,
                                       qcmColumn       * aInsertColumns,
                                       qmmMultiRows    * aInsertRows,
                                       sdiKeyInfo      * aKeyInfo,
                                       idBool          * aIsShardQuery,
                                       sdiAnalysisFlag * aAnalysisFlag,
                                       idBool            aIsSubKey )
{
/***********************************************************************
 *
 * Description :
 *     Insert column 중 shard key column에 대응하는 insert value( shard key value )
 *     를 찾고, 해당 insert value가 host variable이면 bind parameter ID를 반환한다.
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qcmColumn    * sInsertColumn = NULL;
    qmmValueNode * sInsertValue  = NULL;
    idBool         sHasKeyColumn = ID_FALSE;
    sdaQtcNodeType sQtcNodeType  = SDA_NONE;
    UInt           sColOrder;

    const mtdModule     * sKeyModule   = NULL;
    const void          * sKeyValue    = NULL;

    sdiKeyInfo      * sKeyInfoRef = NULL;

    sdiObjectInfo   * sShardObjInfo = NULL;

    // BUG-42764
    IDE_TEST_RAISE( aInsertRows == NULL, ERR_NULL_INSERT_ROWS );

    sdi::getShardObjInfoForSMN( aSMN,
                                aInsertTableRef->mShardObjInfo,
                                &( sShardObjInfo ) );

    IDE_TEST_RAISE( sShardObjInfo == NULL, ERR_NULL_SHARD_OBJECT_INFO );

    for ( sInsertColumn = aInsertColumns, sInsertValue = aInsertRows->values;
          sInsertColumn != NULL;
          sInsertColumn = sInsertColumn->next, sInsertValue = sInsertValue->next )
    {
        sColOrder = sInsertColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

        if ( ( ( aIsSubKey == ID_FALSE ) &&
               ( sShardObjInfo->mKeyFlags[sColOrder] == 1 ) ) ||
             ( ( aIsSubKey == ID_TRUE ) &&
               ( sShardObjInfo->mKeyFlags[sColOrder] == 2 ) ) )
        {
            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 sInsertValue->value,
                                                 &sQtcNodeType,
                                                 &sKeyInfoRef )
                      != IDE_SUCCESS );

            if ( ( sQtcNodeType == SDA_HOST_VAR ) ||
                 ( sQtcNodeType == SDA_CONSTANT_VALUE ) )
            {
                IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                                  sdiValueInfo,
                                                  ID_SIZEOF(sdiValueInfo*),
                                                  (void*)&aKeyInfo->mValuePtrArray )
                          != IDE_SUCCESS );

                aKeyInfo->mValuePtrCount = 1;
            }
            else
            {
                /* Nothing to do. */
            }

            switch ( sQtcNodeType )
            {
                case SDA_HOST_VAR:
                    IDE_TEST( allocAndSetShardHostValueInfo( aStatement,
                                                             sInsertValue->value->node.column,
                                                             aKeyInfo->mValuePtrArray )
                              != IDE_SUCCESS );
                    break;

                case SDA_CONSTANT_VALUE:
                    IDE_TEST( mtd::moduleById( &sKeyModule,
                                               aKeyInfo->mShardInfo.mKeyDataType )
                              != IDE_SUCCESS );

                    IDE_TEST( getKeyConstValue( aStatement,
                                                sInsertValue->value,
                                                sKeyModule,
                                                &sKeyValue )
                              != IDE_SUCCESS );

                    IDE_TEST( allocAndSetShardConstValueInfo( aStatement,
                                                              aKeyInfo->mShardInfo.mKeyDataType,
                                                              sKeyValue,
                                                              aKeyInfo->mValuePtrArray )
                              != IDE_SUCCESS );

                    break;

                default:
                    /* TASK-7219 Non-shard DML */
                    *aIsShardQuery = ID_FALSE;
                    /* TASK-7219 Shard Transformer Refactoring */
                    aAnalysisFlag->mNonShardFlag[ SDI_UNSPECIFIED_SHARD_KEY_VALUE ] = ID_TRUE;
                    break;
            }

            sHasKeyColumn = ID_TRUE;

            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sHasKeyColumn == ID_FALSE )
    {
        IDE_RAISE(ERR_SHARD_KEY_CONDITION_NOT_EXIST);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SHARD_KEY_CONDITION_NOT_EXIST)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getKeyValueID4InsertValue",
                                "The shard key value cannot be found."));
    }
    IDE_EXCEPTION(ERR_NULL_INSERT_ROWS)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getKeyValueID4InsertValue",
                                "The insert rows are NULL."));
    }
    IDE_EXCEPTION(ERR_NULL_SHARD_OBJECT_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getKeyValueID4InsertValue",
                                "Invalid shard object information"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getKeyValueID4ProcArguments( qcStatement     * aStatement,
                                         ULong             aSMN,
                                         qsProcParseTree * aPlanTree,
                                         qsExecParseTree * aParseTree,
                                         sdiKeyInfo      * aKeyInfo,
                                         sdiAnalysisFlag * aAnalysisFlag,
                                         idBool            aIsSubKey )
{
/***********************************************************************
 *
 * Description :
 *     Procedure arguments 중 shard key column에 대응하는 value( shard key value )
 *     를 찾고, 해당 value가 host variable이면 bind parameter ID를 반환한다.
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    qsVariableItems  * sParaDecls;
    qtcNode          * sArgument;

    idBool             sHasKeyColumn = ID_FALSE;
    sdaQtcNodeType     sQtcNodeType  = SDA_NONE;
    UInt               sColOrder;
    const mtdModule  * sKeyModule   = NULL;
    const void       * sKeyValue    = NULL;

    sdiKeyInfo       * sKeyInfoRef = NULL;

    sdiObjectInfo    * sShardObjInfo = NULL;

    sdi::getShardObjInfoForSMN( aSMN,
                                aParseTree->mShardObjInfo,
                                &( sShardObjInfo ) );

    IDE_TEST_RAISE( sShardObjInfo == NULL, ERR_NULL_SHARD_OBJECT_INFO );

    for ( sParaDecls = aPlanTree->paraDecls,
              sArgument = (qtcNode*)aParseTree->callSpecNode->node.arguments,
              sColOrder = 0;
          ( sParaDecls != NULL ) && ( sArgument != NULL );
          sParaDecls = sParaDecls->next,
              sArgument = (qtcNode*)sArgument->node.next,
              sColOrder++ )
    {
        if ( ( ( aIsSubKey == ID_FALSE ) && ( sShardObjInfo->mKeyFlags[sColOrder] == 1 ) ) ||
             ( ( aIsSubKey == ID_TRUE  ) && ( sShardObjInfo->mKeyFlags[sColOrder] == 2 ) ) )
        {
            IDE_TEST( getQtcNodeTypeWithKeyInfo( aStatement,
                                                 aKeyInfo,
                                                 sArgument,
                                                 &sQtcNodeType,
                                                 &sKeyInfoRef )
                      != IDE_SUCCESS );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              sdiValueInfo,
                                              ID_SIZEOF(sdiValueInfo*),
                                              (void*)&aKeyInfo->mValuePtrArray )
                      != IDE_SUCCESS );

            switch ( sQtcNodeType )
            {
                case SDA_HOST_VAR:
                    IDE_TEST( allocAndSetShardHostValueInfo( aStatement,
                                                             sArgument->node.column,
                                                             aKeyInfo->mValuePtrArray )
                              != IDE_SUCCESS );
                    break;

                case SDA_CONSTANT_VALUE:
                    IDE_TEST( mtd::moduleById( &sKeyModule,
                                               aKeyInfo->mShardInfo.mKeyDataType )
                              != IDE_SUCCESS );

                    IDE_TEST( getKeyConstValue( aStatement,
                                                sArgument,
                                                sKeyModule,
                                                &sKeyValue )
                              != IDE_SUCCESS );

                    IDE_TEST( allocAndSetShardConstValueInfo( aStatement,
                                                              aKeyInfo->mShardInfo.mKeyDataType,
                                                              sKeyValue,
                                                              aKeyInfo->mValuePtrArray )
                              != IDE_SUCCESS );
                    break;

                default:
                    /* TASK-7219 Shard Transformer Refactoring */
                    aAnalysisFlag->mNonShardFlag[ SDI_UNSPECIFIED_SHARD_KEY_VALUE ] = ID_TRUE;
                    IDE_CONT( NORMAL_EXIT );
                    break;

            }

            aKeyInfo->mValuePtrCount = 1;
            sHasKeyColumn = ID_TRUE;

            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sHasKeyColumn == ID_FALSE )
    {
        IDE_RAISE(ERR_SHARD_KEY_CONDITION_NOT_EXIST);
    }
    else
    {
        // Nothing to do.
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SHARD_KEY_CONDITION_NOT_EXIST)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDA_NOT_EXIST_SHARD_KEY_CONDITION));
    }
    IDE_EXCEPTION(ERR_NULL_SHARD_OBJECT_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getKeyValueID4ProcArguments",
                                "Invalid shard object information"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::normalizePredicate( qcStatement * aStatement,
                                qmsQuerySet * aQuerySet,
                                qtcNode    ** aPredicate )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *    shard predicate CNF normalization
 *
 ***********************************************************************/

    qmsQuerySet   * sQuerySet   = aQuerySet;
    qmoNormalType   sNormalType;
    qtcNode       * sNormalForm = NULL;
    qtcNode       * sNNFFilter  = NULL;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    //------------------------------------------
    // CNF normalization
    //------------------------------------------

    IDE_TEST_RAISE( sQuerySet->setOp != QMS_NONE,
                    ERR_NOT_SUPPORT_SET );

    IDE_TEST_CONT( sQuerySet->SFWGH->where == NULL,
                   NORMAL_EXIT );

    // PROJ-1413 constant exprssion의 상수 변환
    IDE_TEST( qmoConstExpr::processConstExpr( aStatement,
                                              sQuerySet->SFWGH )
              != IDE_SUCCESS );

    IDE_TEST( qmoCrtPathMgr::decideNormalType4Where( aStatement,
                                                     sQuerySet->SFWGH->from,
                                                     sQuerySet->SFWGH->where,
                                                     sQuerySet->SFWGH->hints,
                                                     ID_TRUE,  // CNF only
                                                     &sNormalType )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    if ( sNormalType != QMO_NORMAL_TYPE_CNF )
    {
        sQuerySet->mShardAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_CNF_NORMALIZATION_FAIL ] = ID_TRUE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    // where 절을 CNF Normalization
    IDE_TEST( qmoNormalForm::normalizeCNF( aStatement,
                                           sQuerySet->SFWGH->where,
                                           &sNormalForm )
              != IDE_SUCCESS );

    // BUG-35155 Partial CNF
    // Partial CNF 에서 제외된 qtcNode 를 NNF 필터로 만든다.
    IDE_TEST( qmoNormalForm::extractNNFFilter4CNF( aStatement,
                                                   sQuerySet->SFWGH->where,
                                                   &sNNFFilter )
              != IDE_SUCCESS );

    /* TASK-7219 Shard Transformer Refactoring */
    if ( sNNFFilter != NULL )
    {
        sQuerySet->mShardAnalysis->mAnalysisFlag.mNonShardFlag[ SDI_CNF_NORMALIZATION_FAIL ] = ID_TRUE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    //------------------------------------------
    // set normal form
    //------------------------------------------

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aPredicate = sNormalForm;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_SET )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::normalizePredicate",
                                "Invalid queryset type"));
    }
    IDE_EXCEPTION(ERR_NULL_STATEMENT)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::normalizePredicate",
                                "The statement is NULL."));
    }
    IDE_EXCEPTION(ERR_NULL_QUERYSET)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::normalizePredicate",
                                "The queryset is NULL"));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getQtcNodeTypeWithShardFrom( qcStatement         * aStatement,
                                         sdaFrom             * aShardFrom,
                                         qtcNode             * aNode,
                                         sdaQtcNodeType      * aQtcNodeType )
{
/***********************************************************************
 *
 * Description :
 *     qtcNode의 shard type을 반환한다.
 *
 *         < shard predicate type >
 *
 *             - SDA_NONE
 *             - SDA_KEY_COLUMN
 *             - SDA_HOST_VAR
 *             - SDA_CONSTANT_VALUE
 *             - SDA_EQUAL
 *             - SDA_OR
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    sdaFrom * sShardFrom = NULL;
    UInt      sKeyCount = 0;
    qtcNode * sNode = aNode;
    
    *aQtcNodeType = SDA_NONE;

    while ( sNode->node.module == &qtc::passModule )
    {
        sNode = (qtcNode*)sNode->node.arguments;
    }

    /* Check shard column kind */

    if ( sNode->node.module == &qtc::columnModule )
    {
        for ( sShardFrom  = aShardFrom;
              sShardFrom != NULL;
              sShardFrom  = sShardFrom->mNext )
        {
            if ( sShardFrom->mTupleId == sNode->node.table )
            {
                for ( sKeyCount = 0;
                      sKeyCount < sShardFrom->mKeyCount;
                      sKeyCount++ )
                {
                    if ( sShardFrom->mKey[sKeyCount] == sNode->node.column )
                    {
                        *aQtcNodeType = SDA_KEY_COLUMN;
                        break;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
            }
            else
            {
                // Nothing to do.
            }

            if ( *aQtcNodeType == SDA_KEY_COLUMN )
            {
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
        if ( qtc::isHostVariable( QC_SHARED_TMPLATE( aStatement ), sNode ) == ID_TRUE )
        {
            *aQtcNodeType = SDA_HOST_VAR;
        }
        else
        {
            if ( sNode->node.module == &mtfEqual )
            {
                *aQtcNodeType = SDA_EQUAL;
            }
            else
            {
                if ( sNode->node.module == &mtfOr )
                {
                    *aQtcNodeType = SDA_OR;
                }
                else
                {
                    if ( qtc::isConstValue( QC_SHARED_TMPLATE( aStatement ), sNode ) == ID_TRUE )
                    {
                        *aQtcNodeType = SDA_CONSTANT_VALUE;
                    }
                    else
                    {
                        if ( sNode->node.module == &sdfShardKey )
                        {
                            *aQtcNodeType = SDA_SHARD_KEY_FUNC;
                        }
                        else
                        {
                            // Nothing to do.
                            // SDA_NONE
                        }
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::getQtcNodeTypeWithKeyInfo( qcStatement         * aStatement,
                                       sdiKeyInfo          * aKeyInfo,
                                       qtcNode             * aNode,
                                       sdaQtcNodeType      * aQtcNodeType,
                                       sdiKeyInfo         ** aKeyInfoRef )
{
/***********************************************************************
 *
 * Description :
 *     qtcNode의 shard type을 반환한다.
 *
 *         < shard predicate type >
 *
 *             - SDA_NONE
 *             - SDA_KEY_COLUMN
 *             - SDA_HOST_VAR
 *             - SDA_CONSTANT_VALUE
 *             - SDA_EQUAL
 *             - SDA_OR
 *
 * Implementation :
 *
 * Arguments :
 *
 ***********************************************************************/

    sdiKeyInfo * sKeyInfo = NULL;
    UInt         sKeyCount = 0;
    qtcNode    * sNode = aNode;

    *aQtcNodeType = SDA_NONE;
    *aKeyInfoRef = NULL;

    while ( sNode->node.module == &qtc::passModule )
    {
        sNode = (qtcNode*)sNode->node.arguments;
    }

    /* Check shard column kind */

    if ( sNode->node.module == &qtc::columnModule )
    {
        for ( sKeyInfo  = aKeyInfo;
              sKeyInfo != NULL;
              sKeyInfo  = sKeyInfo->mNext )
        {
            for ( sKeyCount = 0;
                  sKeyCount < sKeyInfo->mKeyCount;
                  sKeyCount++ )
            {
                if ( ( sKeyInfo->mKey[sKeyCount].mTupleId == sNode->node.table ) &&
                     ( sKeyInfo->mKey[sKeyCount].mColumn == sNode->node.column ) )
                {
                    *aQtcNodeType = SDA_KEY_COLUMN;
                    *aKeyInfoRef = sKeyInfo;
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }

            if ( *aQtcNodeType == SDA_KEY_COLUMN )
            {
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
        if ( qtc::isHostVariable( QC_SHARED_TMPLATE( aStatement ), sNode ) == ID_TRUE )
        {
            *aQtcNodeType = SDA_HOST_VAR;
        }
        else
        {
            if ( sNode->node.module == &mtfEqual )
            {
                *aQtcNodeType = SDA_EQUAL;
            }
            else
            {
                if ( sNode->node.module == &mtfOr )
                {
                    *aQtcNodeType = SDA_OR;
                }
                else
                {
                    if ( qtc::isConstValue( QC_SHARED_TMPLATE( aStatement ), sNode ) == ID_TRUE )
                    {
                        *aQtcNodeType = SDA_CONSTANT_VALUE;
                    }
                    else
                    {
                        if ( sNode->node.module == &sdfShardKey )
                        {
                            *aQtcNodeType = SDA_SHARD_KEY_FUNC;
                        }
                        else
                        {
                            // Nothing to do.
                            // SDA_NONE
                        }
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::getKeyConstValue( qcStatement      * aStatement,
                              qtcNode          * aNode,
                              const mtdModule  * aModule,
                              const void      ** aValue )
{
    const mtcColumn  * sColumn;
    const void       * sValue;
    qtcNode          * sNode;

    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                     aNode,
                                     &sNode,
                                     ID_FALSE,
                                     ID_TRUE,
                                     ID_TRUE,
                                     ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimate( sNode,
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
              != IDE_SUCCESS );

    IDE_TEST( qtc::makeConversionNode( sNode,
                                       aStatement,
                                       QC_SHARED_TMPLATE(aStatement),
                                       aModule )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateConstExpr( sNode,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( qtc::isConstValue( QC_SHARED_TMPLATE(aStatement),
                                       sNode )
                    != ID_TRUE,
                    ERR_CONVERSION );

    sColumn = QTC_STMT_COLUMN( aStatement, sNode );
    sValue  = QTC_STMT_FIXEDDATA( aStatement, sNode );

    IDE_TEST_RAISE( sColumn->module->id != aModule->id,
                    ERR_CONVERSION );

    *aValue = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getKeyConstValue",
                                "Invalid shard value type"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzeAnsiJoin( qcStatement     * aStatement,
                             qmsQuerySet     * aQuerySet,
                             qmsFrom         * aFrom,
                             sdaFrom         * aShardFrom,
                             sdaCNFList     ** aCNFList,
                             sdiAnalysisFlag * aAnalysisFlag )
{
    sdaCNFList    * sCNFList = NULL;

    qmoNormalType   sNormalType;
    qtcNode       * sNNFFilter = NULL;

    idBool          sIsShardNullPadding = ID_FALSE;
    idBool          sIsCloneExists = ID_FALSE;
    idBool          sIsShardExists = ID_FALSE;

    if ( aFrom != NULL )
    {
        if ( aFrom->joinType != QMS_NO_JOIN )
        {
            IDE_TEST( analyzeAnsiJoin( aStatement,
                                       aQuerySet,
                                       aFrom->left,
                                       aShardFrom,
                                       aCNFList,
                                       aAnalysisFlag )
                      != IDE_SUCCESS );

            IDE_TEST( analyzeAnsiJoin( aStatement,
                                       aQuerySet,
                                       aFrom->right,
                                       aShardFrom,
                                       aCNFList,
                                       aAnalysisFlag )
                      != IDE_SUCCESS );

            //---------------------------------------
            // Make CNF tree for on condition
            //---------------------------------------
            if ( aFrom->onCondition != NULL )
            {
                //------------------------------------------
                // Allocation
                //------------------------------------------
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdaCNFList),
                                                         (void**) & sCNFList )
                          != IDE_SUCCESS );

                //------------------------------------------
                // Normalization
                //------------------------------------------
                IDE_TEST( qmoCrtPathMgr::decideNormalType( aStatement,
                                                           aFrom,
                                                           aFrom->onCondition,
                                                           aQuerySet->SFWGH->hints,
                                                           ID_TRUE, // CNF Only임
                                                           & sNormalType )
                          != IDE_SUCCESS );

                /* TASK-7219 Shard Transformer Refactoring */
                if ( sNormalType != QMO_NORMAL_TYPE_CNF )
                {
                    aAnalysisFlag->mNonShardFlag[ SDI_CNF_NORMALIZATION_FAIL ] = ID_TRUE;
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    /* Nothing to do */
                }

                IDE_TEST( qmoNormalForm::normalizeCNF( aStatement,
                                                       aFrom->onCondition,
                                                       & sCNFList->mCNF )
                          != IDE_SUCCESS );

                IDE_TEST( qmoNormalForm::extractNNFFilter4CNF( aStatement,
                                                               aFrom->onCondition,
                                                               &sNNFFilter )
                          != IDE_SUCCESS );

                /* TASK-7219 Shard Transformer Refactoring */
                if ( sNNFFilter != NULL )
                {
                    aAnalysisFlag->mNonShardFlag[ SDI_CNF_NORMALIZATION_FAIL ] = ID_TRUE;
                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    /* Nothing to do */
                }

                //------------------------------------------
                // 연결
                //------------------------------------------
                sCNFList->mNext = *aCNFList;
                *aCNFList = sCNFList;

                //------------------------------------------
                // Shard key column이 NULL-padding 되는지 확인한다.
                //------------------------------------------
                if ( aFrom->joinType == QMS_LEFT_OUTER_JOIN )
                {
                    IDE_TEST( setNullPaddedShardFrom( aFrom->right,
                                                      aShardFrom,
                                                      &sIsShardNullPadding )
                              != IDE_SUCCESS );

                    if ( ( sIsShardNullPadding == ID_TRUE ) &&
                         ( aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] == ID_FALSE ) )
                    {
                        /*
                         * BUG-45338
                         *
                         * hash, range, list 테이블의 outer join null-padding을 지원하면서,
                         * clone table과의 outer-join에 의해 hash, range, list table이 null-padding 되면 결과가 다름
                         * 이에대해 수행 불가로 판별(SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS) 한다.
                         *
                         * hash, range, list 테이블이 outer join에 의해 null-padding 될 경우
                         * join-tree 상 반대쪽에 clone table이 "있으면서",
                         * hash, range, list 테이블이 "없으면" 수행 불가로 판단한다.
                         *
                         *   LEFT     >> 수행가능
                         *   /  \
                         *  H    C    
                         *
                         *   LEFT     >> 수행불가
                         *   /  \     
                         *  C    H    
                         *
                         *   FULL     >> 수행불가
                         *   /  \
                         *  H    C    
                         *
                         *   FULL     >> 수행가능( hash, range, list 가 null-padding되지 않음)
                         *   /  \
                         *  C    C
                         *
                         */
                        IDE_TEST( isCloneAndShardExists( aFrom->left,
                                                         &sIsCloneExists,
                                                         &sIsShardExists )
                                  != IDE_SUCCESS );

                        if ( ( sIsCloneExists == ID_TRUE ) && ( sIsShardExists == ID_FALSE ) )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] = ID_TRUE;
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
                else if ( aFrom->joinType == QMS_RIGHT_OUTER_JOIN )
                {
                    IDE_TEST( setNullPaddedShardFrom( aFrom->left,
                                                      aShardFrom,
                                                      &sIsShardNullPadding )
                              != IDE_SUCCESS );

                    if ( ( sIsShardNullPadding == ID_TRUE ) &&
                         ( aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] == ID_FALSE ) )
                    {
                        IDE_TEST( isCloneAndShardExists( aFrom->right,
                                                         &sIsCloneExists,
                                                         &sIsShardExists )
                                  != IDE_SUCCESS );

                        if ( ( sIsCloneExists == ID_TRUE ) && ( sIsShardExists == ID_FALSE ) )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] = ID_TRUE;
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
                else if ( aFrom->joinType == QMS_FULL_OUTER_JOIN )
                {
                    IDE_TEST( setNullPaddedShardFrom( aFrom->left,
                                                      aShardFrom,
                                                      &sIsShardNullPadding )
                              != IDE_SUCCESS );

                    if ( ( sIsShardNullPadding == ID_TRUE ) &&
                         ( aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] == ID_FALSE ) )
                    {
                        IDE_TEST( isCloneAndShardExists( aFrom->right,
                                                         &sIsCloneExists,
                                                         &sIsShardExists )
                                  != IDE_SUCCESS );

                        if ( ( sIsCloneExists == ID_TRUE ) && ( sIsShardExists == ID_FALSE ) )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] = ID_TRUE;
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

                    sIsShardNullPadding = ID_FALSE;

                    IDE_TEST( setNullPaddedShardFrom( aFrom->right,
                                                      aShardFrom,
                                                      &sIsShardNullPadding )
                              != IDE_SUCCESS );

                    if ( ( sIsShardNullPadding == ID_TRUE ) &&
                         ( aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] == ID_FALSE ) )
                    {
                        sIsCloneExists = ID_FALSE;
                        sIsShardExists = ID_FALSE;
                        
                        IDE_TEST( isCloneAndShardExists( aFrom->left,
                                                         &sIsCloneExists,
                                                         &sIsShardExists )
                                  != IDE_SUCCESS );

                        if ( ( sIsCloneExists == ID_TRUE ) && ( sIsShardExists == ID_FALSE ) )
                        {
                            aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_OUTER_JOIN_EXISTS ] = ID_TRUE;
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
                else
                {
                    /* Nothing to do */
                }
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
    else
    {
        // Nothing to do.
    }

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setNullPaddedShardFrom( qmsFrom * aFrom,
                                    sdaFrom * aShardFrom,
                                    idBool  * aIsShardNullPadding )
{
    qmsParseTree     * sViewParseTree = NULL;
    sdiShardAnalysis * sViewAnalysis  = NULL;
    sdiShardInfo     * sShardInfo     = NULL;

    sdaFrom      * sShardFrom     = NULL;

    if ( aFrom != NULL )
    {
        if ( aFrom->joinType == QMS_NO_JOIN )
        {
            if ( aFrom->tableRef->view != NULL )
            {
                IDE_DASSERT( aFrom->tableRef->view->myPlan != NULL );

                sViewParseTree = (qmsParseTree*)aFrom->tableRef->view->myPlan->parseTree;
                sViewAnalysis = sViewParseTree->mShardAnalysis;

                IDE_TEST_RAISE( sViewAnalysis == NULL, ERR_NULL_ANALYSIS);

                sShardInfo = &(sViewAnalysis->mShardInfo);

                if ( sdi::getSplitType( sShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                {
                    for ( sShardFrom = aShardFrom; sShardFrom != NULL; sShardFrom = sShardFrom->mNext )
                    {
                        if ( aFrom->tableRef->table == sShardFrom->mTupleId )
                        {
                            sShardFrom->mIsNullPadding = ID_TRUE;
                            *aIsShardNullPadding = ID_TRUE;
                            break;
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }

                    /* TASK-7219 Shard Transformer Refactoring
                     *  Analyze 최소 수행 구조로 수정하면서,
                     *   Non Shard Object Error 를 Non Shard Reason 으로 처리하게 되었다.
                     *
                     *    sda::setNullPaddedShardFrom 내 Shard Object 가 아닌 대상도 검사하게 되므로,
                     *     Inner, Outer 대상이 없을 수 있다.
                     */
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                if ( aFrom->tableRef->mShardObjInfo != NULL )
                {
                    if ( sdi::getSplitType( aFrom->tableRef->mShardObjInfo->mTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                    {
                        for ( sShardFrom = aShardFrom; sShardFrom != NULL; sShardFrom = sShardFrom->mNext )
                        {
                            if ( aFrom->tableRef->table == sShardFrom->mTupleId )
                            {
                                sShardFrom->mIsNullPadding = ID_TRUE;
                                *aIsShardNullPadding = ID_TRUE;
                                break;
                            }
                            else
                            {
                                /* Nothing to do. */
                            }
                        }

                        /* TASK-7219 Shard Transformer Refactoring
                         *  Analyze 최소 수행 구조로 수정하면서,
                         *   Non Shard Object Error 를 Non Shard Reason 으로 처리하게 되었다.
                         *
                         *    sda::setNullPaddedShardFrom 내 Shard Object 가 아닌 대상도 검사하게 되므로,
                         *     Inner, Outer 대상이 없을 수 있다.
                         */
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
        }
        else
        {
            IDE_TEST( setNullPaddedShardFrom( aFrom->left,
                                              aShardFrom,
                                              aIsShardNullPadding )
                      != IDE_SUCCESS );

            IDE_TEST( setNullPaddedShardFrom( aFrom->right,
                                              aShardFrom,
                                              aIsShardNullPadding )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_ANALYSIS)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::isShardTableExistOnFrom",
                                "The view analysis info is null."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::isCloneAndShardExists( qmsFrom * aFrom,
                                   idBool  * aIsCloneExists,
                                   idBool  * aIsShardExists )
{
    qmsParseTree     * sViewParseTree = NULL;
    sdiShardAnalysis * sViewAnalysis  = NULL;
    sdiShardInfo     * sShardInfo     = NULL;

    if ( aFrom != NULL )
    {
        if ( aFrom->joinType == QMS_NO_JOIN )
        {
            if ( aFrom->tableRef->view != NULL )
            {
                IDE_DASSERT( aFrom->tableRef->view->myPlan != NULL );

                sViewParseTree = (qmsParseTree*)aFrom->tableRef->view->myPlan->parseTree;
                sViewAnalysis = sViewParseTree->mShardAnalysis;

                IDE_TEST_RAISE( sViewAnalysis == NULL, ERR_NULL_ANALYSIS);

                sShardInfo = &(sViewAnalysis->mShardInfo);

                if ( sShardInfo->mSplitMethod == SDI_SPLIT_CLONE )
                {
                    *aIsCloneExists = ID_TRUE;
                }
                else if ( sdi::getSplitType( sShardInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                {
                    *aIsShardExists = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                if ( aFrom->tableRef->mShardObjInfo != NULL )
                {
                    if ( aFrom->tableRef->mShardObjInfo->mTableInfo.mSplitMethod == SDI_SPLIT_CLONE )
                    {
                        *aIsCloneExists = ID_TRUE;
                    }
                    else if ( sdi::getSplitType( aFrom->tableRef->mShardObjInfo->mTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                    {
                        *aIsShardExists = ID_TRUE;
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
        }
        else
        {
            IDE_TEST( isCloneAndShardExists( aFrom->left,
                                             aIsCloneExists,
                                             aIsShardExists )
                      != IDE_SUCCESS );

            IDE_TEST( isCloneAndShardExists( aFrom->right,
                                             aIsCloneExists,
                                             aIsShardExists )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_ANALYSIS)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::isShardTableExistOnFrom",
                                "The view analysis info is null."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::analyzePredJoin( qcStatement     * aStatement,
                             qtcNode         * aNode,
                             sdaFrom         * aShardFromInfo,
                             sdiAnalysisFlag * aAnalysisFlag,
                             idBool            aIsSubKey )
{
    if ( aNode != NULL )
    {
        /*
         * BUG-45391
         *
         * SEMI/ANTI JOIN 에서 Shard SQL판별 규칙은 다음과 같다.
         *
         * OUTER RELATION / INNER RELATION / SHARD SQL
         *        H               H             O
         *        H               C             O
         *        C               H             X   << 이놈을 찾아서 수행불가 처리한다.
         *        C               C             O
         *
         */
        if ( ( ( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) == QTC_NODE_JOIN_TYPE_SEMI ) ||
             ( ( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) == QTC_NODE_JOIN_TYPE_ANTI ) )
        {
            IDE_TEST( checkInnerOuterTable( aStatement,
                                            aNode,
                                            aShardFromInfo,
                                            aAnalysisFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST( analyzePredJoin( aStatement,
                                   (qtcNode*)aNode->node.next,
                                   aShardFromInfo,
                                   aAnalysisFlag,
                                   aIsSubKey )
                  != IDE_SUCCESS );

        if ( QTC_IS_SUBQUERY( aNode ) == ID_FALSE )
        {
            IDE_TEST( analyzePredJoin( aStatement,
                                       (qtcNode*)aNode->node.arguments,
                                       aShardFromInfo,
                                       aAnalysisFlag,
                                       aIsSubKey )
                      != IDE_SUCCESS );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::allocAndSetShardHostValueInfo( qcStatement      * aStatement,
                                           UInt               aBindParamId,
                                           sdiValueInfo    ** aValueInfo )
{
    sdiValueInfo    * sValueInfo = NULL;

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            sdiValueInfo,
                            (void*) &sValueInfo )
              != IDE_SUCCESS );

    sValueInfo->mType = SDI_VALUE_INFO_HOST_VAR;
    sValueInfo->mDataValueType = MTD_SMALLINT_ID;
    sValueInfo->mValue.mBindParamId = aBindParamId;

    *aValueInfo = sValueInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::allocAndSetShardConstValueInfo( qcStatement      * aStatement,
                                            UInt               aKeyDataType,
                                            const void       * aKeyValue,
                                            sdiValueInfo    ** aValueInfo )
{
    sdiValueInfo        * sValueInfo   = NULL;
    UInt                  sValueInfoSize = 0;

    sValueInfoSize = calculateValueInfoSize( aKeyDataType, aKeyValue );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                      sdiValueInfo,
                                      sValueInfoSize,
                                      (void*) &sValueInfo )
              != IDE_SUCCESS );

    sValueInfo->mType = SDI_VALUE_INFO_CONST_VAL;

    IDE_TEST( setKeyValue( aKeyDataType,
                           aKeyValue,
                           sValueInfo )
              != IDE_SUCCESS );

    *aValueInfo = sValueInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt sda::calculateValueInfoSize( UInt             aKeyDataType,
                                  const void     * aConstValue )
{
    UInt    sSize = 0;

    switch ( aKeyDataType )
    {
        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:

            // char varchar 는 sdiValueInfo 의 정적 크기 보다 클수 있다.
            if ( (((mtdCharType*)aConstValue)->length + 2) <= ID_SIZEOF( sdiValue ) )
            {
                sSize = idlOS::align8(ID_SIZEOF( sdiValueInfo ));
            }
            else
            {
                sSize = idlOS::align8(ID_SIZEOF( sdiValueInfo )) + ((mtdCharType*)aConstValue)->length;
            }
            break;

        case MTD_SMALLINT_ID:
        case MTD_INTEGER_ID:
        case MTD_BIGINT_ID:
        default:
            sSize = idlOS::align8(ID_SIZEOF( sdiValueInfo ));
            break;

    }

    return sSize;
}

IDE_RC sda::setKeyValue( UInt           aKeyDataType,
                         const void   * aConstValue,
                         sdiValueInfo * aValue )
{
    if ( aKeyDataType == MTD_SMALLINT_ID )
    {
        aValue->mValue.mSmallintMax = *(SShort*)aConstValue;                
    }
    else if ( aKeyDataType == MTD_INTEGER_ID )
    {
        aValue->mValue.mIntegerMax = *(SInt*)aConstValue;
    }
    else if ( aKeyDataType == MTD_BIGINT_ID )
    {
        aValue->mValue.mBigintMax = *(SLong*)aConstValue;
    }
    else if ( ( aKeyDataType == MTD_CHAR_ID ) ||
              ( aKeyDataType == MTD_VARCHAR_ID ) )
    {
        idlOS::memcpy( (void*) aValue->mValue.mCharMaxBuf,
                       (void*) aConstValue,
                       ((mtdCharType*)aConstValue)->length + 2 );
    }
    else
    {
        IDE_RAISE( ERR_INVALID_SHARD_KEY_DATA_TYPE );
    }

    aValue->mDataValueType = aKeyDataType;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SHARD_KEY_DATA_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::setKeyValue",
                                "Invalid shard key data type"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getRangeIndexByValue( qcTemplate     * aTemplate,
                                  mtcTuple       * aShardKeyTuple,
                                  sdiAnalyzeInfo * aShardAnalysis,
                                  UShort           aValueIndex,
                                  sdiValueInfo   * aValue,
                                  sdiRangeIndex  * aRangeIndex,
                                  UShort         * aRangeIndexCount,
                                  idBool         * aHasDefaultNode,
                                  idBool           aIsSubKey )
{
    /*
     * PROJ-2655 Composite shard key
     *
     * shard range info에서 shard value의 값 에 해당하는 index를 찾아 반환한다.
     *
     * < sdiRangeInfo >
     *
     *  value     : [100][100][100][200][200][200][300][300][300]...
     *  sub value : [ A ][ B ][ C ][ A ][ B ][ C ][ A ][ B ][ C ]...
     *  nodeId    : [ 1 ][ 2 ][ 1 ][ 3 ][ 1 ][ 3 ][ 2 ][ 2 ][ 3 ]...
     *
     *  value = '100' 이라면
     *  aRangeIndex 는 0,1,2 총 3개가 세팅된다.
     *
     */

    sdiSplitMethod  sSplitMethod;
    UInt            sKeyDataType;

    mtcColumn   * sKeyColumn;
    void        * sKeyValue;
    mtvConvert  * sConvert;
    mtcId         sKeyType;
    UInt          sArguCount;
    mtcColumn   * sColumn;
    void        * sValue;

    const mtdModule  * sKeyModule = NULL;

    sdiSplitMethod     sPriorSplitMethod;
    UInt               sPriorKeyDataType;

    UInt sHashValue;

    /* TASK-7219 Non-shard DML */
    mtcTuple    * sTuple = NULL;

    IDE_TEST_RAISE( aValue == NULL, ERR_NULL_SHARD_VALUE );

    IDE_TEST_RAISE( ( ( ( aValue->mType == 0 ) || ( aValue->mType == 2 ) ) &&
                      ( ( aTemplate == NULL ) || ( aShardKeyTuple == NULL ) ) ),
                    ERR_NULL_TUPLE );

    if ( aIsSubKey == ID_FALSE )
    {
        sSplitMethod = aShardAnalysis->mSplitMethod;
        sKeyDataType = aShardAnalysis->mKeyDataType;
    }
    else
    {
        sSplitMethod = aShardAnalysis->mSubSplitMethod;
        sKeyDataType = aShardAnalysis->mSubKeyDataType;
    }
    
    sPriorSplitMethod = aShardAnalysis->mSplitMethod;
    sPriorKeyDataType = aShardAnalysis->mKeyDataType;

    IDE_TEST_RAISE( !( sdi::getSplitType( sSplitMethod ) == SDI_SPLIT_TYPE_DIST ),
                    ERR_INVALID_SPLIT_METHOD );

    if ( ( aValue->mType == 0 ) || // bind param
         ( aValue->mType == 2 ) /* TASK-7219 Transformed out column bind */
         )
    {
        if ( aValue->mType == 0 )
        {
            sTuple = aShardKeyTuple;
            
            IDE_DASSERT( aValue->mValue.mBindParamId <
                         sTuple->columnCount );

            sKeyColumn = sTuple->columns +
                aValue->mValue.mBindParamId;
        }
        else
        {
            sTuple = & aTemplate->tmplate.rows[ *aValue->mValue.mOutRefInfo.mTuple ];
            sKeyColumn = sTuple->columns +
                *aValue->mValue.mOutRefInfo.mColumn;
        }

        // BUG-45752
        sKeyValue = (UChar*)mtc::value( sKeyColumn,
                                        sTuple->row,
                                        MTD_OFFSET_USE );

        if ( sKeyColumn->module->id != sKeyDataType )
        {
            sKeyType.dataTypeId = sKeyDataType;
            sKeyType.languageId = sKeyColumn->type.languageId;
            sArguCount = sKeyColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;

            IDE_TEST( mtv::estimateConvert4Server(
                          aTemplate->stmt->qmxMem,
                          &sConvert,
                          sKeyType,                     // aDestinationType
                          sKeyColumn->type,             // aSourceType
                          sArguCount,                   // aSourceArgument
                          sKeyColumn->precision,        // aSourcePrecision
                          sKeyColumn->scale,            // aSourceScale
                          &(aTemplate->tmplate) )
                      != IDE_SUCCESS );

            // source value pointer
            sConvert->stack[sConvert->count].value = sKeyValue;
            // destination value pointer
            sColumn = sConvert->stack->column;
            sValue  = sConvert->stack->value;

            IDE_TEST( mtv::executeConvert( sConvert, &(aTemplate->tmplate) )
                      != IDE_SUCCESS);
        }
        else
        {
            sColumn = sKeyColumn;
            sValue  = sKeyValue;
        }

        IDE_DASSERT( sColumn->module->id == sKeyDataType );
        sKeyModule = sColumn->module;
    }
    else if ( aValue->mType == 1 ) // constant value
    {
        IDE_TEST( mtd::moduleById( &sKeyModule,
                                   sKeyDataType )
              != IDE_SUCCESS );

        sValue = (void*)&aValue->mValue;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_VALUE_TYPE );
    }

    switch ( sSplitMethod )
    {
        case SDI_SPLIT_HASH :

            sHashValue = sKeyModule->hash( mtc::hashInitialValue,
                                           NULL,
                                           sValue );

            /* BUG-46288 */
            if ( SDU_SHARD_TEST_ENABLE == 0 )
            {
                sHashValue %= SDI_HASH_MAX_VALUE;
            }
            else
            {
                sHashValue %= SDI_HASH_MAX_VALUE_FOR_TEST;
            }

            IDE_TEST( getRangeIndexFromHash( &aShardAnalysis->mRangeInfo,
                                             aValueIndex,
                                             sHashValue,
                                             aRangeIndex,
                                             aRangeIndexCount,
                                             sPriorSplitMethod,
                                             sPriorKeyDataType,
                                             aHasDefaultNode,
                                             aIsSubKey )
                      != IDE_SUCCESS );
            break;
        case SDI_SPLIT_RANGE :

            IDE_TEST( getRangeIndexFromRange( &aShardAnalysis->mRangeInfo,
                                              sKeyModule,
                                              aValueIndex,
                                              (const void*)sValue,
                                              aRangeIndex,
                                              aRangeIndexCount,
                                              sPriorSplitMethod,
                                              sPriorKeyDataType,
                                              aHasDefaultNode,
                                              aIsSubKey )
                      != IDE_SUCCESS );
            break;
        case SDI_SPLIT_LIST :

            IDE_TEST( getRangeIndexFromList( &aShardAnalysis->mRangeInfo,
                                             sKeyModule,
                                             aValueIndex,
                                             (const void*)sValue,
                                             aRangeIndex,
                                             aRangeIndexCount,
                                             aHasDefaultNode,
                                             aIsSubKey )
                      != IDE_SUCCESS );
            break;
        default :
            IDE_RAISE( ERR_INVALID_SPLIT_METHOD );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_SHARD_VALUE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexByValue",
                                "The shard value is null"));
    }
    IDE_EXCEPTION(ERR_INVALID_VALUE_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexByValue",
                                "Invalid value type"));
    }
    IDE_EXCEPTION(ERR_INVALID_SPLIT_METHOD)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexByValue",
                                "Invalid split method"));
    }
    IDE_EXCEPTION(ERR_NULL_TUPLE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexByValue",
                                "The tuple is null."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getRangeIndexFromHash( sdiRangeInfo    * aRangeInfo,
                                   UShort            aValueIndex,
                                   UInt              aHashValue,
                                   sdiRangeIndex   * aRangeIdxList,
                                   UShort          * aRangeIdxCount,
                                   sdiSplitMethod    aPriorSplitMethod,
                                   UInt              aPriorKeyDataType,
                                   idBool          * aHasDefaultNode,
                                   idBool            aIsSubKey )
{
    UInt              sHashMax;
    idBool            sNewPriorHashGroup = ID_TRUE;
    const mtdModule * sPriorKeyModule;
    UInt              sPriorKeyDataType;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    UShort            sRangeIdxCount = *aRangeIdxCount;

    UShort            i;
    UShort            j;

    if ( aIsSubKey == ID_FALSE )
    {
        for ( i = 0; i < aRangeInfo->mCount; i++ )
        {
            sHashMax = aRangeInfo->mRanges[i].mValue.mHashMax;

            /* BUG-46288 */
            if ( aHashValue < sHashMax )
            {
                for ( j = i; j < aRangeInfo->mCount; j++ )
                {
                    if ( sHashMax == aRangeInfo->mRanges[j].mValue.mHashMax )
                    {
                        IDE_TEST_RAISE ( sRangeIdxCount >= 1000, ERR_RANGE_INDEX_OVERFLOW );

                        aRangeIdxList[sRangeIdxCount].mRangeIndex = j;
                        aRangeIdxList[sRangeIdxCount].mValueIndex = aValueIndex;
                        sRangeIdxCount++;
                    }
                    else
                    {
                        break;
                    }
                }

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
        /* Get prior key module */
        if ( aPriorSplitMethod == SDI_SPLIT_HASH )
        {
            sPriorKeyDataType = MTD_INTEGER_ID;
        }
        else
        {
            sPriorKeyDataType = aPriorKeyDataType;
        }

        IDE_TEST( mtd::moduleById( &sPriorKeyModule,
                                   sPriorKeyDataType )
                  != IDE_SUCCESS );

        for ( i = 0; i < aRangeInfo->mCount; i++ )
        {
            sHashMax = aRangeInfo->mRanges[i].mSubValue.mHashMax;

            // BUG-45462 mValue의 값으로 new group을 판단한다.
            if ( i == 0 )
            {
                sNewPriorHashGroup = ID_TRUE;
            }
            else
            {
                sValueInfo1.column = NULL;
                sValueInfo1.value  = aRangeInfo->mRanges[i - 1].mValue.mMax;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = NULL;
                sValueInfo2.value  = aRangeInfo->mRanges[i].mValue.mMax;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                if ( sPriorKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                             &sValueInfo2 ) != 0 )
                {
                    sNewPriorHashGroup = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }

            /* BUG-46288 */
            if ( ( sNewPriorHashGroup == ID_TRUE ) && ( aHashValue < sHashMax ) )
            {
                IDE_TEST_RAISE ( sRangeIdxCount >= 1000, ERR_RANGE_INDEX_OVERFLOW );

                aRangeIdxList[sRangeIdxCount].mRangeIndex = i;
                aRangeIdxList[sRangeIdxCount].mValueIndex = aValueIndex;
                sRangeIdxCount++;

                sNewPriorHashGroup = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    if ( sRangeIdxCount == *aRangeIdxCount )
    {
        *aHasDefaultNode = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    *aRangeIdxCount = sRangeIdxCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RANGE_INDEX_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexFromHash",
                                "The range index count overflow"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getRangeIndexFromRange( sdiRangeInfo    * aRangeInfo,
                                    const mtdModule * aKeyModule,
                                    UShort            aValueIndex,
                                    const void      * aValue,
                                    sdiRangeIndex   * aRangeIdxList,
                                    UShort          * aRangeIdxCount,
                                    sdiSplitMethod    aPriorSplitMethod,
                                    UInt              aPriorKeyDataType,
                                    idBool          * aHasDefaultNode,
                                    idBool            aIsSubKey )
{
    mtdValueInfo      sRangeValue;
    mtdValueInfo      sRangeMax1;
    mtdValueInfo      sRangeMax2;

    idBool            sNewPriorRangeGroup = ID_TRUE;
    const mtdModule * sPriorKeyModule;
    UInt              sPriorKeyDataType;
    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    UShort sRangeIdxCount = *aRangeIdxCount;

    UShort i;
    UShort j;

    sRangeValue.column = NULL;
    sRangeValue.value  = aValue;
    sRangeValue.flag   = MTD_OFFSET_USELESS;

    if ( aIsSubKey == ID_FALSE )
    {
        for ( i = 0; i < aRangeInfo->mCount; i++ )
        {
            sRangeMax1.column = NULL;
            sRangeMax1.value  = aRangeInfo->mRanges[i].mValue.mMax;
            sRangeMax1.flag   = MTD_OFFSET_USELESS;

            if ( aKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sRangeValue,
                                                                    &sRangeMax1 ) < 0 )
            {
                for ( j = i; j < aRangeInfo->mCount; j++ )
                {
                    sRangeMax2.column = NULL;
                    sRangeMax2.value  = aRangeInfo->mRanges[j].mValue.mMax;
                    sRangeMax2.flag   = MTD_OFFSET_USELESS;

                    if ( aKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sRangeMax1,
                                                                            &sRangeMax2 ) == 0 )
                    {
                        IDE_TEST_RAISE ( sRangeIdxCount >= 1000, ERR_RANGE_INDEX_OVERFLOW );

                        aRangeIdxList[sRangeIdxCount].mRangeIndex = j;
                        aRangeIdxList[sRangeIdxCount].mValueIndex = aValueIndex;
                        sRangeIdxCount++;
                    }
                    else
                    {
                        break;
                    }
                }

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
        /* Get prior key module */
        if ( aPriorSplitMethod == SDI_SPLIT_HASH )
        {
            sPriorKeyDataType = MTD_INTEGER_ID;
        }
        else
        {
            sPriorKeyDataType = aPriorKeyDataType;
        }

        IDE_TEST( mtd::moduleById( &sPriorKeyModule,
                                   sPriorKeyDataType )
                  != IDE_SUCCESS );

        for ( i = 0; i < aRangeInfo->mCount; i++ )
        {
            if ( i == 0 )
            {
                sNewPriorRangeGroup = ID_TRUE;
            }
            else
            {
                sValueInfo1.column = NULL;
                sValueInfo1.value  = aRangeInfo->mRanges[i - 1].mValue.mMax;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = NULL;
                sValueInfo2.value  = aRangeInfo->mRanges[i].mValue.mMax;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                if ( sPriorKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                             &sValueInfo2 ) != 0 )
                {
                    sNewPriorRangeGroup = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }

            sRangeMax1.column = NULL;
            sRangeMax1.value  = aRangeInfo->mRanges[i].mSubValue.mMax;
            sRangeMax1.flag   = MTD_OFFSET_USELESS;

            if ( ( sNewPriorRangeGroup == ID_TRUE ) &&
                 ( aKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sRangeValue,
                                                                      &sRangeMax1 ) < 0 ) )
            {
                IDE_TEST_RAISE ( sRangeIdxCount >= 1000, ERR_RANGE_INDEX_OVERFLOW );

                aRangeIdxList[sRangeIdxCount].mRangeIndex = i;
                aRangeIdxList[sRangeIdxCount].mValueIndex = aValueIndex;
                sRangeIdxCount++;

                sNewPriorRangeGroup = ID_FALSE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    if ( sRangeIdxCount == *aRangeIdxCount )
    {
        *aHasDefaultNode = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    *aRangeIdxCount = sRangeIdxCount;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RANGE_INDEX_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexFromRange",
                                "The range index count overflow"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getRangeIndexFromList( sdiRangeInfo    * aRangeInfo,
                                   const mtdModule * aKeyModule,
                                   UShort            aValueIndex,
                                   const void      * aValue,
                                   sdiRangeIndex   * aRangeIdxList,
                                   UShort          * aRangeIdxCount,
                                   idBool          * aHasDefaultNode,
                                   idBool            aIsSubKey )
{
    mtdValueInfo      sKeyValue;
    mtdValueInfo      sListValue;

    UShort sRangeIdxCount = *aRangeIdxCount;

    UShort i;

    sKeyValue.column = NULL;
    sKeyValue.value  = aValue;
    sKeyValue.flag   = MTD_OFFSET_USELESS;

    sListValue.column = NULL;
    sListValue.flag   = MTD_OFFSET_USELESS;

    for ( i = 0; i < aRangeInfo->mCount; i++ )
    {
        if ( aIsSubKey == ID_FALSE )
        {
            sListValue.value = aRangeInfo->mRanges[i].mValue.mMax;
        }
        else
        {
            sListValue.value = aRangeInfo->mRanges[i].mSubValue.mMax;
        }

        if ( aKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sKeyValue,
                                                                &sListValue ) == 0 )
        {
            IDE_TEST_RAISE ( sRangeIdxCount >= 1000, ERR_RANGE_INDEX_OVERFLOW );

            aRangeIdxList[sRangeIdxCount].mRangeIndex = i;
            aRangeIdxList[sRangeIdxCount].mValueIndex = aValueIndex;
            sRangeIdxCount++;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sRangeIdxCount == *aRangeIdxCount )
    {
        *aHasDefaultNode = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    *aRangeIdxCount = sRangeIdxCount;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RANGE_INDEX_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::getRangeIndexFromList",
                                "The range index count overflow"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkValuesSame( qcTemplate   * aTemplate,
                             mtcTuple     * aShardKeyTuple,
                             UInt           aKeyDataType,
                             sdiValueInfo * aValue1,
                             sdiValueInfo * aValue2,
                             idBool       * aIsSame )
{
    const mtdModule * sKeyModule = NULL;

    mtcColumn   * sKeyColumn;
    void        * sKeyValue;
    mtvConvert  * sConvert;
    mtcId         sKeyType;
    UInt          sArguCount;
    mtcColumn   * sColumn;

    sdiValueInfo * sValue[2];
    mtdValueInfo   sCompareValue[2];

    UShort i;

    IDE_TEST_RAISE( ( ( aValue1 == NULL ) || ( aValue2 == NULL ) ),
                    ERR_NULL_SHARD_VALUE );

    IDE_TEST_RAISE( ( ( ( aValue1->mType == 0 ) || ( aValue2->mType == 0 ) ) &&
                      ( ( aTemplate == NULL ) || ( aShardKeyTuple == NULL ) ) ),
                    ERR_NULL_TUPLE );

    sValue[0] = aValue1;
    sValue[1] = aValue2;

    for ( i = 0; i < 2; i++ )
    {
        sCompareValue[i].column = NULL;
        sCompareValue[i].flag   = MTD_OFFSET_USELESS;

        if ( sValue[i]->mType == 0 )
        {
            IDE_DASSERT( sValue[i]->mValue.mBindParamId <
                         aShardKeyTuple->columnCount );

            sKeyColumn = aShardKeyTuple->columns +
                sValue[i]->mValue.mBindParamId;

            sKeyValue  = (UChar*)aShardKeyTuple->row + sKeyColumn->column.offset;

            if ( sKeyColumn->module->id != aKeyDataType )
            {
                sKeyType.dataTypeId = aKeyDataType;
                sKeyType.languageId = sKeyColumn->type.languageId;
                sArguCount = sKeyColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;

                IDE_TEST( mtv::estimateConvert4Server(
                              aTemplate->stmt->qmxMem,
                              &sConvert,
                              sKeyType,               // aDestinationType
                              sKeyColumn->type,       // aSourceType
                              sArguCount,             // aSourceArgument
                              sKeyColumn->precision,  // aSourcePrecision
                              sKeyColumn->scale,      // aSourceScale
                              &(aTemplate->tmplate) )
                          != IDE_SUCCESS );

                // source value pointer
                sConvert->stack[sConvert->count].value = sKeyValue;
                // destination value pointer
                sColumn   = sConvert->stack->column;
                sCompareValue[i].value = sConvert->stack->value;

                IDE_TEST( mtv::executeConvert( sConvert, &(aTemplate->tmplate) )
                          != IDE_SUCCESS);
            }
            else
            {
                sColumn = sKeyColumn;
                sCompareValue[i].value = sKeyValue;
            }

            IDE_DASSERT( sColumn->module->id == aKeyDataType );
            sKeyModule = sColumn->module;
        }
        else if ( sValue[i]->mType == 1 )
        {
            IDE_TEST( mtd::moduleById( &sKeyModule,
                                       aKeyDataType )
                      != IDE_SUCCESS );

            sCompareValue[i].value = (void*)&sValue[i]->mValue;
        }
        else
        {
            IDE_RAISE( ERR_INVALID_VALUE_TYPE );
        }
    }

    if ( sKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sCompareValue[0],
                                                            &sCompareValue[1] ) == 0 )
    {
        *aIsSame = ID_TRUE;
    }
    else
    {
        *aIsSame = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NULL_SHARD_VALUE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::checkValuesSame",
                                "The shard value is null"));
    }
    IDE_EXCEPTION(ERR_INVALID_VALUE_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::checkValuesSame",
                                "Invalid value type"));
    }
    IDE_EXCEPTION(ERR_NULL_TUPLE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::checkValuesSame",
                                "The tuple is null."));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::isOneNodeSQL( sdaFrom      * aShardFrom,
                          sdiKeyInfo   * aKeyInfo,
                          idBool       * aIsOneNodeSQL,
                          UInt         * aDistKeyCount )
{
    sdaFrom    * sCurrShardFrom = NULL;
    sdiKeyInfo * sCurrKeyInfo   = NULL;
    UInt         sExecNodeCntPrediction = 0;

    *aDistKeyCount = 0;
    *aIsOneNodeSQL = ID_TRUE;

    if ( aShardFrom != NULL )
    {
        for ( sCurrShardFrom  = aShardFrom;
              sCurrShardFrom != NULL;
              sCurrShardFrom  = sCurrShardFrom->mNext )
        {
            if ( sCurrShardFrom->mShardInfo.mSplitMethod == SDI_SPLIT_SOLO )
            {
                *aIsOneNodeSQL = ID_TRUE;
                break;
            }
            else if ( sdi::getSplitType( sCurrShardFrom->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
            {
                /*
                 * SDI_SPLIT_NONE
                 * SDI_SPLIT_HASH
                 * SDI_SPLIT_RANGE
                 * SDI_SPLIT_LIST
                 * SDI_SPLIT_NODES
                 */
                *aIsOneNodeSQL = ID_FALSE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }
    else
    {
        for ( sCurrKeyInfo  = aKeyInfo;
              sCurrKeyInfo != NULL;
              sCurrKeyInfo  = sCurrKeyInfo->mNext )
        {
            if ( sCurrKeyInfo->mShardInfo.mSplitMethod == SDI_SPLIT_SOLO )
            {
                sExecNodeCntPrediction = 0;
                break;
            }
            else if ( sdi::getSplitType( sCurrKeyInfo->mShardInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
            {
                sExecNodeCntPrediction++;

                if ( sCurrKeyInfo->mKeyCount > 0 )
                {
                    (*aDistKeyCount)++;
                }
                else
                {
                    /* Nothing to do. */
                }

                if ( sCurrKeyInfo->mValuePtrCount != 1 )
                {
                    sExecNodeCntPrediction++;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                /* Nothing to do. */
            }
        }

        if ( sExecNodeCntPrediction <= 1 )
        {
            // CLONE OR SOLO OR SINGLE DIST KEY EXIST.
            *aIsOneNodeSQL = ID_TRUE;
        }
        else
        {
            *aIsOneNodeSQL = ID_FALSE;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::checkInnerOuterTable( qcStatement     * aStatement,
                                  qtcNode         * aNode,
                                  sdaFrom         * aShardFrom,
                                  sdiAnalysisFlag * aAnalysisFlag )
{
    SInt      sTable;
    qmsFrom * sFrom;

    sdaFrom        * sShardFrom = NULL;

    sdiSplitMethod sInnerSplit1 = SDI_SPLIT_NONE;
    sdiSplitMethod sInnerSplit2 = SDI_SPLIT_NONE;
    sdiSplitMethod sOuterSplit1 = SDI_SPLIT_NONE;
    sdiSplitMethod sOuterSplit2 = SDI_SPLIT_NONE;

    IDE_DASSERT( ( ( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) == QTC_NODE_JOIN_TYPE_SEMI ) ||
                 ( ( aNode->lflag & QTC_NODE_JOIN_TYPE_MASK ) == QTC_NODE_JOIN_TYPE_ANTI ) );
    
    sTable = qtc::getPosFirstBitSet( &aNode->depInfo );
    while ( sTable != QTC_DEPENDENCIES_END )
    {
        sFrom = QC_SHARED_TMPLATE( aStatement )->tableMap[sTable].from;

        if ( qtc::dependencyContains( &sFrom->semiAntiJoinDepInfo, &aNode->depInfo ) == ID_TRUE )
        {
            // Inner table
            for ( sShardFrom = aShardFrom; sShardFrom != NULL; sShardFrom = sShardFrom->mNext )
            {
                if ( sShardFrom->mTupleId == sTable )
                {
                    if ( sShardFrom->mShardInfo.mSplitMethod == SDI_SPLIT_CLONE )
                    {
                        sInnerSplit1 = sShardFrom->mShardInfo.mSplitMethod;
                    }
                    else
                    {
                        sInnerSplit2 = sShardFrom->mShardInfo.mSplitMethod;
                    }

                    sShardFrom->mIsAntiJoinInner = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
        else
        {
            // Outer table
            for ( sShardFrom = aShardFrom; sShardFrom != NULL; sShardFrom = sShardFrom->mNext )
            {
                if ( sShardFrom->mTupleId == sTable )
                {
                    if ( sShardFrom->mShardInfo.mSplitMethod == SDI_SPLIT_CLONE )
                    {
                        sOuterSplit1 = sShardFrom->mShardInfo.mSplitMethod;
                    }
                    else
                    {
                        sOuterSplit2 = sShardFrom->mShardInfo.mSplitMethod;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        sTable = qtc::getPosNextBitSet( &aNode->depInfo, sTable );
    }

    /* TASK-7219 Shard Transformer Refactoring
     *  Analyze 최소 수행 구조로 수정하면서,
     *   Non Shard Object Error 를 Non Shard Reason 으로 처리하게 되었다.
     *
     *    sda::setNullPaddedShardFrom 내 Shard Object 가 아닌 대상도 검사하게 되므로,
     *     Inner, Outer 대상이 없을 수 있다.
     */

    if ( ( ( sOuterSplit1 == SDI_SPLIT_CLONE ) && ( sOuterSplit2 == SDI_SPLIT_NONE ) ) &&
         ( ( sInnerSplit1 != SDI_SPLIT_CLONE ) || ( sInnerSplit2 != SDI_SPLIT_NONE ) ) )
    {
        aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_SEMI_ANTI_JOIN_EXISTS ] = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    /* IDE_EXCEPTION_END; */

    return IDE_FAILURE;
}

IDE_RC sda::checkAntiJoinInner( sdiKeyInfo * aKeyInfo,
                                UShort       aTable,
                                UShort       aColumn,
                                idBool     * aIsAntiJoinInner )
{
    sdiKeyInfo * sKeyInfo = NULL;
    UInt         sKeyCount = 0;
    idBool       sIsFound = ID_FALSE;

    for ( sKeyInfo  = aKeyInfo;
          sKeyInfo != NULL;
          sKeyInfo  = sKeyInfo->mNext )
    {
        for ( sKeyCount = 0;
              sKeyCount < sKeyInfo->mKeyCount;
              sKeyCount++ )
        {
            if ( ( sKeyInfo->mKey[sKeyCount].mTupleId == aTable ) &&
                 ( sKeyInfo->mKey[sKeyCount].mColumn == aColumn ) )
            {
                sIsFound = ID_TRUE;

                if ( sKeyInfo->mKey[sKeyCount].mIsAntiJoinInner == ID_TRUE )
                {
                    *aIsAntiJoinInner = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }

                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        if ( sIsFound == ID_TRUE )
        {
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;
}

IDE_RC sda::addTableInfo( qcStatement      * aStatement,
                          sdiShardAnalysis * aQuerySetAnalysis,
                          sdiTableInfo     * aTableInfo )
{
    sdiTableInfoList * sTableInfoList;
    idBool             sFound = ID_FALSE;

    for ( sTableInfoList = aQuerySetAnalysis->mTableInfoList;
          sTableInfoList != NULL;
          sTableInfoList = sTableInfoList->mNext )
    {
        if ( sTableInfoList->mTableInfo->mShardID == aTableInfo->mShardID )
        {
            sFound = ID_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sFound == ID_FALSE )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiTableInfoList),
                                                 (void**) &sTableInfoList )
                  != IDE_SUCCESS );

        sTableInfoList->mTableInfo = aTableInfo;

        // link
        sTableInfoList->mNext = aQuerySetAnalysis->mTableInfoList;
        aQuerySetAnalysis->mTableInfoList = sTableInfoList;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::addTableInfoList( qcStatement      * aStatement,
                              sdiShardAnalysis * aQuerySetAnalysis,
                              sdiTableInfoList * aTableInfoList )
{
    sdiTableInfoList * sTableInfoList;

    for ( sTableInfoList = aTableInfoList;
          sTableInfoList != NULL;
          sTableInfoList = sTableInfoList->mNext )
    {
        IDE_TEST( addTableInfo( aStatement,
                                aQuerySetAnalysis,
                                sTableInfoList->mTableInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::mergeTableInfoList( qcStatement      * aStatement,
                                sdiShardAnalysis * aQuerySetAnalysis,
                                sdiShardAnalysis * aLeftQuerySetAnalysis,
                                sdiShardAnalysis * aRightQuerySetAnalysis )
{
    aQuerySetAnalysis->mTableInfoList = NULL;

    IDE_TEST( addTableInfoList( aStatement,
                                aQuerySetAnalysis,
                                aLeftQuerySetAnalysis->mTableInfoList )
              != IDE_SUCCESS );

    IDE_TEST( addTableInfoList( aStatement,
                                aQuerySetAnalysis,
                                aRightQuerySetAnalysis->mTableInfoList )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-45899 */
void sda::increaseAnalyzeCount( qcStatement * aStatement )
{
    (QC_SHARED_TMPLATE(aStatement)->stmt)->mShardPrintInfo.mAnalyzeCount++;
}

void sda::setPrintInfoFromAnalyzeInfo( sdiPrintInfo   * aPrintInfo,
                                       sdiAnalyzeInfo * aAnalyzeInfo )
{
    IDE_DASSERT( aAnalyzeInfo != NULL );

    if ( aPrintInfo->mQueryType == SDI_QUERY_TYPE_NONE )
    {
        // BUG-47247
        IDE_DASSERT( ( aAnalyzeInfo->mIsShardQuery == ID_TRUE ) ||
        		     ( ( aAnalyzeInfo->mIsShardQuery == ID_FALSE ) &&
                       ( aAnalyzeInfo->mNonShardQueryReason <= SDI_NON_SHARD_QUERY_REASON_MAX ) ) );

        aPrintInfo->mQueryType = ( aAnalyzeInfo->mIsShardQuery == ID_TRUE ) ? SDI_QUERY_TYPE_SHARD: SDI_QUERY_TYPE_NONSHARD;
        setNonShardQueryReason( aPrintInfo, aAnalyzeInfo->mNonShardQueryReason ); 
    }
}

void sda::setNonShardQueryReason( sdiPrintInfo * aPrintInfo,
                                  UShort         aReason )
{
    IDE_DASSERT( aPrintInfo != NULL );

    if ( ( aPrintInfo->mNonShardQueryReason >= SDI_NON_SHARD_QUERY_REASON_MAX ) &&
         ( aReason < SDI_NON_SHARD_QUERY_REASON_MAX ) )
    {
        aPrintInfo->mQueryType = SDI_QUERY_TYPE_NONSHARD;
        aPrintInfo->mNonShardQueryReason = aReason;
    }
}

/* TASK-7219 Shard Transformer Refactoring */
IDE_RC sda::allocAnalysis( qcStatement       * aStatement,
                           sdiShardAnalysis ** aAnalysis )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 구조체 동적할당 중복 코드 함수화
 *
 *                 코드 재사용 목적으로 구현
 *                  - sda::allocAndSetAnalysis
 *                  - sda::analyzeSelectCore
 *                  - sdo::allocAndCopyAnalysis
 *
 * Implementation : 1. sdiShardAnalysis 를 동적할당한다.
 *                  2. 초기화한다.
 *                  3. 반환한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    /* 1. sdiShardAnalysis 를 동적할당한다. */
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiShardAnalysis ),
                                               (void **) &( sAnalysis ) )
              != IDE_SUCCESS );

    /* 2. 초기화한다. */
    SDI_INIT_SHARD_ANALYSIS( sAnalysis );

    /* 3. 반환한다. */
    if ( aAnalysis != NULL )
    {
        *aAnalysis = sAnalysis;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::allocAnalysis",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getParseTreeAnalysis( qcParseTree       * aParseTree,
                                  sdiShardAnalysis ** aAnalysis )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 접근 중복 코드 함수화
 *
 *                 코드 재사용 목적으로 구현
 *                  - sda::allocAndSetAnalysis
 *                  - sdo::allocAndCopyAnalysisParseTree
 *                  - sdo::allocAndCopyAnalysisPartialStatement
 *                  - sdo::makeAndSetAnalyzeInfoFromStatement
 *                  - sdo::makeAndSetAnalyzeInfoFromParseTree
 *                  - sdo::isShardParseTree
 *                  - sdo::isShardObject
 *                  - sdo::raiseInvalidShardQueryError
 *
 * Implementation : 1. StmtKind 따라 ParseTree 형변환하여, sAnalysis 를 접근한다.
 *
 ***********************************************************************/

    qmsParseTree     * sSelect   = NULL;
    qmmInsParseTree  * sInsert   = NULL;
    qmmUptParseTree  * sUpdate   = NULL;
    qmmDelParseTree  * sDelete   = NULL;
    qsExecParseTree  * sExecProc = NULL;
    sdiShardAnalysis * sAnalysis = NULL;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    /* 1. StmtKind 따라 ParseTree 형변환하여, sAnalysis 를 접근한다. */
    switch ( aParseTree->stmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            sSelect   = (qmsParseTree *)( aParseTree );
            sAnalysis = sSelect->querySet->mShardAnalysis;
            break;

        case QCI_STMT_INSERT:
            sInsert   = (qmmInsParseTree *)( aParseTree );
            sAnalysis = sInsert->mShardAnalysis;
            break;

        case QCI_STMT_UPDATE:
            sUpdate   = (qmmUptParseTree *)( aParseTree );
            sAnalysis = sUpdate->mShardAnalysis;
            break;

        case QCI_STMT_DELETE:
            sDelete   = (qmmDelParseTree *)( aParseTree );
            sAnalysis = sDelete->mShardAnalysis;
            break;

        case QCI_STMT_EXEC_PROC:
            sExecProc = (qsExecParseTree *)( aParseTree );
            sAnalysis = sExecProc->mShardAnalysis;
            break;

        default:
            IDE_RAISE( ERR_UNEXPECTED );
            break;
    }

    /* 2.   반환한다. */
    if ( aAnalysis != NULL )
    {
        *aAnalysis = sAnalysis;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getParseTreeAnalysis",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getParseTreeAnalysis",
                                  "statement type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setParseTreeAnalysis( qcParseTree      * aParseTree,
                                  sdiShardAnalysis * aAnalysis )
{
 /***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 연결 코드 함수화
 *
 *                 코드 재사용 목적으로 구현
 *                  - sda::allocAndSetAnalysis
 *                  - sdo::allocAndCopyAnalysisParseTree
 *
 * Implementation : 1. StmtKind 따라 ParseTree 형변환하여, sAnalysis 를 연결한다.
 *
 ***********************************************************************/

    qmsParseTree    * sSelect   = NULL;
    qmmInsParseTree * sInsert   = NULL;
    qmmUptParseTree * sUpdate   = NULL;
    qmmDelParseTree * sDelete   = NULL;
    qsExecParseTree * sExecProc = NULL;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );
    IDE_TEST_RAISE( aAnalysis == NULL, ERR_NULL_ANALYSIS );

    /* 1. StmtKind 따라 ParseTree 형변환하여, sAnalysis 를 연결한다. */
    switch ( aParseTree->stmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            sSelect = (qmsParseTree *)( aParseTree );
            sSelect->mShardAnalysis = aAnalysis;
            sSelect->querySet->mShardAnalysis = aAnalysis;
            break;

        case QCI_STMT_INSERT:
            sInsert = (qmmInsParseTree *)( aParseTree );
            sInsert->mShardAnalysis = aAnalysis;
            break;

        case QCI_STMT_UPDATE:
            sUpdate = (qmmUptParseTree *)( aParseTree );
            sUpdate->mShardAnalysis = aAnalysis;
            sUpdate->querySet->mShardAnalysis = aAnalysis;
            break;

        case QCI_STMT_DELETE:
            sDelete = (qmmDelParseTree *)( aParseTree );
            sDelete->mShardAnalysis = aAnalysis;
            sDelete->querySet->mShardAnalysis = aAnalysis;
            break;

        case QCI_STMT_EXEC_PROC:
            sExecProc = (qsExecParseTree *)( aParseTree );
            sExecProc->mShardAnalysis = aAnalysis;
            break;

        default:
            IDE_RAISE( ERR_UNEXPECTED );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setParseTreeAnalysis",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setParseTreeAnalysis",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setParseTreeAnalysis",
                                  "statement type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::preAnalyzeQuerySet( qcStatement * aStatement,
                                qmsQuerySet * aQuerySet,
                                ULong         aSMN )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Partial Analyze 를 수행하는 함수.
 *
 *                 Unnesting, View Merging, CFS Optimize 로
 *                  원본 Parse Tree 의 수정 및 제외가 발생하여,
 *                   분석하지 못하고, Analysis 를 생성 못할 수 있다.
 *
 *                    이때에 해당하는 Query Set 부터 하위로 Pre Analyze 을 수행하고
 *                     Pre Analysis 를 생성한다.
 *
 *                      분석에 사용되는 함수는 일반 Analyze 함수와 동일하다.
 *
 * Implementation : 1. 분석 비용 증가
 *                  2. 분석
 *                  3. 분석 비용 증가
 *                  4. SubKey 분석
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. 분석 비용 증가 */
    increaseAnalyzeCount( aStatement );

    /* 2. 분석 */
    IDE_TEST( analyzeQuerySet( aStatement,
                               aSMN,
                               aQuerySet,
                               NULL,
                               ID_FALSE )
              != IDE_SUCCESS );

    if ( aQuerySet->mShardAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
    {
        /* 3. 분석 비용 증가 */
        increaseAnalyzeCount( aStatement );

        /* 4. SubKey 분석 */
        IDE_TEST( analyzeQuerySet( aStatement,
                                   aSMN,
                                   aQuerySet,
                                   NULL,
                                   ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::preAnalyzeQuerySet",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::analyzePartialQuerySet",
                                  "queryset is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::allocAndSetAnalysis( qcStatement       * aStatement,
                                 qcParseTree       * aParseTree,
                                 qmsQuerySet       * aQuerySet,
                                 idBool              aIsSubKey,
                                 sdiShardAnalysis ** aAnalysis )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                생성한 Analysis 를 연결하는 함수
 *
 *                 새로운 Analysis 은 setAnalysis 로 연결하며
 *                  SubKey 에 해당하는 경우는 Main Analysis 찾아서 연결,
 *                   Partial Analysis 인 경우는 mPreAnalysis 에 연결한다.
 *
 *                    코드 재사용 목적으로 구현
 *                     - sda::analyzeSFWGH
 *                     - sda::analyzeInsertCore
 *                     - sda::analyzeUpdateCore
 *                     - sda::analyzeDeleteCore
 *                     - sda::analyzeExecProcCore
 *
 * Implementation : 1. Analysis 생성
 *                  2. Analysis 연결
 *                  3. Partial Analyze 중이면 mPreAnalysis 에 백업
 *                  4. SubKey 인 경우, Main Analysis 를 찾음
 *                  5. Main 에 SubKey Analysis 연결
 *                  6. Partial Analyze 중이면 mPreAnalysis 에 백업
 *                  7. 생성한 Analysis 반환
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis = NULL;
    sdiShardAnalysis * sMain     = NULL;

    /* 1. Analysis 생성 */
    IDE_TEST( allocAnalysis( aStatement,
                             &( sAnalysis ) )
              != IDE_SUCCESS );

    if ( aIsSubKey == ID_FALSE )
    {
        /* 2. Analysis 연결 */
        if ( aQuerySet == NULL )
        {
            IDE_TEST( setParseTreeAnalysis( aParseTree,
                                            sAnalysis )
                      != IDE_SUCCESS );
        }
        else
        {
            aQuerySet->mShardAnalysis = sAnalysis;

            /* 3. Partial Analyze 중이면 mPreAnalysis 에 백업 */
            if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                SDI_QUERYSET_LIST_STATE_DUMMY_PRE_ANALYZE )
                 == ID_TRUE )
            {
                aQuerySet->mPreAnalysis = sAnalysis;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        if ( aQuerySet == NULL )
        {
            /* 4. SubKey 인 경우, Main Analysis 를 찾음 */
            IDE_TEST( getParseTreeAnalysis( aParseTree,
                                            &( sMain ) )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sMain == NULL, ERR_NULL_SHARD_ANALYSIS_1 );

            /* 5. Main 에 SubKey Analysis 연결 */
            sMain->mNext = sAnalysis;
        }
        else
        {
            /* 4. SubKey 인 경우, Main Analysis 를 찾음 */
            sMain = aQuerySet->mShardAnalysis;

            IDE_TEST_RAISE( sMain == NULL, ERR_NULL_SHARD_ANALYSIS_2 );

            /* 5. Main 에 SubKey Analysis 연결 */
            sMain->mNext = sAnalysis;

            /* 6. Partial Analyze 중이면 mPreAnalysis 에 백업 */
            if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                                SDI_QUERYSET_LIST_STATE_DUMMY_PRE_ANALYZE )
                 == ID_TRUE )
            {
                aQuerySet->mPreAnalysis->mNext = sAnalysis;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    /* 7. 생성한 Analysis 반환 */
    if ( aAnalysis != NULL )
    {
        *aAnalysis = sAnalysis;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SHARD_ANALYSIS_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::allocAndSetAnalysis",
                                  "parse tree analysis is null.") );
    }
    IDE_EXCEPTION( ERR_NULL_SHARD_ANALYSIS_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::allocAndSetAnalysis",
                                  "query set analysis is null.") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::getShardPredicate( qcStatement * aStatement,
                               qmsQuerySet * aQuerySet,
                               idBool        aIsSelect,
                               idBool        aIsSubKey,
                               qtcNode    ** aPredicate )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Query Set 에 Unnesting, View Merging, CFS Optimize 로 변환하는 경우,
 *                 변환 전에 Partial Analyze 를 수행할 수 있으며,
 *                  이때에 ANSI Transform 을 이미 수행하였을 수 있다.
 *
 *                   ANSI Transform 은 Where 절 CNF 형태의 Predicate 를 가지고,
 *                    From 과 OnCondition 대상을 ANSI Style 로 재구성하고
 *                     Transform 을 야기한 Predicate 는 그대로 유지한다.
 *
 *                      따라서 Partial Analyze 이후 Analyze 재수행으로
 *                       다시 ANSI Transform 을 시도할 수 있고, 이때에 Validate 을 실패한다.
 *
 *                         이 함수는 Partial Analyze 으로 발생하는 Validate 실패 방지하기 위해서
 *                          첫 수행 때 SFWGH Flag, 이후에 확인하여 ANSI Transform 을 우회한다.
 *
 * Implementation : 1. Where 절을 CNF Predicate 으로 변환한다.
 *                  2. Select 이고 Predicate 대상이 있다면, ANSI Transform 을 시도한다.
 *                  3. 이미 ANSI Transform 을 수행하였다면, 무시한다.
 *                  4. ANSI Transform 을 수행한다.
 *                  5. Outer Join 을 제거한다.
 *
 ***********************************************************************/

    qtcNode * sShardPredicate = NULL;
    idBool    sChanged        = ID_FALSE;

    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );
    IDE_TEST_RAISE( aQuerySet->mShardAnalysis == NULL, ERR_NULL_ANALYSIS );

    /* Normalize Where and Transform2ANSIJoin */
    if ( aIsSubKey == ID_FALSE )
    {
        /* 1. Where 절을 CNF Predicate 으로 변환한다. */
        IDE_TEST( normalizePredicate( aStatement,
                                      aQuerySet,
                                      &( sShardPredicate ) )
                  != IDE_SUCCESS );

        /* 2. Select 이고 Predicate 대상이 있다면, ANSI Transform 을 시도한다. */
        if ( ( aIsSelect == ID_TRUE )
             &&
             ( sShardPredicate != NULL ) )
        {
            IDE_TEST_RAISE( aQuerySet->SFWGH == NULL, ERR_NULL_SFWGH );

            /* 3. 이미 ANSI Transform 을 수행하였다면, 무시한다. */
            if ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_SHARD_ANSI_TRANSFORM_MASK )
                 == QMV_SFWGH_SHARD_ANSI_TRANSFORM_FALSE )
            {
                /* 4. ANSI Transform 을 수행한다. */
                if ( ( sShardPredicate->lflag & QTC_NODE_JOIN_OPERATOR_MASK )
                     == QTC_NODE_JOIN_OPERATOR_EXIST )
                {
                    IDE_TEST( qmoOuterJoinOper::transform2ANSIJoin( aStatement,
                                                                    aQuerySet,
                                                                    &( sShardPredicate ) )
                              != IDE_SUCCESS );

                    aQuerySet->SFWGH->lflag &= ~QMV_SFWGH_SHARD_ANSI_TRANSFORM_MASK;
                    aQuerySet->SFWGH->lflag |= QMV_SFWGH_SHARD_ANSI_TRANSFORM_TRUE;
                }
                else
                {
                    /* Nothing to do. */
                }

                /* 5. Outer Join 을 제거한다.
                 *  BUG-38375 Outer Join Elimination OracleStyle Join
                 */
                IDE_TEST( qmoOuterJoinElimination::doTransform( aStatement,
                                                                aQuerySet,
                                                                &( sChanged ) )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }
        }
        else
        {
            /* Nothing to do. */
        }

        aQuerySet->mShardAnalysis->mShardPredicate = sShardPredicate;
    }
    else
    {
        sShardPredicate = aQuerySet->mShardAnalysis->mShardPredicate;
    }

    if ( aPredicate != NULL )
    {
        *aPredicate = sShardPredicate;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getShardPredicate",
                                  "queryset is NULL") );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getShardPredicate",
                                  "analysis is NULL") );
    }
    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::getShardPredicate",
                                  "sfwgh is NULL") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::checkAndGetShardObjInfo( ULong             aSMN,
                                     sdiAnalysisFlag * aAnalysisFlag,
                                     sdiObjectInfo   * aShardObjList,
                                     idBool            aIsSelect,
                                     idBool            aIsSubKey,
                                     sdiObjectInfo  ** aShardObjInfo )
{
 /***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                ShardObjInfo 검사하고 가져오는 함수
 *
 *                 적절한 Info를 찾지 못하면, NonShardQueryReason, Flag 를 설정
 *
 *                  코드 재사용 목적으로 구현
 *                   - sda::makeShardFromInfo4Table
 *                   - sda::setDMLCommonAnalysis
 *
 *                   설정하는 Flag 목록
 *                    - SDI_NON_SHARD_OBJECT_TARGET_DML_EXISTS
 *
 * Implementation : 1. aShardObjList 검사
 *                  2. aShardObjList 에서 sShardObjInfo 찾기
 *                  3. sShardObjInfo 검사
 *                  4. 적절한 정보가 없을시, Select 구문 외에는 Flag 설정
 *                  5. 정보에 오류가 있을시, Select 구문 외에는 NonShardQueryReason 를 설정
 *                  6. 찾은 sShardObjInfo 반환
 *
 ***********************************************************************/

    sdiObjectInfo * sShardObjInfo = NULL;

    IDE_TEST_RAISE( aAnalysisFlag == NULL, ERR_NULL_FLAG );

    /* 1. aShardObjList 검사 */
    if ( aShardObjList == NULL )
    {
        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_OBJECT_EXISTS ] = ID_TRUE;

        if ( aIsSelect == ID_FALSE )
        {
            aAnalysisFlag->mCurQueryFlag[ SDI_CQ_LOCAL_TABLE ] = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* 2. aShardObjList 에서 sShardObjInfo 찾기 */
        sdi::getShardObjInfoForSMN( aSMN,
                                    aShardObjList,
                                    &( sShardObjInfo ) );

        /* 3. sShardObjInfo 검사 */
        if ( sShardObjInfo == NULL )
        {
            aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_OBJECT_EXISTS ] = ID_TRUE;

            /* 4. 적절한 정보가 없을시, Select 구문 외에는 Flag 설정 */
            if ( aIsSelect == ID_FALSE )
            {
                aAnalysisFlag->mCurQueryFlag[ SDI_CQ_LOCAL_TABLE ] = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* 5. 정보에 오류가 있을시, Select 구문 외에는 NonShardQueryReason 를 설정 */
            if ( ( aIsSelect == ID_FALSE )
                 &&
                 ( aIsSubKey == ID_TRUE )
                 &&
                 ( sShardObjInfo->mTableInfo.mSubKeyExists == ID_FALSE ) )
            {
                aAnalysisFlag->mNonShardFlag[ SDI_MULTI_SHARD_INFO_EXISTS ] = ID_TRUE;
                aAnalysisFlag->mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] = ID_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    /* 6. 찾은 sShardObjInfo 반환 */
    if ( aShardObjInfo != NULL )
    {
        *aShardObjInfo = sShardObjInfo;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::checkAndGetShardObjInfo",
                                  "analysis flag is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag( qmsQuerySet * aQuerySet,
                             idBool        aIsSubKey )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                변환에서 이용할 Flag 를 설정하는 함수
 *
 *                 이후에 Flag 설정에 따라서 Non-Shard 처리를 수행한다.
 *
 *                  설정하는 Flag 목록
 *                   - SDI_SQ_OUT_DEP_INFO
 *                   - SDI_SQ_NON_SHARD
 *                   - SDI_SQ_UNSUPPORTED
 *                   - SDI_CQ_AGGR_TRANSFORMABLE
 *
 * Implementation : 1. Main, SubKey Reason 결정
 *                  2. Outer Dependency 조건 검사
 *                  3. Shard Query Set 조건 검사
 *                  4. Grouping Set 인 경우
 *                  5. Pivot, Unpivot 인 경우
 *                  6. From Flag 조건 검사
 *                  7. TransformAble 조건 검사
 *
 ***********************************************************************/

    sdiAnalysisFlag * sAnalysisFlag = NULL;
    idBool            sIsShardQuery = ID_FALSE;

    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. Main, SubKey Reason 결정 */
    if ( aIsSubKey == ID_FALSE )
    {
        sAnalysisFlag = &( aQuerySet->mShardAnalysis->mAnalysisFlag );
    }
    else
    {
        sAnalysisFlag = &( aQuerySet->mShardAnalysis->mNext->mAnalysisFlag );
    }

    /* 2. Outer Dependency 조건 검사
     *  SELECT I1
     *   FROM T1 TA
     *    WHERE EXISTS ( SELECT I2 FROM T1 TB WHERE TA.I2 = TB.I1 );
     *                                              *************
     *                                                    \
     *                                             Outer Dependency
     */
    if ( qtc::haveDependencies( &( aQuerySet->outerDepInfo ) ) == ID_TRUE )
    {
        sAnalysisFlag->mSetQueryFlag[ SDI_SQ_OUT_DEP_INFO ]       = ID_TRUE;
        sAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ]          = ID_TRUE;
        sAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    /* 3. Shard Query Set 조건 검사 */
    if ( sAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ] == ID_FALSE )
    {
        IDE_TEST( isShardQuery( sAnalysisFlag,
                                &( sIsShardQuery ) )
                  != IDE_SUCCESS );

        if ( sIsShardQuery == ID_FALSE )
        {
            sAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ] = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do. */
    }

    if ( aQuerySet->setOp == QMS_NONE )
    {
        IDE_TEST_RAISE( aQuerySet->SFWGH == NULL, ERR_NULL_SFWGH );

        /* 4. Grouping Set 인 경우
         *  Grouping Set Group By 절 뒤로 작성되어 있지만,
         *   Parse 단계에 해당 구조가 Statement 로 변환되고,
         *    Analyze, Transform 는 Parse 이후의 결과물로 접근하기 때문에
         *     Query Set 내 Grouping Set 존재 여부를 알 수 없다.
         *
         *      Grouping Set 전체가 Shard 로 판단되면 문제없으나,
         *       변환된 Statement 부분이 Shard 로 판단될 때에는
         *        Shard Query 로 변환할 Query String 이 없다.
         *
         *         따라서 From 절에 새로운 Flag 를 구성하고
         *          해당 구조가 있었는지 Parse 단계에 Flag 를 남기어
         *           이곳에서 Flag 를 Analysis 에 전달한 후
         *            Transform 시 Non-Shard 처리하도록 한다.
         */
        if ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_GBGS_TRANSFORM_MASK ) == QMV_SFWGH_GBGS_TRANSFORM_BOTTOM_MTR )
        {
            sAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_GROUPING_SET_EXISTS ] = ID_TRUE;
            sAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ]                  = ID_TRUE;
            sAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ]         = ID_FALSE;
        }
        else
        {
            /* Nothing to do. */
        }

        /* 5. Pivot, Unpivot 인 경우
         *  Pivot 과 Unpivot 도 Grouping Set 와 동일한 이유로
         *   Parse Tree 로 식별하기 어렵다.
         *
         *    다행히 사용 목적 때문에 변환된 Statement 는 Non-Shard 로 판단된다.
         *
         *     하지만 변환으로 생성된 Group By 로 인해 TransformAble Query 로 판단하여
         *      원본 Query String 부분을 Shard Query 화 하려고 시도한다.
         *
         *       이때에 Parse Tree 에 없으나, Query String 에 있는
         *        Pivot 과 Unpivot 를 제외할 필요가 있다.
         *
         *         위에 언급한 것과 같이 식별할 수 없는데,
         *          Query String 내 정확한 위치를 찾아
         *           제외하는 것은 식별하는 것보다 더욱 어렵다.
         *
         *            따라서 Non-Shard 처리를 하도록 Flag 를 전달한다.
         */
        if ( ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_PIVOT_MASK ) == QMV_SFWGH_PIVOT_TRUE )
             ||
             ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_UNPIVOT_MASK ) == QMV_SFWGH_UNPIVOT_TRUE ) )
        {
            sAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ]          = ID_TRUE;
            sAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_FALSE;
        }
        else
        {
            /* Nothing to do. */
        }

        /*  6. From Flag 조건 검사
         *   SDI_CQ_AGGR_TRANSFORMABLE 값이 수정될 수 있으므로,
         *    setAnalysisFlag4TransformAble 이전에 수행한다.
         */
        IDE_TEST( setAnalysisFlag4From( aQuerySet->SFWGH->from,
                                        sAnalysisFlag )
                  != IDE_SUCCESS );

        /* 7. TransformAble 조건 검사 */
        if ( sAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] == ID_TRUE )
        {
            IDE_TEST( setAnalysisFlag4TransformAble( sAnalysisFlag )
                      != IDE_SUCCESS );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag",
                                  "queryset is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4From( qmsFrom         * aFrom,
                                  sdiAnalysisFlag * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                From Flag 를 검사하고 Flag 설정하는 함수
 *
 *                 이후에 Flag 설정에 따라서 Non-Shard 처리를 수행한다.
 *
 *                  설정하는 Flag 목록
 *                   - SDI_NON_SHARD_QUERYSET
 *                   - SDI_UNSUPPORTED_QUERY
 *                   - SDI_CQ_TRANSFORMABLE
 *
 * Implementation : 1. Join 의 경우, Left, Right 로 재귀
 *                  2. With Statement 인 경우
 *
 ***********************************************************************/

    qmsFrom * sFrom = NULL;

    for ( sFrom  = aFrom;
          sFrom != NULL;
          sFrom  = sFrom->next )
    {
        /* 1. Join 의 경우, Left, Right 로 재귀 */
        if ( sFrom->joinType != QMS_NO_JOIN )
        {
            IDE_TEST( setAnalysisFlag4From( sFrom->left,
                                            aAnalysisFlag )
                      != IDE_SUCCESS );

            IDE_TEST( setAnalysisFlag4From( sFrom->right,
                                            aAnalysisFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( aAnalysisFlag == NULL, ERR_NULL_FLAG );

            /* 2. With Statement 인 경우
             *  With Statement 도 Grouping Set 와 동일한 이유를 가진다.
             *
             *   With Alias 를 포함해서 Shard 로 판단될 때에 문제되고
             *    원본 Query String 의 With Alias 를 With Statement 로 대체할 필요가 있다.
             *
             *     다중 With 구문과 Recursive With, Same View Reference 까지 고려해야 하기 때문에,
             *      With Alias 를 포함한 Shard View 변환을 지원하지 않고,
             *       Non-Shard 처리를 하도록 Flag 를 전달한다.
             */
            if ( sFrom->tableRef->withStmt != NULL )
            {
                aAnalysisFlag->mSetQueryFlag[ SDI_SQ_UNSUPPORTED ]        = ID_TRUE;
                aAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ]          = ID_TRUE;
                aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_FALSE;
            }
            else
            {
                /* Nothing to do. */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4From",
                                  "analysis flag is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4TransformAble( sdiAnalysisFlag * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Transform Able 조건을 검사하는 함수
 *
 *                 TransformAble 속성은 Query Set 분석이 완료되어 있을 때에 확정할 수 있다.
 *                  1. Shard QuerySet 속성이 아니여야 하고
 *                  2. 하위 View 또는 QuerySet 에는 SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS = FALSE
 *                  3. 현재 QuerySet 에는 SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS = TRUE
 *
 *                   이때에 SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS 을 미리 검사하고,
 *                    1번, 2번은 setAnalysisFlag 를 수행하면서,
 *                     3번은 이 함수에서 검사한다.
 *
 *                      SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS 조건을 검사하는 함수
 *                       - sda::setAnalysisFlag4SFWGH
 *                       - sda::setAnalysisFlag4GroupBy
 *
 * Implementation : 1. SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS 외 조건인 경우 불가능하다.
 *
 ***********************************************************************/
    UShort sIdx = 0;

    IDE_TEST_RAISE( aAnalysisFlag == NULL, ERR_NULL_FLAG );

    /* 1. SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS 외 조건인 경우 불가능하다. */
    for ( sIdx = 0;
          sIdx < SDI_NON_SHARD_QUERY_REASON_MAX;
          sIdx++ )
    {
        if ( aAnalysisFlag->mNonShardFlag[ sIdx ] == ID_TRUE )
        {
            if ( sIdx == SDI_NODE_TO_NODE_GROUP_AGGREGATION_EXISTS )
            {
                /* Nothing to do */
            }
            else
            {
                break;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sIdx == SDI_NON_SHARD_QUERY_REASON_MAX )
    {
        aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_TRUE;
    }
    else
    {
        aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4TransformAble",
                                  "analysis flag is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4Target( qcStatement     * aStatement,
                                    qmsTarget       * aTarget,
                                    sdiAnalysisFlag * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Target 절의 Non-Shard 조건을 검사하는 함수
 *
 *                 PSM 변수나, LOB Colunm 를 포함한 Shard Query String 을 지원할 수 없기 때문에,
 *                  여기서 검사하여 Non-Shard 처리를 하도록 Flag를 전달한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsTarget * sTarget       = NULL;
    qtcNode   * sTargetColumn = NULL;

    for ( sTarget  = aTarget;
          sTarget != NULL;
          sTarget  = sTarget->next )
    {
        sTargetColumn = sTarget->targetColumn;

        IDE_TEST( setAnalysisFlag4PsmBind( sTargetColumn,
                                           aAnalysisFlag )
                  != IDE_SUCCESS );

        IDE_TEST( setAnalysisFlag4PsmLob( aStatement,
                                          sTargetColumn,
                                          aAnalysisFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4OrderBy( qmsSortColumns  * aOrderBy,
                                     sdiAnalysisFlag * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Order By 절의 Non-Shard 조건을 검사하는 함수
 *
 *                 PSM 변수를 포함한 Shard Query String 을 지원할 수 없기 때문에,
 *                  여기서 검사하여 Non-Shard 처리를 하도록 Flag를 전달한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSortColumns * sOrderBy    = NULL;
    qtcNode        * sSortColumn = NULL;

    if ( aOrderBy != NULL )
    {
        IDE_TEST_RAISE( aAnalysisFlag == NULL, ERR_NULL_FLAG );

        aAnalysisFlag->mNonShardFlag[ SDI_NODE_TO_NODE_ORDER_BY_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    for ( sOrderBy  = aOrderBy;
          sOrderBy != NULL;
          sOrderBy  = sOrderBy->next )
    {
        sSortColumn = sOrderBy->sortColumn;

        IDE_TEST( setAnalysisFlag4PsmBind( sSortColumn,
                                           aAnalysisFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4OrderBy",
                                  "analysis flag is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4GroupPsmBind( qmsConcatElement * aGroupBy,
                                          sdiAnalysisFlag  * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Group By 절의 Non-Shard 조건을 검사하는 함수
 *
 *                 PSM 변수를 포함한 Shard Query String 을 지원할 수 없기 때문에,
 *                  여기서 검사하여 Non-Shard 처리를 하도록 Flag를 전달한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsConcatElement * sGroupBy     = NULL;
    qtcNode          * sGroupColumn = NULL;

    for ( sGroupBy  = aGroupBy;
          sGroupBy != NULL;
          sGroupBy  = sGroupBy->next )
    {
        if ( sGroupBy->type == QMS_GROUPBY_NORMAL )
        {
            sGroupColumn = sGroupBy->arithmeticOrList;

            IDE_TEST( setAnalysisFlag4PsmBind( sGroupColumn,
                                               aAnalysisFlag )
                      != IDE_SUCCESS );
        }
        else
        {
            /* setAnalysisFlag4GroupBy 에서 이미 처리함 */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4PsmBind( qtcNode         * aNode,
                                     sdiAnalysisFlag * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                PSM 변수인지 검사하는 함수
 *
 *                 PSM 변수를 포함한 Shard Query String 을 지원할 수 없기 때문에,
 *                  여기서 검사하여 Non-Shard 처리를 하도록 Flag를 전달한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode * sNode = NULL;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aAnalysisFlag == NULL, ERR_NULL_FLAG );

    sNode = aNode;

    while ( sNode->node.module == &( qtc::passModule ) )
    {
        sNode = (qtcNode *)( sNode->node.arguments );
    }

    if ( MTC_NODE_IS_DEFINED_TYPE( &( sNode->node ) ) == ID_FALSE )
    {
        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_PSM_BIND_EXISTS ] = ID_TRUE;
        aAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ]              = ID_TRUE;
        aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ]     = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4PsmBind",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4PsmBind",
                                  "aAnalysisFlag is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sda::setAnalysisFlag4PsmLob( qcStatement     * aStatement,
                                    qtcNode         * aNode,
                                    sdiAnalysisFlag * aAnalysisFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Lob Column 인지 검사하는 함수
 *
 *                 Lob Column 를 포함한 Shard Query String 을 지원할 수 없기 때문에,
 *                  여기서 검사하여 Non-Shard 처리를 하도록 Flag를 전달한다.
 *
 * Implementation :
 *
***********************************************************************/

    mtcColumn * sMtcColumn = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aAnalysisFlag == NULL, ERR_NULL_FLAG );

    sMtcColumn = QTC_STMT_COLUMN( aStatement, aNode );

    if ( ( sMtcColumn->module->id == MTD_BLOB_ID )
         ||
         ( sMtcColumn->module->id == MTD_CLOB_ID ) )
    {
        aAnalysisFlag->mNonShardFlag[ SDI_NON_SHARD_PSM_LOB_EXISTS ] = ID_TRUE;
        aAnalysisFlag->mSetQueryFlag[ SDI_SQ_NON_SHARD ]             = ID_TRUE;
        aAnalysisFlag->mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ]    = ID_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4PsmLob",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4PsmLob",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_FLAG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sda::setAnalysisFlag4PsmLob",
                                  "analysis flag is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
