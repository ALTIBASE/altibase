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
#include <cm.h>
#include <qcg.h>
#include <qcgPlan.h>
#include <qcpManager.h>
#include <qtc.h>
#include <qmvShardTransform.h>
#include <qcuSqlSourceInfo.h>
#include <qmv.h>
#include <qmo.h>
#include <qmx.h>
#include <qsvEnv.h>
#include <qcpUtil.h>
#include <qciStmtType.h>
#include <sdi.h>

extern mtfModule mtfAvg;
extern mtfModule mtfCount;
extern mtfModule mtfSum;
extern mtfModule mtfMin;
extern mtfModule mtfMax;
extern mtfModule mtfDivide;
extern mtfModule mtfDecrypt;
extern mtdModule mtdSmallint;
extern mtfModule mtfCast; /* TASK-7219 */

/* TASK-7219 */
#define SHARD_ORDER_BY_PREFIX  "_$"
#define SHARD_ORDER_BY_LENGTH  10 /* _$65535_$9 */

IDE_RC qmvShardTransform::doTransform( qcStatement  * aStatement )
{
/***********************************************************************
 *
 * Description : Shard View Transform
 *     shard table이 포함된 쿼리에서 명시적인 shard view가 아니거나,
 *     shard 키워드가 없는 쿼리를 shard view나 shard 쿼리로
 *     변환한다.
 *
 *     예1) top query, query 전체가 shard query인 경우
 *          select * from t1 where i1=1 order by i1;
 *          --> select * from shard(select * from t1 where i1=1 order by i1);
 *
 *     예2) view가 shard query인 경우
 *          select * from (select * from t1 where i1=1);
 *          --> select * from shard(select * from t1 where i1=1);
 *
 *     예3) querySet이 shard query인 경우
 *          select * from t1 where i1=1 order by i2 loop 2;
 *          --> select * from shard(select * from t1 where i1=1) order by i2 loop 2;
 *
 *          select * from t1 where i1=1
 *          union all
 *          select * from t2 where i2=1;
 *          --> select * from shard(select * from t1 where i1=1)
 *              union all
 *              select * from t2 where i2=1;
 *
 *     예4) from-where가 shard query인 경우
 *          select func1(i1) from t1 where i1=1;
 *          --> select func1(i1) from (select * from t1 where i1=1);
 *
 *          select * from t1, t2 where t1.i1=t2.i1 and t1.i1=1;
 *          --> select * from (select * from t1 where t1.i1=1) v1, t2 where v1.i1=t2.i1;
 *
 *     예5) from만 shard table인 경우
 *          select * from t1, t2 where t1.i1=t2.i1 and t1.i1=1;
 *          --> select * from (select * from t1) v1, t2 where v1.i1=t2.i1 and v1.i1=1;
 *
 *     예6) DML, query 전체가 shard query인 경우
 *          insert into t1 values (1, 2);
 *          --> shard insert into t1 values (1, 2);
 *
 *          update t1 set i2=1 where i1=1;
 *          --> shard update t1 set i2=1 where i1=1;
 *
 *          delete from t1 where i1=1;
 *          --> shard delete from t1 where i1=1;
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool sIsTransformNeeded = ID_FALSE;

    /* PROJ-2745 1. rebuild event 감지 및 rebuild retry 여부 확인 */
    if ( sdi::detectShardMetaChange( aStatement ) == ID_TRUE )
    {
        IDE_TEST( sdi::isRebuildTransformNeeded( aStatement,
                                                 &( sIsTransformNeeded ) )
                  != IDE_SUCCESS );

        if ( sIsTransformNeeded == ID_TRUE )
        {
            IDE_TEST( rebuildTransform( aStatement ) != IDE_SUCCESS );

            /* PROJ-2745 상태를 다시 초기값으로 변경 */
            SDI_SET_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                         SDI_QUERYSET_LIST_STATE_MAIN_MAKE );
        }
        else
        {
            /* Nothing to do */
        }
    }
    /* PROJ-2745 NOT ELSE */
    /* 2. Shard View Transform 수행 */
    if ( sdi::isShardCoordinator( aStatement ) == ID_TRUE )
    {
        IDE_TEST( sdi::isTransformNeeded( aStatement,
                                          &( sIsTransformNeeded ) )
                  != IDE_SUCCESS );

        if ( sIsTransformNeeded == ID_TRUE )
        {
            SDI_SET_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                         SDI_QUERYSET_LIST_STATE_MAIN_ALL );

            IDE_TEST( processTransform( aStatement ) != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else if ( sdi::isPartialCoordinator( aStatement ) == ID_TRUE )
    {
        /* 3. Partial Coordinator 수행 */
        IDE_TEST( sdi::isTransformNeeded( aStatement,
                                          &( sIsTransformNeeded ) )
                  != IDE_SUCCESS );

        if ( sIsTransformNeeded == ID_TRUE )
        {
            SDI_SET_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                         SDI_QUERYSET_LIST_STATE_MAIN_ALL );

            IDE_TEST( partialCoordTransform( aStatement ) != IDE_SUCCESS );
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransform( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : Shard View Transform
 *
 * Implementation :
 *     top query block이나 subquery의 경우 inline view를 한번 더 씌워야 한다.
 *
 ***********************************************************************/

    idBool           sIsShardParseTree = ID_FALSE;
    idBool           sIsShardObject    = ID_TRUE;
    ULong            sSMN              = ID_ULONG(0);        /* PROJ-2701 Online data rebuild */
    qciStmtType      sStmtKind         = QCI_STMT_MASK_MAX;
    qcShardStmtType  sStmtType         = QC_STMT_SHARD_NONE;
    qcNamePosition * sStmtPos          = NULL;
    qcParseTree    * sParseTree        = NULL;
    qmsParseTree   * sSelParseTree     = NULL;
    qmsQuerySet    * sQuerySet         = NULL;

    /* 1. 정합성 검사 */
    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    /* 2. 초기화 */
    sSMN       = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );
    sStmtKind  = aStatement->myPlan->parseTree->stmtKind;
    sStmtType  = aStatement->myPlan->parseTree->stmtShard;
    sStmtPos   = &( aStatement->myPlan->parseTree->stmtPos );
    sParseTree = aStatement->myPlan->parseTree;

    switch ( sStmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            sSelParseTree = (qmsParseTree *)( aStatement->myPlan->parseTree );
            sQuerySet     = sSelParseTree->querySet;
            break;

        case QCI_STMT_INSERT:
        case QCI_STMT_UPDATE:
        case QCI_STMT_DELETE:
        case QCI_STMT_EXEC_PROC:
            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_KIND );
            break;
    }

    /* 3. Shard View Transform의 수행 */
    switch ( sStmtType )
    {
        case QC_STMT_SHARD_NONE:
            /* Shard Keyword 를 사용하지 않는 경우,
             *  Shard Query 여부에 따라서
             *   Shard Query 또는
             *    Non-Shard Query 로 변환한다.
             */
            IDE_TEST( doShardAnalyze( aStatement,
                                      sStmtPos,
                                      sSMN,
                                      sQuerySet )
                      != IDE_SUCCESS );

            IDE_TEST( sdi::isShardParseTree( sParseTree,
                                             &( sIsShardParseTree ) )
                      != IDE_SUCCESS );

            if ( sIsShardParseTree == ID_TRUE )
            {
                IDE_TEST( processTransformForShard( aStatement,
                                                    sParseTree )
                      != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( processTransformForNonShard( aStatement,
                                                       sParseTree )
                          != IDE_SUCCESS );
            }
            break;

        case QC_STMT_SHARD_ANALYZE:
            /* Shard Keyword 인 경우,
             *  분산 가능한 Query 인지 분석한 후,
             *   불가능한 형태를 제외하고, 최대한 Shard Query 로 변환한다.
             *    따라서 Shard 로 수행할 Query 범위를 지정하여 준다.
             */
            IDE_TEST( doShardAnalyze( aStatement,
                                      sStmtPos,
                                      sSMN,
                                      sQuerySet )
                      != IDE_SUCCESS );

            IDE_TEST( sdi::isShardObject( sParseTree,
                                          &( sIsShardObject ) )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsShardObject == ID_FALSE, ERR_NOT_SHARD_OBJECT );

            IDE_TEST( processTransformForShard( aStatement,
                                                sParseTree )
                      != IDE_SUCCESS );
            break;

        case QC_STMT_SHARD_DATA:
            /* Data Keyword 인 경우,
             *  분석없이
             *   지정한 Data Node 에 보내는 Query 로 변환한다.
             *    따라서 수행할 노드를 지정하여 준다.
             */
            IDE_TEST( processTransformForShard( aStatement,
                                                sParseTree )
                      != IDE_SUCCESS );
            break;

        case QC_STMT_SHARD_META:
            /* Meta Keyword 인 경우,
             *  일반 Query 로 처리한다.
             */
            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_TYPE );
            break;
    }

    if ( QC_SHARED_TMPLATE( aStatement )->stmt == aStatement )
    {
        IDE_TEST( sdi::makeAndSetAnalyzeInfoFromStatement( aStatement )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SHARD_OBJECT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransform",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_KIND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransform",
                                  "stmt kind is invalid" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransform",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardStatement( qcStatement    * aStatement,
                                              qcNamePosition * aParsePosition,
                                              qcShardStmtType  aShardStmtType,
                                              sdiAnalyzeInfo * aShardAnalysis )
{
/***********************************************************************
 *
 * Description : Shard View Transform
 *
 *     statement 전체를 shard view로 생성한다.
 *
 *     select i1, i2 from t1 where i1=1 order by i1;
 *     --------------------------------------------
 *     -->
 *     select * from shard(select i1, i2 from t1 where i1=1 order by i1);
 *                         --------------------------------------------
 *
 *     aStatement-parseTree
 *     -->
 *     aStatement-parseTree'-querySet'-SFWGH'-from'-tableRef'-sStatement'-parseTree
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement  * sStatement = NULL;
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsSFWGH     * sSFWGH     = NULL;
    qmsFrom      * sFrom      = NULL;
    qmsTableRef  * sTableRef  = NULL;
    qcNamePosition sNullPosition;

    IDU_FIT_POINT_FATAL( "qmvShardTransform::makeShardStatement::__FT__" );

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qcStatement,
                            &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsParseTree,
                            &sParseTree )
              != IDE_SUCCESS );
    QC_SET_INIT_PARSE_TREE( sParseTree, sNullPosition );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsQuerySet,
                            &sQuerySet )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_QUERY_SET( sQuerySet );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsSFWGH,
                            &sSFWGH )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_SFWGH( sSFWGH );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsFrom,
                            &sFrom )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_FROM( sFrom );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsTableRef,
                            &sTableRef )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

    // aStatement를 교체할 수 없으므로 sStatement를 복사 생성한다.
    idlOS::memcpy( sStatement, aStatement, ID_SIZEOF(qcStatement) );
    // myPlan을 재설정한다.
    sStatement->myPlan = & sStatement->privatePlan;

    sTableRef->view      = sStatement;
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    // parseTree를 생성한다.
    sParseTree->withClause         = NULL;
    sParseTree->querySet           = sQuerySet;
    sParseTree->orderBy            = NULL;
    sParseTree->limit              = NULL;
    sParseTree->loopNode           = NULL;
    sParseTree->forUpdate          = NULL;
    sParseTree->queue              = NULL;
    sParseTree->isTransformed      = ID_FALSE;
    sParseTree->isView             = ID_TRUE;
    sParseTree->isShardView        = ID_FALSE;
    sParseTree->common.currValSeqs = NULL;
    sParseTree->common.nextValSeqs = NULL;
    sParseTree->common.parse       = qmv::parseSelect;
    sParseTree->common.validate    = qmv::validateSelect;
    sParseTree->common.optimize    = qmo::optimizeSelect;
    sParseTree->common.execute     = qmx::executeSelect;

    // aStatement의 parseTree를 변경한다.
    aStatement->myPlan->parseTree = (qcParseTree*) sParseTree;
    aStatement->myPlan->parseTree->stmtKind =
        sStatement->myPlan->parseTree->stmtKind;

    // sStatement를 shard view로 변경한다.
    SET_POSITION( sStatement->myPlan->parseTree->stmtPos, *aParsePosition );
    sStatement->myPlan->parseTree->stmtShard = aShardStmtType;

    // 분석결과를 기록한다.
    sStatement->myPlan->mShardAnalysis = aShardAnalysis;

    /* TASK-7219 */
    IDE_TEST( qmg::makeShardParamOffsetArray( aStatement,
                                              aParsePosition,
                                              &( sStatement->myPlan->mShardParamCount ),
                                              &( sStatement->myPlan->mShardParamOffset ),
                                              &( sStatement->myPlan->mShardParamInfo ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardView( qcStatement    * aStatement,
                                         qmsTableRef    * aTableRef,
                                         sdiAnalyzeInfo * aShardAnalysis,
                                         qcNamePosition * aFilter )
{
/***********************************************************************
 *
 * Description : Shard View Transform
 *
 *     table을 shard view로 생성한다.
 *
 *     select * from sys.t1, t2 where t1.i1=t2.i1;
 *                   ------
 *     -->
 *     select * from shard(select * from sys.t1) t1, t2 where t1.i1=t2.i1 and t1.i1=1;
 *                   ------------------------------
 *
 *     select * from t1 a, t2 where a.i1=t2.i1;
 *                   ----
 *     -->
 *     select * from shard(select * from t1 a) a, t2 where a.i1=t2.i1 and a.i1=1;
 *                   ---------------------------
 *
 *     select * from t1 pivot (...), t2 where ...;
 *                   --
 *     -->
 *     select * from shard(select * from t1) t1 pivot (...), t2 where ...;
 *                   --------------------------
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement     * sStatement = NULL;
    qmsParseTree    * sParseTree = NULL;
    qmsQuerySet     * sQuerySet  = NULL;
    qmsSFWGH        * sSFWGH     = NULL;
    qmsFrom         * sFrom      = NULL;
    qmsTableRef     * sTableRef  = NULL;
    SChar           * sQueryBuf  = NULL;
    qcNamePosition    sQueryPosition;
    qcNamePosition    sNullPosition;
    SInt              sTransformStringMaxSize = (SDU_SHARD_TRANSFORM_STRING_LENGTH_MAX+1);
    qcNamePosition    sPosition; /* TASK-7219 */

    IDU_FIT_POINT_FATAL( "qmvShardTransform::makeShardView::__FT__" );

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qcStatement,
                          &sStatement)
             != IDE_SUCCESS);
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qmsParseTree,
                          &sParseTree)
             != IDE_SUCCESS);
    QC_SET_INIT_PARSE_TREE( sParseTree, sNullPosition );
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qmsQuerySet,
                          &sQuerySet)
             != IDE_SUCCESS);
    QCP_SET_INIT_QMS_QUERY_SET( sQuerySet );
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qmsSFWGH,
                          &sSFWGH)
             != IDE_SUCCESS);
    QCP_SET_INIT_QMS_SFWGH( sSFWGH );

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qmsFrom,
                          &sFrom)
             != IDE_SUCCESS);
    QCP_SET_INIT_QMS_FROM(sFrom);
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          qmsTableRef,
                          &sTableRef)
             != IDE_SUCCESS);
    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

    /* Copy existed TableRef to new TableRef except alias */
    idlOS::memcpy( sTableRef, aTableRef, ID_SIZEOF( qmsTableRef ) );
    QCP_SET_INIT_QMS_TABLE_REF(aTableRef);

    /* Set alias */
    if (QC_IS_NULL_NAME(sTableRef->aliasName) == ID_TRUE)
    {
        SET_POSITION(aTableRef->aliasName, sTableRef->tableName);
    }
    else
    {
        SET_POSITION(aTableRef->aliasName, sTableRef->aliasName);
    }

    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    sParseTree->withClause         = NULL;
    sParseTree->querySet           = sQuerySet;
    sParseTree->orderBy            = NULL;
    sParseTree->limit              = NULL;
    sParseTree->loopNode           = NULL;
    sParseTree->forUpdate          = NULL;
    sParseTree->queue              = NULL;
    sParseTree->isTransformed      = ID_FALSE;
    sParseTree->isSiblings         = ID_FALSE;
    sParseTree->isView             = ID_TRUE;
    sParseTree->isShardView        = ID_FALSE;
    sParseTree->common.currValSeqs = NULL;
    sParseTree->common.nextValSeqs = NULL;
    sParseTree->common.parse       = qmv::parseSelect;
    sParseTree->common.validate    = qmv::validateSelect;
    sParseTree->common.optimize    = qmo::optimizeSelect;
    sParseTree->common.execute     = qmx::executeSelect;

    QC_SET_STATEMENT( sStatement, aStatement, sParseTree );
    sStatement->myPlan->parseTree->stmtKind = QCI_STMT_SELECT;
    sStatement->myPlan->parseTree->stmtShard = QC_STMT_SHARD_ANALYZE;

    // set select query
    IDE_TEST_RAISE( ( QC_IS_NULL_NAME( sTableRef->position ) == ID_TRUE ) ||
                    ( QC_IS_NULL_NAME( aTableRef->aliasName ) == ID_TRUE ),
                    ERR_NULL_POSITION );
    IDE_TEST(STRUCT_ALLOC_WITH_COUNT(QC_QMP_MEM(aStatement),
                                     SChar,
                                     sTransformStringMaxSize,
                                     &sQueryBuf)
             != IDE_SUCCESS);
    /* TASK-7219 */
    sQueryPosition.stmtText = sQueryBuf;
    sQueryPosition.offset = 0;
    /* SELECT * */
    sQueryPosition.size = idlOS::snprintf( sQueryBuf, sTransformStringMaxSize, "SELECT * " );
    /* Make sFrom->fromPosition */
    sPosition.stmtText = sQueryBuf;
    sPosition.offset   = sQueryPosition.size;
    /* FROM */
    sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                            sTransformStringMaxSize - sQueryPosition.size,
                                            "FROM " );
    /* Make sFrom->fromPosition */
    sPosition.size = sQueryPosition.size - sPosition.offset;
    /* Set sFrom->fromPosition */
    SET_POSITION( sFrom->fromPosition, sPosition );
    if ( sTableRef->position.stmtText[sTableRef->position.offset - 1] == '"' )
    {
        // "SYS".t1
        sQueryBuf[sQueryPosition.size] = '"';
        sQueryPosition.size++;

        /* Make sTableRef->position */
        sPosition.stmtText = sQueryBuf;
        sPosition.offset   = sQueryPosition.size;
    }
    else
    {
        /* Make sTableRef->position */
        sPosition.stmtText = sQueryBuf;
        sPosition.offset   = sQueryPosition.size;
    }
    sQueryPosition.size +=
        idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                         sTransformStringMaxSize - sQueryPosition.size,
                         "%.*s",
                         sTableRef->position.size,
                         sTableRef->position.stmtText + sTableRef->position.offset );
    if ( sTableRef->position.stmtText[sTableRef->position.offset +
                                      sTableRef->position.size] == '"' )
    {
        /* Make sTableRef->position */
        sPosition.size = sQueryPosition.size - sPosition.offset;

        // sys."T1"
        sQueryBuf[sQueryPosition.size] = '"';
        sQueryPosition.size++;
    }
    else
    {
        /* Make sTableRef->position */
        sPosition.size = sQueryPosition.size - sPosition.offset;
    }
    /* Set sTableRef->position */
    SET_POSITION( sTableRef->position, sPosition );
    sQueryPosition.size +=
        idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                         sTransformStringMaxSize - sQueryPosition.size,
                         " as " );
    if ( aTableRef->aliasName.stmtText[aTableRef->aliasName.offset +
                                       aTableRef->aliasName.size] == '"' )
    {
        /* Make sTableRef->aliasName */
        sPosition.stmtText = sQueryBuf;
        sPosition.offset   = sQueryPosition.size;

        // t1 "a"
        sQueryPosition.size +=
        idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                         sTransformStringMaxSize - sQueryPosition.size,
                         "\"%.*s\"",
                         aTableRef->aliasName.size,
                         aTableRef->aliasName.stmtText + aTableRef->aliasName.offset );

        /* Make sTableRef->aliasName */
        sPosition.size = sQueryPosition.size - sPosition.offset;
    }
    else if ( aTableRef->aliasName.stmtText[aTableRef->aliasName.offset +
                                            aTableRef->aliasName.size] == '\'' )
    {
        /* Make sTableRef->aliasName */
        sPosition.stmtText = sQueryBuf;
        sPosition.offset   = sQueryPosition.size;

        // t1 'a'
        sQueryPosition.size +=
        idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                         sTransformStringMaxSize - sQueryPosition.size,
                         "'%.*s'",
                         aTableRef->aliasName.size,
                         aTableRef->aliasName.stmtText + aTableRef->aliasName.offset );

        /* Make sTableRef->aliasName */
        sPosition.size = sQueryPosition.size - sPosition.offset;
    }
    else
    {
        /* Make sTableRef->aliasName */
        sPosition.stmtText = sQueryBuf;
        sPosition.offset   = sQueryPosition.size;

        // t1 a
        sQueryPosition.size +=
        idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                         sTransformStringMaxSize - sQueryPosition.size,
                         "%.*s",
                         aTableRef->aliasName.size,
                         aTableRef->aliasName.stmtText + aTableRef->aliasName.offset );

        /* Make sTableRef->aliasName */
        sPosition.size = sQueryPosition.size - sPosition.offset;
    }
    /* Set sTableRef->aliasName */
    SET_POSITION( sTableRef->aliasName, sPosition );

    /* PROJ-2701 Sharding online data rebuild */
    if ( aFilter != NULL )
    {
        sQueryPosition.size +=
            idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                             sTransformStringMaxSize - sQueryPosition.size,
                             " where " );

        /* TASK-7219 */
        sPosition.stmtText = sQueryBuf;
        sPosition.offset   = sQueryPosition.size;

        sQueryPosition.size +=
            idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                             sTransformStringMaxSize - sQueryPosition.size,
                             "%.*s",
                             aFilter->size,
                             aFilter->stmtText + aFilter->offset );

        /* TASK-7219 */
        IDE_TEST_RAISE( sQueryPosition.size >= ( sTransformStringMaxSize - 1 ), ERR_TRANSFORM_STRING_BUFFER_OVERFLOW );

        sStatement->myPlan->stmtText = sPosition.stmtText;
        sStatement->myPlan->stmtTextLen = idlOS::strlen( sPosition.stmtText );

        IDE_TEST( qcpManager::parsePartialForWhere( sStatement,
                                                    sPosition.stmtText,
                                                    sPosition.offset,
                                                    aFilter->size )
                  != IDE_SUCCESS );

        sSFWGH->where = ((qmsParseTree *)sStatement->myPlan->parseTree)->querySet->SFWGH->where;

        QC_SET_STATEMENT( sStatement, aStatement, sParseTree );

        sStatement->myPlan->parseTree->stmtKind = QCI_STMT_SELECT;
        sStatement->myPlan->parseTree->stmtShard = QC_STMT_SHARD_ANALYZE;
    }
    else
    {
        // Nothing to do.
    }

    // Transformation string buffer overflow
    IDE_TEST_RAISE( sQueryPosition.size >= (sTransformStringMaxSize-1), ERR_TRANSFORM_STRING_BUFFER_OVERFLOW );
    
    SET_POSITION( sStatement->myPlan->parseTree->stmtPos, sQueryPosition );

    // 분석결과를 기록한다.
    sStatement->myPlan->mShardAnalysis = aShardAnalysis;

    /* Set transformed inline view */
    aTableRef->view = sStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardView",
                                  "null position" ));
    }
    IDE_EXCEPTION( ERR_TRANSFORM_STRING_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardView",
                                  "Transformation string buffer overflow" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::addColumnListToText( qtcNode        * aNode,
                                               UInt           * aColumnCount,
                                               SChar          * aQueryBuf,
                                               UInt             aQueryBufSize,
                                               qcNamePosition * aQueryPosition )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               Node tree를 순회하면서
 *               순수 column의 position을 string에 기록한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qtcNode * sNode = NULL;

    IDU_FIT_POINT_FATAL( "qmvShardTransform::addColumnListToText::__FT__" );

    IDE_DASSERT( aNode != NULL );

    if ( aNode->node.module == &qtc::columnModule)
    {
        // Add to text
        if ( (*aColumnCount) != 0 )
        {
            aQueryBuf[aQueryPosition->size] = ',';
            aQueryPosition->size++;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aNode->position.stmtText[aNode->position.offset - 1] == '"' )
        {
            aQueryBuf[aQueryPosition->size] = '"';
            aQueryPosition->size++;
        }
        else
        {
            // Nothing to do.
        }

        aQueryPosition->size +=
            idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                             aQueryBufSize - aQueryPosition->size,
                             "%.*s",
                             aNode->position.size,
                             aNode->position.stmtText + aNode->position.offset );

        if ( aNode->position.stmtText[aNode->position.offset +
                                      aNode->position.size] == '"' )
        {
            aQueryBuf[aQueryPosition->size] = '"';
            aQueryPosition->size++;
        }
        else
        {
            // Nothing to do.
        }

        (*aColumnCount)++;
    }
    else if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_SUBQUERY )
    {
        // Nothing to do.
    }
    else
    {
        // Traverse
        for ( sNode  = (qtcNode*)aNode->node.arguments;
              sNode != NULL;
              sNode  = (qtcNode*)sNode->node.next )
        {
            IDE_TEST( addColumnListToText( sNode,
                                           aColumnCount,
                                           aQueryBuf,
                                           aQueryBufSize,
                                           aQueryPosition )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::addAggrListToText( qcStatement       * aStatement,
                                             qcParamOffsetInfo * aParamOffsetInfo,
                                             qmsTarget         * aTarget,
                                             qtcNode           * aNode,
                                             UInt              * aAggrCount,
                                             SChar             * aQueryBuf,
                                             UInt                aQueryBufSize,
                                             qcNamePosition    * aQueryPosition,
                                             idBool            * aUnsupportedAggr )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               Node tree를 순회하면서
 *               Aggregate function의 real position을 string에 기록한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qtcNode * sNode = NULL;

    IDU_FIT_POINT_FATAL( "qmvShardTransform::addAggrListToText::__FT__" );

    IDE_DASSERT( aNode != NULL );

    if ( QTC_IS_AGGREGATE(aNode) == ID_TRUE )
    {
        if ( (*aAggrCount) != 0 )
        {
            aQueryBuf[aQueryPosition->size] = ',';
            aQueryPosition->size++;
        }
        else
        {
            // Nothing to do.
        }

        // BUG-45176
        if ( ( aNode->node.lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_FALSE )
        {
            if ( aNode->node.module == &mtfSum )
            {
                IDE_TEST( addSumMinMaxCountToText( aTarget,
                                                   aNode,
                                                   aQueryBuf,
                                                   aQueryBufSize,
                                                   aQueryPosition )
                          != IDE_SUCCESS );

                IDE_TEST( qmg::findAndCollectParamOffset( aStatement,
                                                          aNode,
                                                          aParamOffsetInfo )
                          != IDE_SUCCESS );
            }
            else if ( aNode->node.module == &mtfMin )
            {
                IDE_TEST( addSumMinMaxCountToText( aTarget,
                                                   aNode,
                                                   aQueryBuf,
                                                   aQueryBufSize,
                                                   aQueryPosition )
                          != IDE_SUCCESS );

                IDE_TEST( qmg::findAndCollectParamOffset( aStatement,
                                                          aNode,
                                                          aParamOffsetInfo )
                          != IDE_SUCCESS );
            }
            else if ( aNode->node.module == &mtfMax )
            {
                IDE_TEST( addSumMinMaxCountToText( aTarget,
                                                   aNode,
                                                   aQueryBuf,
                                                   aQueryBufSize,
                                                   aQueryPosition )
                          != IDE_SUCCESS );

                IDE_TEST( qmg::findAndCollectParamOffset( aStatement,
                                                          aNode,
                                                          aParamOffsetInfo )
                          != IDE_SUCCESS );
            }
            else if ( aNode->node.module == &mtfCount )
            {
                IDE_TEST( addSumMinMaxCountToText( aTarget,
                                                   aNode,
                                                   aQueryBuf,
                                                   aQueryBufSize,
                                                   aQueryPosition )
                          != IDE_SUCCESS );

                IDE_TEST( qmg::findAndCollectParamOffset( aStatement,
                                                          aNode,
                                                          aParamOffsetInfo )
                          != IDE_SUCCESS );
            }
            else if ( aNode->node.module == &mtfAvg )
            {
                IDE_TEST( addAvgToText( aTarget,
                                        aNode,
                                        aQueryBuf,
                                        aQueryBufSize,
                                        aQueryPosition )
                          != IDE_SUCCESS );

                IDE_TEST( qmg::findAndCollectParamOffset( aStatement,
                                                          aNode,
                                                          aParamOffsetInfo )
                          != IDE_SUCCESS );

                IDE_TEST( qmg::findAndCollectParamOffset( aStatement,
                                                          aNode,
                                                          aParamOffsetInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                *aUnsupportedAggr = ID_TRUE;
            }

            (*aAggrCount)++;
        }
        else
        {
            // MTC_NODE_DISTINCT_TRUE
            // Aggregate function에 distinct가 있는 경우 aggregation 분산수행의 결과를 보장하지 못한다.
            *aUnsupportedAggr = ID_TRUE;
        }
    }
    else if ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_SUBQUERY )
    {
        *aUnsupportedAggr = ID_TRUE;
    }
    else
    {
        // Traverse
        for ( sNode  = (qtcNode*)aNode->node.arguments;
              sNode != NULL;
              sNode  = (qtcNode*)sNode->node.next )
        {
            IDE_TEST( addAggrListToText( aStatement,
                                         aParamOffsetInfo,
                                         aTarget,
                                         sNode,
                                         aAggrCount,
                                         aQueryBuf,
                                         aQueryBufSize,
                                         aQueryPosition,
                                         aUnsupportedAggr )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::addSumMinMaxCountToText( qmsTarget      * aTarget, /* TASK-7219 */
                                                   qtcNode        * aNode,
                                                   SChar          * aQueryBuf,
                                                   UInt             aQueryBufSize,
                                                   qcNamePosition * aQueryPosition )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               Aggregate function의 변형 없이 분산부를 이루는 4개 function에 대해서
 *               Query text를 생성한다.
 *
 *
 * Implementation :
 *
 *                SUM(arg),   MIN(arg),   MAX(arg),   COUNT(arg)
 *           ->  "SUM(arg)", "MIN(arg)", "MAX(arg)", "COUNT(arg)"
 *
 ***********************************************************************/

    UShort sTargetPos = 0; /* TASK-7219 */

    IDU_FIT_POINT_FATAL( "qmvShardTransform::addSumMinMaxToText::__FT__" );

    // 정합성 검사
    IDE_FT_ASSERT ( ( aNode->node.module == &mtfSum ) ||
                    ( aNode->node.module == &mtfMin ) ||
                    ( aNode->node.module == &mtfMax ) ||
                    ( aNode->node.module == &mtfCount ) );

    aQueryPosition->size +=
        idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                         aQueryBufSize - aQueryPosition->size,
                         "%.*s",
                         aNode->position.size,
                         aNode->position.stmtText + aNode->position.offset );

    /* TASK-7219 */
    IDE_TEST( addTargetAliasToText( aTarget,
                                    sTargetPos,
                                    aQueryBuf,
                                    aQueryBufSize,
                                    aQueryPosition )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::addAvgToText( qmsTarget      * aTarget, /* TASK-7219 */
                                        qtcNode        * aNode,
                                        SChar          * aQueryBuf,
                                        UInt             aQueryBufSize,
                                        qcNamePosition * aQueryPosition )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               Aggregate function의 변형이 필요한 AVG(arg)에 대해서
 *               SUM(arg),COUNT(arg) 로 변형 된 query text를 생성한다.
 *
 * Implementation :
 *
 *                AVG(arg)  ->  "SUM(arg),COUNT(arg)"
 *
 ***********************************************************************/

    qtcNode * sArg = NULL;
    UShort    sTargetPos = 1; /* TASK-7219 */

    IDU_FIT_POINT_FATAL( "qmvShardTransform::addAvgToText::__FT__" );

    // 정합성 검사
    IDE_FT_ASSERT ( aNode->node.module == &mtfAvg );

    sArg = (qtcNode*)aNode->node.arguments;

    // 1. "SUM("
    aQueryPosition->size +=
        idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                         aQueryBufSize - aQueryPosition->size,
                         "SUM(" );

    // 2. "SUM(arg"
    if ( sArg->position.stmtText[sArg->position.offset - 1] == '"' )
    {
        aQueryBuf[aQueryPosition->size] = '"';
        aQueryPosition->size++;
    }
    else
    {
        // Nothing to do.
    }

    aQueryPosition->size +=
        idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                         aQueryBufSize - aQueryPosition->size,
                         "%.*s",
                         sArg->position.size,
                         sArg->position.stmtText + sArg->position.offset );

    if ( sArg->position.stmtText[sArg->position.offset +
                                 sArg->position.size] == '"' )
    {
        aQueryBuf[aQueryPosition->size] = '"';
        aQueryPosition->size++;
    }
    else
    {
        // Nothing to do.
    }

    // 3. "SUM(arg)"
    aQueryBuf[aQueryPosition->size] = ')';
    aQueryPosition->size++;

    /* TASK-7219 */
    IDE_TEST( addTargetAliasToText( aTarget,
                                    sTargetPos++,
                                    aQueryBuf,
                                    aQueryBufSize,
                                    aQueryPosition )
              != IDE_SUCCESS );

    // 4. "SUM(arg),"
    aQueryBuf[aQueryPosition->size] = ',';
    aQueryPosition->size++;

    // 5. "SUM(arg),COUNT("
    aQueryPosition->size +=
        idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                         aQueryBufSize - aQueryPosition->size,
                         "COUNT(" );

    // 6. "SUM(arg),COUNT(arg"
    if ( sArg->position.stmtText[sArg->position.offset - 1] == '"' )
    {
        aQueryBuf[aQueryPosition->size] = '"';
        aQueryPosition->size++;
    }
    else
    {
        // Nothing to do.
    }

    aQueryPosition->size +=
        idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                         aQueryBufSize - aQueryPosition->size,
                         "%.*s",
                         sArg->position.size,
                         sArg->position.stmtText + sArg->position.offset );

    if ( sArg->position.stmtText[sArg->position.offset +
                                 sArg->position.size] == '"' )
    {
        aQueryBuf[aQueryPosition->size] = '"';
        aQueryPosition->size++;
    }
    else
    {
        // Nothing to do.
    }

    // 7. "SUM(arg),COUNT(arg)"
    aQueryBuf[aQueryPosition->size] = ')';
    aQueryPosition->size++;

    /* TASK-7219 */
    IDE_TEST( addTargetAliasToText( aTarget,
                                    sTargetPos++,
                                    aQueryBuf,
                                    aQueryBufSize,
                                    aQueryPosition )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::modifyOrgAggr( qcStatement  * aStatement,
                                         qtcNode     ** aNode,
                                         UInt         * aViewTargetOrder )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               Original query block의
 *               SELECT, HAVING clause에 존재하는 aggregate function에 대해서
 *               통합부(coord-aggregation) aggregate function으로 변형한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qtcNode ** sNode;

    IDU_FIT_POINT_FATAL( "qmvShardTransform::modifyOrgAggr::__FT__" );

    if ( QTC_IS_AGGREGATE(*aNode) == ID_TRUE )
    {
        IDE_TEST( changeAggrExpr( aStatement,
                                  aNode,
                                  aViewTargetOrder )
                  != IDE_SUCCESS );
    }
    else
    {
        for ( sNode  = (qtcNode**)&(*aNode)->node.arguments;
              *sNode != NULL;
              sNode  = (qtcNode**)&(*sNode)->node.next )
        {
            IDE_TEST( modifyOrgAggr( aStatement,
                                     sNode,
                                     aViewTargetOrder )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::changeAggrExpr( qcStatement  * aStatement,
                                          qtcNode     ** aNode,
                                          UInt         * aViewTargetOrder )
{
/***********************************************************************
 *
 * Description : PROJ-2687 Shard aggregation transform
 *
 *               Aggregate function의 transformation을 수행
 *
 *               SUM(expression)   ->  SUM(column_module)
 *                                         -------------
 *                                          for SUM(arg)
 *
 *               MIN(expression)   ->  MIN(column_module)
 *                                         -------------
 *                                          for MIN(arg)
 *
 *               MAX(expression)   ->  MAX(column_module)
 *                                         -------------
 *                                          for MAX(arg)
 *
 *               COUNT(expression) ->  SUM(column_module)
 *                                         -------------
 *                                          for COUNT(arg)
 *
 *               AVG(expression)   ->  SUM(column_module) / SUM(column_module)
 *                                         -------------        -------------
 *                                          for SUM(arg)         for COUNT(arg)
 *
 *               * column_module은 makeNode를 통해 임의로 생성한다.
 *                 column_module로 생성된 node는
 *                 분산부의 해당 aggr에 대한 column order를
 *                 shardViewTargetPos에 기록한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qcNamePosition sNullPosition;

    qtcNode * sOrgNode;

    qtcNode * sSum1[2];
    qtcNode * sSum2[2];
    qtcNode * sDivide[2];

    qtcNode * sColumn1[2];
    qtcNode * sColumn2[2];

    IDU_FIT_POINT_FATAL( "qmvShardTransform::changeAggrExpr::__FT__" );

    SET_EMPTY_POSITION( sNullPosition );

    sOrgNode = *aNode;

    // 정합성 검사
    IDE_FT_ASSERT ( ( sOrgNode->node.module == &mtfSum ) ||
                    ( sOrgNode->node.module == &mtfMin ) ||
                    ( sOrgNode->node.module == &mtfMax ) ||
                    ( sOrgNode->node.module == &mtfCount ) ||
                    ( sOrgNode->node.module == &mtfAvg ) );

    if ( ( sOrgNode->node.module == &mtfSum ) ||
         ( sOrgNode->node.module == &mtfMin ) ||
         ( sOrgNode->node.module == &mtfMax ) )
    {
        /*
         * before)
         *
         *         AGGR_FUNC
         *             |
         *         expression
         *             |
         *            ...
         *
         * after)
         *
         *         AGGR_FUNC
         *             |
         *        column_node
         *
         */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sColumn1,
                                 &sNullPosition,
                                 &qtc::columnModule )
                  != IDE_SUCCESS );

        sColumn1[0]->lflag = 0;

        sColumn1[0]->shardViewTargetPos = (*aViewTargetOrder)++;
        sColumn1[0]->lflag &= ~QTC_NODE_SHARD_VIEW_TARGET_REF_MASK;
        sColumn1[0]->lflag |= QTC_NODE_SHARD_VIEW_TARGET_REF_TRUE;

        sOrgNode->node.arguments = &sColumn1[0]->node;

        sOrgNode->node.arguments->next = NULL;
        sOrgNode->node.arguments->arguments = NULL;

        sOrgNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sOrgNode->node.lflag |= 1;
    }
    else if ( sOrgNode->node.module == &mtfCount )
    {
        /*
         * before)
         *
         *          COUNT()
         *             |
         *         expression
         *             |
         *            ...
         *
         * after)
         *
         *           SUM()
         *             |
         *        column_node
         *
         */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sSum1,
                                 &sNullPosition,
                                 &mtfSum )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::makeNode( aStatement,
                                 sColumn1,
                                 &sNullPosition,
                                 &qtc::columnModule )
                  != IDE_SUCCESS );

        sColumn1[0]->lflag = 0;

        sColumn1[0]->shardViewTargetPos = (*aViewTargetOrder)++;
        sColumn1[0]->lflag &= ~QTC_NODE_SHARD_VIEW_TARGET_REF_MASK;
        sColumn1[0]->lflag |= QTC_NODE_SHARD_VIEW_TARGET_REF_TRUE;

        sSum1[0]->node.arguments = &sColumn1[0]->node;

        sSum1[0]->node.next = sOrgNode->node.next;
        sSum1[0]->node.arguments->next = NULL;
        sSum1[0]->node.arguments->arguments = NULL;

        sSum1[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSum1[0]->node.lflag |= 1;

        SET_POSITION( sSum1[0]->position, sOrgNode->position );
        SET_POSITION( sSum1[0]->userName, sOrgNode->userName );
        SET_POSITION( sSum1[0]->tableName, sOrgNode->tableName );
        SET_POSITION( sSum1[0]->columnName, sOrgNode->columnName );

        *aNode = sSum1[0];
    }
    else if ( sOrgNode->node.module == &mtfAvg )
    {
        /*
         * before)
         *
         *           AVG()
         *             |
         *         expression
         *             |
         *            ...
         *
         * after)
         *
         *          DIVIDE()
         *             |
         *            SUM()-------SUM()
         *             |            |
         *        column_node   column_node
         *
         */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sDivide,
                                 &sNullPosition,
                                 &mtfDivide )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::makeNode( aStatement,
                                 sSum1,
                                 &sNullPosition,
                                 &mtfSum )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::makeNode( aStatement,
                                 sColumn1,
                                 &sNullPosition,
                                 &qtc::columnModule )
                  != IDE_SUCCESS );

        sColumn1[0]->lflag = 0;

        sColumn1[0]->shardViewTargetPos = (*aViewTargetOrder)++;
        sColumn1[0]->lflag &= ~QTC_NODE_SHARD_VIEW_TARGET_REF_MASK;
        sColumn1[0]->lflag |= QTC_NODE_SHARD_VIEW_TARGET_REF_TRUE;

        sSum1[0]->node.arguments = &sColumn1[0]->node;

        sSum1[0]->node.next = NULL;
        sSum1[0]->node.arguments->next = NULL;
        sSum1[0]->node.arguments->arguments = NULL;

        IDE_TEST( qtc::makeNode( aStatement,
                                 sSum2,
                                 &sNullPosition,
                                 &mtfSum )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::makeNode( aStatement,
                                 sColumn2,
                                 &sNullPosition,
                                 &qtc::columnModule )
                  != IDE_SUCCESS );

        sColumn2[0]->lflag = 0;

        sColumn2[0]->shardViewTargetPos = (*aViewTargetOrder)++;
        sColumn2[0]->lflag &= ~QTC_NODE_SHARD_VIEW_TARGET_REF_MASK;
        sColumn2[0]->lflag |= QTC_NODE_SHARD_VIEW_TARGET_REF_TRUE;

        sSum2[0]->node.arguments = &sColumn2[0]->node;

        sSum2[0]->node.next = NULL;
        sSum2[0]->node.arguments->next = NULL;
        sSum2[0]->node.arguments->arguments = NULL;

        sDivide[0]->node.next = sOrgNode->node.next;
        sDivide[0]->node.arguments = &sSum1[0]->node;
        sDivide[0]->node.arguments->next = &sSum2[0]->node;

        sSum1[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSum1[0]->node.lflag |= 1;

        sSum2[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSum2[0]->node.lflag |= 1;

        sDivide[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sDivide[0]->node.lflag |= 2;

        SET_POSITION( sDivide[0]->position, sOrgNode->position );
        SET_POSITION( sDivide[0]->userName, sOrgNode->userName );
        SET_POSITION( sDivide[0]->tableName, sOrgNode->tableName );
        SET_POSITION( sDivide[0]->columnName, sOrgNode->columnName );

        *aNode = sDivide[0];
    }
    else
    {
        IDE_FT_ASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForExpr( qcStatement  * aStatement,
                                                   qtcNode      * aExpr )
{
/***********************************************************************
 *
 * Description : Shard View Transform
 *     expression의 subquery를 shard view로 변환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement    * sSubQStatement = NULL;
    qtcNode        * sNode = NULL;
    qtcOverColumn  * sOverColumn = NULL;

    IDU_FIT_POINT_FATAL( "qmvShardTransform::processTransformForExpr::__FT__" );

    if ( ( aExpr->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        sSubQStatement = aExpr->subquery;

        IDE_TEST( processTransform( sSubQStatement ) != IDE_SUCCESS );
    }
    else
    {
        // traverse
        for ( sNode = (qtcNode*)aExpr->node.arguments;
              sNode != NULL;
              sNode = (qtcNode*)sNode->node.next )
        {
            IDE_TEST( processTransformForExpr( aStatement, sNode )
                      != IDE_SUCCESS );
        }

        /* TASK-7219 Shard Transformer Refactoring */
        // over
        if ( aExpr->overClause != NULL )
        {
            for ( sOverColumn  = aExpr->overClause->overColumn;
                  sOverColumn != NULL;
                  sOverColumn  = sOverColumn->next )
            {
                IDE_TEST( processTransformForExpr( aStatement,
                                                   sOverColumn->node )
                          != IDE_SUCCESS );
            }
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

IDE_RC qmvShardTransform::raiseInvalidShardQuery( qcStatement  * aStatement )
{
/***********************************************************************
 *
 * Description : Shard Transform 에러를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool           sIsShardParseTree = ID_FALSE;
    qcuSqlSourceInfo sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvShardTransform::setErrorMsg::__FT__" );

    idBool sIsTransformNeeded = ID_FALSE;

    /* 2. Shard View Transform 수행 */
    if ( ( sdi::isShardCoordinator( aStatement ) == ID_TRUE ) ||
         ( sdi::isPartialCoordinator( aStatement ) == ID_TRUE ) )
    {
        IDE_TEST( sdi::isTransformNeeded( aStatement,
                                          &( sIsTransformNeeded ) )
                  != IDE_SUCCESS );

        if ( sIsTransformNeeded == ID_TRUE )
        {
            /* TASK-7219 Shard Transformer Refactoring */
            IDE_TEST( doShardAnalyze( aStatement,
                                      &( aStatement->myPlan->parseTree->stmtPos ),
                                      QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ),
                                      NULL )
                      != IDE_SUCCESS );

            IDE_TEST( sdi::isShardParseTree( aStatement->myPlan->parseTree,
                                             &( sIsShardParseTree ) )
                      != IDE_SUCCESS );

            if ( sIsShardParseTree == ID_FALSE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & aStatement->myPlan->parseTree->stmtPos );
                IDE_RAISE( ERR_INVALID_SHARD_QUERY );
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

    IDE_EXCEPTION( ERR_INVALID_SHARD_QUERY )
    {
        (void)sqlInfo.initWithBeforeMessage(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( sdERR_ABORT_INVALID_SHARD_QUERY,
                                  sqlInfo.getBeforeErrMessage(),
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::rebuildTransform( qcStatement * aStatement )
{
    idBool             sSessionSMNIsShardQuery = ID_FALSE;
    idBool             sDataSMNIsShardQuery    = ID_FALSE;
    sdiAnalyzeInfo   * sSessionSMNAnalysis     = NULL;
    sdiAnalyzeInfo   * sDataSMNAnalysis        = NULL;
    idBool             sIsTansformable         = ID_TRUE;
    sdiNodeInfo      * sNodeInfo4SessionSMN    = NULL;
    SChar            * sMyNodeName             = NULL;
    UShort             sMyNodeId               = ID_USHORT_MAX;
    sdiRebuildInfo     sRebuildInfo;
    idBool             sRebuild                = ID_FALSE;

    /* TASK-7219 Shard Transformer Refactoring */
    qcNamePosition   * sStmtPos                = &( aStatement->myPlan->parseTree->stmtPos );
    ULong              sSessionSMN             = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ); /* sessionSMN */
    ULong              sDataSMN                = sdi::getSMNForDataNode(); /* dataSMN */

    //------------------------------------------
    // get shard SQL type for SMN
    //------------------------------------------

    SDI_SET_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                 SDI_QUERYSET_LIST_STATE_MAIN_ALL );

    /* TASK-7219 Shard Transformer Refactoring */
    // analyze for sessionSMN
    IDE_TEST( doShardAnalyze( aStatement,
                              sStmtPos,
                              sSessionSMN,
                              NULL )
              != IDE_SUCCESS );

    IDE_TEST( sdi::isShardParseTree( aStatement->myPlan->parseTree,
                                     &( sSessionSMNIsShardQuery ) )
                      != IDE_SUCCESS );

    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromParseTree( aStatement,
                                                       aStatement->myPlan->parseTree,
                                                       &( sSessionSMNAnalysis ) )
              != IDE_SUCCESS );

    // 임의적(node prefix, shard view keyword사용)으로 분산 정의를 무시하고 수행시킨
    // SQL에 대해서는 rebuild error를 발생시킨다.
    /* PROJ-2745
     * Rebuild transform 하지 않아도 된다.
     * 분석 불가능하다면 Rebuild retry 하자. */
    if ( sSessionSMNIsShardQuery == ID_FALSE )
    {
        IDE_TEST_RAISE( sSessionSMNAnalysis == NULL,
                        REBUILD_RETRY );

        IDE_TEST_RAISE( sdi::isShardCoordinator( aStatement ) == ID_FALSE,
                        REBUILD_RETRY );
    }

    SDI_SET_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                 SDI_QUERYSET_LIST_STATE_MAIN_ALL );

    /* TASK-7219 Shard Transformer Refactoring */
    // analyze for dataSMN
    IDE_TEST( doShardAnalyze( aStatement,
                              sStmtPos,
                              sDataSMN,
                              NULL )
              != IDE_SUCCESS );

    IDE_TEST( sdi::isShardParseTree( aStatement->myPlan->parseTree,
                                     &( sDataSMNIsShardQuery ) )
              != IDE_SUCCESS );

    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromParseTree( aStatement,
                                                       aStatement->myPlan->parseTree,
                                                       &( sDataSMNAnalysis ) )
              != IDE_SUCCESS );

    //------------------------------------------
    // check rebuild transformable
    //------------------------------------------
    IDE_TEST( checkRebuildTransformable( aStatement,
                                         sSessionSMNAnalysis,
                                         sDataSMNIsShardQuery,
                                         sDataSMNAnalysis,
                                         &sIsTansformable )
              != IDE_SUCCESS );

    if ( sIsTansformable == ID_TRUE )
    {
        //------------------------------------------
        // get node info of sessionSMN & dataSMN
        //------------------------------------------
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                              sdiNodeInfo,
                              &sNodeInfo4SessionSMN)
                 != IDE_SUCCESS);

        IDE_TEST( sdi::getInternalNodeInfo( NULL,
                                            sNodeInfo4SessionSMN,
                                            ID_FALSE,
                                            sSessionSMN )
                  != IDE_SUCCESS );

        //------------------------------------------
        // get my node ID
        //------------------------------------------
        sMyNodeName = qcg::getSessionShardNodeName( aStatement );

        /* PROJ-2745
         *   User session 에서 호출될때 getNodeIdFromName() 함수는
         *     sMyNodeId = ID_USHORT_MAX 로 전달 된다.
         *   User session 에서 rebuild 를 감지하는 경우
         *     현재 수행 노드와 SMN 에 따른 수행 노드 분석을 하지 않는다.
         *     (qmvShardTransform::detectNeedRebuild)
         */
        getNodeIdFromName( sNodeInfo4SessionSMN,
                           sMyNodeName,
                           &sMyNodeId );

        //------------------------------------------
        // set rebuild information
        //------------------------------------------
        sRebuildInfo.mMyNodeId           = sMyNodeId;
        sRebuildInfo.mSessionSMN         = sSessionSMN;
        sRebuildInfo.mSessionSMNAnalysis = sSessionSMNAnalysis;
        sRebuildInfo.mDataSMN            = sDataSMN;
        sRebuildInfo.mDataSMNAnalysis    = sDataSMNAnalysis;

        if ( sDataSMNIsShardQuery == ID_TRUE )
        {
            /* shard query */
            IDE_TEST( checkNeedRebuild4ShardQuery( aStatement,
                                                   &sRebuildInfo,
                                                   &sRebuild )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sRebuild == ID_TRUE,
                            REBUILD_RETRY );
        }
        else /* sDataSMNIsShardQuery == ID_FALSE */
        {
            /* shard query to non-shard query */
            if ( ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT ) ||
                 ( aStatement->myPlan->parseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE ) )
            {
                /* because of JOIN operation or SET operation or Sub-query operation */

                sRebuildInfo.mDataSMNAnalysis = NULL;

                IDE_TEST( rebuildNonShardTransform( aStatement,
                                                    &sRebuildInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                // non-shard DML(distributed sub-query)은 현재 sharding에서 지원하지 않고있다.
                /* PROJ-2745
                 * Rebuild transform 하지 않아도 된다.
                 * 분석 불가능하다면 Rebuild retry 하자. */
                IDE_RAISE( REBUILD_RETRY );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( REBUILD_RETRY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkRebuildTransformable( qcStatement    * aStatement,
                                                     sdiAnalyzeInfo * aSessionSMNAnalysis,
                                                     idBool           aDataSMNIsShardQuery,
                                                     sdiAnalyzeInfo * aDataSMNAnalysis,
                                                     idBool         * aIsTransformable )
{
    UShort i = 0;
    idBool sIsTransformable = ID_TRUE;

    IDE_DASSERT( aSessionSMNAnalysis != NULL );
    IDE_DASSERT( ( ( aDataSMNIsShardQuery == ID_TRUE ) && ( aDataSMNAnalysis != NULL ) ) ||
                 ( aDataSMNIsShardQuery == ID_FALSE ) );

    //------------------------------------------
    // Check online data rebuild enable
    //------------------------------------------

    /* PROJ-2745
     * Rebuild transform 하지 않아도 된다.
     * 분석 불가능하다면 Rebuild retry 하자. */
    IDE_TEST_RAISE( aSessionSMNAnalysis->mSubKeyExists == ID_TRUE,
                    REBUILD_RETRY );

    if ( aDataSMNIsShardQuery == ID_TRUE )
    {
        IDE_TEST_RAISE( aSessionSMNAnalysis->mSplitMethod != aDataSMNAnalysis->mSplitMethod,
                        REBUILD_RETRY );

        if ( sdi::getSplitType( aSessionSMNAnalysis->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
        {
            IDE_TEST_RAISE( aSessionSMNAnalysis->mSubKeyExists != aDataSMNAnalysis->mSubKeyExists,
                            REBUILD_RETRY );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Shard query to non-shard query에 대해서는
        // rebuildNonShardTransform()에서 각 object별로 확인한다.
        // Nothing to do.
    }

    if ( sdi::getSplitType( aSessionSMNAnalysis->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
    {
        //------------------------------------------
        // Check bind param info exists
        //------------------------------------------
        for ( i = 0; i < aSessionSMNAnalysis->mValuePtrCount; i++ )
        {
            if ( aSessionSMNAnalysis->mValuePtrArray[i]->mType == 0 )
            {
                // bind variable
                if ( aStatement->pBindParam == NULL )
                {
                    // shard key value로 bind parameter를 사용 했는데
                    // bind 정보가 없다면 prepare중이므로
                    // transform을 중단하고 가짜 plan을 생성 하도록 한다.
                    // 이후 rebuild of execute 에서 진짜 plan을 생성해서 수행한다.
                    sIsTransformable = ID_FALSE;

                    aStatement->mFlag &= ~QC_STMT_SHARD_REBUILD_FORCE_MASK;
                    aStatement->mFlag |=  QC_STMT_SHARD_REBUILD_FORCE_TRUE;
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
    }
    else
    {
        // Nothing to do.
    }

    *aIsTransformable = sIsTransformable;

    return IDE_SUCCESS;

    IDE_EXCEPTION( REBUILD_RETRY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmvShardTransform::getNodeIdFromName( sdiNodeInfo * aNodeInfo,
                                           SChar       * aNodeName,
                                           UShort      * aNodeId )
{
    UShort i = 0;

    *aNodeId = ID_USHORT_MAX;

    for ( i = 0;
          i < aNodeInfo->mCount;
          i++ )
    {
        if ( idlOS::strncmp(aNodeName,
                            aNodeInfo->mNodes[i].mNodeName,
                            SDI_NODE_NAME_MAX_SIZE + 1) == 0 )
        {
            *aNodeId = aNodeInfo->mNodes[i].mNodeId;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }
}

void qmvShardTransform::getNodeNameFromId( sdiNodeInfo * aNodeInfo,
                                           UShort        aNodeId,
                                           SChar      ** aNodeName )
{
    UShort i = 0;

    *aNodeName = NULL;

    for ( i = 0;
          i < aNodeInfo->mCount;
          i++ )
    {
        if ( aNodeId == aNodeInfo->mNodes[i].mNodeId )
        {
            *aNodeName = aNodeInfo->mNodes[i].mNodeName;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }
}

/* PROJ-2745_FOR_MERGE makeExecNodes4ShardQuery */
IDE_RC qmvShardTransform::checkNeedRebuild4ShardQuery( qcStatement     * aStatement,
                                                       sdiRebuildInfo  * aRebuildInfo,
                                                       idBool          * aRebuild )
{
    idBool   sIsTransformNeeded = ID_FALSE;
    idBool   sEqualClone        = ID_FALSE;
    UInt     i                  = 0;

    PDL_UNUSED_ARG( aStatement );

    if ( aRebuildInfo->mSessionSMNAnalysis->mSplitMethod 
         == aRebuildInfo->mDataSMNAnalysis->mSplitMethod )
    {
        IDE_DASSERT( aRebuildInfo->mSessionSMNAnalysis->mSplitMethod != SDI_SPLIT_NONE );

        if ( sdi::getSplitType( aRebuildInfo->mSessionSMNAnalysis->mSplitMethod ) == SDI_SPLIT_TYPE_NO_DIST )
        {
            if ( aRebuildInfo->mSessionSMNAnalysis->mRangeInfo.mCount
                 == aRebuildInfo->mDataSMNAnalysis->mRangeInfo.mCount )
            {
                /* PROJ-2745
                 *   Rebuild before/after 에 대한 분산 정보가 같다면
                 *   Rebuild RETRY 를 발생 시키지 않아도 된다. */
                sEqualClone = ID_TRUE;

                for ( i = 0; i < aRebuildInfo->mSessionSMNAnalysis->mRangeInfo.mCount; i++ )
                {
                    if ( aRebuildInfo->mSessionSMNAnalysis->mRangeInfo.mRanges[i].mNodeId
                         != aRebuildInfo->mDataSMNAnalysis->mRangeInfo.mRanges[i].mNodeId )
                    {
                        sEqualClone = ID_FALSE;
                        break;
                    }
                }
            }

            if ( sEqualClone == ID_FALSE )
            {
                sIsTransformNeeded = ID_TRUE;
            }
        }
        else /* Clone또는 solo split이 아닌 경우 */
        {
            //------------------------------------------
            // SessionSMN 과 dataSMN 에 대한
            // 분산정의 변경정보를 얻어온다.
            //------------------------------------------
            IDE_TEST( detectNeedRebuild( aRebuildInfo->mMyNodeId,
                                         aRebuildInfo->mSessionSMNAnalysis->mSplitMethod,
                                         aRebuildInfo->mSessionSMNAnalysis->mKeyDataType,
                                         &aRebuildInfo->mSessionSMNAnalysis->mRangeInfo,
                                         aRebuildInfo->mSessionSMNAnalysis->mDefaultNodeId,
                                         &aRebuildInfo->mDataSMNAnalysis->mRangeInfo,
                                         aRebuildInfo->mDataSMNAnalysis->mDefaultNodeId,
                                         &sIsTransformNeeded )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* aRebuildInfo->mSessionSMNAnalysis->mSplitMethod 
           != aRebuildInfo->mDataSMNAnalysis->mSplitMethod */
        sIsTransformNeeded = ID_TRUE;
    }


    *aRebuild = sIsTransformNeeded;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aRebuild = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::rebuildNonShardTransform( qcStatement    * aStatement,
                                                    sdiRebuildInfo * aRebuildInfo )
{
    qmsParseTree     * sParseTree;
    qmsSortColumns   * sCurrSort;

    sParseTree = (qmsParseTree*)aStatement->myPlan->parseTree;

    // transform querySet
    IDE_TEST( rebuildTransformQuerySet( aStatement,
                                        aRebuildInfo,
                                        sParseTree->querySet )
              != IDE_SUCCESS );

    // transform sub-query of orderBy
    for ( sCurrSort = sParseTree->orderBy;
          sCurrSort != NULL;
          sCurrSort = sCurrSort->next )
    {
        if ( sCurrSort->targetPosition <= QMV_EMPTY_TARGET_POSITION )
        {
            IDE_TEST( rebuildTransformExpr( aStatement,
                                            aRebuildInfo,
                                            sCurrSort->sortColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    // transform sub-query of loop
    if ( sParseTree->loopNode != NULL )
    {
        IDE_TEST( rebuildTransformExpr( aStatement,
                                        aRebuildInfo,
                                        sParseTree->loopNode )
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

IDE_RC qmvShardTransform::rebuildTransformQuerySet( qcStatement    * aStatement,
                                                    sdiRebuildInfo * aRebuildInfo,
                                                    qmsQuerySet    * aQuerySet )
{
    qmsFrom          * sFrom = NULL;
    qmsTarget        * sTarget = NULL;
    qmsConcatElement * sConcatElement = NULL;

    //------------------------------------------
    // Rebuild Transform의 수행
    //------------------------------------------
    if ( aQuerySet->setOp == QMS_NONE )
    {
        IDE_DASSERT( aQuerySet->SFWGH != NULL );

        // select target
        for ( sTarget  = aQuerySet->SFWGH->target;
              sTarget != NULL;
              sTarget  = sTarget->next )
        {
            if ( ( sTarget->flag & QMS_TARGET_ASTERISK_MASK )
                 != QMS_TARGET_ASTERISK_TRUE )
            {
                IDE_TEST( rebuildTransformExpr( aStatement,
                                                aRebuildInfo,
                                                sTarget->targetColumn )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
            
        }

        // from
        for ( sFrom  = aQuerySet->SFWGH->from;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {
            IDE_TEST( rebuildTransformFrom( aStatement,
                                            aRebuildInfo,
                                            sFrom )
                      != IDE_SUCCESS );
        }

        // where
        if ( aQuerySet->SFWGH->where != NULL )
        {
            IDE_TEST( rebuildTransformExpr( aStatement,
                                            aRebuildInfo,
                                            aQuerySet->SFWGH->where )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // hierarchy
        IDE_TEST_RAISE( aQuerySet->SFWGH->hierarchy != NULL, ERR_INVALID_SHARD_QUERY_TYPE );

        // group by
        for ( sConcatElement  = aQuerySet->SFWGH->group;
              sConcatElement != NULL;
              sConcatElement  = sConcatElement->next )
        {
            if ( sConcatElement->arithmeticOrList != NULL )
            {
                IDE_TEST( rebuildTransformExpr( aStatement,
                                                aRebuildInfo,
                                                sConcatElement->arithmeticOrList )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }

        // having
        if ( aQuerySet->SFWGH->having != NULL )
        {
            IDE_TEST( rebuildTransformExpr( aStatement,
                                            aRebuildInfo,
                                            aQuerySet->SFWGH->having )
                      != IDE_SUCCESS );
        }
        else
        {
            // Noting to do.
        }
    }
    else
    {
        // left subQuery
        IDE_TEST( rebuildTransformQuerySet( aStatement,
                                            aRebuildInfo,
                                            aQuerySet->left )
                  != IDE_SUCCESS );

        // right subQuery
        IDE_TEST( rebuildTransformQuerySet( aStatement,
                                            aRebuildInfo,
                                            aQuerySet->right )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SHARD_QUERY_TYPE)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::rebuildTransformQuerySet",
                                "Invalid shard query type"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::rebuildTransformFrom( qcStatement    * aStatement,
                                                sdiRebuildInfo * aRebuildInfo,
                                                qmsFrom        * aFrom )
{
    qmsTableRef         * sTableRef      = NULL;
    sdiObjectInfo       * sSessionSMNObj = NULL;
    sdiObjectInfo       * sDataSMNObj    = NULL;
    idBool                sIsTransformNeeded = ID_FALSE;

    UShort i;
    idBool sEqualClone = ID_FALSE;

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        sTableRef = aFrom->tableRef;

        if ( sTableRef->view != NULL )
        {
            // view
            IDE_TEST( rebuildNonShardTransform( sTableRef->view,
                                                aRebuildInfo )
                      != IDE_SUCCESS );
        }
        else if ( sTableRef->mShardObjInfo != NULL )
        {
            // table

            //------------------------------------------
            // Shard query to non-shard query 에 대한
            // Rebuild transformation을 object별로 수행한다.
            //------------------------------------------

            //------------------------------------------
            // Get the shard object of the each SMN
            //------------------------------------------
            sdi::getShardObjInfoForSMN( aRebuildInfo->mSessionSMN,
                                        sTableRef->mShardObjInfo,
                                        &sSessionSMNObj );

            /* PROJ-2745 Session SMN 의 분산 정보가 없을수 있다. rebuild 하자 */
            IDE_TEST_RAISE( sSessionSMNObj == NULL,
                            REBUILD_RETRY );

            sdi::getShardObjInfoForSMN( aRebuildInfo->mDataSMN,
                                        sTableRef->mShardObjInfo,
                                        &sDataSMNObj );
            
            IDE_TEST_RAISE( sDataSMNObj == NULL, ERR_INVALID_SHARD_OBJECT_INFO );

            //------------------------------------------
            // Check non-shard rebuild transformable
            //------------------------------------------
            if ( checkRebuildTransformable4NonShard( sSessionSMNObj,
                                                     sDataSMNObj ) == ID_TRUE )
            {
                if ( sdi::getSplitType( sDataSMNObj->mTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                {
                    IDE_TEST( detectNeedRebuild( aRebuildInfo->mMyNodeId,
                                                 sSessionSMNObj->mTableInfo.mSplitMethod,
                                                 sSessionSMNObj->mTableInfo.mKeyDataType,
                                                 &sSessionSMNObj->mRangeInfo,
                                                 sSessionSMNObj->mTableInfo.mDefaultNodeId,
                                                 &sDataSMNObj->mRangeInfo,
                                                 sDataSMNObj->mTableInfo.mDefaultNodeId,
                                                 &sIsTransformNeeded )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sIsTransformNeeded == ID_TRUE,
                                    REBUILD_RETRY );
                }
                else
                {
                    // CLONE or SOLO split
                    sEqualClone = ID_FALSE;

                    if ( sSessionSMNObj->mRangeInfo.mCount
                         == sDataSMNObj->mRangeInfo.mCount )
                    {
                        /* PROJ-2745
                         *   Rebuild before/after 에 대한 분산 정보가 같다면
                         *   Rebuild RETRY 를 발생 시키지 않아도 된다. */
                        sEqualClone = ID_TRUE;

                        for ( i = 0; i < sSessionSMNObj->mRangeInfo.mCount; i++ )
                        {
                            if ( sSessionSMNObj->mRangeInfo.mRanges[i].mNodeId
                                 != sDataSMNObj->mRangeInfo.mRanges[i].mNodeId )
                            {
                                sEqualClone = ID_FALSE;
                                break;
                            }
                        }
                    }

                    IDE_TEST_RAISE( sEqualClone == ID_FALSE,
                                    REBUILD_RETRY );
                }
            }
            else
            {
                /* PROJ-2745
                 * Rebuild transform 하지 않아도 된다.
                 * 분석 불가능하다면 Rebuild retry 하자. */
                IDE_RAISE( REBUILD_RETRY );
            }

        }
    }
    else
    {
        IDE_TEST( rebuildTransformFrom( aStatement,
                                        aRebuildInfo,
                                        aFrom->left )
                  != IDE_SUCCESS );

        IDE_TEST( rebuildTransformFrom( aStatement,
                                        aRebuildInfo,
                                        aFrom->right )
                  != IDE_SUCCESS );

        IDE_TEST( rebuildTransformExpr( aStatement,
                                        aRebuildInfo,
                                        aFrom->onCondition )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_SHARD_OBJECT_INFO)
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                "sda::rebuildTransformFrom",
                                "Invalid shard object"));
    }
    IDE_EXCEPTION( REBUILD_RETRY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SHARD_META_OUT_OF_DATE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmvShardTransform::checkRebuildTransformable4NonShard( sdiObjectInfo * aSessionSMNObj,
                                                              sdiObjectInfo * aDataSMNObj )
{
    
    idBool sIsTransformable = ID_FALSE;

    if ( ( aSessionSMNObj->mTableInfo.mSplitMethod == aDataSMNObj->mTableInfo.mSplitMethod ) &&
         ( aSessionSMNObj->mTableInfo.mSubKeyExists == aDataSMNObj->mTableInfo.mSubKeyExists ) &&
         ( aSessionSMNObj->mTableInfo.mSubKeyExists == ID_FALSE ) )
    {
        // BUGBUG : 해당 table의 shard key column의 이름 등 이
        //          변경 되었을 경우도 체크해서 rebuild error를 올려주면 좋다.
        sIsTransformable = ID_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    return sIsTransformable;
}

/* PROJ-2745_FOR_MERGE makeRebuildRanges */
IDE_RC qmvShardTransform::detectNeedRebuild( UShort                 aMyNodeId,
                                             sdiSplitMethod         aSplitMethod,
                                             UInt                   aKeyDataType,
                                             sdiRangeInfo         * aSessionSMNRangeInfo,
                                             UShort                 aSessionSMNDefaultNodeId,
                                             sdiRangeInfo         * aDataSMNRangeInfo,
                                             UShort                 aDataSMNDefaultNodeId,
                                             idBool               * aIsTransformNeeded )
{
    UInt sKeyDataType = ID_USHORT_MAX;

    UShort i = 0; // sessionSMN's range index
    UShort j = 0; // dataSMN's range index

    sdiValue * sSessionSMNRangeValue = NULL;
    sdiValue * sDataSMNRangeValue    = NULL;

    UShort sFromNode = ID_USHORT_MAX;
    UShort sToNode   = ID_USHORT_MAX;

    SShort sCompare = 0; // -1 : less, 0 : equal, 1 : greater

    *aIsTransformNeeded = ID_FALSE;

    // get shard key data type for comparison
    if ( aSplitMethod == SDI_SPLIT_HASH )
    {
        sKeyDataType = MTD_INTEGER_ID;
    }
    else
    {
        sKeyDataType = aKeyDataType;
    }

    //------------------------------------------
    // SessionSMNRanges와 dataSMNRanges가
    // union된 rangeInfo를 구성한다.
    //------------------------------------------
    while ( sCompare != ID_SSHORT_MAX )
    {
        if ( i < aSessionSMNRangeInfo->mCount )
        {
            sSessionSMNRangeValue = &aSessionSMNRangeInfo->mRanges[i].mValue;
            sFromNode = aSessionSMNRangeInfo->mRanges[i].mNodeId;
        }
        else
        {
            sSessionSMNRangeValue = NULL;
            sFromNode = aSessionSMNDefaultNodeId;
        }

        if ( j < aDataSMNRangeInfo->mCount )
        {
            sDataSMNRangeValue = &aDataSMNRangeInfo->mRanges[j].mValue;
            sToNode = aDataSMNRangeInfo->mRanges[j].mNodeId;
        }
        else
        {
            sDataSMNRangeValue = NULL;
            sToNode = aDataSMNDefaultNodeId;
        }

        if ( ( sSessionSMNRangeValue != NULL ) && ( sDataSMNRangeValue != NULL ) )
        {
            IDE_TEST( sdi::compareKeyData( sKeyDataType,
                                           sSessionSMNRangeValue, // A
                                           sDataSMNRangeValue,    // B
                                           &sCompare )       // A = B : 0, A > B : 1, A < B : -1
                      != IDE_SUCCESS );

            if ( aSplitMethod == SDI_SPLIT_LIST )
            {
                if ( sCompare == -1 )
                {
                    sToNode = aDataSMNDefaultNodeId;
                }
                else if ( sCompare == 1 )
                {
                    sFromNode = aSessionSMNDefaultNodeId;
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
        else if ( ( sSessionSMNRangeValue == NULL ) && ( sDataSMNRangeValue != NULL ) )
        {
            sCompare = 1;
        }
        else if ( ( sSessionSMNRangeValue != NULL ) && ( sDataSMNRangeValue == NULL ) )
        {
            sCompare = -1;
        }
        else // ( ( sSeesionSMNRangeValue == NULL ) && ( sDataSMNRangeValue == NULL ) )
        {
            sCompare = ID_SSHORT_MAX;
        }

        if ( sCompare == -1 ) // sessionSMNValue < dataSMNValue
        {
            i++;
        }
        else if ( sCompare == 1 ) // sessionSMNVale > dataSMNValue
        {
            j++;
        }
        else if ( sCompare == 0 ) // sessionSMNValue == dataSMNValue
        {
            i++;
            j++;
        }
        else //( sCompare == ID_SSHORT_MAX )
        {
            IDE_DASSERT( sCompare == ID_SSHORT_MAX );
        }

        if ( aMyNodeId == ID_USHORT_MAX )
        {
            /* PROJ-2745
             *   User session 이다.
             *   수행 노드가 변경되면 무조건 Rebuild retry 하자
             */
            if ( sFromNode != sToNode )
            {
                *aIsTransformNeeded = ID_TRUE;
            }
        }
        else
        {
            // set filter type
            if ( sFromNode == aMyNodeId )
            {
                // sessionSMN 기준으로 내가 가지고있는 데이터이다.
                if ( sToNode != aMyNodeId )
                {
                    *aIsTransformNeeded = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else // ( sFromNode != aMyNodeId )
            {
                if ( sToNode == aMyNodeId )
                {
                    // 내가 가지고 있지 않던 데이터를 받았다.
                    *aIsTransformNeeded = ID_TRUE;
                }
                else // ( sToNode != aMyNodeId )
                {
                    // 나와 관련없는 데이터이다.
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::rebuildTransformExpr( qcStatement    * aStatement,
                                                sdiRebuildInfo * aRebuildInfo,
                                                qtcNode        * aExpr )
{
    qtcNode * sNode = NULL;

    if ( ( aExpr->node.lflag & MTC_NODE_OPERATOR_MASK )
         == MTC_NODE_OPERATOR_SUBQUERY )
    {
        IDE_TEST( rebuildNonShardTransform( aExpr->subquery,
                                            aRebuildInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        // traverse
        for ( sNode = (qtcNode*)aExpr->node.arguments;
              sNode != NULL;
              sNode = (qtcNode*)sNode->node.next )
        {
            IDE_TEST( rebuildTransformExpr( aStatement,
                                            aRebuildInfo,
                                            sNode )
                      != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-7219 */
IDE_RC qmvShardTransform::addTargetAliasToText( qmsTarget      * aTarget, /* TASK-7219 */
                                                UShort           aTargetPos,
                                                SChar          * aQueryBuf,
                                                UInt             aQueryBufSize,
                                                qcNamePosition * aQueryPosition )
{
 /****************************************************************************************
 *
 * Description : Shard View Target의 Alias Text를 생성하는 함수이다.
 *               Shard Aggregation Transform에서 Shard View Target를 생성할 때,
 *               Shard Col Trans Node가 분할된다면, 분할되는 Node마다 새로운 Alias를
 *               지정한다. 따라서, _$N이 _$N_$N으로 지정된다.
 *
 *  BEFORE / SELECT AVG( C1 ),AVG( C2 ) AS _$2
 *            FROM T1
 *             GROUP BY C2
 *              ORDER BY _$2
 *                         \______________________________________________________
 *                                                                                |
 *  AFTER  / SELECT SUM( A ) / SUM( B )                                           |
 *            FROM SHARD( SELECT C2, SUM( C1 ) AS      A, COUNT( C1 ) AS      B,  |
 *                                   SUM( C1 ) AS _$2_$1, COUNT( C1 ) AS _$2_$2   |
 *                         FROM T1                ******                 ******   |
 *                          GROUP BY C2 )            \______________________\_____|
 *             GROUP BY C2
 *              ORDER BY _$2
 *
 * Implementation : 1. Shard Col Trans Node인지 검사한다.
 *                  2. 분할되는 경우에 _$N_$N으로 Alias를 지정한다.
 *                  3. 그외에는 기존 Alias를 지정한다.
 *
 ****************************************************************************************/

    IDE_TEST_RAISE( aQueryBuf == NULL, ERR_NULL_QUERY_BUF );
    IDE_TEST_RAISE( aQueryPosition == NULL, ERR_NULL_QUERY_POS );

    if ( aTarget != NULL )
    {
        /* 1. Shard Col Trans Node인지 검사한다. */
        if ( ( aTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
             != QMS_TARGET_SHARD_ORDER_BY_TRANS_NONE )
        {
            IDE_TEST_RAISE( QC_IS_NULL_NAME( aTarget->aliasColumnName ) == ID_TRUE, ERR_NULL_ALIAS );

            /* 2. 분할되는 경우에 _$N_$N으로 Alias를 지정한다. */
            if ( aTargetPos > 0 )
            {
                aQueryPosition->size +=
                    idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                                     aQueryBufSize - aQueryPosition->size,
                                     " AS %.*s"SHARD_ORDER_BY_PREFIX"%"ID_UINT32_FMT,
                                     aTarget->aliasColumnName.size,
                                     aTarget->aliasColumnName.name,
                                     aTargetPos );
            }
            else
            {
                /* . 그외에는 기존 Alias를 지정한다. */
                aQueryPosition->size +=
                    idlOS::snprintf( aQueryBuf + aQueryPosition->size,
                                     aQueryBufSize - aQueryPosition->size,
                                     " AS %.*s",
                                     aTarget->aliasColumnName.size,
                                     aTarget->aliasColumnName.name );
            }
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

    IDE_EXCEPTION( ERR_NULL_QUERY_BUF )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::addTargetAliasToText",
                                  "query buf is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERY_POS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::addTargetAliasToText",
                                  "query position is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ALIAS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::addTargetAliasToText",
                                  "alias is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processOrderByTransform( qcStatement  * aStatement,
                                                   qmsQuerySet  * aQuerySet,
                                                   qmsParseTree * aParseTree,
                                                   UInt           aTransType )
{
 /****************************************************************************************
 *
 * Description : Implicit한 Order By 대상을 Shard에 지원하는 함수이다.
 *
 *               Order By 구문은 Shard Query로 수행할 수 없기 때문에, Order By를 제외한
 *               SFWGH만 Shard View로 생성하고, Order By Query Set으로 감싸서 처리한다.
 *               이때 새로 생성된 Shard View로 인해서, Order By Query Set으로는 Implicit한
 *               대상을 인식할 수 없다.
 *
 *               새로 구현된 qmvQTC::setColumnIDOrderByForShard와 같은 함수로 Shard View
 *               까지 대상을 검색하지만 Column의 경우만 정상적인 작업을 수행한다.
 *
 *               특히, 분산부, 통합부로 나눠 처리하는 Aggregation를 정상적으로 수행하려면,
 *               Aggregation을 Shard View의 Target에 Append해야만 한다.
 *
 *               또한, Aggregation 형태가 변경되는 qmvShardTransform::processAggTransform
 *               함수 호출 전에, SELECT * 로 변환하는 qmvShardTransform::makeShardQuerySet
 *               함수 호출 전 또는 후에 Append해야 한다.
 *               일관성을 위해서, qmvShardTransform::processOrderByTransform 은 위 함수 전에
 *               호출하도록 구현되었다.
 *
 *               그리고 함수 호출 전에 QMV_QUERY_SET_SHARD_ORDER_BY_TRANS_POSITION_MASK를
 *               처리하는데, Aggregation 변경 유무로 Group Key 포함된 Implicit Order By
 *               대상의 처리가 달라서 구별하는 용도로 사용한다.
 *
 *               마지막으로 BUG-47197의 Target절에 존재하는 Order By 대상의
 *               Target Position Transfrom 처리 내용이
 *               qmvShardTransform::processOrderByTransform으로 합쳐지게 되었다.
 *
 *               이러한 Transform을 Shard Col Trans( Shard Column Transform )라고 용어
 *               정의한다.
 *
 *  TARGET / C2 O
 *
 *   QUERRY     /                       SELECT C1, C2        FROM T1   ORDER BY  C2;
 *   EQUIVALENT /                                  **                            **
 *   POSITON    /                              1   2
 *   TRANSFORM  /                       SELECT C1, C2        FROM T1   ORDER BY   2;  <-- 3.2.
 *   SHARD VIEW / SELECT C1 FROM SHARD( SELECT C1, C2        FROM T1 ) ORDER BY   2;
 *
 *  TARGET / C2 X
 *
 *   QUERRY     /                       SELECT C1            FROM T1   ORDER BY  C2;
 *   EQUIVALENT /                                                                 X
 *   POSITON    /                              1   2
 *   TRANSFORM  /                       SELECT C1, C2 AS _$2 FROM T1   ORDER BY _$2;  <-- 4.1.
 *   SHARD VIEW / SELECT C1 FROM SHARD( SELECT C1, C2 AS _$2 FROM T1 ) ORDER BY _$2;
 *
 * Implementation : 1.   Set 연산인지 검사한다.
 *                  2.1. Explicit한 대상은 Target Position 설정한다.
 *                  2.2. Implicit한 대상인지 검사한다.
 *                  2.3. PSM 관련 대상은 Skip 한다.
 *                  2.4. Target이 Asterisk라면 Skip한다.
 *                  2.5. Target절에 존재하는 Order By 대상을 찾는다.
 *                  2.6. 중복된 Target이 존재하면 Skip 한다.
 *                  2.7. Order by절에 Target Position은 SShort 값까지 표현할 수 있다.
 *                  3.1. Target절에 존재하면, Target Position Transfrom으로 처리한다.
 *                  3.2. BUG-47197 / Target Position Transfrom으로 처리한다.
 *                  3.3. 이미 Append된 대상은 추가로 Append하지 않고, Target Position Transfrom으로 처리한다.
 *                  4.1. Target절에 존재하지 않으면, Append 작업을 처리한다.
 *                  4.2. Append 불가능한 대상이므로, 더이상 수행하지 않는다.
 *
 ****************************************************************************************/

    qmsSortColumns * sCurrSort     = NULL;
    qmsTarget      * sTarget       = NULL;
    qmsTarget      * sEquivalent   = NULL;
    qtcNode        * sColumnNode   = NULL;
    UShort           sIdx          = 0;
    UShort           sTargetPos    = 0;
    idBool           sIsFound      = ID_FALSE;
    idBool           sIsEquivalent = ID_FALSE;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERY_SET );

    /* 1.   Set 연산인지 검사한다. */
    IDE_TEST_CONT( aParseTree->querySet->setOp != QMS_NONE, NORMAL_EXIT );

    for ( sCurrSort  = aParseTree->orderBy;
          sCurrSort != NULL;
          sCurrSort  = sCurrSort->next )
    {
        /* 2.1. Explicit한 대상은 Target Position 설정한다. */
        IDE_TEST( qtc::getSortColumnPosition( sCurrSort,
                                              QC_SHARED_TMPLATE( aStatement ) )
                  != IDE_SUCCESS);

        /* 2.2. Implicit한 대상인지 검사한다. */
        if ( sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION )
        {
            /* Nothing to do */
        }
        else
        {
            continue;
        }

        /* 2.3. PSM 관련 대상은 Skip 한다. */
        IDE_TEST_CONT( ( sCurrSort->sortColumn->node.lflag & MTC_NODE_DML_MASK )
                       == MTC_NODE_DML_UNUSABLE,
                       NORMAL_EXIT );

        sEquivalent = NULL;

        for ( sTarget  = aQuerySet->SFWGH->target, sIdx = 1;
              sTarget != NULL;
              sTarget  = sTarget->next, sIdx++ )
        {
            /* 2.4. Target이 Asterisk라면 Skip한다. */
            IDE_TEST( checkAsteriskTarget( aStatement,
                                           sTarget,
                                           sCurrSort->sortColumn,
                                           &( sIsFound ) )
                      != IDE_SUCCESS );

            if ( sIsFound == ID_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }

            /* 2.5. Target절에 존재하는 Order By 대상을 찾는다. */
            IDE_TEST( checkEquivalentTarget( aStatement,
                                             sTarget,
                                             sCurrSort->sortColumn,
                                             &( sIsEquivalent ) )
                      != IDE_SUCCESS );

            if ( sIsEquivalent == ID_TRUE )
            {
                /* 2.6. 중복된 Target이 존재하면 Skip 한다. */
                IDE_TEST_CONT( sEquivalent != NULL, NORMAL_EXIT );

                sEquivalent = sTarget;
                sTargetPos  = sIdx;

                /* 2.7. Order by절에 Target Position은 SShort 값까지 표현할 수 있다. */
                IDE_TEST_RAISE( sTargetPos > ID_SSHORT_MAX, ERR_UNSUPPORTED_TARGET_POSITION );
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sIsFound == ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        /* 3.1. Target절에 존재하면, Target Position Transfrom으로 처리한다. */
        if ( sEquivalent != NULL )
        {
            /* 3.2. BUG-47197 / Target Position Transfrom으로 처리한다. */
            if ( ( sEquivalent->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                 == QMS_TARGET_SHARD_ORDER_BY_TRANS_NONE )
            {
                IDE_TEST( makeExplicitPosition( aStatement,
                                                sTargetPos,
                                                &( sColumnNode ) )
                          != IDE_SUCCESS );

                SET_POSITION( sColumnNode->position, sCurrSort->sortColumn->position );
                SET_POSITION( sColumnNode->columnName, sCurrSort->sortColumn->position );

                sCurrSort->sortColumn = sColumnNode;
            }
            else
            {
                /* 3.3. 이미 Append된 대상은 추가로 Append하지 않고, Target Position Transfrom으로 처리한다. */
                IDE_TEST( makeShardColTransNode( aStatement,
                                                 NULL,
                                                 sTargetPos,
                                                 &( sColumnNode ) )
                          != IDE_SUCCESS );

                SET_POSITION( sColumnNode->position, sCurrSort->sortColumn->position );

                sCurrSort->sortColumn = sColumnNode;
            }
        }
        else
        {
            /* 4.1. Target절에 존재하지 않으면, Append 작업을 처리한다. */
            IDE_TEST( appendShardColTrans( aStatement,
                                           aQuerySet,
                                           sCurrSort,
                                           aTransType )
                      != IDE_SUCCESS );

            /* 4.2. Append 불가능한 대상이므로, 더이상 수행하지 않는다. */
            if ( ( aQuerySet->SFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
                 == QMV_SFWGH_SHARD_ORDER_BY_TRANS_ERROR )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processOrderByTransform",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERY_SET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processOrderByTransform",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_TARGET_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processOrderByTransform",
                                  "exceed a column count of order by" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkAsteriskTarget( qcStatement * aStatement,
                                               qmsTarget   * aTarget,
                                               qtcNode     * aSortNode,
                                               idBool      * aIsFound )
{
/****************************************************************************************
 *
 * Description : Asterisk인 Target과 Order By절에 같은 Target이 있는지 검사하는 함수이다.
 *               Table Name이 있는 경우는 Target Table Name를 기준으로 비교한다.
 *
 *  CASE  1 / SELECT    *       FROM T1     ORDER BY    C2;  |  FOUNND
 *  CASE  2 / SELECT    *       FROM T1     ORDER BY T1.C2;  |  FOUNND
 *  CASE  3 / SELECT    *       FROM T1, T2 ORDER BY    C2;  |  FOUNND
 *  CASE  4 / SELECT    *       FROM T1, T2 ORDER BY T1.C2;  |  FOUNND
 *  CASE  5 / SELECT    *       FROM T1, T2 ORDER BY T2.C2;  |  FOUNND
 *  CASE  6 / SELECT T1.*       FROM T1     ORDER BY T1.C2;  |  FOUNND
 *  CASE  7 / SELECT T1.*       FROM T1, T2 ORDER BY T1.C2;  |  FOUNND
 *  CASE  8 / SELECT T1.*, T2.* FROM T1, T2 ORDER BY T1.C2;  |  FOUNND
 *  CASE  9 / SELECT T1.*, T2.* FROM T1, T2 ORDER BY T2.C2;  |  FOUNND
 *  CASE 10 / SELECT T1.*       FROM T1     ORDER BY    C2;  |  UNKNOWN
 *  CASE 11 / SELECT T1.*       FROM T1, T2 ORDER BY    C2;  |  UNKNOWN
 *  CASE 12 / SELECT T1.*       FROM T1, T2 ORDER BY T2.C2;  |  UNKNOWN
 *  CASE 13 / SELECT T1.*, T2.* FROM T1, T2 ORDER BY    C2;  |  UNKNOWN
 *
 * Implementation : 1. Asterisk인지 검사한다.
 *                  2. Target에 Table Name가 있다면, Table Name을 비교한다.
 *                  3. 똑같다면, 대상을 찾았다.
 *                  4. 없다면, 무조건 찾는다.
 *
 ****************************************************************************************/

    idBool sIsFound = ID_FALSE;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aSortNode == NULL, ERR_NULL_SORT_NODE );
    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_TARGET );

    /* 1. Asterisk인지 검사한다. */
    if ( ( aTarget->flag & QMS_TARGET_ASTERISK_MASK )
         == QMS_TARGET_ASTERISK_TRUE )
    {
        /* 2. Target에 Table Name가 있다면, Table Name을 비교한다. */
        if ( QC_IS_NULL_NAME( aTarget->tableName ) != ID_TRUE )
        {
            if ( QC_IS_NULL_NAME( aSortNode->tableName ) != ID_TRUE )
            {
                /* 3. 똑같다면, 대상을 찾았다. */
                if ( QC_IS_NAME_MATCHED_POS_N_TARGET( aSortNode->tableName,
                                                      aTarget->tableName )
                     != ID_TRUE )
                {
                    /* Nothing to do */
                }
                else
                {
                    sIsFound = ID_TRUE;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /*  4. 없다면, 무조건 찾는다. */
            sIsFound = ID_TRUE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( aIsFound != NULL )
    {
        *aIsFound = sIsFound;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkAsteriskTarget",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SORT_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkAsteriskTarget",
                                  "sort node is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkAsteriskTarget",
                                  "target is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkEquivalentTarget( qcStatement * aStatement,
                                                 qmsTarget   * aTarget,
                                                 qtcNode     * aSortNode,
                                                 idBool      * aIsFound )
{
/****************************************************************************************
 *
 * Description : Target 절과 Order By절에 같은 Target이 있는지 검사하는 함수이다.
 *               Alias Column Name이 있는 경우는 Alias와 비교한다. Order By Transform
 *               으로 Appeand 한 Target에서도 검사한다.
 *
 *  TARGET TYPE  /         C         A
 *  1ST ORDER BY / SELECT C1, C2 AS C3            FROM T1 ORDER BY  C2, C2;
 *                         \_________\______________________________/        <-- 2. 3.
 *
 *  TARGET TYPE  /         C         A   T
 *  2ST ORDER BY / SELECT C1, C2 AS C3, C2 AS _$3 FROM T1 ORDER BY _$3, C2;
 *                         \_________\____\_____________________________/    <-- 4.
 *
 *  TRANSFORM    / SELECT C1, C2 AS C3, C2 AS _$3 FROM T1 ORDER BY _$3,  3;
 *
 * Implementation : 1. Append된 Target인지 검사한다.
 *                  2. Alias가 있다면, Alias와 비교한다.
 *                  3. ColumnName을 비교한다.
 *                  4. Append된 Target은 Alias가 있지만, ColumnName과 비교한다.
 *
 ****************************************************************************************/

    qtcNode      * sTargetNode = NULL;
    idBool         sIsFound    = ID_FALSE;
    qcNamePosition sUserName;
    qcNamePosition sTableName;
    qcNamePosition sPkgName;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aSortNode == NULL, ERR_NULL_SORT_NODE );
    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_TARGET );

    /* 1. Append된 대상인지 검사한다. */
    if ( ( aTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
         == QMS_TARGET_SHARD_ORDER_BY_TRANS_NONE )
    {
        /* 2. Alias가 있다면, Alias와 비교한다. */
        if ( QC_IS_NULL_NAME( aTarget->aliasColumnName ) != ID_TRUE )
        {
            if ( QC_IS_NULL_NAME( aSortNode->tableName ) != ID_TRUE )
            {
                /* Nothing to do */
            }
            else
            {
                if ( QC_IS_NAME_MATCHED_POS_N_TARGET( aSortNode->columnName,
                                                      aTarget->aliasColumnName )
                     != ID_TRUE )
                {
                    /* Nothing to do */
                }
                else
                {
                    sIsFound = ID_TRUE;
                }
            }
        }
        else
        {
            /* PROJ-2002 Column Security - target절에 보안 컬럼이 있는 경우 decrypt함수가 생성될 수 있다. */
            if ( aTarget->targetColumn->node.module == &( mtfDecrypt ) )
            {
                sTargetNode = (qtcNode *)aTarget->targetColumn->node.arguments;
            }
            else
            {
                sTargetNode = aTarget->targetColumn;
            }

            /* 3. ColumnName을 비교한다.
             *      Target 절에 Column Name 과 Alias Name 을 통틀어 중복된 것이 있다면,
             *      Column 을 정확히 지칭할 수 없어 Order By 대상이 될 수 없다.
             *      즉 Order By 가 있는 Target 절에는 중복된 Column Name 이 없어야 된다.
             */
            if ( QC_IS_NULL_NAME( aSortNode->tableName ) != ID_TRUE )
            {
                /* 따라서 Target 에 Alias Name 가 없고, Sort Column 은 Table, Column Name 만 있다면,
                 * Table, Column Name 만 비교 대상으로 유효하다.
                 */
                SET_POSITION( sUserName, sTargetNode->userName );
                SET_POSITION( sPkgName, sTargetNode->pkgName );

                SET_EMPTY_POSITION( sTargetNode->userName );
                SET_EMPTY_POSITION( sTargetNode->pkgName );

                IDE_TEST( qtc::isEquivalentExpressionByName( sTargetNode,
                                                             aSortNode,
                                                             &( sIsFound ) )
                          != IDE_SUCCESS );

                SET_POSITION( sTargetNode->userName, sUserName );
                SET_POSITION( sTargetNode->pkgName, sPkgName );
            }
            else
            {
                /* 따라서 Target 에 Alias Name 가 없고, Sort Column 은 Column Name 만 있다면,
                 * Column Name 만 비교 대상으로 유효하다.
                 */
                SET_POSITION( sUserName, sTargetNode->userName );
                SET_POSITION( sTableName, sTargetNode->tableName );
                SET_POSITION( sPkgName, sTargetNode->pkgName );

                SET_EMPTY_POSITION( sTargetNode->userName );
                SET_EMPTY_POSITION( sTargetNode->tableName );
                SET_EMPTY_POSITION( sTargetNode->pkgName );

                IDE_TEST( qtc::isEquivalentExpressionByName( sTargetNode,
                                                             aSortNode,
                                                             &( sIsFound ) )
                          != IDE_SUCCESS );

                SET_POSITION( sTargetNode->userName, sUserName );
                SET_POSITION( sTargetNode->tableName, sTableName );
                SET_POSITION( sTargetNode->pkgName, sPkgName );
            }
        }
    }
    else
    {
        /* 4. Append된 Target은 Alias가 있지만, ColumnName과 비교한다. */
        IDE_TEST_RAISE( QC_IS_NULL_NAME( aTarget->aliasColumnName ) == ID_TRUE, ERR_NULL_ALIAS );

        /* PROJ-2002 Column Security - target절에 보안 컬럼이 있는 경우 decrypt함수가 생성될 수 있다. */
        if ( aTarget->targetColumn->node.module == &( mtfDecrypt ) )
        {
            sTargetNode = (qtcNode *)aTarget->targetColumn->node.arguments;
        }
        else
        {
            sTargetNode = aTarget->targetColumn;
        }

        IDE_TEST( qtc::isEquivalentExpressionByName( sTargetNode,
                                                     aSortNode,
                                                     &( sIsFound ) )
                  != IDE_SUCCESS );
    }

    if ( aIsFound != NULL )
    {
        *aIsFound = sIsFound;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkEquivalentTarget",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SORT_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkEquivalentTarget",
                                  "sort node is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkEquivalentTarget",
                                  "target is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ALIAS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkEquivalentTarget",
                                  "alias is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeExplicitPosition( qcStatement * aStatement,
                                                UShort        aTargetPos,
                                                qtcNode    ** aValueNode )
{
/****************************************************************************************
 *
 * Description : Target Position으로 Transform하는 함수이다. 해당 Target Position값을
 *               Explicit 값처럼 SShort 상수값을 만든다.
 *
 *   QUERRY     / SELECT C1, C2 FROM T1   ORDER BY C2;
 *   EQUIVALENT /            **                    **
 *   POSITION   /        1   2
 *   TRANSFORM  / SELECT C1, C2 FROM T1   ORDER BY  2;
 *
 * Implementation :
 *
 ****************************************************************************************/

    qtcNode      * sValueNode[2] = { NULL, NULL };
    SChar          sValue[6];
    qcNamePosition sPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aTargetPos > ID_SSHORT_MAX, ERR_UNSUPPORTED_TARGET_POSITION );

    idlOS::snprintf( sValue,
                     6, /* 65535\0 : 6자리 */
                     "%"ID_UINT32_FMT,
                     aTargetPos );

    SET_EMPTY_POSITION( sPosition );

    IDE_TEST( qtc::makeValue( aStatement,
                              sValueNode,
                              (const UChar *)"SMALLINT",
                              8,
                              &( sPosition ),
                              (const UChar *)sValue,
                              idlOS::strlen( sValue ),
                              MTC_COLUMN_NORMAL_LITERAL )
              != IDE_SUCCESS );

    if ( aValueNode != NULL )
    {
        *aValueNode = sValueNode[0];
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeExplicitPosition",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_TARGET_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeExplicitPosition",
                                  "exceed a column count of order by" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardColTransNode( qcStatement * aStatement,
                                                 qmsTarget   * aTarget,
                                                 UShort        aTargetPos,
                                                 qtcNode    ** aColumnNode )
{
/****************************************************************************************
 *
 * Description : Shard Col Trans Node를 생성한다. 전달 받은 Target과 Target Position에
 *               따라서, '', _$N, _$N_$N 이름의 Column Node를 생성한다.
 *
 *  BEFORE  / SELECT AVG( C1 )
 *             FROM T1
 *              GROUP BY C2
 *               ORDER BY AVG( C2 );
 *                        *********
 *
 *  AFTER 1 / SELECT AVG( C1 ), AVG( C2 ) AS _$2
 *             FROM T1
 *              GROUP BY C2
 *               ORDER BY _$2;
 *                        ***
 *
 *  AFTER 2 / SELECT SUM( A ) / SUM( B )
 *             FROM SHARD( SELECT C2, SUM( C1 ) AS      A, COUNT( C1 ) AS      B,
 *                                    SUM( C1 ) AS _$2_$1, COUNT( C1 ) AS _$2_$2
 *                          FROM T1
 *                           GROUP BY C2 )
 *              GROUP BY C2
 *               ORDER BY SUM( _$2_$1 ) / SUM( _$2_$2 )
 *                             ******          ******
 *
 * Implementation : 1. Target이 있다면, Target Alias에 _$N 을 덧붙인 Position를 만든다.
 *                  2. Target Position만 있다면, _$N 인 Position를 만든다.
 *                  3. Null Position를 만든다.
 *                  4. 생성한 Position의 Column을 생성한다.
 *
 ****************************************************************************************/

    qtcNode        * sShardCol[2] = { NULL, NULL };
    SChar          * sColumnName  = NULL;
    qcNamePosition   sPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aTargetPos > ID_SSHORT_MAX, ERR_UNSUPPORTED_TARGET_POSITION );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( SHARD_ORDER_BY_LENGTH,
                                               (void **)&( sColumnName ) )
              != IDE_SUCCESS );

    /* 1. Target이 있다면, Target Alias에 _$N 을 덧붙인 Position를 만든다. */
    if ( aTarget != NULL )
    {
        IDE_TEST_RAISE( QC_IS_NULL_NAME( aTarget->aliasColumnName ) == ID_TRUE, ERR_NULL_ALIAS );
        IDE_TEST_RAISE( aTargetPos == 0, ERR_INVALIDE_POS );

        idlOS::snprintf( (SChar *)sColumnName,
                         SHARD_ORDER_BY_LENGTH,
                         "%.*s"SHARD_ORDER_BY_PREFIX"%"ID_UINT32_FMT,
                         aTarget->aliasColumnName.size,
                         aTarget->aliasColumnName.name,
                         aTargetPos );

        sPosition.stmtText = sColumnName;
        sPosition.offset   = 0;
        sPosition.size     = idlOS::strlen( sColumnName );
    }
    else
    {
        /* 2. Target Position만 있다면, _$N 인 Position를 만든다. */
        if ( aTargetPos > 0 )
        {
            idlOS::snprintf( (SChar *)sColumnName,
                             SHARD_ORDER_BY_LENGTH,
                             SHARD_ORDER_BY_PREFIX"%"ID_UINT32_FMT,
                             aTargetPos );

            sPosition.stmtText = sColumnName;
            sPosition.offset   = 0;
            sPosition.size     = idlOS::strlen( sColumnName );
        }
        else
        {
            /* 3. Null Position를 만든다. */
            SET_EMPTY_POSITION( sPosition );
        }
    }

    /* 4. 생성한 Position의 Column을 생성한다. */
    IDE_TEST( qtc::makeNode( aStatement,
                             sShardCol,
                             &( sPosition ),
                             &( qtc::columnModule ) )
              != IDE_SUCCESS );

    if ( aColumnNode != NULL )
    {
        *aColumnNode = sShardCol[0];
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardColTransNode",
                                  "statemnet is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALIDE_POS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardColTransNode",
                                  "invalid target pos" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_TARGET_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardColTransNode",
                                  "exceed a column count of order by" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ALIAS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardColTransNode",
                                  "alias is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::appendShardColTrans( qcStatement    * aStatement,
                                               qmsQuerySet    * aQuerySet,
                                               qmsSortColumns * aSort,
                                               UInt             aTransType )
{
/****************************************************************************************
 *
 * Description : Order By 대상 형태를 검사해서, Shard Col Trans Node를 생성하고
 *               Target 절에 Append하는 함수이다.
 *
 *               대상을 Validate할 수 있는 경우엔 Skip하며, 대상이 Error인 경우엔
 *               Error 처리를 위해 정보를 남기고, 의미 없는 경우에 Target Position 1로
 *               변환하기도 한다.
 *
 *               주의할 점은 Append Target 사이에 다른 Transform 으로 생성된 Target 이
 *               있으면 안된다.
 *
 *  BEFORE / SELECT AVG( C1 ) FROM T1 ORDER BY C2;
 *  AFTER  / SELECT AVG( C1 ) FROM T1 ORDER BY  1;  <-- 2.
 *           ************************************ ONEROW
 *
 *  BEFORE / SELECT AVG( C1 ) C1 FROM T1 ORDER BY AVG( C1 );  <-- 3.
 *  AFTER  /        ************ NESTED                ** ERROR*
 *
 *  BEFORE / SELECT AVG( C1 ) FROM T1 GROUP BY C2 ORDER BY C2;  <-- 4.
 *  AFTER  /                                   ** GRUOPKEY ** SKIP
 *  SHARD  / SELECT SUM( A ) / SUM( B )
 *            FROM SHARD( SELECT C2, SUM( C1 ) A, COUNT( C1 ) B FROM T1 GROUP BY C2 )
 *              GROUP BY C2      /
 *               ORDER BY C2    /
 *                         \___/
 *
 *  BEFORE / SELECT AVG( C1 )                   FROM T1 GROUP BY C2 ORDER BY SUM( C3 );
 *  AFTER  / SELECT AVG( C1 ), SUM( C3 ) AS _$2 FROM T1 GROUP BY C2 ORDER BY       _$2;  <-- 5.
 *                                 *5.5~5.9*                                    *5.4,5.10*
 *  SHARD  / SELECT SUM( A ) / SUM( B )
 *            FROM SHARD( SELECT C2, SUM( C1 ) AS A, COUNT( C1 ) AS B,
 *                                   SUM( C3 ) AS _$2
 *                         FROM T1 GROUP BY C2 )   /
 *              GROUP BY C2                       /
 *               ORDER BY _$2                    /
 *                          \___________________/
 *
 * Implementation : 1.    Order By 대상 형태를 검사한다.
 *                  2.    의미 없는 경우는 Target Position 1로 변환한다.
 *                  3.    대상이 Error인 경우엔 Error 처리 여부를 남긴다.
 *                  4.    대상을 Validate할 수 있는 경우엔 Skip한다.
 *                  5.1.  Append가 필요한 경우 Transfrom을 수행한다.
 *                  5.2.  Target 마지막을 찾는다.
 *                  5.3.  Order by절에 Target Position은 SShort 값까지 표현할 수 있다
 *                  5.4.  Shard Col Trans Node를 생성한다.
 *                  5.5.  새로운 Target를 생성한다.
 *                  5.6.  Order By Node를 새로운 Target 연결한다.
 *                  5.7.  Shard Col Trans 형태를 남긴다.
 *                  5.8.  새로운 Target에 Shard Col Trans Node의 Alias를 지정한다.
 *                  5.9.  Target 마지막에 새로운 Target를 덧붙인다.
 *                  5.10. Order By Node를 Shard Col Trans Node로 바꾼다.
 *                  5.11. Transform 여부를 남긴다.
 *
 ****************************************************************************************/

    qmsTarget * sTarget     = NULL;
    qmsTarget * sTargetTail = NULL;
    qtcNode   * sColumnNode = NULL;
    UInt        sFlag       = 0;
    UShort      sTargetPos  = 0;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aSort == NULL, ERR_NULL_SORT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERY_SET );

    /* 1.    Order By 대상 형태를 검사한다. */
    IDE_TEST( checkAppendAbleType( aQuerySet->SFWGH,
                                   aSort->sortColumn,
                                   aTransType,
                                   &( sFlag ) )
              != IDE_SUCCESS );

    /* 2.    의미 없는 경우는 Target Position 1로 변환한다. */
    if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
         == QMS_TARGET_SHARD_ORDER_BY_TRANS_POSITION )
    {
        IDE_TEST( makeExplicitPosition( aStatement,
                                        1,
                                        &( sColumnNode ) )
                  != IDE_SUCCESS );

        SET_POSITION( sColumnNode->position, aSort->sortColumn->position );
        SET_POSITION( sColumnNode->columnName, aSort->sortColumn->position );

        aSort->sortColumn = sColumnNode;
    }
    else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
              == QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR )
    {
        /* 3.    대상이 Error인 경우엔 Error 처리 여부를 남긴다. */
        aQuerySet->SFWGH->lflag &= ~QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK;
        aQuerySet->SFWGH->lflag |= QMV_SFWGH_SHARD_ORDER_BY_TRANS_ERROR;
    }
    else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
              == QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP )
    {
        /* 4.   대상을 Validate할 수 있는 경우엔 Skip한다.*/
    }
    else
    {
        /* 5.1.  Append가 필요한 경우 Transfrom을 수행한다. */
        for ( sTarget  = aQuerySet->SFWGH->target, sTargetPos = 1;
              sTarget != NULL;
              sTarget  = sTarget->next, sTargetPos++ )
        {
            /* 5.2.  Target 마지막을 찾는다. */
            sTargetTail = sTarget;
        }

        /* 5.3.  Order by절에 Target Position은 SShort 값까지 표현할 수 있다. */
        IDE_TEST_RAISE( sTargetPos > ID_SSHORT_MAX, ERR_UNSUPPORTED_TARGET_POSITION );

        /* 5.4.  Shard Col Trans Node를 생성한다. */
        IDE_TEST( makeShardColTransNode( aStatement,
                                         NULL,
                                         sTargetPos,
                                         &( sColumnNode ) )
                  != IDE_SUCCESS );

        /* 5.5.  새로운 Target를 생성한다. */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsTarget,
                                &( sTarget ) )
                  != IDE_SUCCESS );

        QMS_TARGET_INIT( sTarget );

        /* 5.6.  Order By Node를 새로운 Target 연결한다. */
        sTarget->targetColumn = aSort->sortColumn;

        /* 5.7.  Shard Col Trans 형태를 남긴다. */
        sTarget->flag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
        sTarget->flag |= sFlag;

        /* 5.8.  새로운 Target에 Shard Col Trans Node의 Alias를 지정한다. */
        sTarget->aliasColumnName.name = sColumnNode->position.stmtText;
        sTarget->aliasColumnName.size = sColumnNode->position.size;

        /* 5.9.  Target 마지막에 새로운 Target를 덧붙인다. */
        sTargetTail->next = sTarget;

        /* 5.10.  Order By Node를 Shard Col Trans Node로 바꾼다. */
        SET_POSITION( sColumnNode->position, aSort->sortColumn->position );

        aSort->sortColumn = sColumnNode;

        /* 5.11. Transform 여부를 남긴다. */
        aQuerySet->SFWGH->lflag &= ~QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK;
        aQuerySet->SFWGH->lflag |= QMV_SFWGH_SHARD_ORDER_BY_TRANS_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::appendShardColTrans",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SORT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::appendShardColTrans",
                                  "sort is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERY_SET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::appendShardColTrans",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_TARGET_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::appendShardColTrans",
                                  "exceed a column count of order by" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkAppendAbleType( qmsSFWGH * aSFWGH,
                                               qtcNode  * aNode,
                                               UInt       aTransType,
                                               UInt     * aFlag )
{
/****************************************************************************************
 *
 * Description : Order By 대상 형태를 검사하는 함수이다.
 *               Order By 대상으로 여러 행태가 가능하지만, C1, T1.C1, AGGR( C1 ),
 *               C1 + C1 형태만 분석하여, 몇가지 Shard Col Trans Type으로 구별한다.
 *
 *               Shard Col Trans Type은 COLUMN, EXPRESSION, AGGREGATION으로 구분하며,
 *               그외 변환하지 않지만 제어할 용도로 POSITION, SKIP, ERROR, GROUPKEY,
 *               NONKEY를 사용한다.
 *
 *               COLUMN은 컬럼만 있는 형태, AGGREGATION은 집합함수가 포함된 모든 형태,
 *               EXPRESSION은 집합함수를 제외한 모든 형태이다.
 *
 *               POSITION은 Target Position Transform 처리가 가능한 형태, SKIP은
 *               Validate이 가능한 형태, ERROR 대상이 Error인 형태이며, GROUPKEY,
 *               NONKEY는 ORDER BY QUERYSET 형태에 따라 SKIP 또는 COLUMN 으로 처리하는
 *               형태이다.
 *
 *               qmvShardTransform::checkSortNodeTree 호출로 대상에 분석된 결과를 받고,
 *               POSITION 형태만 이 함수에서 결정한다.
 *
 *  QUERY     / SELECT AVG( C1 ) FROM T1 ORDER BY C2;
 *  CHECK     / ************************************ ONEROW  <-- 2.
 *  OUTPUT    / QMS_TARGET_SHARD_ORDER_BY_TRANS_COLUMN
 *               -> QMS_TARGET_SHARD_ORDER_BY_TRANS_POSITON   <-- 3.
 *
 *  TRANSFORM / SELECT AVG( C1 ) FROM T1 ORDER BY  1;
 *
 * Implementation : 1.   Subquery 라면 의미없는 형태로 SKIP 이다.
 *                  2.   Order By 대상 형태를 검사한다.
 *                  3.1. COLUMN 이라면 의미없는 형태인지 검사한다.
 *                  3.2. POSITION 으로 변경한다.
 *
 ****************************************************************************************/

    qmsTarget * sTarget = NULL;
    idBool      sIsAggr = ID_FALSE;
    UInt        sFlag   = 0;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /* 1.   Subquery 라면 의미없는 형태로 SKIP 이다. */
    if ( ( QTC_HAVE_SUBQUERY( aNode ) == ID_TRUE ) ||
         ( QTC_IS_SUBQUERY( aNode ) == ID_TRUE ) )
    {
        sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
        sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
    }
    else
    {
        /* 2.   Order By 대상 형태를 검사한다. */
        IDE_TEST( checkSortNodeTree( aSFWGH,
                                     aNode,
                                     aTransType,
                                     ID_TRUE,
                                     &( sFlag ) )
                  != IDE_SUCCESS );
    }

    /* 3.1. COLUMN 이라면 의미없는 형태인지 검사한다.*/
    if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
         == QMS_TARGET_SHARD_ORDER_BY_TRANS_COLUMN )
    {
        if ( aSFWGH->group == NULL )
        {
            for ( sTarget  = aSFWGH->target;
                  sTarget != NULL;
                  sTarget  = sTarget->next )
            {
                IDE_TEST( isAggrNode( sTarget->targetColumn,
                                      &( sIsAggr ) )
                          != IDE_SUCCESS );

                /* 3.2. POSITION 으로 변경한다. */
                if ( sIsAggr == ID_TRUE )
                {
                    sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                    sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_POSITION;

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
            /* Nothing to do */
        }
    }
    else
    {

    }

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isAppendAbleSortNode",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isAppendAbleSortNode",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkSortNodeTree( qmsSFWGH * aSFWGH,
                                             qtcNode  * aNode,
                                             UInt       aTransType,
                                             idBool     aIsRoot,
                                             UInt     * aFlag )
{
/****************************************************************************************
 *
 * Description : Order By 대상 형태를 검사하는 함수이다.
 *               Order By 대상의 Node에 종류에 따러서 해당 Node를 분석하는 함수를
 *               호출한다.
 *
 *               Column이라면 qmvShardTransform::checkTableColumn
 *               Aggregation이라면 qmvShardTransform::checkAggrColumn,
 *               그외에는 qmvShardTransform::checkExprColumn 을 호출하며,
 *               qmvShardTransform::checkAggrColumn, qmvShardTransform::checkExprColumn 은
 *               qmvShardTransform::checkSortNodeTree 를 내부에서 호출하여 재귀 호출한다.
 *
 *               재귀 호출로 분석한 형태를 그대로 반환하지만, 최상위 Node에서는 불필요한
 *               제어용 형태를 변경한다. 즉 SKIP, NONKEY, GROUPKEY, ERROR 형태가 다른
 *               형태로 변경된다.
 *
 *               Expression에 Group By가 있는 경우인, GROUPKEY는 이미 Target절에 있다는
 *               의미이며, NONKEY는 Groupping엔 Aggregation Target만 가능한 점을 고려해서,
 *               모두 SKIP 형태로 변경한다.
 *
 *  COLUMN                                                                         <-- 2.
 *
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY C2;  |  COLUMN    |
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY A2;  |  SKIP      |
 *   SELECT AVG( C1 ) A1 FROM T1 GORUP BY C1 ORDER BY C2;  |  NONKEY    |  ERROR
 *   SELECT AVG( C1 ) A1 FROM T1 GORUP BY C2 ORDER BY C2;  |  GORUPKEY  |  SKIP
 *
 *  EXPRESSION                                                                     <-- 3.
 *
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY C2        + 1;  |  EXPRESSION   |
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY A2        + 1;  |  SKIP         |
 *   SELECT AVG( C1 ) A1 FROM T1 GORUP BY C1 ORDER BY C2        + 1;  |  NONKEY       |  SKIP
 *   SELECT AVG( C1 ) A1 FROM T1 GORUP BY C2 ORDER BY C2        + 1;  |  GROUPKEY     |  SKIP
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY AVG( C2 ) + 1;  |  AGGREGATION  |
 *
 * Implementation : 1.1. Column 형태를 분석한다.
 *                  1.2. 최상위라면 COLUMN 형태 외에는 변환하지 않는다.
 *                  2.   Aggregaion 형태를 분석한다.
 *                  3.1. Expression 형태를 분석한다.
 *                  3.2. 최상위라면 GROUPKEY, NONKEY 형태는 변환하지 않는다.
 *
 ****************************************************************************************/

    UInt sFlag = 0;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /*  1.1. Column 형태를 분석한다. */
    if ( aNode->node.module == &( qtc::columnModule ) )
    {
        IDE_TEST( checkTableColumn( aSFWGH,
                                    aNode,
                                    aTransType,
                                    &( sFlag ) )
                  != IDE_SUCCESS );

        /* 1.2. 최상위라면 COLUMN 형태 외에는 변환하지 않는다. */
        if ( aIsRoot == ID_TRUE )
        {
            if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                 == QMS_TARGET_SHARD_ORDER_BY_TRANS_GROUPKEY )
            {
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
            }
            else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                      == QMS_TARGET_SHARD_ORDER_BY_TRANS_NONKEY )
            {
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR;
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
        /* 2.   Aggregaion 형태를 분석한다. */
        if ( QTC_IS_AGGREGATE( aNode ) == ID_TRUE )
        {
            IDE_TEST( checkAggrColumn( aSFWGH,
                                       aNode,
                                       aTransType,
                                       &( sFlag ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /*  3.1. Expression 형태를 분석한다. */
            IDE_TEST( checkExprColumn( aSFWGH,
                                       aNode,
                                       aTransType,
                                       &( sFlag ) )
                      != IDE_SUCCESS );

            /* 3.2. 최상위라면 GROUPKEY, NONKEY 형태는 변환하지 않는다. */
            if ( aIsRoot == ID_TRUE )
            {
                if ( ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                       == QMS_TARGET_SHARD_ORDER_BY_TRANS_NONKEY )
                     ||
                     ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                       == QMS_TARGET_SHARD_ORDER_BY_TRANS_GROUPKEY ) )
                {
                     sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                     sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
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

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkSortNodeType",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkSortNodeType",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkTableColumn( qmsSFWGH * aSFWGH,
                                            qtcNode  * aNode,
                                            UInt       aTransType,
                                            UInt     * aFlag )
{
/****************************************************************************************
 *
 * Description : Order By Column 대상 형태를 검사하는 함수이다.
 *               Table Name 유무에 따라서 qmvShardTransform::isValidTableName 이나
 *               qmvShardTransform::isValidAliasName 를 호출한다.
 *
 *               Table Name이 유효하면 qmvShardTransform::checkGroupColumn 를 검사한다.
 *               Alias Name이 아니면 qmvShardTransform::checkGroupColumn 를 검사한다.
 *
 *               특히, Aggregation에 Alias가 Target 절의 Aggregation Alias인 경우는
 *               Nested Aggregation으로 대상이 Erorr인 경우로, ERROR 형태로 반환한다.
 *
 *  Table Name / O /
 *
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY     T1.C2;  |  COLUMN  <-- 1.2
 *   SELECT AVG( C1 ) A1 FROM T1 TA          ORDER BY     TA.C2;  |  COLUMN  <-- 1.2.
 *   SELECT AVG( C1 ) A1 FROM T1 TA          ORDER BY     T1.C2;  |  SKIP    <-- 2.4.
 *
 *  Table Name / X /
 *
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY        C2;  |  COLUMN  <-- 2.5
 *
 *  Alias Name / O /
 *
 *   SELECT      C1   A1 FROM T1 GROUP BY C1 ORDER BY AVG( A1 );  |  SKIP    <-- 2.2.
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY AVG( A1 );  |  ERROR   <-- 2.3.
 *
 *
 * Implementation : 1.1. Table Name 있다면 Table Name을 분석한다.
 *                  1.2. 유효하면 Column 분석 함수를 호출한다.
 *                  2.1. 없다면 Alias 분석 함수를 호출한다.
 *                  2.2. 맞고, AGGREGATION 이면, AGGR 형태이다.
 *                  2.3. 아니면, SKIP 형태이다.
 *                  2.4. 그외는 GROUP KEY 검사 함수를 호출한다.
 *
 ****************************************************************************************/

    idBool sIsFound = ID_FALSE;
    idBool sIsAggr  = ID_FALSE;
    UInt   sFlag    = 0;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /* 1.1. Table Name 있다면 Table Name을 분석한다. */
    if ( QC_IS_NULL_NAME( aNode->tableName ) != ID_TRUE )
    {
        IDE_TEST( isValidTableName( aSFWGH->from,
                                    aNode,
                                    &( sIsFound ) )
                  != IDE_SUCCESS );

        /* 1.2. 유효하면 Column 분석 함수를 호출한다. */
        if ( sIsFound == ID_TRUE )
        {
            IDE_TEST( checkGroupColumn( aSFWGH,
                                        aNode,
                                        aTransType,
                                        &( sFlag ) )
                      != IDE_SUCCESS );
        }
        else
        {
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
        }
    }
    else
    {
        /* 2.1. 없다면 Alias 분석 함수를 호출한다. */
        IDE_TEST( isValidAliasName( aSFWGH,
                                    aNode,
                                    &( sIsFound ),
                                    &( sIsAggr ) )
                  != IDE_SUCCESS );

        if ( sIsFound == ID_TRUE )
        {
            if ( sIsAggr == ID_TRUE )
            {
                /* 2.2. 맞고, AGGREGATION 이면, AGGR 형태이다. */
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGR;
            }
            else
            {
                /* 2.3. 아니면, SKIP 형태이다. */
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
            }
        }
        else
        {
            /* 2.4. GROUP KEY 검사 함수를 호출한다. */
            IDE_TEST( checkGroupColumn( aSFWGH,
                                        aNode,
                                        aTransType,
                                        &( sFlag ) )
                      != IDE_SUCCESS );
        }
    }

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkTableColumn",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkTableColumn",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::isValidTableName( qmsFrom * aFrom,
                                            qtcNode * aNode,
                                            idBool  * aIsValid )
{
/****************************************************************************************
 *
 * Description : Table Name이 유효한 것인지 검사하는 함수이다.
 *               Alias Table Name과 Join 형태도 고려하고 있다.
 *
 * Implementation : 1.1. Join 형태인지 검사한다.
 *                  1.2. Alias Table Name이 있다면 검사한다.
 *                  1.3. 없다면 Table Name을 검사한다.
 *                  2.1. Left 로 재귀한다.
 *                  2.2. Right 로 재귀한다.
 *
 ****************************************************************************************/

    qmsFrom * sFrom    = NULL;
    idBool    sIsValid = ID_FALSE;

    IDE_TEST_RAISE( aFrom == NULL, ERR_NULL_FROM );
    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );

    /* 1.1. Join 형태인지 검사한다. */
    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        for ( sFrom  = aFrom;
              sFrom != NULL;
              sFrom  = sFrom->next )
        {
            /* 1.2. Alias Table Name이 있다면 검사한다. */
            if ( QC_IS_NULL_NAME( sFrom->tableRef->aliasName ) != ID_TRUE )
            {
                if ( QC_IS_NAME_MATCHED_OR_EMPTY( sFrom->tableRef->aliasName,
                                                  aNode->tableName )
                     != ID_TRUE )
                {
                    /* Nothing to do */
                }
                else
                {
                    sIsValid = ID_TRUE;

                    break;
                }
            }
            else
            {
                /* 1.3. 없다면 Table Name을 검사한다. */
                if ( QC_IS_NAME_MATCHED_OR_EMPTY( sFrom->tableRef->tableName,
                                                  aNode->tableName )
                     != ID_TRUE )
                {
                    /* Nothing to do */
                }
                else
                {
                    sIsValid = ID_TRUE;

                    break;
                }
            }
        }
    }
    else
    {
        /* 2.1. Left 로 재귀한다. */
        IDE_TEST( isValidTableName( aFrom->left,
                                    aNode,
                                    &( sIsValid ) )
                  != IDE_SUCCESS );

        if ( sIsValid == ID_TRUE )
        {
            /* Nothing to do */
        }
        else
        {
            /* 2.2. Right 로 재귀한다. */
            IDE_TEST( isValidTableName( aFrom->right,
                                        aNode,
                                        &( sIsValid ) )
                      != IDE_SUCCESS );
        }
    }

    if ( aIsValid != NULL )
    {
        *aIsValid = sIsValid;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_FROM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isValidTableName",
                                  "from is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isValidTableName",
                                  "node is NULL" ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::isValidAliasName( qmsSFWGH * aSFWGH,
                                            qtcNode  * aNode,
                                            idBool   * aIsValid,
                                            idBool   * aIsAggr )
{
/****************************************************************************************
 *
 * Description : Order By Column 대상 형태를 검사하는 함수이다.
 *               Alias 대상인 경우를 검사한다. Alias 대상이 아니라면 Group Key 검사
 *               함수를 호출한다.
 *
 *               qmvShardTransform::procssOrderByTransform 으로 Alias 단독인 경우는
 *               Target Position Transform으로 처리하였으나, Expression이나 Aggregation에
 *               Alias를 사용한 것은 이 함수에서 검사한다.
 *
 *               특히, Aggregation에 Alias가 Target 절의 Aggregation Alias인 경우는
 *               Nested Aggregation으로 대상이 Erorr인 경우로, ERROR 형태로 반환한다.
 *
 *  SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY AVG( A1 );  |  ERROR   <--2.2.
 *  SELECT      C1   A1 FROM T1 GROUP BY C1 ORDER BY AVG( A1 );  |  SKIP    <--2.3.
 *
 * Implementation : 1. Target 절 Alias 와 비교한다.
 *
 ****************************************************************************************/

    qmsTarget * sTarget  = NULL;
    idBool      sIsValid = ID_FALSE;
    idBool      sIsAggr  = ID_FALSE;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /* 1.   Target 절 Alias 와 비교한다. */
    for ( sTarget  = aSFWGH->target;
          sTarget != NULL;
          sTarget  = sTarget->next )
    {
        if ( QC_IS_NULL_NAME( sTarget->aliasColumnName ) != ID_TRUE )
        {
            if ( QC_IS_NAME_MATCHED_POS_N_TARGET( aNode->columnName,
                                                  sTarget->aliasColumnName )
                 != ID_TRUE )
            {
                /* Nothing to do */
            }
            else
            {
                sIsValid = ID_TRUE;

                break;
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( sIsValid == ID_TRUE )
    {
        IDE_TEST( isAggrNode( sTarget->targetColumn,
                              &( sIsAggr ) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( aIsValid != NULL )
    {
        *aIsValid = sIsValid;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aIsAggr != NULL )
    {
        *aIsAggr = sIsAggr;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isValidAliasName",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isValidAliasName",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::isAggrNode( qtcNode * aNode,
                                      idBool  * aIsAggr )
{
/****************************************************************************************
 *
 * Description : Aggregation이 있는 Node인기 검사하는 함수이다.
 *
 * Implementation : 1. Aggregation Node인지 검사한다.
 *                  2. Arguement로 재귀한다.
 *
 ****************************************************************************************/

    qtcNode * sNode   = NULL;
    idBool    sIsAggr = ID_FALSE;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );

    /* 1. Aggregation Node인지 검사한다. */
    if ( QTC_IS_AGGREGATE( aNode ) == ID_TRUE )
    {
        sIsAggr = ID_TRUE;
    }
    else
    {
        /* 2. Arguement로 재귀한다. */
        for ( sNode  = (qtcNode *)aNode->node.arguments;
              sNode != NULL;
              sNode  = (qtcNode *)sNode->node.next )
        {
            IDE_TEST( isAggrNode( sNode,
                                  &( sIsAggr ) )
                      != IDE_SUCCESS );
        }
    }

    if ( aIsAggr != NULL )
    {
        *aIsAggr = sIsAggr;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isAggrNode",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::isSupportedNode( qtcNode * aNode,
                                           idBool  * aIsSupported )
{
/****************************************************************************************
 *
 * Description : Aggregation이 있는 Node인기 검사하는 함수이다.
 *
 * Implementation : 1. Aggregation Node인지 검사한다.
 *                  2. Arguement로 재귀한다.
 *
 ****************************************************************************************/

    idBool    sIsSupported = ID_FALSE;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );

    /* 1. Aggregation Node인지 검사한다. */
    if ( ( aNode->node.module != &( mtfSum ) ) &&
         ( aNode->node.module != &( mtfMin ) ) &&
         ( aNode->node.module != &( mtfMax ) ) &&
         ( aNode->node.module != &( mtfAvg ) ) &&
         ( aNode->node.module != &( mtfCount ) ) )
    {
        sIsSupported = ID_FALSE;
    }
    else
    {
        sIsSupported = ID_TRUE;
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

    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::isSupportedNode",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkGroupColumn( qmsSFWGH * aSFWGH,
                                            qtcNode  * aNode,
                                            UInt       aTransType,
                                            UInt     * aFlag )
{
/****************************************************************************************
 *
 * Description : Order By Column 대상 형태를 검사하는 함수이다.
 *               Group By 절이 있는 경우에, Group Key과 같은 대상인지 검사한다.
 *
 *               qmvShardTransform::procssAggrTransform 를 이후에 호출한다면, Group By 절
 *               대상을 Shard View Target에 추가한다. 이것을 구별하기 위해서, Group By 절
 *               과 동일한 대상이 있다면 GROUPKEY로 없다면 NONKEY 형태로 반환한다.
 *
 *               또한, Group By 절에는 해당 Target 절에 있는 Column만 사용한 점을 이용해서,
 *               Group By 대상을 기준으로 비교를 수행한다.
 *
 *               이 반환값은 이후에 이 함수를 호출한 상위 함수에서 해당 Node의 형태에 따라
 *               다른 형태로 변경하는 제어값로 사용된다.
 *
 *  QUERY     /                      SELECT AVG( C1 ) FROM T1 GROUP BY C1 .....;
 *  TRANSFORM / SELECT * FROM SHARD( SELECT AVG( C1 ) FROM T1 GROUP BY C1 ) ...;
 *
 *  QUERY     / SELECT AVG( C1 ) FROM T1 GROUP BY C1 ORDER BY C1;  |  COLUMN   <-- 4.
 *  QUERY     / SELECT AVG( C1 ) FROM T1 GROUP BY C1 ORDER BY C2;  |  COLUMN   <-- 4.
 *
 *  QUERY     /         SELECT     AVG( C1 )                  FROM T1 GROUP BY C2 .....;
 *  TRANSFORM / SELECT SUM(A) / SUM(B) FROM
 *               SHARD( SELECT C2, SUM( C1 ) A, COUNT( C1 ) B FROM T1 GROUP BY C2 ) ...;
 *                             **
 *
 *  QUERY     / SELECT AVG( C1 ) FROM T1 GROUP BY C2 ORDER BY C1;  |  NONKEY     <-- 3.4.
 *  QUERY     / SELECT AVG( C1 ) FROM T1 GROUP BY C2 ORDER BY C2;  |  GROUPKEY   <-- 3.3.
 *
 * Implementation : 1.   Transfrom 형태를 검사한다.
 *                  2.1. Table Name이 포함되었다면, 같이 비교한다.
 *                  2.2. Column Name을 비교한다.
 *                  3.1. 찾았다면, GROUPKEY 형태이다.
 *                  3.2. Group By 절 유무를 검사한다.
 *                  3.3. 없다면, NONEKEY 형태이다.
 *                  4.   그외는 COLUMN 형태이다.
 *
 ****************************************************************************************/

    qmsConcatElement * sGroup   = NULL;
    qtcNode          * sNode    = NULL;
    UInt               sFlag    = 0;
    idBool             sIsFound = ID_FALSE;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /* 1.   Group By 절 유무를 검사한다. */
    if ( ( aTransType == QMS_ORDER_BY_TRANSFORM_AGGREGATION ) &&
         ( aSFWGH->group != NULL ) )
    {
        for ( sGroup  = aSFWGH->group;
              sGroup != NULL;
              sGroup  = sGroup->next )
        {
            sNode = sGroup->arithmeticOrList;

            /* 2.1. Table Name이 포함되었다면, 같이 비교한다. */
            if ( QC_IS_NULL_NAME( sNode->tableName ) != ID_TRUE )
            {
                if ( ( QC_IS_NAME_MATCHED_OR_EMPTY( sNode->tableName,
                                                    aNode->tableName )
                       != ID_TRUE )
                     ||
                     ( QC_IS_NAME_MATCHED_OR_EMPTY( sNode->columnName,
                                                    aNode->columnName )
                       != ID_TRUE ) )
                {
                    /* Nothing to do */
                }
                else
                {
                    sIsFound = ID_TRUE;

                    break;
                }
            }
            else
            {
                /* 2.2. Column Name을 비교한다. */
                if ( QC_IS_NAME_MATCHED_OR_EMPTY( sNode->columnName,
                                                  aNode->columnName )
                     != ID_TRUE )
                {
                    /* Nothing to do */
                }
                else
                {
                    sIsFound = ID_TRUE;

                    break;
                }

            }
        }

        /* 3.1. 찾았다면, GROUPKEY 형태이다. */
        if ( sIsFound == ID_TRUE )
        {
            /* 3.2. Transfrom 형태를 검사한다. */
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_GROUPKEY;
        }
        else
        {
            /* 3.3. 없다면, NONEKEY 형태이다. */
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_NONKEY;
        }
    }
    else
    {
        /* 4.   그외는 COLUMN 형태이다. */
        sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
        sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_COLUMN;
    }

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkGroupColumn",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkGroupColumn",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkAggrColumn( qmsSFWGH * aSFWGH,
                                           qtcNode  * aNode,
                                           UInt       aTransType,
                                           UInt     * aFlag )
{
/****************************************************************************************
 *
 * Description : Order By Aggregation 대상 형태를 검사하는 함수이다.
 *               Nested Aggregation으로 대상이 Query Set 내에서 Validate 불가능한 ERROR와
 *               Validate 가능한 SKIP 이 아니면 무조건 AGGREGATION 형태이다.
 *
 *  SELECT      C1   A1 FROM T1 GROUP BY C1 ORDER BY AVG( A1 );        |  SKIP         <-- 2.
 *  SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY AVG( A1 );        |  SKIP         <-- 4.1.
 *  SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY AVG( A1 );        |  ERROR        <-- 4.2.
 *  SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY AVG( C2 );        |  AGGREGATION  <-- 3.
 *  SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY AVG( C2 + 1 );    |  AGGREGATION  <-- 3.
 *  SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY AVG( F1( C2 ) );  |  AGGREGATION  <-- 3.
 *
 * Implementation : 1.   지원하는 Aggregation 인지 검사한다.
 *                  2.   Node Argument 먼저 분석한다.
 *                  3.   AGGR 와 SKIP 인지 검사한다.
 *                  4.   아니면 무조건 AGGREGATION 형태이다.
 *                  5.1. AGGREGATION TRANSFORM이면, SKIP 형태이다.
 *                  5.2. QUERYSET TRANSFORM이면, ERROR 형태이다.
 *                  6.   COUNT( * ) 면 무조건 AGGREGATION 형태이다.
 *
 ****************************************************************************************/

    qtcNode * sNode        = NULL;
    UInt      sFlag        = 0;
    idBool    sIsSupported = ID_TRUE;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    IDE_TEST( isSupportedNode( aNode,
                               &( sIsSupported ) )
               != IDE_SUCCESS );

    /* 1.   지원하는 Aggregation 인지 검사한다. */
    if ( ( aTransType == QMS_ORDER_BY_TRANSFORM_AGGREGATION ) &&
         ( sIsSupported == ID_FALSE ) )
    {
        sFlag = QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
    }
    else
    {
        /* 2.   Node Argument 먼저 분석한다. */
        for ( sNode  = (qtcNode*)aNode->node.arguments;
              sNode != NULL;
              sNode  = (qtcNode*)sNode->node.next )
        {
            IDE_TEST( checkSortNodeTree( aSFWGH,
                                         sNode,
                                         aTransType,
                                         ID_FALSE,
                                         &( sFlag )  )
                      != IDE_SUCCESS );

            /* 3.   AGGR 와 SKIP 인지 검사한다. */
            if ( ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                   == QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP )
                 ||
                 ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                   == QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGR ) )
            {
                break;
            }
            else
            {
                /* 4.   아니면 무조건 AGGREGATION 형태이다. */
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGREGATION;
            }
        }

        if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
             == QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGR )
        {
            /* 5.1. AGGREGATION TRANSFORM이면, SKIP 형태이다. */
            if ( aTransType == QMS_ORDER_BY_TRANSFORM_AGGREGATION )
            {
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;
            }
            else
            {
                /* 5.2. QUERYSET TRANSFORM이면, ERROR 형태이다. */
                sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
                sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR;
            }
        }
        else
        {
            /* Nothing to do */
        }

        /* 6.   COUNT( * ) 면 무조건 AGGREGATION 형태이다. */
        if ( ( aNode->node.arguments == NULL ) &&
             ( aNode->node.module == &( mtfCount ) ) )
        {
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGREGATION;
        }
        else
        {
            /* Nothing to do */
        }
    }

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkAggrColumn",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkAggrColumn",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::checkExprColumn( qmsSFWGH * aSFWGH,
                                           qtcNode  * aNode,
                                           UInt       aTransType,
                                           UInt     * aFlag )
{
/****************************************************************************************
 *
 * Description : Order By Expression 대상 형태를 검사하는 함수이다.
 *               Arguement가 AGGREGATION 형태라면, AGGREGATION 같이 검사하고, 제어용
 *               형태인 GROUPKEY, NONKEY, ERROR, SKIP를 제외하면, EXPRESSION 형태로
 *               반환한다.
 *
 *               EXPRESSION 형태로 결정할 때에는 Groupping 요소를 고려하지 않는다.
 *               일단 Append 해야, 그 결과 잘못된 Query하여도, Validate 시점에 정확한
 *               Error를 반환할 수 있다.
 *
 *  VALID
 *
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY      A1   +  1;  |  SKIP
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY AVG( A1 ) +  1;  |  ERROR
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY AVG( C2 ) + C1;  |  ERROR
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY AVG( C2 ) +  1;  |  AGGREGATION
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY      C2   +  1;  |  GROUPKEY
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C2 ORDER BY      C1   +  1;  |  NONKEY
 *   SELECT      C2   A1 FROM T1 GROUP BY C2 ORDER BY      C2   +  1;  |  GROUPKEY
 *   SELECT      C2   A1 FROM T1 GROUP BY C2 ORDER BY      C1   +  1;  |  NONKEY
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY      C1   +  1;  |  EXPRESSION
 *   SELECT      C1   A1 FROM T1 GROUP BY C1 ORDER BY      C1   +  1;  |  EXPRESSION
 *   SELECT      C1   A1 FROM T1             ORDER BY      C2   +  1;  |  EXPRESSION
 *   SELECT      C1   A1 FROM T1             ORDER BY      C1   +  1;  |  EXPRESSION
 *
 *  INVALID
 *
 *   SELECT AVG( C1 ) A1 FROM T1 GROUP BY C1 ORDER BY      C2   +  1;  |  EXPRESSION
 *   SELECT      C1   A1 FROM T1 GROUP BY C1 ORDER BY      C2   +  1;  |  EXPRESSION
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY      C2   +  1;  |  EXPRESSION
 *   SELECT AVG( C1 ) A1 FROM T1             ORDER BY      C1   +  1;  |  EXPRESSION
 *
 * Implementation : 1.   Node Argument 먼저 분석한다.
 *                  2.1. SKIP 형태가 포함되면 SKIP 형태이다.
 *                  2.2. AGGREGATION 형태가 포함되는지 검사한다.
 *                  2.3. 제어용 형태가 포함되는지 검사한다.
 *                  3.1. AGGREGATION 형태가 포함되었다면, AGGREGATION 같이 처리한다.
 *                  3.2. 제어용 형태가 포함되면 ERROR 형태이다.
 *                  3.3. 그외는 AGGREGATION 형태이다.
 *                  4.1. 제어용 형태가 없다면, EXPRESSION 형태이다.
 *                  4.1. 제어용 형태를 그래도 반환한다.
 *
 ****************************************************************************************/

    qtcNode * sNode    = NULL;
    idBool    sIsAggr  = ID_FALSE;
    idBool    sIsExpr  = ID_TRUE;
    idBool    sIsError = ID_FALSE;
    UInt      sFlag    = 0;
    UInt      sKeep    = 0;

    IDE_TEST_RAISE( aNode == NULL, ERR_NULL_NODE );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /* 1.   Node Argument 먼저 분석한다. */
    for ( sNode  = (qtcNode*)aNode->node.arguments;
          sNode != NULL;
          sNode  = (qtcNode*)sNode->node.next )
    {
        IDE_TEST( checkSortNodeTree( aSFWGH,
                                     sNode,
                                     aTransType,
                                     ID_FALSE,
                                     &( sFlag ) )
                  != IDE_SUCCESS );

        /* 2.1. SKIP 형태가 포함되면 SKIP 형태이다. */
        if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
             == QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP )
        {
            sKeep &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sKeep |= QMS_TARGET_SHARD_ORDER_BY_TRANS_SKIP;

            sIsExpr  = ID_FALSE;

            break;
        }
        else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                  == QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGREGATION )
        {
            /* 2.2. AGGREGATION 형태가 포함되는지 검사한다. */
            sIsAggr = ID_TRUE;
        }
        else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                  == QMS_TARGET_SHARD_ORDER_BY_TRANS_GROUPKEY )
        {
            /* 2.3. 제어용 형태가 포함되는지 검사한다. */
            sKeep &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sKeep |= QMS_TARGET_SHARD_ORDER_BY_TRANS_GROUPKEY;

            sIsExpr = ID_FALSE;
        }
        else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                  == QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR )
        {
            /* 2.3. 제어용 형태가 포함되는지 검사한다. */
            sKeep &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sKeep |= QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR;

            sIsExpr  = ID_FALSE;
            sIsError = ID_TRUE;
        }
        else if ( ( sFlag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                  == QMS_TARGET_SHARD_ORDER_BY_TRANS_NONKEY )
        {
            /* 2.3. 제어용 형태가 포함되는지 검사한다. */
            sKeep &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sKeep |= QMS_TARGET_SHARD_ORDER_BY_TRANS_NONKEY;

            sIsExpr  = ID_FALSE;
            sIsError = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* 3.1. AGGREGATION 형태가 포함되었다면, AGGREGATION 같이 처리한다. */
    if ( sIsAggr == ID_TRUE )
    {
        /* 3.2. 제어용 형태가 포함되면 ERROR 형태이다. */
        if ( sIsError == ID_TRUE )
        {
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_ERROR;
        }
        else
        {
            /* 3.3. 그외는 AGGREGATION 형태이다. */
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGREGATION;
        }
    }
    else
    {
        /* 4.1. 제어용 형태가 없다면, EXPRESSION 형태이다. */
        if ( sIsExpr == ID_TRUE )
        {
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= QMS_TARGET_SHARD_ORDER_BY_TRANS_EXPRESSION;
        }
        else
        {
            /* 4.1. 제어용 형태를 그래도 반환한다. */
            sFlag &= ~QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK;
            sFlag |= sKeep;
        }
    }

    if ( aFlag != NULL )
    {
        *aFlag = sFlag;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkExprColumn",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::checkExprColumn",
                                  "node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::adjustTargetListAndSortNode( qcStatement  * aStatement,
                                                       qmsParseTree * aParseTree,
                                                       qmsParseTree * aViewParseTree,
                                                       UInt         * aViewTargetOrder )
{
 /****************************************************************************************
 *
 * Description : Shard View Transform 시에, Oreder By QuerySet의 Target List를 재구성하는
 *               함수이다.
 *
 *               Transfrom 형태에 따라서, Append된 Oreder By 대상이 Shard View QuerySet
 *               이나 Oreder By QuerySet에 덧붙여 있을 수 있다.
 *
 *               전자의 경우, qmvQuerySet::makeTargetListForTableRef에서 Append된
 *               대상을 제외해야도록 Flag를 남기고, 후자의 경우 Oreder By Target에
 *               Append된 대상을 제외하고 Shard View Target이 변경될 형태와 동일하게,
 *               Order By Node를 변경해야 한다.
 *
 *               Shard Col Trans 형태에 따라서, COLUMN와 EXPRESSION인 경우는 Target
 *               대상 이동으로 처리하며, AGGREGATION인 경우는 Target 대상은 제외하고,
 *               Order By Node를 변경한다. 또한 Shard View Target이 _$N_$N으로
 *               qmvShardTransform::addTargetAliasToText 호출로 분할될 수도 있어,
 *               Shard Col Trans 형태 내려주고, qmg::checkPushProject,
 *               Push Projection에서 이용한다.
 *
 *               그리고 Error인 경우에 정확한 Error를 출력하도록, Order By를 Shard View로
 *               내려준다.
 *
 *  QUERRY      / SELECT AVG( C1 ) A                   FROM T1 GROUP BY C1 ORDER BY AVG( C2 );
 *  APPEND      / SELECT AVG( C1 ) A, AVG( C2 ) AS _$2 FROM T1 GROUP BY C1 ORDER BY       _$2;
 *  SHARD VIEW  / SELECT           *                   FROM SHARD(.......) ORDER BY       _$2;
 *  TARGET LIST / SELECT           A                   FROM SHARD(.......) ORDER BY       _$2;  <-- 4.
 *
 *
 *  COLUMN & EXPRESSION
 *
 *   QUERRY      / SELECT C2                FROM T1 GROUP BY C2 ORDER BY C2 + 1;
 *   APPEND      / SELECT C2, C2 + 1 AS _$2 FROM T1 GROUP BY C2 ORDER BY    _$2;
 *   SHARD VIEW  / SELECT C2, C2 + 1 AS _$2
 *                  FROM SHARD( SELECT C2                FROM T1 GROUP BY C2 )
 *                   GROUP BY C2
 *                    ORDER BY _$2;
 *
 *   ADJUST      / SELECT C2  *************
 *                  FROM SHARD( SELECT C2, C2 + 1 AS _$2 FROM T1 GROUP BY C2 )
 *                   GROUP BY C2           ************* MOVE                   <-- 5.2.
 *                    ORDER BY _$2;
 *
 *  AGGREGATION
 *
 *   QUERRY      / SELECT AVG( C1 ) A                   FROM T1 GROUP BY C2 ORDER BY AVG( C2 );
 *   APPEND      / SELECT AVG( C1 ) A, AVG( C2 ) AS _$2 FROM T1 GROUP BY C2 ORDER BY       _$2;
 *   SHARD VIEW  / SELECT AVG( C1 ) A, AVG( C2 ) AS _$2
 *                  FROM SHARD( SELECT C2, SUM( C1 ) AS     A, SUM( C1 ) AS     B,
 *                                         SUM( C2 ) AS _$2_1, SUM( C2 ) AS _$2_2,
 *                               FROM T1
 *                                GROUP BY C2 )
 *                   GROUP BY C2
 *                    ORDER BY _$2;
 *
 *   ADJUST      / SELECT AVG( C1 ) A  **************** REMOVE
 *                  FROM SHARD( SELECT C2, SUM( C1 ) AS     A, SUM( C1 ) AS     B,
 *                                         SUM( C2 ) AS _$2_1, SUM( C2 ) AS _$2_2,
 *                               FROM T1
 *                                GROUP BY C2 )
 *                   GROUP BY C2
 *                    ORDER BY SUM( _$2_$1 ) / SUM( _$2_$2 );
 *                             ***************************** ADJUST  <-- 5.3.
 *
 * Implementation : 1.   Set 연산인지 검사한다.
 *                  2.   Shard View QuerySet에 Append 되었는지 검사한다.
 *                  3.   Error인 경우 Order By를 Shard View로 내려준다.
 *                  4.   Append된 대상을 제외해야도록 Flag를 저장한다.
 *                  5.1. Append된 Shard Col Trans 대상을 찾는다.
 *                  5.2. COLUMN과 EXPRESSION 형태라면, Shard View Target으로 옮긴다.
 *                  5.3. AGGREGATION 형태라면, Order By Node를 변경한다.
 *                  5.4. 관련된 Shard View Target에 Shard Col Trans 형태를 남긴다.
 *
 ****************************************************************************************/

    qmsSortColumns * sCurrSort    = NULL;
    qmsSFWGH       * sSFWGH       = NULL;
    qmsSFWGH       * sViewSFWGH   = NULL;
    qmsTarget      * sTarget      = NULL;
    qmsTarget      * sTargetTail  = NULL;
    qmsTarget      * sTargetPrev  = NULL;
    qmsTarget      * sViewTarget  = NULL;
    qtcNode        * sTargetNode  = NULL;
    UInt             sTargetOrder = 0;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aViewParseTree == NULL, ERR_NULL_VIEW_PARSE_TREE );

    /* 1.   Set 연산인지 검사한다. */
    IDE_TEST_CONT( aParseTree->querySet->setOp != QMS_NONE, NORMAL_EXIT );
    IDE_TEST_CONT( aViewParseTree->querySet->setOp != QMS_NONE, NORMAL_EXIT );

    sSFWGH     = aParseTree->querySet->SFWGH;
    sViewSFWGH = aViewParseTree->querySet->SFWGH;

    IDE_TEST_RAISE( sSFWGH == NULL, ERR_NULL_SFWGH );
    IDE_TEST_RAISE( sViewSFWGH == NULL, ERR_NULL_VIEW_SFWGH );

    /* 2.   Shard View QuerySet에 Append 되었는지 검사한다. */
    if ( sSFWGH->target == NULL )
    {
        /* 3.   Error인 경우 Order By를 Shard View로 내려준다. */
        if ( ( sViewSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
             == QMV_SFWGH_SHARD_ORDER_BY_TRANS_ERROR )
        {
            aViewParseTree->orderBy = aParseTree->orderBy;
            aParseTree->orderBy     = NULL;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            IDE_TEST_CONT( ( sViewSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
                           != QMV_SFWGH_SHARD_ORDER_BY_TRANS_TRUE,
                           NORMAL_EXIT );
        }

        /* 4.   Append된 대상을 제외해야도록 Flag를 저장한다. */
        sSFWGH->lflag |= sViewSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK;

        sViewSFWGH->lflag &= ~QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK;
    }
    else
    {
        /* 3.   Error인 경우 Order By를 Shard View로 내려준다. */
        if ( ( sSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
             == QMV_SFWGH_SHARD_ORDER_BY_TRANS_ERROR )
        {
            aViewParseTree->orderBy = aParseTree->orderBy;
            aParseTree->orderBy     = NULL;

            IDE_CONT( NORMAL_EXIT );
        }
        else
        {
            IDE_TEST_CONT( ( sSFWGH->lflag & QMV_SFWGH_SHARD_ORDER_BY_TRANS_MASK )
                           != QMV_SFWGH_SHARD_ORDER_BY_TRANS_TRUE,
                           NORMAL_EXIT );
        }

        /* 5.1. Append된 Shard Col Trans 대상을 찾는다. */
        for ( sTarget  = sSFWGH->target;
              sTarget != NULL;
              sTarget  = sTarget->next )
        {
            sTargetNode = sTarget->targetColumn;

            /* 5.2. COLUMN과 EXPRESSION 형태라면, Shard View Target으로 옮긴다. */
            if ( ( ( sTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                   == QMS_TARGET_SHARD_ORDER_BY_TRANS_COLUMN )
                 ||
                 ( ( sTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                   == QMS_TARGET_SHARD_ORDER_BY_TRANS_EXPRESSION ) )
            {
                for ( sViewTarget  = sViewSFWGH->target;
                      sViewTarget != NULL;
                      sViewTarget  = sViewTarget->next )
                {
                    sTargetTail = sViewTarget;
                }

                sTargetTail->next = sTarget;

                sTargetPrev->next = sTarget->next;

                sTargetOrder++;
            }
            else if ( ( sTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK )
                      == QMS_TARGET_SHARD_ORDER_BY_TRANS_AGGREGATION )
            {
                /* 5.3. AGGREGATION 형태라면, Order By Node를 변경한다. */
                IDE_TEST( getShardColTransSort( sTarget,
                                                aParseTree->orderBy,
                                                &( sCurrSort ) )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sCurrSort == NULL, ERR_NOT_FOUND );

                IDE_TEST( modifySortNode( aStatement,
                                          sTarget,
                                          sCurrSort->sortColumn,
                                          &( sTargetNode ),
                                          &( sTargetOrder ) )
                          != IDE_SUCCESS );

                sCurrSort->sortColumn = sTargetNode;

                /* 5.4. 관련된 Shard View Target에 Shard Col Trans 형태를 남긴다. */
                IDE_TEST( pushShardColTransFlag( sTarget,
                                                 sViewSFWGH )
                          != IDE_SUCCESS );

                sTargetPrev->next = sTarget->next;
            }
            else
            {
                sTargetPrev = sTarget;
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    if ( aViewTargetOrder != NULL )
    {
        *aViewTargetOrder += sTargetOrder;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::adjustTargetListAndSortNode",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_VIEW_PARSE_TREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::adjustTargetListAndSortNode",
                                  "view parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::adjustTargetListAndSortNode",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_VIEW_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::adjustTargetListAndSortNode",
                                  "view sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::adjustTargetListAndSortNode",
                                  "not found sort node" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::getShardColTransSort( qmsTarget       * aTarget,
                                                qmsSortColumns  * aOrderBy,
                                                qmsSortColumns ** aSortNode )
{
/****************************************************************************************
 *
 * Description : Adjust할 Order By Node를 검색해서 반환하는 함수이다.
 *
 *  SELECT C2, C2 + 1 AS _$2 FROM
 *   SHARD( SELECT C2     /  FROM T1 GROUP BY C2 )
 *    GROUP BY C2.       /
 *     ORDER BY _$2;    /
 *                \____/
 *
 * Implementation :
 *
 ****************************************************************************************/

    qmsSortColumns * sCurrSort = NULL;

    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_TARGET );
    IDE_TEST_RAISE( aOrderBy == NULL, ERR_NULL_ORDER_BY );

    for ( sCurrSort  = aOrderBy;
          sCurrSort != NULL;
          sCurrSort  = sCurrSort->next )
    {
        if ( QC_IS_NULL_NAME( sCurrSort->sortColumn->columnName ) != ID_TRUE )
        {
            if ( QC_IS_NAME_MATCHED_POS_N_TARGET( sCurrSort->sortColumn->columnName,
                                                  aTarget->aliasColumnName )
                 != ID_TRUE )
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

    if ( aSortNode != NULL )
    {
        *aSortNode = sCurrSort;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::getShardColTransSort",
                                  "target NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ORDER_BY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::getShardColTransSort",
                                  "order by NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::modifySortNode( qcStatement * aStatement,
                                          qmsTarget   * aTarget,
                                          qtcNode     * aSortNode,
                                          qtcNode    ** aNode,
                                          UInt        * aViewTargetOrder )
{
/****************************************************************************************
 *
 * Description : Order By 대상에 Aggregation을 분할하는 경우를 지원하기 위해서,
 *               Aggregation을 검색하는 함수이다. qmvShardTransform::modifyOrgAggr 를
 *               참조하였다.
 *
 * Implementation : 
 *
 ****************************************************************************************/

    qtcNode ** sNode = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_TARGET );
    IDE_TEST_RAISE( aSortNode == NULL, ERR_NULL_SORT_NODE );

    if ( QTC_IS_AGGREGATE( *aNode ) == ID_TRUE )
    {
        IDE_TEST( changeSortExpr( aStatement,
                                  aTarget,
                                  aSortNode,
                                  aNode,
                                  aViewTargetOrder )
                  != IDE_SUCCESS );
    }
    else
    {
        for ( sNode   = (qtcNode **)&( *aNode )->node.arguments;
              *sNode != NULL;
              sNode   = (qtcNode **)&( *sNode )->node.next )
        {
            IDE_TEST( modifySortNode( aStatement,
                                      aTarget,
                                      aSortNode,
                                      sNode,
                                      aViewTargetOrder )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::modifySortNode",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::modifySortNode",
                                  "target is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SORT_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::modifySortNode",
                                  "sort node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::changeSortExpr( qcStatement * aStatement,
                                          qmsTarget   * aTarget,
                                          qtcNode     * aSortNode,
                                          qtcNode    ** aNode,
                                          UInt        * aViewTargetOrder )
{
/****************************************************************************************
 *
 * Description : Order By 대상을 분할되는 Aggregation인 Shard View Target에 맞춰서 변경한다.
 *               qmvShardTransform::changeAggrExpr 를 참조하였다.
 *
 *  TARGET                             |  ORDER BY
 *                                     |
 *   SUM/MIN/MAX( C1 ) AS SHARD$COL$3  |   SUM/MIN/MAX( SHARD$COL$3 )
 *   COUNT( C1 )       AS SHARD$COL$3  |   SUM( SHARD$COL$3 )
 *   AVG( C1 )         AS SHARD$COL$3  |   SUM( SHARD$COL$3$1 ) / SUM( SHARD$COL$3$2 )
 *
 * Implementation : 1.   SUM/MIN/MAX 는 Arguemnt만 Order By Node로 바꿔친다.
 *                  2.1. COUNT는 SUM으로 한번 감싼다.
 *                  2.2. SUM의 Arguemnt를 Order By Node로 연결한다.
 *                  3.1. AVG는 DIVDE로 한번 감싼다.
 *                  3.2. _$N_$1을 참조하는 SUM1을 만든다.
 *                  3.3. _$N_$2을 참조하는 SUM2을 만든다.
 *                  3.2. DIVDE에 SUM1, SUM2를 연결한다.
 *
 ****************************************************************************************/

    qtcNode      * sOrgNode     = NULL;
    qtcNode      * sSortNode[2] = { NULL, NULL };
    qtcNode      * sSumNode1[2] = { NULL, NULL };
    qtcNode      * sSumNode2[2] = { NULL, NULL };
    qtcNode      * sColNode1    = NULL;
    qtcNode      * sColNode2    = NULL;
    UShort         sTargetPos   = 1;
    qcNamePosition sPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_TARGET );
    IDE_TEST_RAISE( aSortNode == NULL, ERR_NULL_SORT_NODE );

    sOrgNode = *aNode;

    /* 1.   SUM/MIN/MAX 는 Arguemnt만 Order By Node로 바꿔친다. */
    if ( ( sOrgNode->node.module == &( mtfSum ) ) ||
         ( sOrgNode->node.module == &( mtfMin ) ) ||
         ( sOrgNode->node.module == &( mtfMax ) ) )
    {
        /* BEFORE
         *
         *  TARGET                      |  ORDER BY
         *                              |
         *    AGGR_FUNC AS SHARD$COL$3  |   SHARD$COL$3
         *        |                     |
         *   expression                 |
         *        |                     |
         *
         * AFTER
         *
         *  TARGET  |  ORDER BY
         *          |
         *          |    AGGR_FUNC
         *          |        |
         *          |   SHARD$COL$3
         */
        sOrgNode->node.arguments = &( aSortNode->node );

        sOrgNode->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sOrgNode->node.lflag |= 1;

        *aNode = sOrgNode;
        (*aViewTargetOrder)++;
    }
    else if ( sOrgNode->node.module == &( mtfCount ) )
    {
        /* BEFORE
         *
         *  TARGET                     |  ORDER BY
         *                             |
         *     COUNT() AS SHARD$COL$3  |   SHARD$COL$3
         *        |                    |
         *   expression                |
         *        |                    |
         *
         * AFTER
         *
         *  TARGET  |  ORDER BY
         *          |
         *          |      SUM()
         *          |        |
         *          |   SHARD$COL$3
         */
        SET_EMPTY_POSITION( sPosition );

        /* 2.1. COUNT는 SUM으로 한번 감싼다. */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sSortNode,
                                 &( sPosition ),
                                 &( mtfSum ) )
                  != IDE_SUCCESS );

        /* 2.2. SUM의 Arguemnt를 Order By Node로 연결한다. */
        sSortNode[0]->node.next = sOrgNode->node.next;
        sSortNode[0]->node.arguments = &( aSortNode->node );

        sSortNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSortNode[0]->node.lflag |= 1;

        SET_POSITION( sSortNode[0]->position, sOrgNode->position );
        SET_POSITION( sSortNode[0]->userName, sOrgNode->userName );
        SET_POSITION( sSortNode[0]->tableName, sOrgNode->tableName );
        SET_POSITION( sSortNode[0]->columnName, sOrgNode->columnName );

        *aNode = sSortNode[0];
        (*aViewTargetOrder)++;
    }
    else if ( sOrgNode->node.module == &( mtfAvg ) )
    {
        /* BEFORE
         *
         *  TARGET                    |  ORDER BY
         *                            |
         *      AVG() AS SHARD$COL$3  |   SHARD$COL$3
         *        |                   |
         *   expression               |
         *        |                   |
         *
         * AFTER
         *
         *  TARGET  |  ORDER BY
         *          |
         *          |      DIVIDE()
         *          |        |
         *          |       SUM()---------- SUM()
         *          |        |               |
         *          |   SHARD$COL$3$1   SHARD$COL$3$2
         */
        SET_EMPTY_POSITION( sPosition );

        /* 3.1. AVG는 DIVDE로 한번 감싼다. */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sSortNode,
                                 &( sPosition ),
                                 &( mtfDivide ) )
                  != IDE_SUCCESS );

        /* 3.2. _$N_$1을 참조하는 SUM1을 만든다. */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sSumNode1,
                                 &( sPosition ),
                                 &( mtfSum ) )
                  != IDE_SUCCESS );

        IDE_TEST( makeShardColTransNode( aStatement,
                                         aTarget,
                                         sTargetPos++,
                                         &( sColNode1 ) )
                  != IDE_SUCCESS );

        sSumNode1[0]->node.arguments = &( sColNode1->node );

        sSumNode1[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSumNode1[0]->node.lflag |= 1;

        /* 3.3. _$N_$2을 참조하는 SUM2을 만든다. */
        IDE_TEST( qtc::makeNode( aStatement,
                                 sSumNode2,
                                 &( sPosition ),
                                 &( mtfSum ) )
                  != IDE_SUCCESS );

        IDE_TEST( makeShardColTransNode( aStatement,
                                         aTarget,
                                         sTargetPos++,
                                         &( sColNode2 ) )
                  != IDE_SUCCESS );

        sSumNode2[0]->node.arguments = &( sColNode2->node );

        sSumNode2[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSumNode2[0]->node.lflag |= 1;

        /* 3.4. DIVDE에 SUM1, SUM2를 연결한다. */
        sSortNode[0]->node.next = sOrgNode->node.next;
        sSortNode[0]->node.arguments = &( sSumNode1[0]->node );
        sSortNode[0]->node.arguments->next = &( sSumNode2[0]->node );

        sSortNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        sSortNode[0]->node.lflag |= 2;

        SET_POSITION( sSortNode[0]->position, sOrgNode->position );
        SET_POSITION( sSortNode[0]->userName, sOrgNode->userName );
        SET_POSITION( sSortNode[0]->tableName, sOrgNode->tableName );
        SET_POSITION( sSortNode[0]->columnName, sOrgNode->columnName );

        *aNode = sSortNode[0];
        (*aViewTargetOrder)++;
        (*aViewTargetOrder)++;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::changeSortExpr",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::changeSortExpr",
                                  "target is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SORT_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::changeSortExpr",
                                  "sort node is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::pushShardColTransFlag( qmsTarget * aTarget,
                                                 qmsSFWGH  * aSFWGH )
{
/****************************************************************************************
 *
 * Description : Shard View Target이 _$N_$N으로 분할되었을 때에, Shard Col Trans 형태를
 *               내려준다. _$N 까지만 비교해서 내려준다.
 *
 *  SELECT AVG( C1 ) A, AVG( C2 ) AS _$2 FROM
 *   SHARD( SELECT C2, SUM( C1 ) AS     A, SUM( C1 ) AS     B,
 *                     SUM( C2 ) AS _$2_1, SUM( C2 ) AS _$2_2,
 *           FROM T1 GROUP BY C2 ).  /                   /
 *    GROUP BY C2                   /                   /
 *     ORDER BY _$2;               /                   /
 *                \_______________/___________________/
 *
 * Implementation :
 *
 ****************************************************************************************/

    qmsTarget  * sTarget = NULL;

    IDE_TEST_RAISE( aTarget == NULL, ERR_NULL_TARGET );
    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );
    IDE_TEST_RAISE( QC_IS_NULL_NAME( aTarget->aliasColumnName ) == ID_TRUE, ERR_NULL_ALIAS )

    for ( sTarget  = aSFWGH->target;
          sTarget != NULL;
          sTarget  = sTarget->next )
    {
        if ( QC_IS_NULL_NAME( sTarget->aliasColumnName ) != ID_TRUE )
        {
            if ( idlOS::strMatch( sTarget->aliasColumnName.name,
                                  aTarget->aliasColumnName.size,
                                  aTarget->aliasColumnName.name,
                                  aTarget->aliasColumnName.size ) == 0 )
            {
                sTarget->flag |= ( aTarget->flag & QMS_TARGET_SHARD_ORDER_BY_TRANS_MASK );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_TARGET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::pushShardColTransFlag",
                                  "target is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::pushShardColTransFlag",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_ALIAS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::pushShardColTransFlag",
                                  "alias is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-7219 Shard Transformer Refactoring */
IDE_RC qmvShardTransform::doShardAnalyze( qcStatement     * aStatement,
                                          qcNamePosition  * aStmtPos,
                                          ULong             aSMN,
                                          qmsQuerySet     * aQuerySet )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                qmvShardTransform::isShardQuery 수정 함수 1
 *
 *                 일단 Analysis 를 생성하였다면, 이후에 호출되어도 Analyze 하지 않는다.
 *                  다만 Optimize 이후에 특정 QuerySet 만 재수행할 수 있도록
 *                   SDI_QUERYSET_LIST_STATE_OPTIMIZE 단계를 지원한다.
 *
 * Implementation : 1. Transform 된 Query 인 경우, String 이 없어 분석할 수 없으므로 무시한다.
 *                  2. Analysis 없는 경우에만 Analyze 를 수행한다.
 *                  3. SDI_QUERYSET_LIST_STATE_OPTIMIZE 단계라면 해당 aQuerySet 만 Analyze 를 수행한다.
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aStatement->mShardQuerySetList == NULL, ERR_NULL_LIST );
    IDE_TEST_RAISE( aStmtPos == NULL, ERR_NULL_POSITION );

    /* 1. Transform 된 Query 인 경우, String 이 없어 분석할 수 없으므로 무시한다. */
    IDE_TEST_CONT( QC_IS_NULL_NAME( (*aStmtPos) ) == ID_TRUE, NORMAL_EXIT );

    /* 2. Analysis 없는 경우에만 Analyze 를 수행한다. */
    if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                        SDI_QUERYSET_LIST_STATE_MAIN_ALL )
         == ID_TRUE )
    {
        IDE_TEST( doShardAnalyzeForStatement( aStatement,
                                              aSMN,
                                              aStmtPos,
                                              NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 3. SDI_QUERYSET_LIST_STATE_OPTIMIZE 단계라면 해당 aQuerySet 만 Analyze 를 수행한다. */
        if ( SDI_CHECK_QUERYSET_LIST_STATE( aStatement->mShardQuerySetList,
                                            SDI_QUERYSET_LIST_STATE_MAIN_PARTIAL )
             == ID_TRUE )
        {
            IDE_TEST( doShardAnalyzeForStatement( aStatement,
                                                  aSMN,
                                                  aStmtPos,
                                                  aQuerySet )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::doShardAnalyze",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_LIST )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::doShardAnalyze",
                                  "list is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::doShardAnalyze",
                                  "position is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::doShardAnalyzeForStatement( qcStatement    * aStatement,
                                                      ULong            aSMN,
                                                      qcNamePosition * aStmtPos,
                                                      qmsQuerySet    * aQuerySet )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                qmvShardTransform::isShardQuery 수정 함수 2
 *
 *                 분석용 Dummy Statement 를 생성하고, 전달받은 Query String 를 연결하고,
 *                  Prepare - P.V.O. - Shard Analyze - Make Analysis 순으로 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    /* BUG-45994 - 컴파일러 최적화 회피 */
    volatile qcStatement sStatement;
    volatile SInt        sStage;
    UShort               sParamCount = 0;
    UShort               sIdx        = 0;

    IDE_FT_BEGIN();

    sStage = 0;

    //-----------------------------------------
    // PREPARE
    //-----------------------------------------

    /* Resolve 를 위해 aStatement 의 Session 이 필요 */
    IDE_TEST( qcg::allocStatement( (qcStatement *)&( sStatement ),
                                   aStatement->session,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    sStage = 1;

    /* stmtText 지정 */
    sStatement.myPlan->stmtText    = aStmtPos->stmtText;
    sStatement.myPlan->stmtTextLen = idlOS::strlen( aStmtPos->stmtText );

    qcg::setSmiStmt( (qcStatement *)&( sStatement ), QC_SMI_STMT( aStatement ) );

    /* TASK-7219 Non-shard DML */
    sStatement.mShardPartialExecType = aStatement->mShardPartialExecType;

    /* Bind 의 Shard 구분을 위한 Offset 설정 */
    IDE_TEST( qmg::setHostVarOffset( aStatement ) != IDE_SUCCESS );

    //-----------------------------------------
    // PARSING
    //-----------------------------------------

    IDE_TEST( qcpManager::parsePartialForAnalyze( (qcStatement *)&( sStatement ),
                                                  aStmtPos->stmtText,
                                                  aStmtPos->offset,
                                                  aStmtPos->size )
              != IDE_SUCCESS );

    //-----------------------------------------
    // BIND
    //-----------------------------------------

    /* PSM SUPPORT */
    sStatement.calledByPSMFlag = aStatement->calledByPSMFlag;

    /* UNDEF INITIALIZE */
    sParamCount = qcg::getBindCount( (qcStatement *)&( sStatement ) );

    for ( sIdx = 0;
          sIdx < sParamCount;
          sIdx++ )
    {
        IDE_TEST( qcg::setBindColumn( (qcStatement *)&( sStatement ),
                                      sIdx,
                                      MTD_UNDEF_ID,
                                      0,
                                      0,
                                      0 )
                  != IDE_SUCCESS );
    }

    //-----------------------------------------
    // P.V.O
    //-----------------------------------------

    SDI_SET_QUERYSET_LIST_STATE( ( (qcStatement *)&( sStatement ) )->mShardQuerySetList,
                                 SDI_QUERYSET_LIST_STATE_DUMMY_MAKE );

    IDE_TEST( sStatement.myPlan->parseTree->parse( (qcStatement *)&( sStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterParsing( QC_SHARED_TMPLATE( (qcStatement *)&( sStatement ) ) )
              != IDE_SUCCESS );

    sStage = 2;

    IDE_TEST( sStatement.myPlan->parseTree->validate( (qcStatement *)&( sStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( qtc::fixAfterValidation( QC_QMP_MEM( (qcStatement *)&( sStatement ) ),
                                       QC_SHARED_TMPLATE( (qcStatement *)&( sStatement ) ) )
              != IDE_SUCCESS );

    //-----------------------------------------
    // ANALYZE
    //-----------------------------------------

    SDI_SET_QUERYSET_LIST_STATE( ( (qcStatement *)&( sStatement ) )->mShardQuerySetList,
                                 SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE );

    IDE_TEST( sdi::analyze( (qcStatement *)&( sStatement ),
                            aSMN )
              != IDE_SUCCESS );

    /* Bind 의 Shard 구분을 위한 Offset 설정 원복 */
    IDE_TEST( qmg::setHostVarOffset( aStatement ) != IDE_SUCCESS );

    //-----------------------------------------
    // RESULT
    //-----------------------------------------

    /* BUG-45899 */
    QC_SHARED_TMPLATE( aStatement )->stmt->mShardPrintInfo.mAnalyzeCount
        += sStatement.mShardPrintInfo.mAnalyzeCount;

    IDE_TEST( sdi::makeAndSetAnalysis( (qcStatement *)&( sStatement ),
                                       aStatement,
                                       aQuerySet )
              != IDE_SUCCESS );

    //-----------------------------------------
    // FINISH
    //-----------------------------------------

    sStage = 1;

    if( sStatement.spvEnv->latched == ID_TRUE )
    {
        IDE_TEST( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                  != IDE_SUCCESS );

        sStatement.spvEnv->latched = ID_FALSE;
    }
    else
    {
        /* Nothing To Do */
    }

    sStage = 0;

    /* session은 내것이 아니다. */
    sStatement.session = NULL;

    (void) qcg::freeStatement( (qcStatement *)&( sStatement ) );

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL() /* PROJ-2617 */
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    switch ( sStage )
    {
        case 2:
            if ( qsxRelatedProc::unlatchObjects( sStatement.spvEnv->procPlanList )
                 == IDE_SUCCESS )
            {
                sStatement.spvEnv->latched = ID_FALSE;
            }
            else
            {
                IDE_ERRLOG( IDE_QP_1 );
            }

        case 1:
            sStatement.session = NULL;

            (void) qcg::freeStatement( (qcStatement *)&( sStatement ) );

        default:
            break;
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForConvert( qcStatement * aStatement,
                                               qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View 변환 함수
 *
 *                 중복 코드 사용성 향상 목적으로 구현하였다.
 *                  일반 View 를 Shard View 로 변환한다.
 *
 *                   select * from ( select * from t1 ) order by 1;
 *                                 ********************
 *
 *                    -> select * from shard( select * from t1 ) order by 1
 *                                     *************************
 *
 *                     insert into t1 values ( 1, 1 );
 *                     *******************************
 *
 *                      -> shard insert into t1 values ( 1, 1 );
 *                        *************************************
 *
 * Implementation : 1. Shard Analyze Info 를 생성한다.
 *                  2. Shard Param Offset Array 를 구성한다.
 *                  3. myPlan 의 Statement 관련 정보를 설정한다.
 *                  4. myPlan 의 Shard 관련 정보를 설정한다.
 *
 ***********************************************************************/

    qcShardStmtType    sStmtType    = QC_STMT_SHARD_NONE;
    qcNamePosition   * sStmtPos     = NULL;
    sdiAnalyzeInfo   * sAnalyzeInfo = NULL;
    qcShardParamInfo * sParamInfo   = NULL;
    UShort             sParamCount  = 0;
    UShort             sParamOffset = ID_USHORT_MAX;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtType = aParseTree->stmtShard;
    sStmtPos  = &( aParseTree->stmtPos );

    /* 1. Shard Analyze Info 를 생성한다. */
    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromParseTree( aStatement,
                                                       aParseTree,
                                                       &( sAnalyzeInfo ) )
              != IDE_SUCCESS );

    /* 2. Shard Param Offset Array 를 구성한다. */
    IDE_TEST( qmg::makeShardParamOffsetArray( aStatement,
                                              sStmtPos,
                                              &( sParamCount ),
                                              &( sParamOffset ),
                                              &( sParamInfo ) )
              != IDE_SUCCESS );

    /* 3. myPlan 의 Statement 관련 정보를 설정한다. */
    IDE_TEST( sdi::setShardPlanStmtVariable( aStatement,
                                             sStmtType,
                                             sStmtPos )
              != IDE_SUCCESS );

    /* 4. myPlan 의 Shard 관련 정보를 설정한다. */
    IDE_TEST( sdi::setShardPlanCommVariable( aStatement,
                                             sAnalyzeInfo,
                                             sParamCount,
                                             sParamOffset,
                                             sParamInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForConvert",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForStatement( qcStatement * aStatement,
                                                 qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View 변환 함수
 *
 *                 중복 코드 사용성 향상 목적으로 구현하였다.
 *                  Statement 전체를 Shard View 로 생성한다.
 *
 *                   select i1, i2 from t1 where i1=1;
 *                   ********************************
 *
 *                    -> select * from shard( select i1, i2 from t1 where i1=1 );
 *                                     *****************************************
 *
 *                    aStatement-parseTree
 *                     -> aStatement-PARSETREE'-QUERYSET'-SFWGH'-FROM'-TABLEREF'-STATEMENT'-parseTree
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement      * sStatement   = NULL;
    qmsParseTree     * sParseTree   = NULL;
    qmsQuerySet      * sQuerySet    = NULL;
    qmsSFWGH         * sSFWGH       = NULL;
    qmsFrom          * sFrom        = NULL;
    qmsTableRef      * sTableRef    = NULL;
    sdiAnalyzeInfo   * sAnalyzeInfo = NULL;
    qcShardParamInfo * sParamInfo   = NULL;
    qcShardStmtType    sStmtType    = QC_STMT_SHARD_NONE;
    qcNamePosition   * sStmtPos     = NULL;
    UShort             sParamCount  = 0;
    UShort             sParamOffset = ID_USHORT_MAX;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtType = aParseTree->stmtShard;
    sStmtPos  = &( aParseTree->stmtPos );

    /* 필요한 구조체를 생성한다. */
    IDE_TEST( alllocQueryStruct( aStatement,
                                 &( sStatement ),
                                 &( sParseTree ),
                                 &( sQuerySet ),
                                 &( sSFWGH ),
                                 &( sFrom ),
                                 &( sTableRef ) )
              != IDE_SUCCESS );

    /* aStatement 를 교체할 수 없으므로 sStatement 를 복사 생성한다. */
    idlOS::memcpy( sStatement, aStatement, ID_SIZEOF( qcStatement ) );

    /* myPlan 을 재설정한다. */
    sStatement->myPlan = &( sStatement->privatePlan );

    /* sStatement 를 연결한다. */
    sTableRef->view      = sStatement;
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    /* sQuerySet 를 연결한다. */
    SDI_SET_PARSETREE( sParseTree, sQuerySet );

    /* sParseTree 를 연결한다. */
    aStatement->myPlan->parseTree = (qcParseTree *)( sParseTree );

    aStatement->myPlan->parseTree->stmtKind =
        sStatement->myPlan->parseTree->stmtKind;

    /* sStatement 를 Shard View 로 변경한다. */
    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromParseTree( aStatement,
                                                       aParseTree,
                                                       &( sAnalyzeInfo ) )
              != IDE_SUCCESS );

    /* BUG-49209 */
    if ( aParseTree->stmtKind == QCI_STMT_SELECT_FOR_UPDATE )
    {
        sAnalyzeInfo->mTopQueryFlag[ SDI_TQ_FOR_UPDATE_EXISTS ] = ID_TRUE;
    }
    else
    {
        /* Nothing to do. */
    }

    IDE_TEST( qmg::makeShardParamOffsetArray( aStatement,
                                              sStmtPos,
                                              &( sParamCount ),
                                              &( sParamOffset ),
                                              &( sParamInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanStmtVariable( sStatement,
                                             sStmtType,
                                             sStmtPos )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanCommVariable( sStatement,
                                             sAnalyzeInfo,
                                             sParamCount,
                                             sParamOffset,
                                             sParamInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForStatement",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForQuerySet( qcStatement    * aStatement,
                                                qcNamePosition * aStmtPos,
                                                qmsQuerySet    * aQuerySet,
                                                qcParseTree    * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View 변환 함수
 *
 *                 중복 코드 사용성 향상 목적으로 구현하였다.
 *                  QuerySet 을 Shard View 로 생성한다.
 *
 *                   select i1, i2 from t1 where i1=1 order by i1;
 *                   ********************************
 *
 *                    -> select * from shard( select i1, i2 from t1 where i1=1 ) order by i1;
 *                                     *****************************************
 *
 *                     aStatement-parseTree-querySet-sfwgh
 *                                    |
 *                                 orderBy
 *
 *                      -> aStatement-parseTree-querySet-SFWGH'-FROM'-TABLEREF'-STATEMENT'-PARSETREE'-QUERYSET'-sfwgh
 *                                        |
 *                                     orderBy
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement      * sStatement    = NULL;
    qmsParseTree     * sOldParseTree = NULL;
    qmsParseTree     * sNewParseTree = NULL;
    qmsQuerySet      * sQuerySet     = NULL;
    qmsSFWGH         * sSFWGH        = NULL;
    qmsFrom          * sFrom         = NULL;
    qmsTableRef      * sTableRef     = NULL;
    sdiAnalyzeInfo   * sAnalyzeInfo  = NULL;
    qcShardParamInfo * sParamInfo    = NULL;
    qcShardStmtType    sStmtType     = QC_STMT_SHARD_NONE;
    UShort             sParamCount   = 0;
    UShort             sParamOffset  = ID_USHORT_MAX;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    sOldParseTree = (qmsParseTree *)( aParseTree );
    sStmtType     = aParseTree->stmtShard;

    IDE_TEST_RAISE( sStmtType != QC_STMT_SHARD_NONE, ERR_INVALID_STMT_TYPE );

    /* 필요한 구조체를 생성한다. */
    IDE_TEST( alllocQueryStruct( aStatement,
                                 &( sStatement ),
                                 &( sNewParseTree ),
                                 &( sQuerySet ),
                                 &( sSFWGH ),
                                 &( sFrom ),
                                 &( sTableRef ) )
              != IDE_SUCCESS );

    /* aQuerySet 를 교체할 수 없으므로 sQuerySet 를 복사 생성한다. */
    idlOS::memcpy( sQuerySet, aQuerySet, ID_SIZEOF( qmsQuerySet ) );

    QCP_SET_RESET_QMS_QUERY_SET( aQuerySet );

    /* sQuerySet 를 연결한다. */
    SDI_SET_PARSETREE( sNewParseTree, sQuerySet );

    /* sParseTree 를 연결한다. */
    QC_SET_STATEMENT( sStatement, aStatement, sNewParseTree );

    sStatement->myPlan->parseTree->stmtKind =
        aStatement->myPlan->parseTree->stmtKind;

    /* sStatement 를 연결한다. */
    sTableRef->view      = sStatement;
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = aQuerySet;

    /* aQuerySet 에 연결한다. */
    aQuerySet->SFWGH = sSFWGH;

    /* sStatement 를 Shard View 로 변경한다. */
    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromQuerySet( aStatement,
                                                      aQuerySet,
                                                      &( sAnalyzeInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( qmg::makeShardParamOffsetArray( aStatement,
                                              aStmtPos,
                                              &( sParamCount ),
                                              &( sParamOffset ),
                                              &( sParamInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanStmtVariable( sStatement,
                                             sStmtType,
                                             aStmtPos )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanCommVariable( sStatement,
                                             sAnalyzeInfo,
                                             sParamCount,
                                             sParamOffset,
                                             sParamInfo )
              != IDE_SUCCESS );

    /* Param 조정 후, Order by Transform 을 수행한다. */
    IDE_TEST( adjustTargetListAndSortNode( aStatement,
                                           sOldParseTree,
                                           sNewParseTree,
                                           NULL )
               != IDE_SUCCESS );

    sSFWGH->lflag &= ~QMV_SFWGH_SHARD_TRANS_VIEW_MASK;
    sSFWGH->lflag |= QMV_SFWGH_SHARD_TRANS_VIEW_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForQuerySet",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForQuerySet",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForQuerySet",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForFrom( qcStatement    * aStatement,
                                            qcShardStmtType  aStmtType,
                                            qmsTableRef    * aTableRef,
                                            sdiObjectInfo  * aShardObjInfo )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View 변환 함수
 *
*                  qmvShardTransform::makeShrdView 함수 분리 1
 *                  Table 을 Shard View 로 생성한다.
 *
 *                   select * from sys.t1, t2 where t1.i2=t2.i1;
 *                                 ******
 *
 *                    -> select * from shard( select * from sys.t1 ) t1, t2 where t1.i2=t2.i1;
 *                                     ********************************
 *
 *                     select * from t1 a, t2 where a.i2=t2.i1;
 *                                   ****
 *
 *                      -> select * from shard( select * from t1 a ) a, t2 where a.i2=t2.i1;
 *                                       *****************************
 *
 *                     aStatement-parseTree-querySet-sfwgh-from-tableRef
 *
 *                      -> aStatement-parseTree-querySet-sfwgh-from-tableRef-
 *                          STATEMENT'-PARSETREE'-QUERYSET'-SFWGH'-FROM'-TABLEREF'
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement    * sStatement   = NULL;
    qmsParseTree   * sParseTree   = NULL;
    qmsQuerySet    * sQuerySet    = NULL;
    qmsSFWGH       * sSFWGH       = NULL;
    qmsFrom        * sFrom        = NULL;
    qmsTableRef    * sTableRef    = NULL;
    sdiAnalyzeInfo * sAnalyzeInfo = NULL;
    qcNamePosition * sStmtPos     = NULL;
    qcNamePosition   sQuery;

    IDE_TEST_RAISE( aStmtType != QC_STMT_SHARD_NONE, ERR_INVALID_STMT_TYPE );

    sStmtPos = &( sQuery );

    /* 필요한 구조체를 생성한다. */
    IDE_TEST( alllocQueryStruct( aStatement,
                                 &( sStatement ),
                                 &( sParseTree ),
                                 &( sQuerySet ),
                                 &( sSFWGH ),
                                 &( sFrom ),
                                 &( sTableRef ) )
              != IDE_SUCCESS );

    /* aTableRef 를 sTableRef 에 복제한 후
     *  aTableRef 를 초기화하고
     *   Shard View Query String 을 생성한다.
     */
    IDE_TEST( makeShardViewQueryString( aStatement,
                                        aTableRef,
                                        sTableRef,
                                        sFrom,
                                        sStmtPos )
              != IDE_SUCCESS );

    /* sTableRef 를 연결한다.*/
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    /* sQuerySet 를 연결한다. */
    SDI_SET_PARSETREE( sParseTree, sQuerySet );

    /* sParseTree 를 연결한다. */
    QC_SET_STATEMENT( sStatement, aStatement, sParseTree );

    sStatement->myPlan->parseTree->stmtKind =
        aStatement->myPlan->parseTree->stmtKind;

    /* sStatement 를 연결한다. */
    aTableRef->view = sStatement;

    /* sStatement 를 Shard View 로 변경한다. */
    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromObjectInfo( aStatement,
                                                        aShardObjInfo,
                                                        &( sAnalyzeInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanStmtVariable( sStatement,
                                             aStmtType,
                                             sStmtPos )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanCommVariable( sStatement,
                                             sAnalyzeInfo,
                                             0,
                                             ID_USHORT_MAX,
                                             NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForFrom",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardViewQueryString( qcStatement    * aStatement,
                                                    qmsTableRef    * aOldTableRef,
                                                    qmsTableRef    * aNewTableRef,
                                                    qmsFrom        * aFrom,
                                                    qcNamePosition * aStmtPos )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View Query String 생성 함수
 *
 *                 qmvShardTransform::makeShrdView 함수 분리 2
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar        * sQueryBuf = NULL;
    SInt           sMaxSize  = ( SDU_SHARD_TRANSFORM_STRING_LENGTH_MAX + 1 );
    qcNamePosition sQueryPosition;
    qcNamePosition sTempPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aOldTableRef == NULL, ERR_NULL_TABLE_REF_1 );
    IDE_TEST_RAISE( aNewTableRef == NULL, ERR_NULL_TABLE_REF_2 );
    IDE_TEST_RAISE( aFrom == NULL, ERR_NULL_FROM );
    IDE_TEST_RAISE( aStmtPos == NULL, ERR_NULL_STMTPOS );

    /********************************************************************************
     * INITIALIZE AND ALLOC
     ********************************************************************************/

    idlOS::memcpy( aNewTableRef,
                   aOldTableRef,
                   ID_SIZEOF( qmsTableRef ) );

    QCP_SET_INIT_QMS_TABLE_REF( aOldTableRef );

    if ( QC_IS_NULL_NAME( aNewTableRef->aliasName ) == ID_TRUE )
    {
        SET_POSITION( aOldTableRef->aliasName, aNewTableRef->tableName );
    }
    else
    {
        SET_POSITION( aOldTableRef->aliasName, aNewTableRef->aliasName );
    }

    /* BUG-47907 Pivot을 포함한 non-shard query에 대해 잘못된 transform을 수행하고 있습니다.*/
    if ( aNewTableRef->pivot != NULL )
    {
        aNewTableRef->position.size = aNewTableRef->pivot->position.offset - aNewTableRef->position.offset;

        aOldTableRef->pivot = aNewTableRef->pivot;
        aNewTableRef->pivot = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aNewTableRef->unpivot != NULL )
    {
        aNewTableRef->position.size = aNewTableRef->unpivot->position.offset - aNewTableRef->position.offset;

        aOldTableRef->unpivot = aNewTableRef->unpivot;
        aNewTableRef->unpivot = NULL;;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST_RAISE( ( QC_IS_NULL_NAME( aNewTableRef->position ) == ID_TRUE )
                    ||
                    ( QC_IS_NULL_NAME( aOldTableRef->aliasName ) == ID_TRUE ),
                    ERR_NULL_POSITION );

    IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                       SChar,
                                       sMaxSize,
                                       &( sQueryBuf ) )
              != IDE_SUCCESS );

    /********************************************************************************
     * SELECT *
     ********************************************************************************/

    sQueryPosition.stmtText = sQueryBuf;
    sQueryPosition.offset   = 0;
    sQueryPosition.size     = idlOS::snprintf( sQueryBuf,
                                               sMaxSize,
                                               "SELECT * " );

    /********************************************************************************
     * FROM
     ********************************************************************************/

    sTempPosition.stmtText = sQueryBuf;
    sTempPosition.offset   = sQueryPosition.size;

    sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                            sMaxSize - sQueryPosition.size,
                                            "FROM " );

    sTempPosition.size = sQueryPosition.size - sTempPosition.offset;

    SET_POSITION( aFrom->fromPosition, sTempPosition );

    /********************************************************************************
     * TABLE NAME
     ********************************************************************************/

    if ( aNewTableRef->position.stmtText[ aNewTableRef->position.offset - 1 ] == '"' )
    {
        sQueryBuf[ sQueryPosition.size ] = '"';
        sQueryPosition.size++;
    }
    else
    {
        /* Nothing to do */
    }

    sTempPosition.stmtText = sQueryBuf;
    sTempPosition.offset   = sQueryPosition.size;

    sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                            sMaxSize - sQueryPosition.size,
                                            "%.*s",
                                            aNewTableRef->position.size,
                                            aNewTableRef->position.stmtText + aNewTableRef->position.offset );

    sTempPosition.size = sQueryPosition.size - sTempPosition.offset;

    if ( aNewTableRef->position.stmtText[ aNewTableRef->position.offset + aNewTableRef->position.size ] == '"' )
    {
        sQueryBuf[ sQueryPosition.size ] = '"';
        sQueryPosition.size++;
    }
    else
    {
        /* Nothing to do */
    }

    SET_POSITION( aNewTableRef->position, sTempPosition );

    /********************************************************************************
     * TABLE ALTIAS NAME
     ********************************************************************************/

    sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                            sMaxSize - sQueryPosition.size,
                                            " as " );

    sTempPosition.stmtText = sQueryBuf;
    sTempPosition.offset   = sQueryPosition.size;

    if ( aOldTableRef->aliasName.stmtText[ aOldTableRef->aliasName.offset + aOldTableRef->aliasName.size] == '"' )
    {
        sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                                sMaxSize - sQueryPosition.size,
                                                "\"%.*s\"",
                                                aOldTableRef->aliasName.size,
                                                aOldTableRef->aliasName.stmtText + aOldTableRef->aliasName.offset );
    }
    else if ( aOldTableRef->aliasName.stmtText[ aOldTableRef->aliasName.offset + aOldTableRef->aliasName.size] == '\'' )
    {
        sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                                sMaxSize - sQueryPosition.size,
                                                "'%.*s'",
                                                aOldTableRef->aliasName.size,
                                                aOldTableRef->aliasName.stmtText + aOldTableRef->aliasName.offset );
    }
    else
    {
        sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                                sMaxSize - sQueryPosition.size,
                                                "%.*s",
                                                aOldTableRef->aliasName.size,
                                                aOldTableRef->aliasName.stmtText + aOldTableRef->aliasName.offset );
    }

    sTempPosition.size = sQueryPosition.size - sTempPosition.offset;

    SET_POSITION( aNewTableRef->aliasName, sTempPosition );

    /********************************************************************************
     * CHECK
     ********************************************************************************/

    IDE_TEST_RAISE( sQueryPosition.size >= ( sMaxSize - 1 ), ERR_TRANSFORM_STRING_BUFFER_OVERFLOW );

    /********************************************************************************
     * RETURN
     ********************************************************************************/

    SET_POSITION( (*aStmtPos), sQueryPosition );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_TABLE_REF_1 )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "old table ref is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_TABLE_REF_2 )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "new table ref is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_FROM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "from is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMTPOS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "stmt pos is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_POSITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "position is NULL" ) );
    }
    IDE_EXCEPTION( ERR_TRANSFORM_STRING_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardViewQueryString",
                                  "Transformation string buffer overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForAggr( qcStatement       * aStatement,
                                            qcShardStmtType     aStmtType,
                                            qmsQuerySet       * aQuerySet,
                                            qcParseTree       * aParseTree,
                                            qcParamOffsetInfo * aParamOffsetInfo,
                                            qcNamePosition    * aTransfromedQuery,
                                            UInt              * aOrderByTarget )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View 변환 함수
 *
 *                 qmvShardTransform::isTransformAble 함수 분리 1
 *                  Aggregate function으로 인해 non-shard query로 판별된 query set에 대하여
 *                   분산/통합부 로 query set을 나누는 transformation을 통하여 shard query로 변형한다.
 *                    여기서 분산부를 생성한다.
 *
 *                     select avg( c3 ) avg
 *                     ********************
 *                      from t1 group by c2;
 *                      ********************
 *
 *                       -> select sum( a ) / sum( b ) avg
 *
 *                           from shard( select c2, sum( c3 ) a, count( c3 ) b from t1 group c2 ) group by c2;
 *                                ***************************************************************
 *
 *                            aStatement-parseTree-querySet-sfwgh
 *
 *                             -> aStatement-parseTree-querySet-sfwgh-FROM'-TABLEREF'-STATEMENT'
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement      * sStatement     = NULL;
    qmsFrom          * sFrom          = NULL;
    qmsTableRef      * sTableRef      = NULL;
    sdiAnalyzeInfo   * sAnalyzeInfo   = NULL;
    qmsParseTree     * sOldParseTree  = NULL;
    qmsParseTree     * sNewParseTree  = NULL;
    qcShardParamInfo * sParamInfo     = NULL;
    UShort             sParamCount    = 0;
    UShort             sParamOffset   = ID_USHORT_MAX;
    UInt               sOrderByTarget = 0;

    IDE_TEST_RAISE( aTransfromedQuery == NULL, ERR_NULL_QUERY );

    /* 필요한 구조체를 생성한다. */
    IDE_TEST( alllocQueryStruct( aStatement,
                                 &( sStatement ),
                                 NULL,
                                 NULL,
                                 NULL,
                                 &( sFrom ),
                                 &( sTableRef ) )
              != IDE_SUCCESS );

    QC_SET_STATEMENT( sStatement, aStatement, NULL );

    sStatement->myPlan->stmtText    = aTransfromedQuery->stmtText;
    sStatement->myPlan->stmtTextLen = idlOS::strlen( aTransfromedQuery->stmtText );

    /* Bind 의 Shard 구분을 위한 Offset 설정 */
    IDE_TEST( qmg::setHostVarOffset( aStatement ) != IDE_SUCCESS );

    /* Parsing */
    IDE_TEST( qcpManager::parsePartialForQuerySet( sStatement,
                                                   aTransfromedQuery->stmtText,
                                                   aTransfromedQuery->offset,
                                                   aTransfromedQuery->size )
              != IDE_SUCCESS );

    sOldParseTree = (qmsParseTree *)aParseTree;
    sNewParseTree = (qmsParseTree *)sStatement->myPlan->parseTree;

    /* sStatement 를 연결한다. */
    sTableRef->view = sStatement;
    sFrom->tableRef = sTableRef;

    /* aQuerySet 에 연결한다. */
    aQuerySet->SFWGH->from = sFrom;

    /* sStatement 를 Shard View 로 변경한다. */
    IDE_TEST( sdi::makeAndSetAnalyzeInfoFromQuerySet( aStatement,
                                                      aQuerySet,
                                                      &( sAnalyzeInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( qmg::makeShardParamOffsetArrayWithInfo( aStatement,
                                                      sAnalyzeInfo,
                                                      aParamOffsetInfo,
                                                      &( sParamCount ),
                                                      &( sParamOffset ),
                                                      &( sParamInfo ) )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanStmtVariable( sStatement,
                                             aStmtType,
                                             aTransfromedQuery )
              != IDE_SUCCESS );

    IDE_TEST( sdi::setShardPlanCommVariable( sStatement,
                                             sAnalyzeInfo,
                                             sParamCount,
                                             sParamOffset,
                                             sParamInfo )
              != IDE_SUCCESS );

    /* Param 조정 후, Order by Transform 을 수행한다. */
    IDE_TEST( adjustTargetListAndSortNode( aStatement,
                                           sOldParseTree,
                                           sNewParseTree,
                                           &( sOrderByTarget ) )
              != IDE_SUCCESS );

    aQuerySet->SFWGH->lflag &= ~QMV_SFWGH_SHARD_TRANS_VIEW_MASK;
    aQuerySet->SFWGH->lflag |= QMV_SFWGH_SHARD_TRANS_VIEW_TRUE;

    /* Shard Query 로 처리 */
    sAnalyzeInfo->mIsShardQuery = ID_TRUE;

    /* Bind 의 Shard 구분을 위한 Offset 설정 원복 */
    IDE_TEST( qmg::setHostVarOffset( aStatement ) != IDE_SUCCESS );

    if ( aOrderByTarget != NULL )
    {
        *aOrderByTarget = sOrderByTarget;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForAggr",
                                  "transformed query is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardAggrQueryString( qcStatement       * aStatement,
                                                    qmsQuerySet       * aQuerySet,
                                                    qcParamOffsetInfo * aParamOffsetInfo,
                                                    idBool            * aUnsupportedAggr,
                                                    UInt              * aGroupKeyCount,
                                                    qcNamePosition    * aStmtPos )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard View Query String 생성 함수
 *
 *                 qmvShardTransform::isTransformAble 함수 분리 2
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode           * sNode            = NULL;
    qtcNode           * sLast            = NULL;
    qmsConcatElement  * sGroup           = NULL;
    qmsTarget         * sTarget          = NULL;
    SChar             * sQueryBuf        = NULL;
    UInt                sQueryBufSize    = 0;
    UInt                sTargetCount     = 0;
    UInt                sGroupKeyCount   = 0;
    SInt                sFromWhereStart  = 0;
    SInt                sFromWhereEnd    = 0;
    idBool              sUnsupportedAggr = ID_FALSE;
    qcNamePosition      sQueryPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    sQueryBufSize = aStatement->myPlan->parseTree->stmtPos.size + ( SDU_SHARD_TRANSFORM_STRING_LENGTH_MAX + 1 );

    /********************************************************************************
     * INITIALIZE AND ALLOC
     ********************************************************************************/

    IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                       SChar,
                                       sQueryBufSize,
                                       &( sQueryBuf ) )
              != IDE_SUCCESS );

    /********************************************************************************
     * SELECT
     ********************************************************************************/

    sQueryPosition.stmtText = sQueryBuf;
    sQueryPosition.offset   = 0;
    sQueryPosition.size     = idlOS::snprintf( sQueryBuf,
                                          sQueryBufSize,
                                          "SELECT " );

    /********************************************************************************
     * TARGET
     ********************************************************************************/

    for ( sGroup  = aQuerySet->SFWGH->group;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        /*
         * sGroup->arithmeticOrList == NULL 인 경우(ROLLUP, CUBE, GROUPING SETS)는
         * 앞서 수행된 shard analysis에서 걸러진다.
         */
        IDE_TEST( addColumnListToText( sGroup->arithmeticOrList,
                                       &( sTargetCount ),
                                       sQueryBuf,
                                       sQueryBufSize,
                                       &( sQueryPosition ) )
                  != IDE_SUCCESS );
    }

    for ( sTarget  = aQuerySet->SFWGH->target;
          sTarget != NULL;
          sTarget  = sTarget->next )
    {
        IDE_TEST( addAggrListToText( aStatement,
                                     aParamOffsetInfo,
                                     sTarget,
                                     sTarget->targetColumn,
                                     &( sTargetCount ),
                                     sQueryBuf,
                                     sQueryBufSize,
                                     &( sQueryPosition ),
                                     &( sUnsupportedAggr ) )
                  != IDE_SUCCESS );
    }

    for ( sNode  = aQuerySet->SFWGH->having;
          sNode != NULL;
          sNode  = (qtcNode *)( sNode->node.next ) )
    {
        IDE_TEST( addAggrListToText( aStatement,
                                     aParamOffsetInfo,
                                     NULL,
                                     sNode,
                                     &( sTargetCount ),
                                     sQueryBuf,
                                     sQueryBufSize,
                                     &( sQueryPosition ),
                                     &( sUnsupportedAggr ) )
                  != IDE_SUCCESS );
    }

    IDE_TEST_CONT( sUnsupportedAggr == ID_TRUE, NORMAL_EXIT );

    /********************************************************************************
     * FROM WHERE
     ********************************************************************************/

    sQueryBuf[ sQueryPosition.size ] = ' ';
    sQueryPosition.size++;

    sFromWhereStart = aQuerySet->SFWGH->from->fromPosition.offset;

    /* Where clause가 존재하면 where의 마지막 node의 end offset을 찾는다. */
    if ( aQuerySet->SFWGH->where != NULL )
    {
        for ( sNode  = aQuerySet->SFWGH->where;
              sNode != NULL;
              sNode  = (qtcNode*)sNode->node.next )
        {
            sLast = sNode;
        }

        IDE_TEST( qmg::getNodeOffset( sLast,
                                      ID_FALSE,
                                      NULL,
                                      &( sFromWhereEnd ) )
                  != IDE_SUCCESS );

    }
    else
    {
        /* Where clause가 존재하지 않으면 from를 순회하며 from의 end offset을 찾는다. */
        IDE_TEST( qmg::getFromOffset( aQuerySet->SFWGH->from,
                                      NULL,
                                      &( sFromWhereEnd ) )
                  != IDE_SUCCESS );
    }

    sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                            sQueryBufSize - sQueryPosition.size,
                                            "%.*s",
                                            sFromWhereEnd - sFromWhereStart,
                                            aQuerySet->SFWGH->startPos.stmtText + sFromWhereStart );

    /********************************************************************************
     * GROUP BY
     ********************************************************************************/

    for ( sGroup  = aQuerySet->SFWGH->group;
          sGroup != NULL;
          sGroup  = sGroup->next )
    {
        if ( sGroupKeyCount == 0 )
        {
            sQueryPosition.size += idlOS::snprintf( sQueryBuf + sQueryPosition.size,
                                                    sQueryBufSize - sQueryPosition.size,
                                                    " GROUP BY " );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( addColumnListToText( sGroup->arithmeticOrList,
                                       &( sGroupKeyCount ),
                                       sQueryBuf,
                                       sQueryBufSize,
                                       &( sQueryPosition ) )
                  != IDE_SUCCESS );
    }

    /*  */
    IDE_TEST( qmg::collectReaminParamOffset( aStatement,
                                             sFromWhereStart,
                                             sFromWhereEnd,
                                             aParamOffsetInfo )
              != IDE_SUCCESS );

    /********************************************************************************
     * RETURN
     ********************************************************************************/

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    SET_POSITION( (*aStmtPos), sQueryPosition );

    if ( aUnsupportedAggr != NULL )
    {
        *aUnsupportedAggr = sUnsupportedAggr;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aGroupKeyCount != NULL )
    {
        *aGroupKeyCount = sGroupKeyCount;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardAggrQueryString",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardAggrQueryString",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::alllocQueryStruct( qcStatement   * aStatement,
                                             qcStatement  ** aOutStatement,
                                             qmsParseTree ** aOutParseTree,
                                             qmsQuerySet  ** aOutQuerySet,
                                             qmsSFWGH     ** aOutSFWGH,
                                             qmsFrom      ** aOutFrom,
                                             qmsTableRef  ** aOutTableRef )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                변환에 사용되는 구조체를 할당하는 함수
 *
 *                 중복 코드 사용성 향상 목적으로 구현하였다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcStatement  * sStatement = NULL;
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsSFWGH     * sSFWGH     = NULL;
    qmsFrom      * sFrom      = NULL;
    qmsTableRef  * sTableRef  = NULL;
    qcNamePosition sNullPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );

    SET_EMPTY_POSITION( sNullPosition );

    if ( aOutStatement != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qcStatement,
                                &( sStatement ) )
                  != IDE_SUCCESS );

        *aOutStatement = sStatement;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutParseTree != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsParseTree,
                                &( sParseTree ) )
                  != IDE_SUCCESS );

        QC_SET_INIT_PARSE_TREE( sParseTree, sNullPosition );

        *aOutParseTree = sParseTree;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutQuerySet != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsQuerySet,
                                &( sQuerySet ) )
                  != IDE_SUCCESS );

        QCP_SET_INIT_QMS_QUERY_SET( sQuerySet );

        *aOutQuerySet = sQuerySet;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutSFWGH != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsSFWGH,
                                &( sSFWGH ) )
                  != IDE_SUCCESS );

        QCP_SET_INIT_QMS_SFWGH( sSFWGH );

        *aOutSFWGH = sSFWGH;
    }
    else
    {
        /* Nothing to do */
    }

    if ( aOutFrom != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsFrom,
                                &( sFrom ) )
                  != IDE_SUCCESS );

        QCP_SET_INIT_QMS_FROM( sFrom );

        *aOutFrom = sFrom;
    }
    else
    {
        /* Nothing to do */
    }


    if ( aOutTableRef != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsTableRef,
                                &( sTableRef ) )
                  != IDE_SUCCESS );

        QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

        *aOutTableRef = sTableRef;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::alllocQueryStruct",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForShard( qcStatement * aStatement,
                                                    qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Shard 인 Query 에 대한 변환 함수
 *
 *                 Select 의 경우, View 존재에 따라
 *                  qmvShardTransform::makeShardForConvert
 *                  qmvShardTransform::makeShardForStatement
 *
 *                   그외는
 *                    qmvShardTransform::makeShardForConvert
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree  * sParseTree = NULL;
    qciStmtType     sStmtKind  = QCI_STMT_MASK_MAX;
    qcShardStmtType sStmtType  = QC_STMT_SHARD_NONE;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtKind = aParseTree->stmtKind;
    sStmtType = aParseTree->stmtShard;

    switch ( sStmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            sParseTree = (qmsParseTree *)( aParseTree );

            if ( sParseTree->isView == ID_TRUE )
            {
                IDE_TEST( makeShardForConvert( aStatement,
                                               aParseTree )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( makeShardForStatement( aStatement,
                                                 aParseTree )
                          != IDE_SUCCESS );
            }

            break;

        case QCI_STMT_INSERT:
        case QCI_STMT_UPDATE:
        case QCI_STMT_DELETE:
        case QCI_STMT_EXEC_PROC:
            IDE_TEST_RAISE( sStmtType == QC_STMT_SHARD_DATA, ERR_UNSUPPORTED_SHARD_DATA_IN_DML );

            IDE_TEST( makeShardForConvert( aStatement,
                                           aParseTree )
                      != IDE_SUCCESS );

            aParseTree->optimize = qmo::optimizeShardDML;
            aParseTree->execute  = qmx::executeShardDML;

            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_KIND );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForShard",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_SHARD_DATA_IN_DML )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_UNSUPPORTED_SHARD_DATA_IN_DML ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_KIND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForShard",
                                  "stmt kind is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForNonShard( qcStatement * aStatement,
                                                       qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 Select 의 경우, View 존재에 따라
 *                  qmvShardTransform::processTransformForQuerySet
 *
 *                   그 외는
 *                    qmvShardTransform::processTransformForInsUptDel
 *
 *                     Subquery 가 있다면
 *                      qmvShardTransform::processTransformForSubquery
 *
 * Implementation :
 *
 ***********************************************************************/

    qciStmtType        sStmtKind  = QCI_STMT_MASK_MAX;
    qcShardStmtType    sStmtType  = QC_STMT_SHARD_NONE;
    qmsParseTree     * sParseTree = NULL;
    qmsQuerySet      * sQuerySet  = NULL;
    sdiShardAnalysis * sAnalysis  = NULL;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtKind = aParseTree->stmtKind;
    sStmtType = aParseTree->stmtShard;

    IDE_TEST_RAISE( sStmtType != QC_STMT_SHARD_NONE, ERR_INVALID_STMT_TYPE );

    switch ( sStmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            sParseTree = (qmsParseTree *)( aParseTree );
            sQuerySet  = sParseTree->querySet;

            IDE_TEST( processTransformForQuerySet( aStatement,
                                                   sQuerySet,
                                                   aParseTree )
                      != IDE_SUCCESS );
            break;

        case QCI_STMT_INSERT:
        case QCI_STMT_UPDATE:
        case QCI_STMT_DELETE:
            IDE_TEST( processTransformForInsUptDel( aStatement,
                                                    aParseTree )
                      != IDE_SUCCESS );
            break;

        case QCI_STMT_EXEC_PROC:

            /* BUG-49051
             * Unspecified shard key parameter를 가지는 sharded procedure에 대해서 제약한다.
             */
            IDE_TEST( sdi::getParseTreeAnalysis( aParseTree,
                                                 &( sAnalysis ) )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sAnalysis == NULL, ERR_INVALID_ANALYSIS_INFO );

            if ( sAnalysis->mAnalysisFlag.mNonShardFlag[SDI_UNSPECIFIED_SHARD_KEY_VALUE] == ID_TRUE )
            {
                IDE_TEST( sdi::raiseInvalidShardQueryError( aStatement,
                                                            aParseTree )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do. */
            }

            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_KIND );
            break;
    }

    IDE_TEST( processTransformForSubquery( aStatement,
                                           NULL, /* aQuerySet */
                                           aParseTree )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForNonShard",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForNonShard",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_KIND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForNonShard",
                                  "stmt kind is invalid" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ANALYSIS_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForNonShard",
                                  "Shard analysis information is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForQuerySet( qcStatement * aStatement,
                                                       qmsQuerySet * aQuerySet,
                                                       qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 Query Set 에 대하여 Top-Down 으로 순회하며 Shard View Transform 을 수행한다.
 *
 *                  select i2 from t1 order by i1;
 *                  *****************************
 *
 *                   -> select i2 from shard( select i2, i1 _1 from t1 ) t1 order by _1;
 *                                     ************************************          **
 *
 *                  select avg( c3 ) avg from t1 group by c2;
 *                  *****************************************
 *
 *                   -> select sum( a ) / sum( b ) avg
 *                             *******************
 *                      from shard( select c2, sum( c3 ) a, count( c3 ) b from t1 group c2 ) group by c2;
 *                           ***************************************************************
 *
 *
 *                  select ( select c1 from t1 limit 1 ) c1, c2 from t1;
 *                         *****************************
 *
 *                   -> select ( select c1 from shard( select c1 from t1 ) limit 1 ) c1, c2 from t1;
 *                                              **************************
 *
 *                  select c1 from ( select c1 from t1 limit 1 ) a, t1 b;
 *                                 *****************************
 *
 *                   -> select c1 from ( select c1 from shard( select c1 from t1 ) limit 1 ) a, t1 b;
 *                                                      **************************
 *
 *                  select c1 from t1 where c2 = ( select c1 from t1 limit 1 );
 *                                               *****************************
 *
 *                   -> select c1 from t1 where c2 = ( select c1 from shard( select c1 from t1 ) limit 1 );
 *                                                                    **************************
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree   * sParseTree       = NULL;
    idBool           sIsTransformed   = ID_FALSE;
    idBool           sIsShardQuerySet = ID_FALSE;
    idBool           sIsTransformAble = ID_FALSE;
    qcNamePosition * sStmtPos         = NULL;
    qcNamePosition   sQuery;

    /* 1. 정합성 검사 */
    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    /* 2. 초기화 */
    sParseTree = (qmsParseTree *)( aParseTree );
    sStmtPos   = &( sQuery );

    /* 3. Shard 여부 확인 */
    IDE_TEST( sdi::isShardQuerySet( aQuerySet,
                                    &( sIsShardQuerySet ),
                                    &( sIsTransformAble ) )
              != IDE_SUCCESS );

    /* 4. Shard Query 분석 */
    if ( ( QC_IS_NULL_NAME( aQuerySet->startPos ) == ID_FALSE )
         &&
         ( QC_IS_NULL_NAME( aQuerySet->endPos ) == ID_FALSE ) )
    {
        /* startPos 는 첫번째 Token, endPos 는 마지막 Token */
        sQuery.stmtText = aQuerySet->startPos.stmtText;
        sQuery.offset   = aQuerySet->startPos.offset;
        sQuery.size     =
            aQuerySet->endPos.offset + aQuerySet->endPos.size -
            aQuerySet->startPos.offset;
    }
    else
    {
        IDE_TEST_RAISE( sIsShardQuerySet == ID_TRUE, ERR_NULL_STMT_1 );
        IDE_TEST_RAISE( sIsTransformAble == ID_TRUE, ERR_NULL_STMT_2 );
    }

    /* 5. QuerySet 이 Shard Query 이면 QuerySet 를 Shard View 로 변환 */
    if ( sIsShardQuerySet == ID_TRUE )
    {
        /* Shard Query 를 위한 Order By Transform */
        IDE_TEST( processOrderByTransform( aStatement,
                                           aQuerySet,
                                           sParseTree,
                                           QMS_ORDER_BY_TRANSFORM_QUERYSET )
                  != IDE_SUCCESS );

        /* QuerySet 를 Shard View 로 변환 */
        IDE_TEST( makeShardForQuerySet( aStatement,
                                        sStmtPos,
                                        aQuerySet,
                                        aParseTree )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 6. Non Shard Query 이면 SFWGH 순으로 Shard View 변환 시도 */
        if ( aQuerySet->setOp == QMS_NONE )
        {
            IDE_TEST_RAISE( aQuerySet->SFWGH == NULL, ERR_NULL_SFWGH );

            /* Shard View 로 변환 시도해 볼만한 TransformAble Query 인 경우 */
            if ( sIsTransformAble == ID_TRUE )
            {
                /* Shard Query 를 위한 Order By Transform */
                IDE_TEST( processOrderByTransform( aStatement,
                                                   aQuerySet,
                                                   sParseTree,
                                                   QMS_ORDER_BY_TRANSFORM_AGGREGATION )
                          != IDE_SUCCESS );

                /* PROJ-2687 Shard Aggregation Transform */
                IDE_TEST( processTransformForAggr( aStatement,
                                                   aQuerySet,
                                                   aParseTree,
                                                   &( sIsTransformed ) )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            /* TransformAble Query 가 아니거나
             *  TransformAble Query 변환 실패인 경우
             */
            if ( sIsTransformed == ID_FALSE )
            {
                IDE_TEST( processTransformForSubquery( aStatement,
                                                       aQuerySet,
                                                       aParseTree )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( sdi::setPrintInfoFromTransformAble( aStatement ) != IDE_SUCCESS );
            }
        }
        else
        {
            /* 7. setOp 이 있는 경우, Left, Right 순으로 재귀 */
            IDE_TEST( processTransformForQuerySet( aStatement,
                                                   aQuerySet->left,
                                                   aParseTree )
                      != IDE_SUCCESS );

            IDE_TEST( processTransformForQuerySet( aStatement,
                                                   aQuerySet->right,
                                                   aParseTree )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForQuerySet",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForQuerySet",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForQuerySet",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMT_1 )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForQuerySet",
                                  "shard queryset stmt is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_STMT_2 )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForQuerySet",
                                  "transformable query stmt is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForAggr( qcStatement * aStatement,
                                                   qmsQuerySet * aQuerySet,
                                                   qcParseTree * aParseTree,
                                                   idBool      * aIsTransformed )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 Aggregate function으로 인해 non-shard query로 판별된 query set에 대하여
 *                  분산/통합부 로 query set을 나누는 transformation을 통하여 shard query로 변형한다.
 *
 *                   분산부는 qmvShardTransform::makeShardAggrQueryString,
 *                    qmvShardTransform::makeShardForAggr 를 호출하여 수행하고
 *                     여기서 통합부를 변환한다.
 *
 *                      select avg( c3 ) avg
 *                      ********************
 *                       from t1 group by c2;
 *                       ********************
 *
 *                        -> select sum( a ) / sum( b ) avg
 *                                  ***********************
 *                            from shard( select c2, sum( c3 ) a, count( c3 ) b from t1 group c2 ) group by c2;
 *
 *                             aStatement-parseTree-querySet-sfwgh
 *
 *                              -> aStatement-parseTree-querySet-sfwgh-FROM'-TABLEREF'-STATEMENT'
 *                                                                *
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode           * sNode            = NULL;
    qmsTarget         * sTarget          = NULL;
    qcParamOffsetInfo * sParamOffsetInfo = NULL;
    UInt                sGroupKeyCount   = 0;
    UInt                sViewTargetOrder = 0;
    UInt                sOrderByTarget   = 0;
    idBool              sUnsupportedAggr = ID_FALSE;
    idBool              sIsTransformed   = ID_FALSE;
    qcShardStmtType     sStmtType        = QC_STMT_SHARD_NONE;
    qcNamePosition      sQueryPosition;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aQuerySet == NULL, ERR_NULL_QUERYSET );

    sStmtType = aParseTree->stmtShard;

    IDE_TEST_RAISE( sStmtType != QC_STMT_SHARD_NONE, ERR_INVALID_STMT_TYPE );

    /* Bind 변환 정보 */
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qcParamOffsetInfo,
                            &( sParamOffsetInfo ) )
              != IDE_SUCCESS );

    QC_INIT_PARAM_OFFSET_INFO( sParamOffsetInfo );

    /* Aggr Transform Query 생성 */
    IDE_TEST( makeShardAggrQueryString( aStatement,
                                        aQuerySet,
                                        sParamOffsetInfo,
                                        &( sUnsupportedAggr ),
                                        &( sGroupKeyCount ),
                                        &( sQueryPosition ) )
              != IDE_SUCCESS );

    if ( sUnsupportedAggr == ID_FALSE )
    {
        /* Shard View 로 변환 */
        IDE_TEST( makeShardForAggr( aStatement,
                                    sStmtType,
                                    aQuerySet,
                                    aParseTree,
                                    sParamOffsetInfo,
                                    &( sQueryPosition ),
                                    &( sOrderByTarget ) )
                  != IDE_SUCCESS );

        /* TARGET */
        sViewTargetOrder = sGroupKeyCount;

        for ( sTarget  = aQuerySet->SFWGH->target;
              sTarget != NULL;
              sTarget  = sTarget->next )
        {
            IDE_TEST( modifyOrgAggr( aStatement,
                                     &( sTarget->targetColumn ),
                                     &( sViewTargetOrder ) )
                      != IDE_SUCCESS );
        }

        /* Implicit Order By Transform 으로 추가된 Target 개수도 포함해서 전달해야 한다. */
        sViewTargetOrder += sOrderByTarget;

        /* WHERE */
        aQuerySet->SFWGH->where = NULL;

        /* HAVING */
        for ( sNode  = aQuerySet->SFWGH->having;
              sNode != NULL;
              sNode  = (qtcNode*)( sNode->node.next ) )
        {
            IDE_TEST( modifyOrgAggr( aStatement,
                                     &( sNode ),
                                     &( sViewTargetOrder ) )
                      != IDE_SUCCESS );
        }

        sIsTransformed = ID_TRUE;
    }
    else
    {
        sIsTransformed = ID_FALSE;
    }

    if ( aIsTransformed != NULL )
    {
        *aIsTransformed = sIsTransformed;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForAggr",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_QUERYSET )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForAggr",
                                  "query set is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForAggr",
                                  "stmt type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForFrom( qcStatement    * aStatement,
                                                   qcShardStmtType  aStmtType,
                                                   qmsFrom        * aFrom )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 From 절 대한 Shard 변환을 시도한다.
 *                  View 라면
 *                   qmvShardTransform::processTransform 로 재귀한다.
 *
 *                    Table 이라면
 *                     qmvShardTransform::makeShardForFrom 를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsTableRef   * sTableRef        = NULL;
    sdiObjectInfo * sShardObjectInfo = NULL;
    ULong           sSMN             = ID_ULONG(0); /* PROJ-2701 Online data rebuild */
    qmsPivotAggr  * sPivotNode       = NULL;

    IDE_TEST_RAISE( aStatement == NULL, ERR_NULL_STATEMENT );
    IDE_TEST_RAISE( aFrom == NULL, ERR_NULL_FROM );

    sSMN = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );

    if ( aFrom->joinType == QMS_NO_JOIN )
    {
        sTableRef = aFrom->tableRef;

        if ( sTableRef->view != NULL )
        {
            IDE_TEST( processTransform( sTableRef->view ) != IDE_SUCCESS );

            /* Shard Trasform 된 View 임을 기록해놓고, Same View 처리 시 검사한다. */
            sTableRef->flag &= ~QMS_TABLE_REF_SHARD_TRANSFROM_MASK;
            sTableRef->flag |= QMS_TABLE_REF_SHARD_TRANSFROM_TRUE;
        }
        else
        {
            if ( sTableRef->mShardObjInfo != NULL )
            {
                sdi::getShardObjInfoForSMN( sSMN,
                                            sTableRef->mShardObjInfo,
                                            &( sShardObjectInfo ) );

                if ( sShardObjectInfo != NULL )
                {
                    IDE_TEST( makeShardForFrom( aStatement,
                                                aStmtType,
                                                sTableRef,
                                                sShardObjectInfo )
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
        }

        /* BUG-47907 Pivot을 포함한 non-shard query에 대해 잘못된 transform을 수행하고 있습니다.*/
        if ( sTableRef->pivot != NULL )
        {
            for ( sPivotNode  = sTableRef->pivot->aggrNodes;
                      sPivotNode != NULL;
                  sPivotNode  = sPivotNode->next )
            {
                if ( sPivotNode->node != NULL )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sPivotNode->node )
                              != IDE_SUCCESS );
                    }
                else
                {
                    /* Nothing to do */
                }
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        IDE_TEST( processTransformForFrom( aStatement,
                                           aStmtType,
                                           aFrom->left )
                  != IDE_SUCCESS );

        IDE_TEST( processTransformForFrom( aStatement,
                                           aStmtType,
                                           aFrom->right )
                  != IDE_SUCCESS );

        IDE_TEST( processTransformForExpr( aStatement,
                                           aFrom->onCondition )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_STATEMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForFrom",
                                  "statement is NULL" ) );
    }
    IDE_EXCEPTION( ERR_NULL_FROM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForFrom",
                                  "from is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForInsUptDel( qcStatement * aStatement,
                                                        qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 Non Shard DML에 대해서 partial coordinating query로 변환한다.
 *                  UPDATE : 
 *                  DELETE : 다음의 경우를 제외하고 전체 SQL를 data node에게 전달하여, partial coordinating 하도록 transform
 *                    0. Non-deterministic operation이 존재하는 경우 에러 발생
 *                     0-1. Limit clause가 존재하는 경우
 *                     0-2. Non-deterministic rull 추가 필요
 *                  
 *                  INSERT :
 *                   INSERT VALUES : 다음의 경우를 제외하고, 전체 SQL를 data node에게 전달하여, partial coordinating 하도록 transform
 *                    1. Shard key value가 unspecified인 경우 SDIN(SHARD-INSERT)로 수행 되도록 transform
 *                    2. Non-shard object가 존재하는 경우 SDIN(SHARD-INSERT)로 수행 되도록 transform
 *                    3. Shard keyword가 존재하는 경우 SDIN(SHARD-INSERT)로 수행 되도록 transform
 *                    4. Insert multi rows 인 경우 SDIN(SHARD-INSERT)로 수행 되도록 transform
 *                    5. Sub-shard key가 존재한는 경우 SDIN(SHARD-INSERT)로 수행 되도록 transform
 *                   INSERT SELECT : 추가로 아래의 경우를 제외하고,
 *                                   전체 SQL를 data node에게 전달하여, partial coordinating 하도록 transform
 *                    6. Insert target 테이블의 분산 정의가 shard key value의 모든 경우의수를 커버하도록 정의되어있지 않은 경우
 *                       SDIN(SHARD-INSERT)로 수행 되도록 transform
 *
 *                   Local Table 의 경우에 무시하며,
 *                    qmvShardTransform::processTransformForNonShard 의
 *                     qmvShardTransform::processTransformForSubquery 에서
 *                      Subquery 에 대해 Shard 로 변환 시도한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool      sIsShardObject = ID_TRUE;
    qciStmtType sStmtKind      = QCI_STMT_MASK_MAX;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtKind = aParseTree->stmtKind;

    IDE_TEST( sdi::isShardObject( aParseTree,
                                  &( sIsShardObject ) )
              != IDE_SUCCESS );

    if ( sIsShardObject == ID_TRUE )
    {
        switch ( sStmtKind )
        {
            case QCI_STMT_INSERT:
                IDE_TEST( makeShardForInsert( aStatement,
                                              aParseTree )
                          != IDE_SUCCESS );
                break;
            case QCI_STMT_UPDATE:
            case QCI_STMT_DELETE:
                IDE_TEST( makeShardForUptDel( aStatement,
                                              aParseTree )
                          != IDE_SUCCESS );
                break;
            case QCI_STMT_LOCK_TABLE:
                /* Nothing to do. */
                break;
            default:
                IDE_RAISE(ERR_UNSUPPORTED_NON_SHARD_STMT_TYPE);
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForInsUptDel",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_NON_SHARD_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "Unsupported non-shard SQL",
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForSubquery( qcStatement * aStatement,
                                                       qmsQuerySet * aQuerySet,
                                                       qcParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 Subquery 에 대해 Shard 로 변환 시도한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsParseTree    * sSelParseTree  = NULL;
    qmmInsParseTree * sInsParseTree  = NULL;
    qmmDelParseTree * sDelParseTree  = NULL;
    qmmUptParseTree * sUptParseTree  = NULL;
    qsExecParseTree * sExecParseTree = NULL;
    qmmValueNode    * sValues        = NULL;
    qmmSubqueries   * sSubQuery      = NULL;
    qmsSortColumns  * sOrderBy       = NULL;
    qciStmtType       sStmtKind      = QCI_STMT_MASK_MAX;
    qcShardStmtType   sStmtType      = QC_STMT_SHARD_NONE;

    IDE_TEST_RAISE( aParseTree == NULL, ERR_NULL_PARSETREE );

    sStmtKind = aParseTree->stmtKind;
    sStmtType = aParseTree->stmtShard;

    switch ( sStmtKind )
    {
        case QCI_STMT_SELECT:
        case QCI_STMT_SELECT_FOR_UPDATE:
            if ( aQuerySet != NULL )
            {
                IDE_TEST( processTransformForSFWGH( aStatement,
                                                    sStmtType,
                                                    aQuerySet->SFWGH )
                          != IDE_SUCCESS );
            }
            else
            {
                sSelParseTree = (qmsParseTree *)( aParseTree );

                for ( sOrderBy  = sSelParseTree->orderBy;
                      sOrderBy != NULL;
                      sOrderBy  = sOrderBy->next )
                {
                    if ( sOrderBy->targetPosition <= QMV_EMPTY_TARGET_POSITION )
                    {
                        IDE_TEST( processTransformForExpr( aStatement,
                                                           sOrderBy->sortColumn )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                /* LOOP ( Subquery가 허용되지는 않으나 잘못된 에러가 발생한다. ) */
                if ( sSelParseTree->loopNode != NULL)
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sSelParseTree->loopNode )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            break;

        case QCI_STMT_INSERT:
            sInsParseTree = (qmmInsParseTree *)( aParseTree );

            if ( sInsParseTree->common.parse == qmv::parseInsertSelect )
            {
                IDE_TEST( processTransform( sInsParseTree->select )
                          != IDE_SUCCESS );
            }
            else if ( sInsParseTree->common.parse == qmv::parseInsertValues )
            {
                for ( sValues  = sInsParseTree->rows->values;
                      sValues != NULL;
                      sValues  = sValues->next )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sValues->value )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do. */
            }
            break;

        case QCI_STMT_UPDATE:
            sUptParseTree = (qmmUptParseTree *)( aParseTree );

            if ( sUptParseTree->querySet->SFWGH->where != NULL )
            {
                IDE_TEST( processTransformForExpr( aStatement,
                                                   sUptParseTree->querySet->SFWGH->where )
                          != IDE_SUCCESS);
            }
            else
            {
                /* Nothing to do. */
            }

            for ( sValues  = sUptParseTree->values;
                  sValues != NULL;
                  sValues  = sValues->next )
            {
                if ( sValues->value != NULL )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sValues->value )
                              != IDE_SUCCESS);
                }
                else
                {
                    /* Nothing to do. */
                }
            }

            for ( sSubQuery  = sUptParseTree->subqueries;
                  sSubQuery != NULL;
                  sSubQuery  = sSubQuery->next )
            {
                if ( sSubQuery->subquery != NULL )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sSubQuery->subquery )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            break;

        case QCI_STMT_DELETE:
            sDelParseTree = (qmmDelParseTree *)( aParseTree );

            if ( sDelParseTree->querySet->SFWGH->where != NULL )
            {
                IDE_TEST( processTransformForExpr( aStatement,
                                                   sDelParseTree->querySet->SFWGH->where )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            break;

        case QCI_STMT_EXEC_PROC:
            sExecParseTree = (qsExecParseTree *)( aParseTree );

            if ( sExecParseTree->callSpecNode != NULL )
            {
                IDE_TEST( processTransformForExpr( aStatement,
                                                   sExecParseTree->callSpecNode )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
            break;

        default:
            IDE_RAISE( ERR_INVALID_STMT_KIND );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_PARSETREE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForSubquery",
                                  "parse tree is NULL" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_STMT_KIND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForSubquery",
                                  "stmt kind is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::processTransformForSFWGH( qcStatement    * aStatement,
                                                    qcShardStmtType  aStmtType,
                                                    qmsSFWGH       * aSFWGH )
{
/***********************************************************************
 *
 * Description : TASK-7219 Shard Transformer Refactoring
 *                Non Shard 인 Query 에 대한 변환 함수
 *
 *                 Subquery 에 대해 Shard 로 변환 시도한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsFrom          * sFrom          = NULL;
    qmsTarget        * sTarget        = NULL;
    qmsConcatElement * sConcatElement = NULL;

    IDE_TEST_RAISE( aSFWGH == NULL, ERR_NULL_SFWGH );

    /* Target */
    for ( sTarget  = aSFWGH->target;
          sTarget != NULL;
          sTarget  = sTarget->next )
    {
        if ( ( sTarget->flag & QMS_TARGET_ASTERISK_MASK )
             != QMS_TARGET_ASTERISK_TRUE )
        {
            IDE_TEST( processTransformForExpr( aStatement,
                                               sTarget->targetColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* From */
    for ( sFrom  = aSFWGH->from;
          sFrom != NULL;
          sFrom  = sFrom->next )
    {
        IDE_TEST( processTransformForFrom( aStatement,
                                           aStmtType,
                                           sFrom )
                  != IDE_SUCCESS );
    }

    /* Where */
    if ( aSFWGH->where != NULL )
    {
        IDE_TEST( processTransformForExpr( aStatement,
                                           aSFWGH->where )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* Hierarchy */
    if ( aSFWGH->hierarchy != NULL )
    {
        if ( aSFWGH->hierarchy->startWith != NULL )
        {
            IDE_TEST( processTransformForExpr( aStatement,
                                               aSFWGH->hierarchy->startWith )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        if ( aSFWGH->hierarchy->connectBy != NULL )
        {
            IDE_TEST( processTransformForExpr( aStatement,
                                               aSFWGH->hierarchy->connectBy )
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

    /* Group By */
    for ( sConcatElement  = aSFWGH->group;
          sConcatElement != NULL;
          sConcatElement  = sConcatElement->next )
    {
        if ( sConcatElement->arithmeticOrList != NULL )
        {
            IDE_TEST( processTransformForExpr( aStatement,
                                               sConcatElement->arithmeticOrList )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* Having */
    if ( aSFWGH->having != NULL )
    {
        IDE_TEST( processTransformForExpr( aStatement,
                                           aSFWGH->having )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NULL_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::processTransformForSFWGH",
                                  "sfwgh is NULL" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForInsert( qcStatement      * aStatement,
                                              qcParseTree      * aParseTree )
{
    idBool             sCanPartialCoordExec   = ID_FALSE;
    sdiShardAnalysis * sAnalysis              = NULL;
    idBool             sIsFullRange           = ID_FALSE;
    sdiObjectInfo    * sShardObjInfo          = NULL;
    sdiObjectInfo    * sDMLTargetShardObjInfo = NULL;

    sShardObjInfo = ( (qmmInsParseTree*)aStatement->myPlan->parseTree )->tableRef->mShardObjInfo;

    /* Convert the statement to the shard view statement */
    IDE_TEST( makeShardForConvert( aStatement,
                                   aParseTree )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getParseTreeAnalysis( aParseTree,
                                         &( sAnalysis ) )
              != IDE_SUCCESS );

    if ( ( ( QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement ) == ID_TRUE ) &&
           ( QCU_DISPLAY_PLAN_FOR_NATC == 0 ) ) ||
         ( sAnalysis->mAnalysisFlag.mNonShardFlag[SDI_UNSPECIFIED_SHARD_KEY_VALUE] == ID_TRUE ) ||
         ( sAnalysis->mAnalysisFlag.mNonShardFlag[SDI_NON_SHARD_OBJECT_EXISTS] == ID_TRUE ) ||
         ( sAnalysis->mAnalysisFlag.mNonShardFlag[SDI_SHARD_KEYWORD_EXISTS] == ID_TRUE ) ||
         ( sAnalysis->mAnalysisFlag.mNonShardFlag[SDI_UNKNOWN_REASON] == ID_TRUE ) ||
         ( sAnalysis->mAnalysisFlag.mTopQueryFlag[SDI_TQ_SUB_KEY_EXISTS] == ID_TRUE ) )
    {
        sCanPartialCoordExec = ID_FALSE;
    }
    else
    {
        if ( aParseTree->parse == qmv::parseInsertSelect )
        {
            sdi::getShardObjInfoForSMN( QCG_GET_SESSION_SHARD_META_NUMBER( aStatement ),
                                        sShardObjInfo,
                                        &sDMLTargetShardObjInfo );

            IDE_TEST( isFullRange( sDMLTargetShardObjInfo,
                                   &sIsFullRange )
                      != IDE_SUCCESS );

            if ( sIsFullRange == ID_TRUE )
            {
                sAnalysis->mAnalysisFlag.mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] = ID_TRUE;
                aStatement->myPlan->mShardAnalysis->mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] = ID_TRUE;
                sCanPartialCoordExec = ID_TRUE;
            }
            else
            {
                sCanPartialCoordExec = ID_FALSE;
            }
        }
        else
        {
            sAnalysis->mAnalysisFlag.mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] = ID_TRUE;
            aStatement->myPlan->mShardAnalysis->mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] = ID_TRUE;
            sCanPartialCoordExec = ID_TRUE;
        }
    }

    if ( sCanPartialCoordExec == ID_FALSE )
    {
        aParseTree->optimize = qmo::optimizeShardInsert;
        aParseTree->execute  = qmx::executeShardInsert;
    }
    else
    {
        aParseTree->optimize = qmo::optimizeShardDML;
        aParseTree->execute  = qmx::executeShardDML;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeShardForUptDel( qcStatement      * aStatement,
                                              qcParseTree      * aParseTree )
{
    sdiShardAnalysis * sAnalysis              = NULL;

    switch ( aParseTree->stmtKind )
    {
        case QCI_STMT_UPDATE:
            if ( ( (qmmUptParseTree*)aParseTree )->limit != NULL )
            {
                IDE_TEST( sdi::raiseInvalidShardQueryError( aStatement,
                                                            aParseTree )
                          != IDE_SUCCESS );
            }
            break;
        case QCI_STMT_DELETE:
            if ( ( (qmmDelParseTree*)aParseTree )->limit != NULL )
            {
                IDE_TEST( sdi::raiseInvalidShardQueryError( aStatement,
                                                            aParseTree )
                          != IDE_SUCCESS );
            }
            break;
        default:
            IDE_RAISE(ERR_INVALID_NON_SHARD_TRANSFORM_TYPE);
            break;
    }

    /* Convert the statement to the shard view statement */
    IDE_TEST( makeShardForConvert( aStatement,
                                   aParseTree )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getParseTreeAnalysis( aParseTree,
                                         &( sAnalysis ) )
              != IDE_SUCCESS );

    aStatement->myPlan->parseTree->optimize = qmo::optimizeShardDML;
    aStatement->myPlan->parseTree->execute  = qmx::executeShardDML;

    sAnalysis->mAnalysisFlag.mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] = ID_TRUE;
    aStatement->myPlan->mShardAnalysis->mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] = ID_TRUE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NON_SHARD_TRANSFORM_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeShardForUptDel",
                                  "Not a UPDATE or DELETE statement for shard transformation" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::isFullRange( sdiObjectInfo  * aObjectInfo,
                                       idBool         * aIsFullRange )
{
/****************************************************************************************
 *
 * Description : NULL data를 고려하여, default node 및 hash split value maximum을 확인하여
 *               포함되지 않는 distribution value가 있을지 판단한다.
 *
 * Implementation :
 *
 ****************************************************************************************/
    *aIsFullRange = ID_FALSE;

    if ( sdi::getSplitType( aObjectInfo->mTableInfo.mSplitMethod  ) == SDI_SPLIT_TYPE_DIST )
    {
        if ( aObjectInfo->mTableInfo.mDefaultNodeId != SDI_NODE_NULL_ID )
        {
            *aIsFullRange = ID_TRUE;
        }
        else
        {
            if ( aObjectInfo->mTableInfo.mSplitMethod == SDI_SPLIT_HASH )
            {
                if ( aObjectInfo->mRangeInfo.mCount > 0 )
                {
                    if ( aObjectInfo->mRangeInfo.mRanges[(aObjectInfo->mRangeInfo.mCount-1)].mValue.mHashMax == (UInt)SDI_RANGE_MAX_COUNT )
                    {
                        *aIsFullRange = ID_TRUE;
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
        *aIsFullRange = ID_TRUE;
    }

    return IDE_SUCCESS;
}

IDE_RC qmvShardTransform::partialCoordTransform( qcStatement    * aStatement )
{
/****************************************************************************************
 *
 * Description : Non-shard DML의 수행을 위해, data node가 부분적으로 분산 수행부 변형을 수행한다.
 *
 *
 * Implementation : a. Insert values의 sub-query에 대해 shard view transformation을 수행한다.
 *                  b. Insert select의 select statement에 대해 shard view transformation을 수행한다.
 *                  b. Update set, where clause의 sub-query에 대해 shard view transformation을 수행한다.
 *                  c. Delete where clause의 sub-query에 대해 shard view transformation을 수행한다.
 *
 ****************************************************************************************/

    qcmTableInfo * sTableInfo4ShardDMLTransform = NULL;

    qmmInsParseTree  * sInsParseTree  = NULL;
    qmmUptParseTree  * sUptParseTree  = NULL;
    qmmDelParseTree  * sDelParseTree  = NULL;
    sdiObjectInfo    * sShardObjInfo  = NULL;

    ULong              sTransformSMN  = ID_ULONG(0);

    qtcNode          * sNode          = NULL;
    qmmSubqueries    * sSetSubqueries = NULL;
    
    idBool             sIsShardQuery  = ID_FALSE;
    qciStmtType        sStmtType      = aStatement->myPlan->parseTree->stmtKind;

    qmmValueNode     * sValues        = NULL;

    qmsQuerySet      * sQuerySet      = NULL;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aStatement->myPlan->parseTree != NULL );

    switch ( aStatement->myPlan->parseTree->stmtKind )
    {
        case QCI_STMT_INSERT:
            sInsParseTree = (qmmInsParseTree*) aStatement->myPlan->parseTree;
            sTableInfo4ShardDMLTransform = sInsParseTree->tableRef->tableInfo;
            sShardObjInfo = sInsParseTree->tableRef->mShardObjInfo;
            break;

        case QCI_STMT_UPDATE:
            sUptParseTree = (qmmUptParseTree *)aStatement->myPlan->parseTree;
            sTableInfo4ShardDMLTransform = sUptParseTree->querySet->SFWGH->from->tableRef->tableInfo;
            sShardObjInfo = sUptParseTree->querySet->SFWGH->from->tableRef->mShardObjInfo;
            break;

        case QCI_STMT_DELETE:
            sDelParseTree = (qmmDelParseTree *)aStatement->myPlan->parseTree;
            sTableInfo4ShardDMLTransform = sDelParseTree->querySet->SFWGH->from->tableRef->tableInfo;
            sShardObjInfo = sDelParseTree->querySet->SFWGH->from->tableRef->mShardObjInfo;
            break;

        default:
            IDE_FT_ASSERT(0);
            break;
    }

    sTransformSMN = QCG_GET_SESSION_SHARD_META_NUMBER( aStatement );

    IDE_TEST( doShardAnalyze( aStatement,
                              &aStatement->myPlan->parseTree->stmtPos,
                              sTransformSMN,
                              sQuerySet )
              != IDE_SUCCESS );

    IDE_TEST( sdi::isShardParseTree( aStatement->myPlan->parseTree,
                                     &( sIsShardQuery ) )
              != IDE_SUCCESS );

    if ( sIsShardQuery == ID_FALSE )
    {
        switch( sStmtType )
        {
            case QCI_STMT_INSERT :
                if ( aStatement->myPlan->parseTree->parse == qmv::parseInsertValues )
                {
                    sValues = ((qmmInsParseTree*)aStatement->myPlan->parseTree)->rows->values;

                    for ( ;
                          sValues != NULL;
                          sValues  = sValues->next )
                    {
                        IDE_TEST( processTransformForExpr( aStatement,
                                                           sValues->value )
                                  != IDE_SUCCESS );
                    }
                }
                else if ( aStatement->myPlan->parseTree->parse == qmv::parseInsertSelect )
                {
                    IDE_TEST( processTransform( ((qmmInsParseTree*)aStatement->myPlan->parseTree)->select )
                              != IDE_SUCCESS );

                    if ( sdi::getSplitType( sShardObjInfo->mTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
                    {
                        IDE_TEST( partialInsertSelectTransform( ((qmmInsParseTree*)aStatement->myPlan->parseTree)->select,
                                                                sTableInfo4ShardDMLTransform,
                                                                sShardObjInfo,
                                                                sInsParseTree->insertColumns,
                                                                sTransformSMN )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
                else
                {
                    IDE_RAISE( ERR_INVALID_PARTIAL_COORD_EXEC_TYPE );
                }

                break;

            case QCI_STMT_UPDATE :
                sNode = ((qmmUptParseTree*)aStatement->myPlan->parseTree)->querySet->SFWGH->where;
                sValues = ((qmmUptParseTree*)aStatement->myPlan->parseTree)->values;
                sSetSubqueries = ((qmmUptParseTree*)aStatement->myPlan->parseTree)->subqueries;

                // Where clause of update statement
                for ( ;
                      sNode != NULL;
                      sNode  = (qtcNode*)sNode->node.next )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sNode )
                              != IDE_SUCCESS );
                }

                // Set clause of update statement
                for ( ;
                      sValues != NULL;
                      sValues  = sValues->next )
                {
                    if ( sValues->value != NULL )
                    {
                        IDE_TEST( processTransformForExpr( aStatement,
                                                           sValues->value )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }

                for ( ;
                      sSetSubqueries != NULL;
                      sSetSubqueries  = sSetSubqueries->next )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sSetSubqueries->subquery )
                              != IDE_SUCCESS );
                }

                break;

            case QCI_STMT_DELETE :
                sNode = ((qmmDelParseTree*)aStatement->myPlan->parseTree)->querySet->SFWGH->where;

                // Where clause of delete statement
                for ( ;
                      sNode != NULL;
                      sNode  = (qtcNode*)sNode->node.next )
                {
                    IDE_TEST( processTransformForExpr( aStatement,
                                                       sNode )
                              != IDE_SUCCESS );
                }

                break;
            default :
                IDE_RAISE( ERR_INVALID_PARTIAL_COORD_EXEC_TYPE );
                break;
        }

        sShardObjInfo->mIsLocalForce = ID_TRUE;
    }
    else
    {
        /* Shard DML */
        /* Local로 동작한다. */
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_PARTIAL_COORD_EXEC_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::partialCoordTransform",
                                  "Invalid partial execution statement type" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::partialInsertSelectTransform( qcStatement   * aStatement,
                                                        qcmTableInfo  * aTableInfo,
                                                        sdiObjectInfo * aShardObj,
                                                        qcmColumn     * aInsertColumns,
                                                        ULong           aSMN )
{
    qcStatement  * sStatement = NULL;
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsSFWGH     * sSFWGH     = NULL;
    qmsFrom      * sFrom      = NULL;
    qmsTableRef  * sTableRef  = NULL;
    qcNamePosition sNullPosition;

    SET_EMPTY_POSITION( sNullPosition );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qcStatement,
                            &sStatement )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsParseTree,
                            &sParseTree )
              != IDE_SUCCESS );
    QC_SET_INIT_PARSE_TREE( sParseTree, sNullPosition );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsQuerySet,
                            &sQuerySet )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_QUERY_SET( sQuerySet );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsSFWGH,
                            &sSFWGH )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_SFWGH( sSFWGH );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsFrom,
                            &sFrom )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_FROM( sFrom );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qmsTableRef,
                            &sTableRef )
              != IDE_SUCCESS );
    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

    // aStatement를 교체할 수 없으므로 sStatement를 복사 생성한다.
    idlOS::memcpy( sStatement, aStatement, ID_SIZEOF(qcStatement) );

    // myPlan을 재설정한다.
    sStatement->myPlan = & sStatement->privatePlan;
    sTableRef->view      = sStatement;

    // view merging 되지 않도록 한다.
    sTableRef->noMergeHint = ID_TRUE;
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    IDE_TEST( makeRangeCondition( aStatement,
                                  sSFWGH,
                                  aTableInfo,
                                  aShardObj,
                                  aInsertColumns,
                                  aSMN )
              != IDE_SUCCESS );

    // parseTree를 생성한다.
    sParseTree->withClause         = NULL;
    sParseTree->querySet           = sQuerySet;
    sParseTree->orderBy            = NULL;
    sParseTree->limit              = NULL;
    sParseTree->loopNode           = NULL;
    sParseTree->forUpdate          = NULL;
    sParseTree->queue              = NULL;
    sParseTree->isTransformed      = ID_FALSE;
    sParseTree->isView             = ID_TRUE;
    sParseTree->isShardView        = ID_FALSE;
    sParseTree->common.currValSeqs = NULL;
    sParseTree->common.nextValSeqs = NULL;
    sParseTree->common.parse       = qmv::parseSelect;
    sParseTree->common.validate    = qmv::validateSelect;
    sParseTree->common.optimize    = qmo::optimizeSelect;
    sParseTree->common.execute     = qmx::executeSelect;

    // aStatement의 parseTree를 변경한다.
    aStatement->myPlan->parseTree = (qcParseTree*) sParseTree;
    aStatement->myPlan->parseTree->stmtKind =
        sStatement->myPlan->parseTree->stmtKind;

    // sStatement를 view로 변경한다.
    SET_POSITION( sStatement->myPlan->parseTree->stmtPos, ((qmsParseTree *)aStatement->myPlan->parseTree)->common.stmtPos );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeRangeCondition( qcStatement   * aStatement,
                                              qmsSFWGH      * aSFWGH,
                                              qcmTableInfo  * aTableInfo,
                                              sdiObjectInfo * aShardObj,
                                              qcmColumn     * aInsertColumns,
                                              ULong           aSMN )
{
/****************************************************************************************
 *
 * Description : Insert select의 partial transformation을 수행한다.
 *
 *               IF myNode = node1 : SPLIT_HASH(I1) < 300 THEN
 *
 *               INSERT INTO HASH_T1(I1,I2) SELECT I2,I1
 *                                            FROM HASH_T1;
 *               ->
 *               INSERT INTO HASH_T1(I1,I2) SELECT *
 *                                            FROM ( SELECT I2,I1 FROM HASH_T1 )
 *                                           WHERE MOD(HASH(CAST( I2 VARCHAR(10) )),1000) < 300;
 *
 * Implementation : 1. Transformation 기초 정보 셋업
 *                    1-1. My node ID를 구해온다. ( by session callback )
 *                    1-2. DML target object의 default node ID를 구해온다.
 *                    1-3. Shard key column의 정보를 구해온다.
 *                  2. DML target object의 분산 정보를 구해온다.
 *                  3. Shard key condition string을 생성한다.
 *                  4. 기초정보 및 분산 정보와 key string을 활용해 내 노드에 포함시켜야 하는 my data range string을 생성한다.
 *                  5. My data range string을 partial parsing하여 추가할 자료구조를 생성한다.
 *                    5-1. setColumnID of validation에서 estimation할 수 있도록 shardViewTargetPos를 설정한다.
 *
 ****************************************************************************************/
    sdiNodeInfo    * sNodeInfo         = NULL;

    UShort           sMyNodeId         = ID_USHORT_MAX;
    SChar          * sMyNodeName       = NULL;
    UInt             sDefaultNodeId    = ID_UINT_MAX;

    SChar          * sMyKeyColName     = NULL;
    UInt             sMyKeyDataType    = ID_UINT_MAX;
    UShort           sMyKeyColOrder    = ID_USHORT_MAX;
    SInt             sMyKeyPrecision   = ID_UINT_MAX;
    sdiSplitMethod   sMyKeySplitMethod = SDI_SPLIT_NONE;

    SChar          * sMyKeyString      = NULL;
    sdiMyRanges    * sMyRanges         = NULL;
    SChar          * sMyRangeString    = NULL;
    UInt             sMyRangeStringLen = 0 ;

    qcNamePosition   sPosition;
    qcStatement      sStatement;

    UShort           sMyKeyInsertOrder = ID_USHORT_MAX;
    qcmColumn      * sInsertColumn     = NULL;
    UInt             sColOrderOnTable  = 0;
    idBool           sKeyAppears       = ID_FALSE;

    //------------------------------------------
    // 유효성 검사
    //------------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSFWGH != NULL );
    IDE_DASSERT( aSFWGH->where == NULL );
    IDE_DASSERT( aTableInfo != NULL );
    IDE_DASSERT( aShardObj != NULL );

    IDE_TEST_RAISE ( aShardObj->mTableInfo.mSubKeyExists == ID_TRUE, ERR_UNSUPPORTED_SPLIT_TYPE_FOR_PARTIAL_COORD );

    //------------------------------------------
    // set my node name & ID
    //------------------------------------------
    sMyNodeName = qcg::getSessionShardNodeName( aStatement );

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          sdiNodeInfo,
                          &sNodeInfo)
             != IDE_SUCCESS);

    IDE_TEST( sdi::getInternalNodeInfo( NULL,
                                        sNodeInfo,
                                        ID_FALSE,
                                        aSMN )
              != IDE_SUCCESS );

    getNodeIdFromName( sNodeInfo,
                       sMyNodeName,
                       &sMyNodeId );

    //------------------------------------------
    // get DML taget table common Info
    //------------------------------------------
    sDefaultNodeId = aShardObj->mTableInfo.mDefaultNodeId;

    //------------------------------------------
    // set DML target table shard key Info
    //------------------------------------------
    sMyKeyColName     = aShardObj->mTableInfo.mKeyColumnName;
    sMyKeyDataType    = aShardObj->mTableInfo.mKeyDataType;
    sMyKeyColOrder    = aShardObj->mTableInfo.mKeyColOrder;
    sMyKeySplitMethod = aShardObj->mTableInfo.mSplitMethod;
    sMyKeyPrecision   = aTableInfo->columns[sMyKeyColOrder].basicInfo->precision;

    if ( aInsertColumns != NULL )
    {
        for ( sInsertColumn = aInsertColumns, sMyKeyInsertOrder = 0;
              sInsertColumn != NULL;
              sInsertColumn = sInsertColumn->next, sMyKeyInsertOrder++ )
        {
            sColOrderOnTable = sInsertColumn->basicInfo->column.id & SMI_COLUMN_ID_MASK;

            if ( aShardObj->mKeyFlags[sColOrderOnTable] == 1 )
            {
                sKeyAppears = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do. */
            }
        }

        IDE_TEST_RAISE ( sKeyAppears == ID_FALSE, ERR_SHARD_KEY_NOT_EXIST );
    }
    else
    {
        sMyKeyInsertOrder = sMyKeyColOrder;
    }

    //------------------------------------------
    // make my ranges
    //------------------------------------------
    IDE_TEST( makeMyRanges( aStatement,
                            sMyNodeId,
                            sDefaultNodeId,
                            &aShardObj->mRangeInfo,
                            &sMyRanges )
              != IDE_SUCCESS );

    //------------------------------------------
    // make my key string
    //------------------------------------------
    IDE_TEST( makeKeyString( aStatement,
                             sMyKeyColName,
                             sMyKeyDataType,
                             sMyKeySplitMethod,
                             sMyKeyPrecision,
                             &sMyKeyString)
               != IDE_SUCCESS );

    //------------------------------------------
    // make my range string
    //------------------------------------------
    IDE_TEST( makeRangeString( aStatement,
                               sMyRanges,
                               sMyKeyDataType,
                               sMyKeySplitMethod,
                               sMyKeyString,
                               &sMyRangeString,
                               &sMyRangeStringLen )
              != IDE_SUCCESS );

    //------------------------------------------
    // parse partial
    //------------------------------------------
    sPosition.stmtText = sMyRangeString;
    sPosition.offset   = 0;
    sPosition.size = sMyRangeStringLen;

    if ( sPosition.size  > 0 )
    {
        QC_SET_STATEMENT( ( &sStatement ), aStatement, NULL );

        sStatement.myPlan->stmtText = sPosition.stmtText;
        sStatement.myPlan->stmtTextLen = idlOS::strlen( sPosition.stmtText );

        IDE_TEST( qcpManager::parsePartialForWhere( &sStatement,
                                                    sPosition.stmtText,
                                                    sPosition.offset,
                                                    sPosition.size )
                  != IDE_SUCCESS );

        aSFWGH->where = ((qmsParseTree*)sStatement.myPlan->parseTree)->querySet->SFWGH->where;

        //------------------------------------------
        // set column order for validation
        //------------------------------------------
        IDE_TEST( setColumnOrderForce( aSFWGH->where,
                                       sMyKeyColName,
                                       sMyKeyInsertOrder )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNSUPPORTED_SPLIT_TYPE_FOR_PARTIAL_COORD )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeRangeCondition",
                                  "Unsupported split type for partil coordinator execution" ));
    }
    IDE_EXCEPTION( ERR_SHARD_KEY_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeRangeCondition",
                                  "The shard key of the DML target table doesn't exist." ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeMyRanges( qcStatement  * aStatement,
                                        UInt           aMyNodeId,
                                        UInt           aDefaultNodeId,
                                        sdiRangeInfo * aRangeInfo,
                                        sdiMyRanges ** aMyRanges )
{
/****************************************************************************************
 *
 * Description : Partial coordinator transformation을 위해 sdiMyRanges를 구성한다.
 *
 * Implementation :
 *               e.x.
 *                 HASH_T1 : <200  <500  <800  Default
 *                            N1     N2    N3    N1
 *                           (me)
 *               mValueMin :   -    200   500   800
 *               mValueMax :  200   500   800    -
 *               mIsMyNode :   Y     N     N     Y
 *
 *               * List split 역시 동일한 규칙으로 자료구조를 설정한다. (mValueMax를 활용)
 *
 ****************************************************************************************/
    UShort        sRangeIdx    = 0;

    sdiValue    * sMinValue    = NULL;
    sdiValue    * sMaxValue    = NULL;

    sdiMyRanges * sCurrMyRange = NULL;
    sdiMyRanges * sPrevMyRange = NULL;

    for ( sRangeIdx = 0;
          sRangeIdx < aRangeInfo->mCount;
          sRangeIdx++ )
    {
        //------------------------------------------
        // set value MIN & MAX
        //------------------------------------------
        sMinValue = sMaxValue;
        sMaxValue = &aRangeInfo->mRanges[sRangeIdx].mValue;

        //------------------------------------------
        // set my node ranges
        //------------------------------------------
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                sdiMyRanges,
                                &sCurrMyRange )
                  != IDE_SUCCESS );

        sCurrMyRange->mValueMin = sMinValue;
        sCurrMyRange->mValueMax = sMaxValue;

        if ( aMyNodeId == aRangeInfo->mRanges[sRangeIdx].mNodeId )
        {
            sCurrMyRange->mIsMyNode = ID_TRUE;
        }
        else
        {
            sCurrMyRange->mIsMyNode = ID_FALSE;
        }

        sCurrMyRange->mNext = NULL;

        if ( *aMyRanges == NULL )
        {
            *aMyRanges = sCurrMyRange;
        }
        else
        {
            sPrevMyRange->mNext = sCurrMyRange;
        }

        sPrevMyRange = sCurrMyRange;
    }

    if ( aDefaultNodeId != ID_UINT_MAX )
    {
        //------------------------------------------
        // set default range
        //------------------------------------------
        sMinValue = sMaxValue;
        sMaxValue = NULL;

        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                sdiMyRanges,
                                &sCurrMyRange )
                  != IDE_SUCCESS );

        sCurrMyRange->mValueMin = sMinValue;
        sCurrMyRange->mValueMax = sMaxValue;

        sCurrMyRange->mNext = NULL;

        if ( aMyNodeId == aDefaultNodeId )
        {
            sCurrMyRange->mIsMyNode = ID_TRUE;
        }
        else
        {
            sCurrMyRange->mIsMyNode = ID_FALSE;
        }

        if ( *aMyRanges == NULL )
        {
            *aMyRanges = sCurrMyRange;
        }
        else
        {
            sPrevMyRange->mNext = sCurrMyRange;
        }

        sPrevMyRange = sCurrMyRange;
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeKeyString( qcStatement     * aStatement,
                                         SChar           * aMyKeyColName,
                                         UInt              aMyKeyDataType,
                                         sdiSplitMethod    aMyKeySplitMethod,
                                         SInt              aMyKeyPrecision,
                                         SChar          ** aMyKeyString )
{
/****************************************************************************************
 *
 * Description : 임의의 data filter string을 생성하기 위해
 *               Split method, shard key의 data type 및 precision을 고려하여
 *               Key string을 생성한다.
 *
 * Implementation :
 *               e.x.
 *                     SPLIT HASH  : MOD( HASH( CAST( sKeyColName AS TYPE(PRECISION) ), 1000 )
 *                           LIST  :
 *                           RANGE : CAST( sKeyColName AS TYPE(PRECISION) )
 *
 ****************************************************************************************/
    UInt    sLen = 0;
    UInt    sLenMax = 0;
    SChar * sMyKeyString = NULL;

    /* CHAR/VARCHAR precision maxlen = 5 (32000 or 65534)
     *
     * MOD( HASH( CAST( sKeyColName AS TYPE(PRECISION) ), 1000 )'\0'
     * -------------------------------------------------------------
     *  3 11 4  11  4 11   L1      1 21  8 1   20    11111  4 11 1
     *
     * sKeyColName : QC_MAX_OBJECT_NAME_LEN + 1
     * TYPE        : 8 MAX_LEN('VARCHAR', 'CHAR', 'BIGINT', 'INTEGER', 'SMALLINT')
     * PRECISION   : 5
     */
    sLenMax = 3+1+1+4+1+1+4+1+1+(QC_MAX_OBJECT_NAME_LEN)+1+2+1+8+1+20+1+1+1+1+1+4+1+1+1;

    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sLenMax,
                                             (void**) &sMyKeyString )
              != IDE_SUCCESS );

    switch( aMyKeySplitMethod )
    {
        case SDI_SPLIT_HASH :
            sLen +=
                idlOS::snprintf( sMyKeyString + sLen,
                                 sLenMax - sLen,
                                 "MOD( " );
            sLen +=
                idlOS::snprintf( sMyKeyString + sLen,
                                 sLenMax - sLen,
                                 "HASH( " );
            /* fall through */
        case SDI_SPLIT_RANGE :
        case SDI_SPLIT_LIST :
            sLen +=
                idlOS::snprintf( sMyKeyString + sLen,
                                 sLenMax - sLen,
                                 "CAST( " );
            sLen +=
                idlOS::snprintf( sMyKeyString + sLen,
                                 sLenMax - sLen,
                                 aMyKeyColName );

            sLen +=
                idlOS::snprintf( sMyKeyString + sLen,
                                 sLenMax - sLen,
                                 " AS " );

            switch( aMyKeyDataType )
            {
                case MTD_CHAR_ID :
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "CHAR( " );
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "%d",
                                         aMyKeyPrecision );
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         ")" );
                    break;
                case MTD_VARCHAR_ID :
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "VARCHAR(" );
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "%d",
                                         aMyKeyPrecision );
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         ")" );
                    break;
                case MTD_SMALLINT_ID :
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "SMALLINT " );
                    break;
                case MTD_BIGINT_ID :
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "BIGINT " );
                    break;
                case MTD_INTEGER_ID :
                    sLen +=
                        idlOS::snprintf( sMyKeyString + sLen,
                                         sLenMax - sLen,
                                         "INTEGER " );
                    break;
                default :
                    IDE_DASSERT(0);
            }

            sLen +=
                idlOS::snprintf( sMyKeyString + sLen,
                                 sLenMax - sLen,
                                 ")" );
            break;
        case SDI_SPLIT_CLONE :
        case SDI_SPLIT_SOLO :
        default :
            IDE_RAISE(ERR_UNEXPECTED_SPLIT_TYPE_FOR_PARTIAL_TRANSFORM);
            break;
    }

    if ( aMyKeySplitMethod == SDI_SPLIT_HASH )
    {
        sLen +=
            idlOS::snprintf( sMyKeyString + sLen,
                             sLenMax - sLen,
                             "), 1000)" );
    }
    else
    {
        /* Nothing to do. */
    }

    *aMyKeyString = sMyKeyString;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_SPLIT_TYPE_FOR_PARTIAL_TRANSFORM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::makeKeyString",
                                  "Unexpected split type for partial coordinator transformation" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeRangeString( qcStatement     * aStatement,
                                           sdiMyRanges     * aMyRanges,
                                           UInt              aMyKeyDataType,
                                           sdiSplitMethod    aMyKeySplitMethod,
                                           SChar           * aMyKeyString,
                                           SChar          ** aMyRangeString,
                                           UInt            * aMyRangeStringLen )
{
/****************************************************************************************
 *
 * Description : myKeyString 을 활용해 myRanges 에 해당하는 분산 data filter string을 생성한다.
 *
 * Implementation :
 *
 ****************************************************************************************/
    SChar       * sMyRangeString = NULL;
    UInt          sLen = 0;
    UInt          sLenForChecking = 0;

    UInt          sLenMax = (SDU_SHARD_TRANSFORM_STRING_LENGTH_MAX+1);

    sdiMyRanges * sMyRange = NULL;
    sdiMyRanges * sMyDefaultList = NULL;

    UInt sKeyCompareType = ( aMyKeySplitMethod == SDI_SPLIT_HASH )?MTD_INTEGER_ID:aMyKeyDataType;

    IDE_TEST(STRUCT_ALLOC_WITH_COUNT(QC_QMP_MEM(aStatement),
                                     SChar,
                                     sLenMax,
                                     &sMyRangeString)
             != IDE_SUCCESS)

    for ( sMyRange  = aMyRanges;
          sMyRange != NULL;
          sMyRange  = sMyRange->mNext )
    {
        if ( ( sMyRange->mIsMyNode == ID_TRUE ) &&
             ( ( sMyRange->mValueMin != NULL ) ||
               ( sMyRange->mValueMax != NULL ) ) )
        {
            if ( sLen != sLenForChecking )
            {
                // 두번째 myRange 부터는 OR를 이어서 붙이고 시작한다.
                //------------------------------------------
                // ( KEY < 300 ) OR
                //               ^
                //------------------------------------------
                sLen +=
                    idlOS::snprintf( sMyRangeString + sLen,
                                     sLenMax - sLen,
                                     " OR " );
            }

            sLen +=
                idlOS::snprintf( sMyRangeString + sLen,
                                 sLenMax - sLen,
                                 "( " );

            if ( sMyRange->mValueMin != NULL )
            {
                if ( aMyKeySplitMethod != SDI_SPLIT_LIST )
                {
                    //------------------------------------------
                    // case 1.
                    // ...OR ( 600 <= KEY AND 900 > KEY )...
                    //         ^            ^
                    // case 2. ( Default node for HASH/RANGE
                    // ...OR ( 900 <= KEY )
                    //         ^        ^
                    //------------------------------------------
                    IDE_TEST( makeMinValueString( sKeyCompareType,
                                                  (const SChar*)" >= ",
                                                  sMyRange->mValueMin,
                                                  aMyKeyString,
                                                  &sMyRangeString,
                                                  &sLen,
                                                  sLenMax )
                              != IDE_SUCCESS );

                    if ( sMyRange->mValueMax != NULL )
                    {
                        sLen +=
                            idlOS::snprintf( sMyRangeString + sLen,
                                             sLenMax - sLen,
                                             " AND " );
                    }
                    else
                    {
                        if ( aMyKeySplitMethod == SDI_SPLIT_RANGE )
                        {
                            sLen +=
                                idlOS::snprintf( sMyRangeString + sLen,
                                                 sLenMax - sLen,
                                                 " OR " );

                            sLen +=
                                idlOS::snprintf( sMyRangeString + sLen,
                                                 sLenMax - sLen,
                                                 "( " );

                            sLen +=
                                idlOS::snprintf( sMyRangeString + sLen,
                                                 sLenMax - sLen,
                                                 "%s IS NULL",
                                                 aMyKeyString );

                            sLen +=
                                idlOS::snprintf( sMyRangeString + sLen,
                                                 sLenMax - sLen,
                                                 ") " );
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                }
                else
                {
                    /* SDI_SPLIT_LIST */
                    if ( sMyRange->mValueMax == NULL )
                    {
                        //------------------------------------------
                        // ...OR ( 'A' != KEY AND 'B' != KEY AND C != KEY )...
                        //         ^                                    ^
                        //------------------------------------------
                        sLenForChecking = sLen;

                        for ( sMyDefaultList  = aMyRanges;
                              sMyDefaultList != NULL;
                              sMyDefaultList  = sMyDefaultList->mNext )
                        {
                            if ( sMyDefaultList->mIsMyNode == ID_FALSE )
                            {
                                if ( sLenForChecking != sLen )
                                {
                                    //------------------------------------------
                                    // ( 'A' != KEY AND
                                    //              ^ ^
                                    //------------------------------------------
                                    sLen +=
                                        idlOS::snprintf( sMyRangeString + sLen,
                                                         sLenMax - sLen,
                                                         " AND " );
                                }

                                IDE_TEST( makeMinValueString( sKeyCompareType,
                                                              (const SChar*)" != ",
                                                              sMyDefaultList->mValueMax,
                                                              aMyKeyString,
                                                              &sMyRangeString,
                                                              &sLen,
                                                              sLenMax )
                                          != IDE_SUCCESS );
                            }
                        }

                        if ( sLenForChecking != sLen )
                        {
                            sLen +=
                                idlOS::snprintf( sMyRangeString + sLen,
                                                 sLenMax - sLen,
                                                 " OR " );
                        }

                        sLen +=
                            idlOS::snprintf( sMyRangeString + sLen,
                                             sLenMax - sLen,
                                             "( " );

                        sLen +=
                            idlOS::snprintf( sMyRangeString + sLen,
                                             sLenMax - sLen,
                                             "%s IS NULL",
                                             aMyKeyString );

                        sLen +=
                            idlOS::snprintf( sMyRangeString + sLen,
                                             sLenMax - sLen,
                                             ") " );
                    }
                }
            }

            if ( sMyRange->mValueMax != NULL )
            {
                if ( aMyKeySplitMethod == SDI_SPLIT_LIST )
                {
                    IDE_TEST( makeMinValueString( sKeyCompareType,
                                                  (const SChar*)" = ",
                                                  sMyRange->mValueMax,
                                                  aMyKeyString,
                                                  &sMyRangeString,
                                                  &sLen,
                                                  sLenMax )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( makeMinValueString( sKeyCompareType,
                                                  (const SChar*)" < ",
                                                  sMyRange->mValueMax,
                                                  aMyKeyString,
                                                  &sMyRangeString,
                                                  &sLen,
                                                  sLenMax )
                              != IDE_SUCCESS );
                }
            }

            sLen +=
                idlOS::snprintf( sMyRangeString + sLen,
                                 sLenMax - sLen,
                                 " )" );
        }
        else
        {
            /* Nothing to do. */
        }
    }

    IDE_TEST_RAISE( sLen >= ( sLenMax - 1 ), ERR_TRANSFORM_STRING_BUFFER_OVERFLOW );

    *aMyRangeString = sMyRangeString;
    *aMyRangeStringLen = sLen;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANSFORM_STRING_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "Transformation string buffer overflow",
                                  "check the system property SHARD_TRANSFORM_STRING_LENGTH_MAX" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::makeMinValueString( UInt              aCompareType,
                                              const SChar     * aCompareString,
                                              sdiValue        * aValue,
                                              SChar           * aMyKeyString,
                                              SChar          ** aMyRangeString,
                                              UInt            * aLen,
                                              UInt              aLenMax )
{
    sdiValue   sValueStr;

    IDE_TEST( sdi::getValueStr( aCompareType,
                                aValue,
                                &sValueStr )
              != IDE_SUCCESS );

    *aLen +=
        idlOS::snprintf( *aMyRangeString + *aLen,
                         aLenMax - *aLen,
                         "%s",
                         aMyKeyString );

    *aLen +=
        idlOS::snprintf( *aMyRangeString + *aLen,
                         aLenMax - *aLen,
                         "%s",
                         aCompareString );

    if ( ( aCompareType == MTD_CHAR_ID ) ||
         ( aCompareType == MTD_VARCHAR_ID ) )
    {
        (*aMyRangeString)[(*aLen)++] = '\'';
    }

    *aLen +=
        idlOS::snprintf( *aMyRangeString + *aLen,
                         aLenMax - *aLen,
                         "%.*s",
                         sValueStr.mCharMax.length,
                         (SChar*)sValueStr.mCharMax.value );

    if ( ( aCompareType == MTD_CHAR_ID ) ||
         ( aCompareType == MTD_VARCHAR_ID ) )
    {
        (*aMyRangeString)[(*aLen)++] = '\'';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvShardTransform::setColumnOrderForce( qtcNode * aNode,
                                               SChar   * aMyKeyColName,
                                               UShort    aMyKeyColOrder )
{
/****************************************************************************************
 *
 * Description : 임의로 생성한 data filter string 에 대한 자료구조가 validation 때
 *               지정된 위치의 target column을 지칭하도록 estimate 되게 하기 위해
 *               shardViewTargetPos를 세팅한다.
 *
 * Implementation :
 *               e.x.
 *                 INSERT INTO HASH_T1(I1,I2) SELECT I2,I1        ___________qmvQtc::setColumnID()_______
 *                                              FROM HASH_T1;    /                                       |
 *                 ->                                            v                                       |
 *                 INSERT INTO HASH_T1(I1,I2) SELECT *        ------                                     |
 *                                     --       FROM ( SELECT SUM(I2),AVG(I2) FROM HASH_T1 )             |
 *                                             WHERE MOD(HASH(CAST(   I1' AS VARCHAR(10) )),1000) < 300; |
 *                                                                    ---                                |
 *                                                                     \_________________________________|
 *                                                            shardViewTargetPos : 0 (첫 번째 target)
 *
 ****************************************************************************************/
    if ( aNode != NULL )
    {
        if ( aNode->node.module == &qtc::columnModule )
        {
            if ( idlOS::strncmp( aMyKeyColName,
                                 aNode->columnName.stmtText + aNode->columnName.offset,
                                 aNode->columnName.size ) == 0 )
            {
                aNode->shardViewTargetPos = aMyKeyColOrder;
            }
            else
            {
                IDE_RAISE( ERR_INVALID_DATA_FILTER_STRING ) 
            }
        }
        else
        {
            /* Nothing to do. */
        }

        setColumnOrderForce( (qtcNode*)aNode->node.arguments,
                             aMyKeyColName,
                             aMyKeyColOrder );

        setColumnOrderForce( (qtcNode*)aNode->node.next,
                             aMyKeyColName,
                             aMyKeyColOrder );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_FILTER_STRING )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvShardTransform::setColumnOrderForce",
                                  "Invalid data filter string was generated." ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
