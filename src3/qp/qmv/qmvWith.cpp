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
 **********************************************************************/

#include <idl.h>
#include <qc.h>
#include <qcm.h>
#include <qmv.h>
#include <qmsParseTree.h>
#include <qcpManager.h>
#include <qmvWith.h>
#include <qcuSqlSourceInfo.h>
#include <sdi.h> /* TASK-7219 Shard Transformer Refactoring */

/*
 withStmt 노드의 구성

    with q1 as (select 1 from dual),
         q2 as (select 2 from dual),
         q3 as (select 3 from dual)
      select * from
    (
      with q21 as (select 21 from dual),
           q22 as (select 22 from dual),
           q23 as (select 23 from dual)
      select * from
      (
        with q31 as (select 31 from dual),
             q32 as (select 32 from dual),
             q33 as (select 33 from dual)
        select * from q3
      )
    );

 위와 같은 쿼리는 다음과 같은 withStmt 노드가 구성된다.

                                      ( head (backup) null)
                                        ^
                                        |
                             q1-->q2-->q3                  <----최상위 WITH
                             ^
                             |
                q21-->q22-->q23                            <----from절에 WITH
                ^
                |
    q31-->q32-->q33                                        <----

    qmvWith::validate() head 포인터를 새로 생성 해서 노드를 추가한다.
    with 절이 새로 시작 될때 위 그림 처럼 시작 포인터가 달라진다.
    validate()함수의 하단 부분에 마지막 추가된 노드의 next를 이전 head에 연결해 준다.

    qmvWith::parseViewInTableRef()함수는 주어진 tablename을 위 노드 리스트에 head로 부터
    탐색 하게 된다.

    qmv::parseSelect()함수에서 하위 쿼리문으로 진입 할때 노드의 시작 포인터를 백업 하고
    함수 끝에서 복원 해주어 하위 쿼리가 완료 되면 진입시 레벨이 된다.

    ex)
    select * from
    (
      with q1 as (select i1, i2 from t1),
        q2 as (select i1, i2 from t2)
      select * from
      (
        with q1 as (select i1 from t1)
        select * from q2
      )
    ) v1,
    (
      with q3 as (select i1, i2 from t1),
        q4 as (select i1, i2 from t2)
      select * from
      (
        with q1 as (select i1 from t1)
        select * from q2
      )
    ) v2;

                           head (backup) --> null
                           ^  ^
                           |  |
                    q1 --> q2 |
                     ^        |
                     |        |
                    q1        |
                              |
                       q3 --> q4
                        ^
                        |
                       q1
    q1 --> q1 --> q2 찾음

    q1 --> q3 --> q4 --> top --> null
    q2를 찾지 못함
*/
IDE_RC qmvWith::validate( qcStatement * aStatement )
{
    qcStmtListMgr     * sStmtListMgr; /* BUG-45994 - 컴파일러 최적화 회피 */
    qmsWithClause     * sWithClause;
    qmsParseTree      * sParseTree;
    qmsParseTree      * sViewParseTree = NULL;
    qcWithStmt        * sPrevHead = NULL;
    qcWithStmt        * sNewNode  = NULL;
    qcWithStmt        * sCurNode  = NULL;
    idBool              sFirstQueryBlcok = ID_TRUE;
    idBool              sOrgPSMFlag = ID_FALSE; /* TAKS-7219 Shard Transformer Refactoring */
    idBool              sIsChanged  = ID_FALSE; /* TAKS-7219 Shard Transformer Refactoring */

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmvWith::validate::__FT__" );

    IDE_FT_ASSERT( aStatement != NULL );

    sParseTree = (qmsParseTree*)aStatement->myPlan->parseTree;
    sStmtListMgr = aStatement->myPlan->stmtListMgr;

    /* BUG-45994 */
    IDU_FIT_POINT_FATAL( "qmvWith::validate::__FT__::STAGE1" );

    // codesonar::Null Pointer Dereference
    IDE_FT_ERROR( sStmtListMgr != NULL );

    if ( sParseTree->withClause != NULL )
    {
        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( sdi::setQuerySetListState( aStatement,
                                             NULL, /* aParseTree */
                                             &( sIsChanged ) )
              != IDE_SUCCESS );

        sPrevHead = sStmtListMgr->head;

        for ( sWithClause =  sParseTree->withClause;
              sWithClause != NULL;
              sWithClause =  sWithClause->next )
        {
            /* make new node */
            IDE_TEST( makeWithStmtFromWithClause( aStatement,
                                                  sStmtListMgr,
                                                  sWithClause,
                                                  &sNewNode )
                      != IDE_SUCCESS );

            // recursive with를 검사하기 위해 현재 stmt를 기록한다.
            sStmtListMgr->current = sNewNode;

            /* TASK-7219 Shard Transformer Refactoring */
            IDE_TEST( qmv::parseSelectInternal( sWithClause->stmt )
                      != IDE_SUCCESS );

            IDE_TEST( sdi::setStatementFlagForShard( aStatement,
                                                     sWithClause->stmt->mFlag )
                      != IDE_SUCCESS );

            if ( sNewNode->isRecursiveView == ID_TRUE )
            {
                sViewParseTree = (qmsParseTree*) sWithClause->stmt->myPlan->parseTree;
                
                // union all이 아니면 에러
                if ( sViewParseTree->querySet->setOp != QMS_UNION_ALL )
                {
                    IDE_RAISE( ERR_NOT_NON_EXIST_UNION_ALL_RECURSIVE_VIEW );
                }
                else
                {
                    if ( ( sViewParseTree->querySet->left->setOp != QMS_NONE ) ||
                         ( sViewParseTree->querySet->right->setOp != QMS_NONE ) )
                    {
                        IDE_RAISE( ERR_NOT_MULTI_SET_RECURSIVE_VIEW );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                
                sViewParseTree->querySet->lflag &= ~QMV_QUERYSET_RECURSIVE_VIEW_MASK;
                sViewParseTree->querySet->lflag |= QMV_QUERYSET_RECURSIVE_VIEW_TOP;
            }
            else
            {
                // Nothing to do.
            }

            /* TASK-7219 Shard Transformer Refactoring */
            sOrgPSMFlag                        = sWithClause->stmt->calledByPSMFlag;
            sWithClause->stmt->calledByPSMFlag = aStatement->calledByPSMFlag;

            IDE_TEST( qmv::validateSelect( sWithClause->stmt )
                      != IDE_SUCCESS );

            /* TASK-7219 Shard Transformer Refactoring */
            sWithClause->stmt->calledByPSMFlag = sOrgPSMFlag;

            sStmtListMgr->current = NULL;
            
            // column alias
            IDE_TEST( validateColumnAlias( sWithClause )
                      != IDE_SUCCESS );
            
            /* 상위 노드와 연결 */
            sNewNode->next = sPrevHead;

            if ( sFirstQueryBlcok == ID_TRUE )
            {
                /* 최초 생성시에는 헤드에 붙인다. */
                sStmtListMgr->head     = sNewNode;
                sCurNode               = sNewNode;
                sFirstQueryBlcok       = ID_FALSE;
            }
            else
            {
                /* 이후에는 뒤에 붙인다. */
                sCurNode->next = sNewNode;
                sCurNode       = sNewNode;
            }
        }

        /* TASK-7219 Shard Transformer Refactoring */
        IDE_TEST( sdi::unsetQuerySetListState( aStatement,
                                               sIsChanged )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do. */
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_NON_EXIST_UNION_ALL_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NON_EXIST_UNION_ALL_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION( ERR_NOT_MULTI_SET_RECURSIVE_VIEW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_MULTI_SET_RECURSIVE_VIEW ) );
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    if ( sStmtListMgr != NULL )
    {
        sStmtListMgr->current = NULL;
    }
    else
    {
        // Nothing to do.
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}


IDE_RC qmvWith::parseViewInTableRef( qcStatement * aStatement,
                                     qmsTableRef * aTableRef )
{
    qcWithStmt     * sCurWithStmt;
    idBool           sIsFound =  ID_FALSE;
    qmsQuerySet    * sQuerySet = NULL;

    IDU_FIT_POINT_FATAL( "qmvWith::parseViewInTableRef::__FT__" );

    IDE_FT_ASSERT( aStatement != NULL );

    while ( 1 )
    {
        //-----------------------------------
        // with subquery
        //-----------------------------------
        
        sCurWithStmt = aStatement->myPlan->stmtListMgr->head;

        while ( sCurWithStmt != NULL )
        {
            if ( QC_IS_NAME_MATCHED( aTableRef->tableName,
                                     sCurWithStmt->stmtName ) == ID_TRUE )
            {
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }

            sCurWithStmt = sCurWithStmt->next;
        }

        if ( sIsFound == ID_TRUE )
        {
            if ( sCurWithStmt->isRecursiveView == ID_TRUE )
            {
                if ( sCurWithStmt->isTop == ID_TRUE )
                {
                    // 최상위 recursive view
                    IDE_TEST( makeParseTree( aStatement,
                                             aTableRef,
                                             sCurWithStmt )
                              != IDE_SUCCESS );

                    // view를 recursive view로 변경
                    aTableRef->recursiveView = aTableRef->view;

                    IDE_TEST( makeParseTree( aStatement,
                                             aTableRef,
                                             sCurWithStmt )
                              != IDE_SUCCESS );

                    // view를 recursive view로 변경 (재귀호출용 임시)
                    aTableRef->tempRecursiveView = aTableRef->view;
                    
                    aTableRef->view = NULL;

                    /* BUG-46932 from 절에 recursive with view를 사용하고 있다 */
                    sQuerySet = ((qmsParseTree*)aStatement->myPlan->parseTree)->querySet;
                    sQuerySet->lflag &= ~QMV_QUERYSET_FROM_RECURSIVE_WITH_MASK;
                    sQuerySet->lflag |= QMV_QUERYSET_FROM_RECURSIVE_WITH_TRUE;

                    // 다음 부터는 하위 recursive view이다.
                    sCurWithStmt->isTop = ID_FALSE;
                }
                else
                {
                    // 하위 recursive view
                    // Nothing to do.
                }
                
                // recursive view인 경우
                aTableRef->flag &= ~QMS_TABLE_REF_RECURSIVE_VIEW_MASK;
                aTableRef->flag |= QMS_TABLE_REF_RECURSIVE_VIEW_TRUE;
            }
            else
            {
                IDE_TEST( makeParseTree( aStatement,
                                         aTableRef,
                                         sCurWithStmt )
                          != IDE_SUCCESS );
            }

            // 찾은 withStmtList를 할당 한다.
            aTableRef->withStmt = sCurWithStmt;
            
            // BUG-48090 aTableRef에 With View 플래그를 설정한다.
            aTableRef->flag &= ~QMS_TABLE_REF_WITH_VIEW_MASK;
            aTableRef->flag |= QMS_TABLE_REF_WITH_VIEW_TRUE;
            break;
        }
        else
        {
            // Nothing to do.
        }

        //-----------------------------------
        // PROJ-2582 recursive with
        //-----------------------------------
        
        sCurWithStmt = aStatement->myPlan->stmtListMgr->current;
        
        if ( sCurWithStmt != NULL )
        {
            if ( QC_IS_NAME_MATCHED( aTableRef->tableName,
                                     sCurWithStmt->stmtName ) == ID_TRUE )
            {
                sIsFound = ID_TRUE;
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
        
        if ( sIsFound == ID_TRUE )
        {
            // recursive with이다.
            sCurWithStmt->isRecursiveView = ID_TRUE;
            // 다음은 최상위 recursive view
            sCurWithStmt->isTop = ID_TRUE;
            
            aTableRef->flag &= ~QMS_TABLE_REF_RECURSIVE_VIEW_MASK;
            aTableRef->flag |= QMS_TABLE_REF_RECURSIVE_VIEW_TRUE;
            
            // 찾은 withStmtList를 할당 한다.
            aTableRef->withStmt = sCurWithStmt;
            break;
        }
        else
        {
            // Nothing to do.
        }

        // 못찾은 경우
        aTableRef->withStmt = NULL;
        break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * with clause를 이용해서 with stmt에 값을 세팅 한다.
 */
IDE_RC qmvWith::makeWithStmtFromWithClause( qcStatement        * aStatement,
                                            qcStmtListMgr      * aStmtListMgr,
                                            qmsWithClause      * aWithClause,
                                            qcWithStmt        ** aNewNode )
{
    qcWithStmt * sNewNode;
    UInt         sNextTableID;

    IDU_FIT_POINT_FATAL( "qmvWith::makeWithStmtFromWithClause::__FT__" );

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcWithStmt),
                                             (void**)&sNewNode )
              != IDE_SUCCESS );

    /* data setting */
    SET_POSITION( sNewNode->stmtName, aWithClause->stmtName );
    SET_POSITION( sNewNode->stmtText, aWithClause->stmtText );
    sNewNode->columns         = aWithClause->columns;
    sNewNode->stmt            = aWithClause->stmt;
    sNewNode->isRecursiveView = ID_FALSE;
    sNewNode->isTop           = ID_FALSE;
    sNewNode->tableInfo       = NULL;
    sNewNode->next            = NULL;

    sNextTableID = (UInt)(aStmtListMgr->tableIDSeqNext + QCM_WITH_TABLES_SEQ_MINVALUE );

    /* tableID boundary check */
    IDE_TEST_RAISE( sNextTableID >= UINT_MAX, ERR_EXCEED_TABLEID );

    sNewNode->tableID = sNextTableID;

    /* tableID sequence 증가 */
    aStmtListMgr->tableIDSeqNext++;

    *aNewNode = sNewNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_TABLEID )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvWith::makeWithStmtFromWithClause",
                                  "Invalid tableIDSeqNext" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * with stmt list 에서 얻은 stmtText를 이용해서 parse tree를 생성한다.
 * 1. stmtText를 이용해서 parse tree를 만든다.
 * 2. aTableRef->view에 statement를 할당해서 inline 뷰로 만들어준다.
 * 3. stmtName으로 alias를 생성한다.
 */
IDE_RC qmvWith::makeParseTree( qcStatement * aStatement,
                               qmsTableRef * aTableRef,
                               qcWithStmt  * aWithStmt )
{
    qcStatement * sStatement;

    IDU_FIT_POINT_FATAL( "qmvWith::makeParseTree::__FT__" );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcStatement, &sStatement )
              != IDE_SUCCESS);

    // set meber of qcStatement
    idlOS::memcpy( sStatement, aStatement, ID_SIZEOF(qcStatement) );

    /* TASK-7219 */
    SDI_INIT_PRINT_INFO( &( sStatement->mShardPrintInfo ) );

    // myPlan을 재설정한다.
    sStatement->myPlan = &sStatement->privatePlan;
    sStatement->myPlan->planEnv = NULL;

    sStatement->myPlan->parseTree   = NULL;
    sStatement->myPlan->plan        = NULL;

    sStatement->myPlan->stmtText    = aWithStmt->stmtText.stmtText;
    sStatement->myPlan->stmtTextLen = idlOS::strlen( aWithStmt->stmtText.stmtText );

    // parsing view    
    IDE_TEST( qcpManager::parsePartialForQuerySet( sStatement,
                                                   aWithStmt->stmtText.stmtText,
                                                   aWithStmt->stmtText.offset,
                                                   aWithStmt->stmtText.size )
              != IDE_SUCCESS );

    // set parse tree
    aTableRef->view = sStatement;

    // alias name setting
    if ( QC_IS_NULL_NAME( aTableRef->aliasName ) == ID_TRUE )
    {
        SET_POSITION( aTableRef->aliasName, aTableRef->tableName );
    }
    else
    {
        /* Nothing to do. */
    }

    // planEnv를 재설정한다.
    aTableRef->view->myPlan->planEnv = aStatement->myPlan->planEnv;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvWith::validateColumnAlias( qmsWithClause  * aWithClause )
{
    qmsParseTree * sParseTree = NULL;
    qmsTarget    * sTarget = NULL;
    qcmColumn    * sColumn = NULL;
    SInt           sColumnCount = 0;

    IDU_FIT_POINT_FATAL( "qmvWith::validateColumnAlias::__FT__" );

    sParseTree = (qmsParseTree *)aWithClause->stmt->myPlan->parseTree;
    
    if ( aWithClause->columns != NULL )
    {
        // columns의 갯수와 target의 갯수가 동일해야 한다.
        for ( sColumn = aWithClause->columns,
                  sTarget = sParseTree->querySet->target;
              sColumn != NULL;
              sColumn = sColumn->next, sTarget = sTarget->next )
        {
            IDE_TEST_RAISE( sTarget == NULL, ERR_MISMATCH_NUMBER_OF_COLUMN );
            
            sColumnCount++;
        }

        IDE_TEST_RAISE( sTarget != NULL, ERR_MISMATCH_NUMBER_OF_COLUMN );
    }
    else
    {
        // with clause target column count
        for ( sTarget = sParseTree->querySet->target;
              sTarget != NULL;
              sTarget = sTarget->next )
        {
            sColumnCount++;
        }
    }
    
    IDE_TEST_RAISE( sColumnCount > QC_MAX_COLUMN_COUNT,
                    ERR_INVALID_COLUMN_COUNT );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MISMATCH_NUMBER_OF_COLUMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_MISMATCH_COL_COUNT ) );
    }
    IDE_EXCEPTION( ERR_INVALID_COLUMN_COUNT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_INVALID_COLUMN_COUNT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
