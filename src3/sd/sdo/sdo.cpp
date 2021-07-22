/*
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
 *  the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/***********************************************************************
 * $Id: sdo.cpp 90252 2021-03-18 06:46:29Z andrew.shin $
 **********************************************************************/

#include <sda.h>
#include <sdi.h>
#include <sdo.h>
#include <qcg.h>
#include <qmo.h>
#include <qsvEnv.h>

IDE_RC sdo::allocAndInitQuerySetList( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Query Set List 초기화 함수
 *
 *                 Query Set List 에는 현재 Query String 으로 구성된,
 *                  Statement 내 존재하는 모든 Query Set 을 연결한 구조이다.
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiQuerySetList * sList = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiQuerySetList ),
                                               (void **)&( sList ) )
              != IDE_SUCCESS );

    sList->mState = SDI_QUERYSET_LIST_STATE_MAIN_MAKE;
    sList->mCount = 0;
    sList->mHead  = NULL;
    sList->mTail  = NULL;

    aStatement->mShardQuerySetList = sList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndInitQuerySetList",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetQuerySetList( qcStatement * aStatement,
                                    qmsQuerySet * aQuerySet )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Query Set List 구성 함수
 *
 *                 List 는 Parse 단계에서 구성되며, 경우에 따라 동작이 다르다.
 *                  1. With 정의 시점의 Query Set 은 구성에서 제외
 *                  2. Shard Keyword 사용한 Query Set 은 구성에서 제외
 *
 *                   구성 시점의 함수
 *                    - qmv::parseViewInQuerySet
 *
 * Implementation : 1. SDI_QUERYSET_LIST_STATE_MAIN_SKIP, DUMMY_SKIP 단계에는 구성에서 제외한다.
 *                  2. Query Set List 를 동적할당, 초기화 한다.
 *                  3. List 를 구성한다.
 *
 ***********************************************************************/

    sdiQuerySetEntry * sEntry = NULL;
    sdiQuerySetList  * sList  = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );
    IDE_TEST_RAISE( aStatement->mShardQuerySetList == NULL, ERR_NULL_LIST );

    sList = aStatement->mShardQuerySetList;

    /* 1. SDI_QUERYSET_LIST_STATE_MAIN_SKIP, DUMMY_SKIP 단계에는 구성에서 제외한다. */
    IDE_TEST_CONT( ( SDI_CHECK_QUERYSET_LIST_STATE( sList,
                                                    SDI_QUERYSET_LIST_STATE_MAIN_SKIP )
                     == ID_TRUE )
                   ||
                   ( SDI_CHECK_QUERYSET_LIST_STATE( sList,
                                                    SDI_QUERYSET_LIST_STATE_DUMMY_SKIP )
                     == ID_TRUE ),
                   NORMAL_EXIT );

    /* 2. Query Set List 를 동적할당, 초기화 한다. */
    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiQuerySetEntry ),
                                               (void **)&( sEntry ) )
              != IDE_SUCCESS );

    sEntry->mId       = sList->mCount;
    sEntry->mQuerySet = aQuerySet;
    sEntry->mNext     = NULL;
    sList->mCount     = sList->mCount + 1;

    /* 3. List 를 구성한다. */
    if ( sList->mTail == NULL )
    {
        sList->mHead = sEntry;
        sList->mTail = sEntry;
    }
    else
    {
        sList->mTail->mNext = sEntry;
        sList->mTail        = sEntry;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetQuerySetList",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetQuerySetList",
                                  "queryset is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetQuerySetList",
                                  "list is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::setQuerySetListState( qcStatement  * aStatement,
                                  qmsParseTree * aParseTree,
                                  idBool       * aIsChanged )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Query Set List 상태를 SKIP 단계로 변경한다.
 *
 * Implementation : 1. MAKE 단계에만 변경이 가능하다.
 *                  2. Shard Keyword 사용을 검사한다.
 *                  3. SKIP 단계로 변경한다.
 *
 ***********************************************************************/

    qcShardStmtType      sStmtType  = QC_STMT_SHARD_NONE;
    sdiQuerySetList    * sList      = NULL;
    sdiQuerySetListState sState     = SDI_QUERYSET_LIST_STATE_MAX;
    idBool               sIsChanged = ID_FALSE;
    idBool               sNeedCheck = ID_FALSE;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStatement->mShardQuerySetList == NULL, ERR_NULL_LIST );

    sList = aStatement->mShardQuerySetList;

    /* 1. MAKE 단계에만 변경이 가능하다. */
    if ( SDI_CHECK_QUERYSET_LIST_STATE( sList,
                                        SDI_QUERYSET_LIST_STATE_MAIN_MAKE )
         == ID_TRUE )
    {
        sNeedCheck = ID_TRUE;
        sState     = SDI_QUERYSET_LIST_STATE_MAIN_SKIP;
    }
    else if ( SDI_CHECK_QUERYSET_LIST_STATE( sList,
                                             SDI_QUERYSET_LIST_STATE_DUMMY_MAKE )
              == ID_TRUE )
    {
        sNeedCheck = ID_TRUE;
        sState     = SDI_QUERYSET_LIST_STATE_DUMMY_SKIP;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sNeedCheck == ID_TRUE )
    {
        if ( aParseTree != NULL )
        {
            sStmtType  = aParseTree->common.stmtShard;

            /* 2. Shard Keyword 사용을 검사한다. */
            if ( ( sStmtType == QC_STMT_SHARD_DATA )
                 ||
                 ( sStmtType == QC_STMT_SHARD_META ) )
            {
                /* 3. SKIP 단계로 변경한다. */
                SDI_SET_QUERYSET_LIST_STATE( sList, sState );

                sIsChanged = ID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* 3. SKIP 단계로 변경한다. */
            SDI_SET_QUERYSET_LIST_STATE( sList, sState );

            sIsChanged = ID_TRUE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aIsChanged != NULL )
    {
        *aIsChanged = sIsChanged;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setQuerySetListState",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setQuerySetListState",
                                  "list is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::unsetQuerySetListState( qcStatement * aStatement,
                                    idBool        aIsChanged )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Query Set List 상태의 Keyword 를 원복
 *
 * Implementation : 1. SKIP 단계에만 변경이 가능하다.
 *                  2. MAKE 단계로 변경한다.
 *
 ***********************************************************************/

    sdiQuerySetList * sList = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStatement->mShardQuerySetList == NULL, ERR_NULL_LIST );

    if ( aIsChanged == ID_TRUE )
    {
        sList = aStatement->mShardQuerySetList;

        /* 1. SKIP 단계에만 변경이 가능하다. */
        if ( SDI_CHECK_QUERYSET_LIST_STATE( sList,
                                            SDI_QUERYSET_LIST_STATE_MAIN_SKIP )
             == ID_TRUE )
        {
            /* 2. MAKE 단계로 변경한다. */
            SDI_SET_QUERYSET_LIST_STATE( sList,
                                         SDI_QUERYSET_LIST_STATE_MAIN_MAKE );
        }
        else if ( SDI_CHECK_QUERYSET_LIST_STATE( sList,
                                                 SDI_QUERYSET_LIST_STATE_DUMMY_SKIP )
             == ID_TRUE )
        {
            /* 2. MAKE 단계로 변경한다. */
            SDI_SET_QUERYSET_LIST_STATE( sList,
                                         SDI_QUERYSET_LIST_STATE_DUMMY_MAKE );
        }
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::unsetQuerySetListState",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::unsetQuerySetListState",
                                  "list is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::setStatementFlagForShard( qcStatement * aStatement,
                                      UInt          aFlag )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Statement Flag 함수
 *
 *                 Shard Object 유무와 Shard Keyword 사용 유무를 기록한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    aStatement->mFlag |= ( aFlag & QC_STMT_SHARD_OBJ_MASK );
    aStatement->mFlag |= ( aFlag & QC_STMT_SHARD_KEYWORD_MASK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setStatementFlagForShard",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::isTransformNeeded( qcStatement * aStatement,
                               idBool      * aIsTransformNeeded )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard 분석 조건을 검사하는 함수
 *
 * Implementation : 1. 필수 조건
 *                  2. 미지원 Shard Keyword 조건
 *                  3. Shard 제약 조건
 *                  4. Statement Type 제약 조건
 *
 ***********************************************************************/

    qciStmtType      sStmtType          = aStatement->myPlan->parseTree->stmtKind;
    qcShardStmtType  sPrefixType        = aStatement->myPlan->parseTree->stmtShard;
    idBool           sIsTransformNeeded = ID_TRUE;
    qcuSqlSourceInfo sqlInfo;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    /* 1. 필수 조건 */
    if ( ( aStatement->spvEnv->createPkg == NULL )
         &&
         ( aStatement->spvEnv->createProc == NULL )
         &&
         ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                          SDI_QUERYSET_LIST_STATE_MAIN_MAKE )
           == ID_TRUE ) )
    {
        /* 2. 미지원 Shard Keyword 조건 */
        switch( sPrefixType )
        {
            case QC_STMT_SHARD_NONE :
                if ( ( aStatement->mFlag & QC_STMT_SHARD_KEYWORD_MASK )
                     == QC_STMT_SHARD_KEYWORD_DUPLICATE )
                {
                    sda::setNonShardQueryReason( &( aStatement->mShardPrintInfo ),
                                                 SDI_SHARD_KEYWORD_EXISTS );

                    IDE_RAISE( ERR_SHARD_PREFIX_EXISTS );
                }
                else
                {
                    /* Nothing to do */
                }

                break;

            default :
                if ( ( aStatement->mFlag & QC_STMT_SHARD_KEYWORD_MASK )
                     != QC_STMT_SHARD_KEYWORD_NO_USE )
                {
                    sda::setNonShardQueryReason( &( aStatement->mShardPrintInfo ),
                                                 SDI_SHARD_KEYWORD_EXISTS );

                    IDE_RAISE( ERR_SHARD_PREFIX_EXISTS );
                }
                else
                {
                    /* Nothing to do */
                }

                break;
        }

        /* 3. Shard 제약 조건 */
        switch( sPrefixType )
        {
            case QC_STMT_SHARD_NONE :
                if ( ( aStatement->mFlag & QC_STMT_SHARD_OBJ_MASK )
                     != QC_STMT_SHARD_OBJ_EXIST )
                {
                    sda::setNonShardQueryReason( &( aStatement->mShardPrintInfo ),
                                                 SDI_NON_SHARD_OBJECT_EXISTS );

                    sIsTransformNeeded = ID_FALSE;

                    IDE_CONT( NORMAL_EXIT );
                }
                else
                {
                    /* Nothing to do */
                }

                break;

            case QC_STMT_SHARD_META :
                sda::setNonShardQueryReason( &( aStatement->mShardPrintInfo ),
                                             SDI_SHARD_KEYWORD_EXISTS );

                sIsTransformNeeded = ID_FALSE;

                IDE_CONT( NORMAL_EXIT );

                break;

            default :
                break;
        }

        /* 4. Statement Type 제약 조건 */
        switch( sStmtType )
        {
            case QCI_STMT_SELECT :
            case QCI_STMT_SELECT_FOR_UPDATE :
            case QCI_STMT_INSERT :
            case QCI_STMT_UPDATE :
            case QCI_STMT_DELETE :
            case QCI_STMT_EXEC_PROC :
                break;

            case QCI_STMT_EXEC_FUNC:
                sIsTransformNeeded = ID_FALSE;

                break;

            default :
                sqlInfo.setSourceInfo( aStatement, &( aStatement->myPlan->parseTree->stmtPos ) );

                IDE_RAISE( ERR_UNSUPPORTED_STMT );
                break;
        }
    }
    else
    {
        sIsTransformNeeded = ID_FALSE;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    if ( aIsTransformNeeded != NULL )
    {
        *aIsTransformNeeded = sIsTransformNeeded;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::IsTransformNeeded",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_STMT )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Unsupported statement type",
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
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

IDE_RC sdo::makeAndSetAnalysis( qcStatement * aSrcStatement,
                                qcStatement * aDstStatement,
                                qmsQuerySet * aDstQuerySet )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                분석 결과인 Analysis 정보를 생성하는 함수
 *
 *                 Statement 내 Query Set 마다 Analysis 정보를 생성하거나
 *                  전달 받은 Query Set 의 Analysis 정보만 생성한다.
 *
 * Implementation : 1. Statement 내 Query Set 마다 Analysis 정보를 생성한다.
 *                  2. 전달 받은 Query Set 의 Analysis 정보만 생성한다.
 *
 ***********************************************************************/

    /* 1. Statement 내 Query Set 마다 Analysis 정보를 생성한다. */
    if ( aDstQuerySet == NULL )
    {
        IDE_TEST( makeAndSetAnalysisStatement( aDstStatement,
                                               aSrcStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 2. 전달 받은 Query Set 의 Analysis 정보만 생성한다. */
        IDE_TEST( makeAndSetAnalysisPartialStatement( aDstStatement,
                                                      aDstQuerySet,
                                                      aSrcStatement )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalysisStatement( qcStatement * aDstStatement,
                                         qcStatement * aSrcStatement )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Statement 내 Query Set 마다 Analysis 정보를 생성하는 함수
 *
 *                 Query Set List 구성된 Query Set 마다 정보를 생성하고
 *                  최상위 Analysis 정보는 생성 또는 Parse Tree 에 연결한다.
 *
 * Implementation : 1. Query Set List 구성된 Query Set 마다 정보를 생성한다.
 *                  2. 최상위 Analysis 정보는 생성 후, Parse Tree 에 연결한다.
 *                  3. 완료 단계로 설정하고, Plan 정보를 설정한다.
 *
 ***********************************************************************/

    sdiQuerySetList  * sDstList     = NULL;
    sdiQuerySetEntry * sDstEntry    = NULL;
    qmsQuerySet      * sDstQuerySet = NULL;
    sdiQuerySetList  * sSrcList     = NULL;
    sdiQuerySetEntry * sSrcEntry    = NULL;
    qmsQuerySet      * sSrcQuerySet = NULL;

    IDE_TEST_RAISE( aSrcStatement == NULL, ERR_NULL_STATEMENT_1 );
    IDE_TEST_RAISE( aDstStatement == NULL, ERR_NULL_STATEMENT_2 );

    sDstList = aDstStatement->mShardQuerySetList;
    sSrcList = aSrcStatement->mShardQuerySetList;

    IDE_TEST_RAISE( sDstList == NULL, ERR_NULL_LIST_1 );
    IDE_TEST_RAISE( sSrcList == NULL, ERR_NULL_LIST_2 );

    IDE_TEST_RAISE( SDI_CHECK_QUERYSET_LIST_STATE( sDstList,
                                                   SDI_QUERYSET_LIST_STATE_MAIN_ANALYZE_DONE )
                    == ID_TRUE,
                    ERR_UNEXPACTED );

    /* 1. Query Set List 구성된 Query Set 마다 정보를 생성한다. */
    for ( sDstEntry = sDstList->mHead, sSrcEntry = sSrcList->mHead;
          ( ( sDstEntry != NULL ) && ( sSrcEntry != NULL ) );
          sDstEntry = sDstEntry->mNext, sSrcEntry = sSrcEntry->mNext )
    {
        IDE_TEST_RAISE( sDstEntry->mId != sSrcEntry->mId, ERR_INVALID_ENTRY );

        sDstQuerySet = sDstEntry->mQuerySet;
        sSrcQuerySet = sSrcEntry->mQuerySet;

        IDE_TEST( allocAndCopyAnalysisQuerySet( aDstStatement,
                                                sDstQuerySet,
                                                sSrcQuerySet )
                  != IDE_SUCCESS );
    }

    IDE_TEST_RAISE( sDstEntry != NULL, ERR_INVALID_ENTRY_1 );
    IDE_TEST_RAISE( sSrcEntry != NULL, ERR_INVALID_ENTRY_2 );

    /* 2. 최상위 Analysis 정보는 생성 후, Parse Tree 에 연결한다. */
    IDE_TEST( allocAndCopyAnalysisParseTree( aDstStatement,
                                             aDstStatement->myPlan->parseTree,
                                             aSrcStatement->myPlan->parseTree )
              != IDE_SUCCESS );

    /* 3. 완료 단계로 설정하고, Plan 정보를 설정한다. */
    SDI_SET_QUERYSET_LIST_STATE( sDstList,
                                 SDI_QUERYSET_LIST_STATE_MAIN_ANALYZE_DONE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "dst statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STATEMENT_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "src statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPACTED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "list is invalid" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "dst list is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "src list is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ENTRY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "entry is invalid" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ENTRY_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "dst entry is invalid" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ENTRY_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisStatement",
                                  "src entry is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalysisPartialStatement( qcStatement * aDstStatement,
                                                qmsQuerySet * aDstQuerySet,
                                                qcStatement * aSrcStatement )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                전달 받은 Query Set 의 Analysis 정보만 생성하는 함수
 *
 *                 qmgShardSelect 에서 최적화 후 다시 정보를 생성할 때에 이용한다.
 *
 * Implementation : 1. 전달받은 Query Set 의 정보를 생성한다.
 *                  2. 완료 단계로 설정한다.
 *
 ***********************************************************************/

    qcParseTree      * sSrcParseTree = NULL;
    sdiQuerySetList  * sDstList      = NULL;
    sdiQuerySetList  * sSrcList      = NULL;

    IDE_TEST_RAISE( aSrcStatement == NULL, ERR_NULL_STATEMENT_1 );
    IDE_TEST_RAISE( aDstStatement == NULL, ERR_NULL_STATEMENT_2 );

    sDstList = aDstStatement->mShardQuerySetList;
    sSrcList = aSrcStatement->mShardQuerySetList;

    IDE_TEST_RAISE( sDstList == NULL, ERR_NULL_LIST_1 );
    IDE_TEST_RAISE( sSrcList == NULL, ERR_NULL_LIST_2 );

    sSrcParseTree = aSrcStatement->myPlan->parseTree;

    IDE_TEST_RAISE( SDI_CHECK_QUERYSET_LIST_STATE( sDstList,
                                                   SDI_QUERYSET_LIST_STATE_MAIN_ANALYZE_DONE )
                    == ID_TRUE,
                    ERR_UNEXPACTED );

    /* 1. 전달받은 Query Set 의 정보를 생성한다. */
    IDE_TEST( allocAndCopyAnalysisPartialStatement( aDstStatement,
                                                    aDstQuerySet,
                                                    sSrcParseTree )
              != IDE_SUCCESS );

    /* 2. 완료 단계로 설정한다.
     *  특정 부분만 다시 정보를 생성하는 경우로,
     *   전체를 생성할 때 수행한 Plan 정보 기록은 제외한다.
     */
    SDI_SET_QUERYSET_LIST_STATE( sDstList,
                                 SDI_QUERYSET_LIST_STATE_MAIN_ANALYZE_DONE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisPartialStatement",
                                  "dst statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STATEMENT_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisPartialStatement",
                                  "src statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPACTED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisPartialStatement",
                                  "list is invalid" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisPartialStatement",
                                  "dst list is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalysisPartialStatement",
                                  "src list is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyAnalysisQuerySet( qcStatement * aStatement,
                                          qmsQuerySet * aDstQuerySet,
                                          qmsQuerySet * aSrcQuerySet )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 정보를 생성하는 함수
 *
 *                 Analysis 정보는 Main, SubKey 의 Set 로 구성할 수 있다.
 *                  Analyze 수행된 Src QuerySet 에서 Analysis 를 가져와
 *                   Dst QuerySet 복제하여 생성한다.
 *
 * Implementation : 1. Src QuerySet 에서 Analysis 를 가져온다.
 *                  2. Dst Analysis  에 복제하여 생성한다.
 *                  3. SubKey 존재 시, SubKey Analysis 도 복제하여 생성한다.
 *                  4. Analysis 를 Dst QuerySet 에 연결한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sSrcAnalysis  = NULL;
    sdiShardAnalysis * sDstAnalysis  = NULL;
    sdiShardAnalysis * sNextAnalysis = NULL;

    IDE_TEST_RAISE( aDstQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. Src QuerySet 에서 Analysis 를 가져온다. */
    IDE_TEST( getQuerySetAnalysis( aSrcQuerySet,
                                   &( sSrcAnalysis ) )
              != IDE_SUCCESS );

    if ( sSrcAnalysis != NULL )
    {
        /* 2. Dst Analysis 에 복제하여 생성한다. */
        IDE_TEST( allocAndCopyAnalysis( aStatement,
                                        sSrcAnalysis,
                                        &( sDstAnalysis ) )
                  != IDE_SUCCESS );

        /* 3. SubKey 존재 시, SubKey Analysis 도 복제하여 생성한다. */
        if ( sDstAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
        {
            IDE_TEST( allocAndCopyAnalysis( aStatement,
                                            sSrcAnalysis->mNext,
                                            &( sNextAnalysis ) )
                      != IDE_SUCCESS );

            sDstAnalysis->mNext = sNextAnalysis;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* A. SET 연산이 있는 경우, Left, Right 분석 중 에러 발생으로
         *    SET 연산인 Query Set 의 Analysis 를 생성하지 못할 수 있다.
         *    현재 에러 발생 시, 더 이상 수행하지 않으므로 상관없다.
         *
         * B. Scala Subquery 의 경우,
         *    해당 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * C. Composite Method 인 Subquery 의 경우,
         *    해당 SubKey 분석 시 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 SubKey 분석 시 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * D. 일반 테이블 대상 Insert, Update, Delete 에 Subquery 의 경우
         *    해당 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 SubKey 분석 시 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * E. Unnesting, View Merging, CFS 의 경우,
         *    해당 Query Set 를 건너뛰어 분석하지 않는다.
         *    현재 analyzePartialQuerySet 로 바로 분석하여 Analysis 를 생성하여 상관없다.
         */
        IDE_RAISE( ERR_NULL_ANALYSIS );
    }

    /* 4. Analysis 를 Dst QuerySet 에 연결한다. */
    aDstQuerySet->mShardAnalysis = sDstAnalysis;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysisQuerySet",
                                  "dst parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysisQuerySet",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyAnalysisParseTree( qcStatement * aStatement,
                                           qcParseTree * aDstParseTree,
                                           qcParseTree * aSrcParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 정보를 생성하는 함수
 *
 *                 Analysis 정보는 Main, SubKey 의 Set 로 구성할 수 있다.
 *                  Analyze 수행된 Src ParseTree 에서 Analysis 를 가져와
 *                   Dst ParseTree 에 복제하여 생성한다.
 *
 * Implementation : 1. Src ParseTree 에서 Analysis 를 가져온다
 *                  2. Select 의 경우, 이미 생성된 QuerySet 의 Analysis 를 가져온다.
 *                  3. Dst Analysis 에 복제하여 생성한다.
 *                  4. SubKey 존재 시, SubKey Analysis 도 복제하여 생성한다.
 *                  5. Analysis 를 Dst ParseTree 에 연결한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sSrcAnalysis  = NULL;
    sdiShardAnalysis * sDstAnalysis  = NULL;
    sdiShardAnalysis * sNextAnalysis = NULL;

    IDE_TEST_RAISE( aDstParseTree == NULL, ERR_NULL_PARSETREE );

    /* 1. Src ParseTree 에서 Analysis 를 가져온다. */
    IDE_TEST( sda::getParseTreeAnalysis( aSrcParseTree,
                                         &( sSrcAnalysis ) )
              != IDE_SUCCESS );

    if ( sSrcAnalysis != NULL )
    {
        /* 2. Select 의 경우, 이미 생성된 QuerySet 의 Analysis 를 가져온다. */
        if ( ( aDstParseTree->stmtKind == QCI_STMT_SELECT )
             ||
             ( aDstParseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE ) )
        {
            sDstAnalysis = ( (qmsParseTree *)aDstParseTree )->querySet->mShardAnalysis;
        }
        else
        {
            /* 3. Dst Analysis 에 복제하여 생성한다. */
            IDE_TEST( allocAndCopyAnalysis( aStatement,
                                            sSrcAnalysis,
                                            &( sDstAnalysis ) )
                      != IDE_SUCCESS );

            /* 4. SubKey 존재 시, SubKey Analysis 도 복제하여 생성한다. */
            if ( sDstAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
            {
                IDE_TEST( allocAndCopyAnalysis( aStatement,
                                                sSrcAnalysis->mNext,
                                                &( sNextAnalysis ) )
                          != IDE_SUCCESS );

                sDstAnalysis->mNext = sNextAnalysis;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* A. SET 연산이 있는 경우, Left, Right 분석 중 에러 발생으로
         *    SET 연산인 Query Set 의 Analysis 를 생성하지 못할 수 있다.
         *    현재 에러 발생 시, 더 이상 수행하지 않으므로 상관없다.
         *
         * B. Scala Subquery 의 경우,
         *    해당 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * C. Composite Method 인 Subquery 의 경우,
         *    해당 SubKey 분석 시 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 SubKey 분석 시 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * D. 일반 테이블 대상 Insert, Update, Delete 에 Subquery 의 경우
         *    해당 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 SubKey 분석 시 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * E. Unnesting, View Merging, CFS 의 경우,
         *    해당 Query Set 를 건너뛰어 분석하지 않는다.
         *    현재 analyzePartialQuerySet 로 바로 분석하여 Analysis 를 생성하여 상관없다.
         */
        IDE_RAISE( ERR_NULL_ANALYSIS );
    }

    /* 5. Analysis 를 Dst ParseTree 에 연결한다. */
    IDE_TEST( sda::setParseTreeAnalysis( aDstParseTree,
                                         sDstAnalysis )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysisParseTree",
                                  "dst parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysisParseTree",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyAnalysisPartialStatement( qcStatement * aStatement,
                                                  qmsQuerySet * aDstQuerySet,
                                                  qcParseTree * aSrcParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 정보를 생성하는 함수
 *
 *                 Analysis 정보는 Main, SubKey 의 Set 로 구성할 수 있다.
 *                  Analyze 수행된 Src ParseTree 에서 Analysis 를 가져와
 *                   Dst QuserySet 복제하여 생성한다.
 *
 * Implementation : 1. Src ParseTree 에서 Analysis 를 가져온다
 *                  2. Dst Analysis 에 복제하여 생성한다.
 *                  3. SubKey 존재 시, SubKey Analysis 도 복제하여 생성한다.
 *                  4. Analysis 를 Dst QuerySet 에 연결한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sSrcAnalysis  = NULL;
    sdiShardAnalysis * sDstAnalysis  = NULL;
    sdiShardAnalysis * sNextAnalysis = NULL;

    IDE_TEST_RAISE( aDstQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. Src ParseTree 에서 Analysis 를 가져온다. */
    IDE_TEST( sda::getParseTreeAnalysis( aSrcParseTree,
                                         &( sSrcAnalysis ) )
              != IDE_SUCCESS );

    if ( sSrcAnalysis != NULL )
    {
        /* 2. Dst Analysis 에 복제하여 생성한다. */
        IDE_TEST( allocAndCopyAnalysis( aStatement,
                                        sSrcAnalysis,
                                        &( sDstAnalysis ) )
                  != IDE_SUCCESS );

        /* 3. SubKey 존재 시, SubKey Analysis 도 복제하여 생성한다. */
        if ( sDstAnalysis->mAnalysisFlag.mTopQueryFlag[ SDI_TQ_SUB_KEY_EXISTS ] == ID_TRUE )
        {
            IDE_TEST( allocAndCopyAnalysis( aStatement,
                                            sSrcAnalysis->mNext,
                                            &( sNextAnalysis ) )
                      != IDE_SUCCESS );

            sDstAnalysis->mNext = sNextAnalysis;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* A. SET 연산이 있는 경우, Left, Right 분석 중 에러 발생으로
         *    SET 연산인 Query Set 의 Analysis 를 생성하지 못할 수 있다.
         *    현재 에러 발생 시, 더 이상 수행하지 않으므로 상관없다.
         *
         * B. Scala Subquery 의 경우,
         *    해당 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * C. Composite Method 인 Subquery 의 경우,
         *    해당 SubKey 분석 시 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 SubKey 분석 시 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * D. 일반 테이블 대상 Insert, Update, Delete 에 Subquery 의 경우
         *    해당 Query Set 이 Non Shard 이므로, Subquery 까지 분석하지 않는다.
         *    현재 SubKey 분석 시 Query Set 의 Non Shard 여부에 상관없이 Subquery 를 분석하도록
         *    sda::analyzeSFWGHCore 의 sda::subqueryAnalysis4SFWGH 위치를 수정하여 상관없다.
         *
         * E. Unnesting, View Merging, CFS 의 경우,
         *    해당 Query Set 를 건너뛰어 분석하지 않는다.
         *    현재 analyzePartialQuerySet 로 바로 분석하여 Analysis 를 생성하여 상관없다.
         */
        IDE_RAISE( ERR_NULL_ANALYSIS );
    }

    /* 5. Analysis 를 Dst ParseTree 에 연결한다. */
    aDstQuerySet->mShardAnalysis = sDstAnalysis;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysisPartialStatement",
                                  "dst queryset is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysisPartialStatement",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::getQuerySetAnalysis( qmsQuerySet       * aQuerySet,
                                 sdiShardAnalysis ** aAnalysis )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 정보를 가져오는 함수
 *
 *                 Pre Analysis 관련 함수
 *                  - sdo::allocAndCopyAnalysisQuerySet
 *
 * Implementation : 1. Pre Analysis 가 있다면, Pre Analysis 를 가져온다.
 *                  2. QuetSet 에 해당하는 Analysis 를 가져온다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis = NULL;

    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. Pre Analysis 가 있다면, Pre Analysis 를 가져온다.
     *  Shard Analysis 는 Optimize 된 QuerySet 의 정보이며,
     *   Pre Analysis 는 Optimize 전 QuerySet 의 정보이다.
     *
     *    Analyze 에서는 Shard Analysis 기반으로 Shard 여부를 판단하고,
     *     Non-Shard 변환이 필요한 Transform 에서는 Pre Analysis 정보를 필요로 한다.
     */
    if ( aQuerySet->mPreAnalysis != NULL )
    {
        sAnalysis = aQuerySet->mPreAnalysis;
    }
    else
    {
        /* 2. QuetSet 에 해당하는 Analysis 를 가져온다. */
        sAnalysis = aQuerySet->mShardAnalysis;
    }

    if ( aAnalysis != NULL )
    {
        *aAnalysis = sAnalysis;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::getQuerySetAnalysis",
                                  "queryset is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyAnalysis( qcStatement       * aStatement,
                                  sdiShardAnalysis  * aSrcAnalysis,
                                  sdiShardAnalysis ** aDstAnalysis )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analysis 정보 복제하는 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis = NULL;

    IDE_TEST_RAISE( aSrcAnalysis == NULL, ERR_NULL_ANALYSIS );

    IDE_TEST( sda::allocAnalysis( aStatement,
                                  &( sAnalysis ) )
              != IDE_SUCCESS );

    sAnalysis->mIsShardQuery = aSrcAnalysis->mIsShardQuery;

    /* Reason Flag 까지 모든 NonShardQueryReason 를 복제한다. */
    IDE_TEST( sda::mergeAnalysisFlag( &( aSrcAnalysis->mAnalysisFlag ),
                                      &( sAnalysis->mAnalysisFlag ),
                                      SDI_ANALYSIS_FLAG_NON_TRUE |
                                      SDI_ANALYSIS_FLAG_TOP_TRUE |
                                      SDI_ANALYSIS_FLAG_SET_TRUE |
                                      SDI_ANALYSIS_FLAG_CUR_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( allocAndCopyShardInfo( aStatement,
                                     &( aSrcAnalysis->mShardInfo ),
                                     &( sAnalysis->mShardInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( allocAndCopyKeyInfo( aStatement,
                                   aSrcAnalysis->mKeyInfo,
                                   &( sAnalysis->mKeyInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( allocAndCopyTableInfoList( aStatement,
                                         aSrcAnalysis->mTableInfoList,
                                         &( sAnalysis->mTableInfoList ) )
              != IDE_SUCCESS );

    if ( aDstAnalysis != NULL )
    {
        *aDstAnalysis = sAnalysis;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyAnalysis",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyKeyInfo( qcStatement * aStatement,
                                 sdiKeyInfo  * aSrcKeyInfo,
                                 sdiKeyInfo ** aDstKeyInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                sdiKeyInfo 정보 복제하는 함수
 *
 *                 mLeft, mRight, mOrgKeyInfo 의 리스트 구성은 제외하고,
 *                  같은 KeyInfo 중복 사용을 고려하지 않고 모두 복제한다.
 *
 *                   따라서 같은 Key Info 이지만 여러 Analysis 에 복제되어 존재할 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiKeyInfo * sKeyInfo    = NULL;
    sdiKeyInfo * sHead       = NULL;
    sdiKeyInfo * sTail       = NULL;
    sdiKeyInfo * sSrcKeyInfo = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    for ( sSrcKeyInfo  = aSrcKeyInfo;
          sSrcKeyInfo != NULL;
          sSrcKeyInfo  = sSrcKeyInfo->mNext )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiKeyInfo ),
                                                   (void **)&( sKeyInfo ) )
                  != IDE_SUCCESS );

        SDI_INIT_KEY_INFO( sKeyInfo );

        IDE_TEST( allocAndCopyKeyTarget( aStatement,
                                         sSrcKeyInfo->mKeyTargetCount,
                                         sSrcKeyInfo->mKeyTarget,
                                         &( sKeyInfo->mKeyTargetCount ),
                                         &( sKeyInfo->mKeyTarget ) )
                  != IDE_SUCCESS );

        IDE_TEST( allocAndCopyKeyColumn( aStatement,
                                         sSrcKeyInfo->mKeyCount,
                                         sSrcKeyInfo->mKey,
                                         &( sKeyInfo->mKeyCount ),
                                         &( sKeyInfo->mKey ) )
                  != IDE_SUCCESS );

        IDE_TEST( sdi::allocAndCopyValues( aStatement,
                                           &( sKeyInfo->mValuePtrArray ),
                                           &( sKeyInfo->mValuePtrCount ),
                                           sSrcKeyInfo->mValuePtrArray,
                                           sSrcKeyInfo->mValuePtrCount )
                  != IDE_SUCCESS );

        IDE_TEST( allocAndCopyShardInfo( aStatement,
                                         &( sSrcKeyInfo->mShardInfo ),
                                         &( sKeyInfo->mShardInfo ) )
                  != IDE_SUCCESS );

        sKeyInfo->mIsJoined = sSrcKeyInfo->mIsJoined;

        if ( sHead == NULL )
        {
            sHead = sKeyInfo;
            sTail = sKeyInfo;
        }
        else
        {
            sTail->mNext = sKeyInfo;
            sTail        = sKeyInfo;
        }
    }

    if ( aDstKeyInfo != NULL )
    {
        *aDstKeyInfo = sHead;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyKeyInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyKeyTarget( qcStatement  * aStatement,
                                   UInt           aSrcCount,
                                   UShort       * aSrcTarget,
                                   UInt         * aDstCount,
                                   UShort      ** aDstTarget )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Key Target 정보를 복제하는 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort * sTarget = NULL;

    if ( aSrcCount > 0 )
    {
        IDE_TEST_RAISE( aSrcTarget == NULL, ERR_NULL_TARGET );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( UShort ) * aSrcCount,
                                                   (void **)&( sTarget ) )
                  != IDE_SUCCESS );

        idlOS::memcpy( (void *) sTarget,
                       (void *) aSrcTarget,
                       ID_SIZEOF( UShort ) * aSrcCount );
    }
    else
    {
        sTarget = NULL;
    }

    if ( ( aDstTarget != NULL )
         &&
         ( aDstCount != NULL ) )
    {
        *aDstTarget = sTarget;
        *aDstCount  = aSrcCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyKeyTarget",
                                  "target is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyKeyColumn( qcStatement        * aStatement,
                                   UInt                 aSrcCount,
                                   sdiKeyTupleColumn  * aSrcColumn,
                                   UInt               * aDstCount,
                                   sdiKeyTupleColumn ** aDstColumn )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Key Column 정보를 복제하는 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiKeyTupleColumn * sColum = NULL;

    if ( aSrcCount > 0 )
    {
        IDE_TEST_RAISE( aSrcColumn == NULL, ERR_NULL_COLUMN );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiKeyTupleColumn ) * aSrcCount,
                                                   (void **)&( sColum ) )
                  != IDE_SUCCESS );

        idlOS::memcpy( (void *) sColum,
                       (void *) aSrcColumn,
                       ID_SIZEOF( sdiKeyTupleColumn ) * aSrcCount );
    }
    else
    {
        sColum = NULL;
    }

    if ( ( aDstColumn != NULL )
         &&
         ( aDstCount != NULL ) )
    {
        *aDstColumn = sColum;
        *aDstCount  = aSrcCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_COLUMN )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyKeyColumn",
                                  "column is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyShardInfo( qcStatement  * aStatement,
                                   sdiShardInfo * aSrcShardInfo,
                                   sdiShardInfo * aDstShardInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard Info 정보를 복제하는 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiRangeInfo * sRangeInfo = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aSrcShardInfo == NULL, ERR_NULL_SHARD_INFO_1 );
    IDE_TEST_RAISE( aDstShardInfo == NULL, ERR_NULL_SHARD_INFO_2 );

    aDstShardInfo->mKeyDataType   = aSrcShardInfo->mKeyDataType;
    aDstShardInfo->mDefaultNodeId = aSrcShardInfo->mDefaultNodeId;
    aDstShardInfo->mSplitMethod   = aSrcShardInfo->mSplitMethod;

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiRangeInfo ),
                                               (void **)&( sRangeInfo ) )
              != IDE_SUCCESS );

    sRangeInfo->mCount  = 0;
    sRangeInfo->mRanges = NULL;

    IDE_TEST( sdi::allocAndCopyRanges( aStatement,
                                       sRangeInfo,
                                       aSrcShardInfo->mRangeInfo )
              != IDE_SUCCESS );

    aDstShardInfo->mRangeInfo = sRangeInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyShardInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SHARD_INFO_1 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyShardInfo",
                                  "src shard info is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SHARD_INFO_2 )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyShardInfo",
                                  "dst shard info is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::allocAndCopyTableInfoList( qcStatement       * aStatement,
                                       sdiTableInfoList  * aSrcTableInfoList,
                                       sdiTableInfoList ** aDstTableInfoList )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Table Info List 정보를 복제하는 함수
 *
 *                 리스트 내 Table 순서 그래도 복제하여, Plan Diff 발생하였다.
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiTableInfoList * sTableInfoList = NULL;
    sdiTableInfoList * sTableInfo     = NULL;
    sdiTableInfoList * sHead          = NULL;
    sdiTableInfoList * sTail          = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    for ( sTableInfoList  = aSrcTableInfoList;
          sTableInfoList != NULL;
          sTableInfoList  = sTableInfoList->mNext )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiTableInfoList ) +
                                                   ID_SIZEOF( sdiTableInfo ),
                                                   (void **)&( sTableInfo ) )
                  != IDE_SUCCESS );

        sTableInfo->mTableInfo = (sdiTableInfo *)( (UChar *)sTableInfo + ID_SIZEOF( sdiTableInfoList ) );
        sTableInfo->mNext      = NULL;

        idlOS::memcpy( sTableInfo->mTableInfo,
                       sTableInfoList->mTableInfo,
                       ID_SIZEOF( sdiTableInfo ) );

        if ( sHead == NULL )
        {
            sHead = sTableInfo;
            sTail = sTableInfo;
        }
        else
        {
            sTail->mNext = sTableInfo;
            sTail        = sTableInfo;
        }
    }

    if ( aDstTableInfoList != NULL )
    {
        *aDstTableInfoList = sHead;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::allocAndCopyTableInfoList",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalyzeInfoFromStatement( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard Transform 을 완료한 후,
 *                 Shard Plan 출력을 위한 AnalyzeInfo 를 생성하는 함수이다.
 *
 *                  이미 생성된 AnalyzeInfo 가 있다면 건너뛰고,
 *                   없는 경우에 적정한 것을 연결하거나,
 *                    현재 ParseTree 의 Analysis 정보로 생성한다.
 *
 * Implementation : 1.   Select 의 경우에, Analysis 정보의 유무를 먼저 확인한다.
 *                  1.1. Analysis 정보로 AnalyzeInfo 를 생성하여 연결한다.
 *                  1.2. 없는 경우, 하위 View 의 AnalyzeInfo 를 연결한다.
 *                  2.   그 외에는 QC_STMT_SHARD_NONE 시, AnalyzeInfo 를 생성하여 연결한다.
 *
 ***********************************************************************/


    sdiShardAnalysis * sAnalysis      = NULL;
    sdiAnalyzeInfo   * sAnalyzeResult = NULL;
    qcStatement      * sStatement     = NULL;
    qmsParseTree     * sSelParseTree  = NULL;
    qcParseTree      * sParseTree     = NULL;
    qciStmtType        sStmtKind      = QCI_STMT_MASK_MAX;
    qcShardStmtType    sStmtType      = QC_STMT_SHARD_NONE;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    sParseTree = aStatement->myPlan->parseTree;
    sStmtKind  = sParseTree->stmtKind;
    sStmtType  = sParseTree->stmtShard;

    switch ( sStmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            /* 1.   Select 의 경우에, Analysis 정보의 유무를 먼저 확인한다. */
            IDE_TEST( sda::getParseTreeAnalysis( sParseTree,
                                                 &( sAnalysis ) )
                      != IDE_SUCCESS );

            /* 1.1. Analysis 정보로 AnalyzeInfo 를 생성하여 연결한다. */
            if ( sAnalysis != NULL )
            {
                IDE_TEST( sda::makeAndSetAnalyzeInfo( aStatement,
                                                      sAnalysis,
                                                      &( sAnalyzeResult ) )
                          != IDE_SUCCESS );

                aStatement->myPlan->mShardAnalysis = sAnalyzeResult;
            }
            else
            {
                /* 1.2. 없는 경우, 하위 View 의 AnalyzeInfo 를 연결한다.
                 *       최상위 Statement 에서
                 *        qmvShardTransform::makeShardForStatement 함수를 수행한 경우,
                 *
                 *         Transformer 가 Statement 의 ParseTree 로 교체하여
                 *          Analysis 가 없을 수 있고
                 *           하위 View 가 된 이전 Statement 의 AnalyzeInfo 를 연결한다.
                 */
                sSelParseTree  = (qmsParseTree *)( sParseTree );
                sStatement     = sSelParseTree->querySet->SFWGH->from->tableRef->view;
                sAnalyzeResult = sStatement->myPlan->mShardAnalysis;

                aStatement->myPlan->mShardAnalysis = sAnalyzeResult;
            }

            break;

        case QCI_STMT_INSERT:
        case QCI_STMT_UPDATE:
        case QCI_STMT_DELETE:
        case QCI_STMT_EXEC_PROC:
            /* 2.   그 외에는 QC_STMT_SHARD_NONE 시,AnalyzeInfo 를 생성하여 연결한다.
             *       Local Target Table 경우에 해당한다.
             */
            switch ( sStmtType )
            {
                case QC_STMT_SHARD_NONE:
                    IDE_TEST( sda::getParseTreeAnalysis( sParseTree,
                                                         &( sAnalysis ) )
                              != IDE_SUCCESS );

                    IDE_TEST( sda::makeAndSetAnalyzeInfo( aStatement,
                                                          sAnalysis,
                                                          &( sAnalyzeResult ) )
                              != IDE_SUCCESS );

                    aStatement->myPlan->mShardAnalysis = sAnalyzeResult;

                    break;

                default:
                    sAnalyzeResult = aStatement->myPlan->mShardAnalysis;

                    break;
            }

            break;

        default:
            IDE_RAISE( ERR_INVALID_STMTKIND );

            break;
    }

    IDE_TEST( setPrintInfoFromAnalyzeInfo( aStatement,
                                           sAnalyzeResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromStatement",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMTKIND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromStatement",
                                  "stmtkind is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalyzeInfoFromParseTree( qcStatement     * aStatement,
                                                qcParseTree     * aParseTree,
                                                sdiAnalyzeInfo ** aAnalyzeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Parse Tree 수준에서 Shard View 로 변환할 때,
 *                 AnalyzeInfo 를 생성하는 함수이다.
 *
 *                  Shard Keyword 사용 여부에 따라서,
 *                   적정한 AnalyzeInfo 를 생성해야 한다.
 *                    - sda::makeAndSetAnalyzeInfo
 *                    - sdo::makeAndSetAnalyzeInfoFromNodeInfo
 *
 *                    아래의 함수에서 호출한다.
 *                     - qmvShardTransform::rebuildTransform
 *                     - qmvShardTransform::makeShardForConvert
 *                     - qmvShardTransform::makeShardForStatement
 *
 * Implementation : 1. Analysis 정보로 AnalyzeInfo 를 생성한다.
 *                  2. Analysis 정보로 AnalyzeInfo 를 생성하고, 검증한다.
 *                  3. NodeInfo 정보로 AnalyzeInfo 를 생성한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis      = NULL;
    sdiAnalyzeInfo   * sAnalyzeResult = NULL;
    qcShardStmtType    sStmtType      = QC_STMT_SHARD_NONE;
    qcuSqlSourceInfo   sqlInfo;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtType = aParseTree->stmtShard;

    switch ( sStmtType )
    {
        case QC_STMT_SHARD_NONE:
            /* 1. Analysis 정보로 AnalyzeInfo 를 생성한다. */
            IDE_TEST( sda::getParseTreeAnalysis( aParseTree,
                                                 &( sAnalysis ) )
                      != IDE_SUCCESS );

            IDE_TEST( sda::makeAndSetAnalyzeInfo( aStatement,
                                                  sAnalysis,
                                                  &( sAnalyzeResult ) )
                      != IDE_SUCCESS );

            break;

        case QC_STMT_SHARD_ANALYZE:
            /* 2. Analysis 정보로 AnalyzeInfo 를 생성하고, 검증한다. */
            IDE_TEST( sda::getParseTreeAnalysis( aParseTree,
                                                 &( sAnalysis ) )
                      != IDE_SUCCESS );

            IDE_TEST( sda::makeAndSetAnalyzeInfo( aStatement,
                                                  sAnalysis,
                                                  &( sAnalyzeResult ) )
              != IDE_SUCCESS );

            if ( ( sAnalyzeResult->mSplitMethod == SDI_SPLIT_NONE )
                 ||
                 ( sAnalyzeResult->mNonShardQueryReason == SDI_MULTI_SHARD_INFO_EXISTS ) )
            {
                IDE_TEST( raiseInvalidShardQueryError( aStatement,
                                                       aParseTree )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            break;

        case QC_STMT_SHARD_DATA:
            /* 3. NodeInfo 정보로 AnalyzeInfo 를 생성한다. */
            IDE_TEST( makeAndSetAnalyzeInfoFromNodeInfo( aStatement,
                                                         aParseTree->nodes,
                                                         &( sAnalyzeResult ) )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( ERR_UNEXPECTED );
            break;
    }

    if ( aAnalyzeInfo != NULL )
    {
        *aAnalyzeInfo = sAnalyzeResult;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromParseTree",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromParseTree",
                                  "shard type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalyzeInfoFromQuerySet( qcStatement     * aStatement,
                                               qmsQuerySet     * aQuerySet,
                                               sdiAnalyzeInfo ** aAnalyzeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Query 수준에서 Shard View 로 변환할 때,
 *                 AnalyzeInfo 를 생성하는 함수이다.
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiAnalyzeInfo * sAnalyzeResult = NULL;

    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    IDE_TEST( sda::makeAndSetAnalyzeInfo( aStatement,
                                          aQuerySet->mShardAnalysis,
                                          &( sAnalyzeResult ) )
              != IDE_SUCCESS );

    if ( aAnalyzeInfo != NULL )
    {
        *aAnalyzeInfo = sAnalyzeResult;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromQuerySet",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalyzeInfoFromNodeInfo( qcStatement     * aStatement,
                                               qcShardNodes    * aNodeInfo,
                                               sdiAnalyzeInfo ** aAnalyzeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Parse Tree 의 Node 정보 기준으로
 *                 Shard Analyze Info 를 생성하는 함수이다.
 *
 * Implementation : 1. 분석결과에 상관없이 전노드 분석결과로 교체한다.
 *                  2. Node 정보로 Analyze Info 를 생성한다.
 *
 ***********************************************************************/

    sdiAnalyzeInfo * sAnalyzeResult = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStatement->mShardQuerySetList == NULL, ERR_NULL_LIST );

    /* 1. 분석결과에 상관없이 전노드 분석결과로 교체한다. */
    if ( aNodeInfo == NULL )
    {
        sAnalyzeResult = sdi::getAnalysisResultForAllNodes();
    }
    else
    {
        /* 2. Node 정보로 Analyze Info 를 생성한다. */
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiAnalyzeInfo ),
                                                   (void **)&( sAnalyzeResult ) )
                  != IDE_SUCCESS );

        SDI_INIT_ANALYZE_INFO( sAnalyzeResult );

        IDE_TEST( sdi::validateNodeNames( aStatement,
                                          aNodeInfo )
                  != IDE_SUCCESS );

        sAnalyzeResult->mSplitMethod         = SDI_SPLIT_NODES;
        sAnalyzeResult->mNodeNames           = aNodeInfo;
        sAnalyzeResult->mIsShardQuery        = ID_FALSE;
        sAnalyzeResult->mNonShardQueryReason = SDI_SHARD_KEYWORD_EXISTS;
    }

    if ( aAnalyzeInfo != NULL )
    {
        *aAnalyzeInfo = sAnalyzeResult;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromNodeInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromNodeInfo",
                                  "list is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::makeAndSetAnalyzeInfoFromObjectInfo( qcStatement     * aStatement,
                                                 sdiObjectInfo   * aShardObjInfo,
                                                 sdiAnalyzeInfo ** aAnalyzeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard Object Info 기준로 Shard Analyze Info 를 생성하는 함수이다.
 *
 * Implementation :
 *
***********************************************************************/

    sdiAnalyzeInfo * sAnalysisResult = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aShardObjInfo == NULL, ERR_NULL_INFO );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiAnalyzeInfo ),
                                               (void **)&( sAnalysisResult ) )
              != IDE_SUCCESS );

    SDI_INIT_ANALYZE_INFO( sAnalysisResult );

    sAnalysisResult->mIsShardQuery   = ID_TRUE;
    sAnalysisResult->mSplitMethod    = aShardObjInfo->mTableInfo.mSplitMethod;
    sAnalysisResult->mKeyDataType    = aShardObjInfo->mTableInfo.mKeyDataType;
    sAnalysisResult->mSubKeyExists   = aShardObjInfo->mTableInfo.mSubKeyExists;
    sAnalysisResult->mSubSplitMethod = aShardObjInfo->mTableInfo.mSubSplitMethod;
    sAnalysisResult->mSubKeyDataType = aShardObjInfo->mTableInfo.mSubKeyDataType;
    sAnalysisResult->mDefaultNodeId  = aShardObjInfo->mTableInfo.mDefaultNodeId;

    IDE_TEST( sdi::allocAndCopyRanges( aStatement,
                                       &( sAnalysisResult->mRangeInfo ),
                                       &( aShardObjInfo->mRangeInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( sdiTableInfoList ),
                                               (void **)&( sAnalysisResult->mTableInfoList ) )
              != IDE_SUCCESS );

    sAnalysisResult->mTableInfoList->mTableInfo = &( aShardObjInfo->mTableInfo );
    sAnalysisResult->mTableInfoList->mNext      = NULL;

    if ( aAnalyzeInfo != NULL )
    {
        *aAnalyzeInfo = sAnalysisResult;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromObjectInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::makeAndSetAnalyzeInfoFromObjectInfo",
                                  "shard object info is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::isShardParseTree( qcParseTree * aParseTree,
                              idBool      * aIsShardParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                전달받은 ParseTree 의 Shard 여부를 검사하는 함수
 *
 *                 sda::isShardQuery 는 NonShardQueryReason 만 고려,
 *                  sdo::isShradParseTree 는 SubKey 와 Reason Flag 를 고려해서 검사한다.
 *
 * Implementation : 1. Shard Analysis 를 가져온다.
 *                  2. Main, SubKey 순으로 Shard 여부를 검사한다.
 *                  3. 모두 Shard 여야 Shard 이다.
 *                  4. SDI_OUT_DEP_INFO_EXISTS 의 경우, Non-Shard 이다.
 *                  5. Shard 여부를 검사한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis         = NULL;
    sdiShardAnalysis * sTarget           = NULL;
    idBool             sIsShardParseTree = ID_TRUE;

    /* 1. Shard Analysis 를 가져온다. */
    IDE_TEST( sda::getParseTreeAnalysis( aParseTree,
                                         &( sAnalysis ) )
              != IDE_SUCCESS );

    if ( sAnalysis != NULL )
    {
        /* 2. Main, SubKey 순으로 Shard ParseTree 여부를 검사한다. */
        for ( sTarget  = sAnalysis;
              sTarget != NULL;
              sTarget  = sTarget->mNext )
        {
            /* 3. 모두 Shard ParseTree 여야 Shard ParseTree 이다. */
            if ( sIsShardParseTree == ID_TRUE )
            {
                /* 4. SDI_OUT_DEP_INFO_EXISTS 의 경우, Non-Shard 이다.
                 *  SDI_OUT_DEP_INFO_EXISTS 로 Outer Dependency 유무를 확인할 수 있다.
                 *   해당 QuerySet 는 Syntax Error 가 발생할 요소가 있어,
                 *    Non-Shard 처리를 할 필요가 있다.
                 *
                 *     SELECT I1
                 *      FROM T1 TA
                 *       WHERE EXISTS ( SELECT I2 FROM T1 TB WHERE TA.I2 = TB.I1 );
                 *                      |--------------------------------------|
                 *                                                 **           \
                 *                                                  \        aQuerySet
                 *                                             Syntax Error
                 */
                if( sTarget->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_OUT_DEP_INFO ] == ID_TRUE )
                {
                    sIsShardParseTree = ID_FALSE;
                }
                else
                {
                    /* 5. Shard 여부를 검사한다. */
                    IDE_TEST( sda::isShardQuery( &( sTarget->mAnalysisFlag ),
                                                 &( sIsShardParseTree ) )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        sIsShardParseTree = ID_FALSE;
    }

    if ( aIsShardParseTree != NULL )
    {
        *aIsShardParseTree = sIsShardParseTree;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::isShardQuerySet( qmsQuerySet * aQuerySet,
                             idBool      * aIsShardQuerySet,
                             idBool      * aIsTransformAble )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                전달받은 QuerySet 의 Shard QuerySet, TransformAble 여부를 검사하는 함수
 *
 * Implementation : 1. Shard Analysis 를 가져온다.
 *                  2. Main, SubKey 순으로 Shard ParseTree 여부를 검사한다.
 *                  3. 모두 Shard QuerySet 여야 Shard QuerySet 이다.
 *                  4. Shard QuerySet 를 검사한다.
 *                  5. 모두 TransformAble 여야 TransformAble 이다.
 *                  6. TransformAble 를 검사한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis        = NULL;
    sdiShardAnalysis * sTarget          = NULL;
    idBool             sIsShardQuerySet = ID_TRUE;
    idBool             sIsTransformAble = ID_TRUE;

    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. Shard Analysis 를 가져온다. */
    sAnalysis = aQuerySet->mShardAnalysis;

    if ( sAnalysis != NULL )
    {
        /* 2. Main, SubKey 순으로 Shard QuerySet, TransformAble 여부를 검사한다. */
        for ( sTarget  = sAnalysis;
              sTarget != NULL;
              sTarget  = sTarget->mNext )
        {
            /* 3. 모두 Shard QuerySet 여야 Shard QuerySet 이다. */
            if ( sIsShardQuerySet == ID_TRUE )
            {
                /* 4. Shard QuerySet 를 검사한다. */
                if ( sTarget->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_NON_SHARD ] == ID_TRUE )
                {
                    sIsShardQuerySet = ID_FALSE;
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

            /* 5. 모두 TransformAble 여야 TransformAble 이다. */
            if ( sIsTransformAble == ID_TRUE )
            {
                /* 6. TransformAble 를 검사한다. */
                if ( ( SDU_SHARD_AGGREGATION_TRANSFORM_ENABLE == 0 )
                     ||
                     ( sTarget->mAnalysisFlag.mCurQueryFlag[ SDI_CQ_AGGR_TRANSFORMABLE ] == ID_FALSE ) )
                {
                    sIsTransformAble = ID_FALSE;
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
    }
    else
    {
        sIsShardQuerySet = ID_FALSE;
        sIsTransformAble = ID_FALSE;
    }

    if ( aIsShardQuerySet != NULL )
    {
        *aIsShardQuerySet = sIsShardQuerySet;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aIsTransformAble != NULL )
    {
        *aIsTransformAble = sIsTransformAble;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::isShardQuerySet",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::isShardObject( qcParseTree * aParseTree,
                           idBool      * aIsShardObject )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                전달받은 ParseTree 의 Shard Object 여부를 검사하는 함수
 *
 *                 INSERT, DELETE, UPDATE 의 Target Table 속성을 검사한다.
 *
 * Implementation : 1. Shard Analysis 를 가져온다.
 *                  2. Main, SubKey 순으로 Shard Object 여부를 검사한다.
 *                  3. 모두 Shard Object 여야 Shard Object이다.
 *                  4. Shard Object 를 검사한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis      = NULL;
    sdiShardAnalysis * sTarget        = NULL;
    idBool             sIsShardObject = ID_TRUE;

    /* 1. Shard Analysis 를 가져온다.*/
    IDE_TEST( sda::getParseTreeAnalysis( aParseTree,
                                         &( sAnalysis ) )
              != IDE_SUCCESS );

    if ( sAnalysis != NULL )
    {
        /* 2. Main, SubKey 순으로 Shard Object 여부를 검사한다. */
        for ( sTarget  = sAnalysis;
              sTarget != NULL;
              sTarget  = sTarget->mNext )
        {
            /* 3. 모두 Shard Object 여야 Shard Object이다. */
            if ( sIsShardObject == ID_TRUE )
            {
                /* 4. Shard Object 를 검사한다. */
                if ( sTarget->mAnalysisFlag.mCurQueryFlag[ SDI_CQ_LOCAL_TABLE ] == ID_TRUE )
                {
                    sIsShardObject = ID_FALSE;
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
    }
    else
    {
        sIsShardObject = ID_FALSE;
    }

    if ( aIsShardObject != NULL )
    {
        *aIsShardObject = sIsShardObject;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::isSupported( qmsQuerySet * aQuerySet,
                         idBool      * aIsSupported )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                전달받은 QuerySet 의 Supported 여부를 검사하는 함수
 *
 *                 Shard Keyword, With Alias 가 포함된 Query 는 Unsupported 이다.
 *
 * Implementation : 1. Shard Analysis 를 가져온다.
 *                  2. Main, SubKey 순으로 Supported 여부를 검사한다.
 *                  3. 모두 Supported 여야 Supported 이다.
 *                  4. Supported 를 검사한다.
 *
 ***********************************************************************/

    sdiShardAnalysis * sAnalysis    = NULL;
    sdiShardAnalysis * sTarget      = NULL;
    idBool             sIsSupported = ID_TRUE;

    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 1. Shard Analysis 를 가져온다. */
    sAnalysis = aQuerySet->mShardAnalysis;

    if ( sAnalysis != NULL )
    {
        /* 2. Main, SubKey 순으로 Supported 여부를 검사한다. */
        for ( sTarget  = sAnalysis;
              sTarget != NULL;
              sTarget  = sTarget->mNext )
        {
            /* 3. 모두 Supported 여야 Supported 이다. */
            if ( sIsSupported == ID_TRUE )
            {
                /* 4. Supported 를 검사한다. */
                if ( sTarget->mAnalysisFlag.mSetQueryFlag[ SDI_SQ_UNSUPPORTED ] == ID_TRUE )
                {
                    sIsSupported = ID_FALSE;
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
    }
    else
    {
        sIsSupported = ID_FALSE;
    }

    if ( aIsSupported != NULL )
    {
        *aIsSupported = sIsSupported;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::isShardQuerySet",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::setPrintInfoFromAnalyzeInfo( qcStatement    * aStatement,
                                         sdiAnalyzeInfo * aAnalyzeInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Analyze Info 기준으로 Plan 정보를 기록
 *
 * Implementation :
 *
 ***********************************************************************/


    sdiPrintInfo * sPrintInfo = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aAnalyzeInfo == NULL, ERR_NULL_ANALYZE_INFO );

    sPrintInfo = &( QC_SHARED_TMPLATE( aStatement )->stmt->mShardPrintInfo );

    IDE_TEST_RAISE( sPrintInfo == NULL, ERR_NULL_PRINT_INFO );

    sda::setPrintInfoFromAnalyzeInfo( sPrintInfo, aAnalyzeInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setPrintInfoFromAnalyzeInfo",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYZE_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setPrintInfoFromAnalyzeInfo",
                                  "analyze info is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PRINT_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setPrintInfoFromAnalyzeInfo",
                                  "print info is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::setPrintInfoFromTransformAble( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                TransformAble 여부를 Plan 정보에 기록
 *
 * Implementation :
 *
 ***********************************************************************/

    sdiPrintInfo * sPrintInfo = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    sPrintInfo = &( QC_SHARED_TMPLATE( aStatement )->stmt->mShardPrintInfo );

    IDE_TEST_RAISE( sPrintInfo == NULL, ERR_NULL_PRINT_INFO );

    sPrintInfo->mTransformable = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setPrintInfoFromTransformAble",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PRINT_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::setPrintInfoFromTransformAble",
                                  "print info is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::raiseInvalidShardQueryError( qcStatement * aStatement,
                                         qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Invalid Shard Query Error 를 발생시킨다.
 *
 * Implementation : 1. Shard Analysis 를 가져온다.
 *                  2. Error Text 를 설정한다.
 *                  3. Error 를 발생시킨다.
 *
 ***********************************************************************/

    UShort             sErrIdx   = ID_USHORT_MAX;
    sdiShardAnalysis * sAnalysis = NULL;
    qcNamePosition   * sStmtPos  = NULL;
    qcuSqlSourceInfo   sqlInfo;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtPos  = &( aParseTree->stmtPos );

    /* 1. Shard Analysis 를 가져온다.*/
    IDE_TEST( sda::getParseTreeAnalysis( aParseTree,
                                         &( sAnalysis ) )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sAnalysis == NULL, ERR_NULL_ANALYSIS );

    /* 2. Error Text 를 설정한다. */
    (void)sda::setNonShardQueryReasonErr( &( sAnalysis->mAnalysisFlag ),
                                          &( sErrIdx ) );

    sqlInfo.setSourceInfo( aStatement, sStmtPos );

    /* 3. Error 를 발생시킨다. */
    IDE_RAISE( ERR_INVALID_SHARD_QUERY );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::raiseInvalidShardQueryError",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::raiseInvalidShardQueryError",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ANALYSIS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::raiseInvalidShardQueryError",
                                  "analysis is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_QUERY )
    {
        (void)sqlInfo.initWithBeforeMessage( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( sdERR_ABORT_INVALID_SHARD_QUERY,
                                  sqlInfo.getBeforeErrMessage(),
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::preAnalyzeQuerySet( qcStatement * aStatement,
                                qmsQuerySet * aQuerySet,
                                ULong         aSMN )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Partial Analyze 를 수행하는 함수.
 *
 *                 Query Set List 에 구성된 Query Set 중, Pre Analysis 이 필요한 경우,
 *                  SDI_QUERYSET_LIST_STATE_PRE_ANALYZE 단계로 설정하고
 *                   sda::preAnalyzeQuerySet 호출한다.
 *
 * Implementation : 1. Query Set List 를 순회한다.
 *                  2. 해당 Query Set 이 대상인지 검사한다.
 *                  3. 이미 수행한 경우는 무시한다.
 *                  4. SDI_QUERYSET_LIST_STATE_PRE_ANALYZE 단계로 설정한다.
 *                  5. Partial 분석을 수행한다.
 *
 ***********************************************************************/

    sdiQuerySetList  * sList  = NULL;
    sdiQuerySetEntry * sEntry = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    sList = aStatement->mShardQuerySetList;

    IDE_TEST_RAISE( sList == NULL, ERR_NULL_LIST );

    /* 1. Query Set List 를 순회한다. */
    for ( sEntry  = sList->mHead;
          sEntry != NULL;
          sEntry  = sEntry->mNext )
    {
        /* 2. 해당 Query Set 이 대상인지 검사한다. */
        if ( sEntry->mQuerySet == aQuerySet )
        {
            /* 3. 이미 수행한 경우는 무시한다. */
            if ( ( aQuerySet->mShardAnalysis == NULL )
                 &&
                 ( aQuerySet->mPreAnalysis == NULL ) )
            {
                /* 4. SDI_QUERYSET_LIST_STATE_PRE_ANALYZE 단계로 설정한다.
                 *  SDI_QUERYSET_LIST_STATE_PRE_ANALYZE 단계는
                 *   Analysis 생성 후 QuerySet 의 mPreAnalysis 에 연결하도록 유도한다.
                 *    따라서 특정 시점에 QuerySet 은 mShardAnalysis 와 mPreAnalysis 로 2개의 정보를 유지할 수 있다.
                 *
                 *     mShardAnalysis 는 최적화 후 분석결과, mPreAnalysis 는 최적화 전 분석결과로 사용된다.
                 *      Transform 은 Non-Shard 변환을 위해서 mPreAnalysis 를 필요로 한다.
                 */
                SDI_SET_QUERYSET_LIST_STATE( sList,
                                             SDI_QUERYSET_LIST_STATE_DUMMY_PRE_ANALYZE );

                /* 5. Partial 분석을 수행한다. */
                IDE_TEST( sda::preAnalyzeQuerySet( aStatement,
                                                   aQuerySet,
                                                   aSMN )
                          != IDE_SUCCESS );

                SDI_SET_QUERYSET_LIST_STATE( sList,
                                             SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE );
            }
            else
            {
                /* Nohing to do */
            }

            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::preAnalyzeQuerySet",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::preAnalyzeQuerySet",
                                  "queryset is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdo::preAnalyzeQuerySet",
                                  "list is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::preAnalyzeSubquery( qcStatement * aStatement,
                                qtcNode     * aNode,
                                ULong         aSMN )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Subquery 에 대해서 Pre Analyze 를 수행하는 함수.
 *
 *                 CFS 변환인 경우에 수행한다.
 *
 * Implementation : 1. Subquery 라면 Pre Analyze 수행한다.
 *                  2. Subquery 를 가지고 있다면 재귀한다.
 *                  3. Over Column 에 Subquery 를 가지고 있다면 재귀한다.
 *                  4. Next 로 재귀한다.
 *
 ***********************************************************************/

    qmsParseTree  * sParseTree  = NULL;
    qtcOverColumn * sOverColumn = NULL;

    if ( aNode != NULL )
    {
        /* 1. Subquery 라면 Partial Analyze 수행한다. */
        if ( QTC_IS_SUBQUERY( aNode ) == ID_TRUE )
        {
            sParseTree = (qmsParseTree *)( aNode->subquery->myPlan->parseTree );

            IDE_TEST( preAnalyzeQuerySet( aStatement,
                                          sParseTree->querySet,
                                          aSMN )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 2. Subquery 를 가지고 있다면 재귀한다. */
            if ( QTC_HAVE_SUBQUERY( aNode ) == ID_TRUE )
            {
                IDE_TEST( preAnalyzeSubquery( aStatement,
                                              (qtcNode *)( aNode->node.arguments ),
                                              aSMN )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }

            if ( aNode->overClause != NULL )
            {
                /* 3. Over Column 에 Subquery 를 가지고 있다면 재귀한다. */
                for ( sOverColumn  = aNode->overClause->overColumn;
                      sOverColumn != NULL;
                      sOverColumn  = sOverColumn->next )
                {
                    if ( QTC_HAVE_SUBQUERY( sOverColumn->node ) == ID_TRUE )
                    {
                        IDE_TEST( preAnalyzeSubquery( aStatement,
                                                      sOverColumn->node,
                                                      aSMN )
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
                /* Nothing to do. */
            }
        }

        /* 4. Next 로 재귀한다. */
        IDE_TEST( preAnalyzeSubquery( aStatement,
                                      (qtcNode *)( aNode->node.next ),
                                      aSMN )
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

IDE_RC sdo::setShardPlanStmtVariable( qcStatement    * aStatement,
                                      qcShardStmtType  aStmtType,
                                      qcNamePosition * aStmtPos )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                myPlan 의 Statment 정보를 기록한다.
 *                 함수 재사용성 강화 목적으로 구현하였다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStmtPos == NULL, ERR_NULL_POSITION );

    SET_POSITION( aStatement->myPlan->parseTree->stmtPos, *aStmtPos );

    switch ( aStmtType )
    {
        case QC_STMT_SHARD_NONE:
        case QC_STMT_SHARD_ANALYZE:
            aStatement->myPlan->parseTree->stmtShard = QC_STMT_SHARD_ANALYZE;
            break;

        case QC_STMT_SHARD_DATA:
            aStatement->myPlan->parseTree->stmtShard = QC_STMT_SHARD_DATA;
            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardPlanStmtVariable",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardPlanStmtVariable",
                                  "position is null" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardPlanStmtVariable",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::setShardPlanCommVariable( qcStatement    * aStatement,
                                      sdiAnalyzeInfo * aAnalyzeInfo,
                                      UShort           aParamCount,
                                      UShort           aParamOffset,
                                      qcShardParamInfo * aParamInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                myPlan 의 Shard 관련 정보를 기록한다.
 *                 함수 재사용성 강화 목적으로 구현하였다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aAnalyzeInfo == NULL, ERR_NULL_INFO );

    if ( aParamInfo == NULL )
    {
        aStatement->myPlan->mShardAnalysis         = aAnalyzeInfo;
        aStatement->myPlan->mShardParamCount       = 0;
        aStatement->myPlan->mShardParamOffset      = ID_USHORT_MAX;
        aStatement->myPlan->mShardParamInfo        = NULL;
    }
    else
    {
        aStatement->myPlan->mShardAnalysis         = aAnalyzeInfo;
        aStatement->myPlan->mShardParamCount       = aParamCount;
        aStatement->myPlan->mShardParamOffset      = aParamOffset;
        aStatement->myPlan->mShardParamInfo        = aParamInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardPlanCommVariable",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardPlanCommVariable",
                                  "analyze info is null" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdo::setShardStmtType( qcStatement * aStatement,
                              qcStatement * aViewStatement )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                shardStmtType 을 하위 view, subquery 로 전파하는 함수
 *
 * Implementation :
 *
 ***********************************************************************/

    qcParseTree * sParseTree     = NULL;
    qcParseTree * sViewParseTree = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_STATEMENT_NULL_1 );
    IDE_TEST_RAISE( aViewStatement == NULL, ERR_STATEMENT_NULL_2 );

    sParseTree     = aStatement->myPlan->parseTree;
    sViewParseTree = aViewStatement->myPlan->parseTree;

    if ( ( sViewParseTree->stmtShard == QC_STMT_SHARD_NONE )
         &&
         ( sParseTree->stmtShard != QC_STMT_SHARD_NONE ) )
    {
        /* Non Shard Insert 인 qmo::optimizeShardInsert 라면
         *  최상위 shardStmtType 이 QC_STMT_SHARD_ANALYZE 로 고정이므로
         *   shardStmtType 를 전달할 필요가 없다.
         *    최상위를 제외한 Subquery 내에서만 전달하도록 한다.
         */
        if ( sParseTree->optimize != qmo::optimizeShardInsert )
        {
            sViewParseTree->stmtShard = sParseTree->stmtShard;
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

    IDE_EXCEPTION( ERR_STATEMENT_NULL_1 );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardStmtType",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_STATEMENT_NULL_2 );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdo::setShardStmtType",
                                  "view statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
