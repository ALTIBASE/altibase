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
 * $Id: qmvPivotTransform.cpp 90008 2021-02-17 06:03:16Z andrew.shin $
 *
 * Pivot Transform 은 크게 2가지 부분으로 나뉜다.
 * 1. validateQmsTableRef 단계에서 Pivot 구문을 해석해서 기존 TableRef->view에
 *    pivot용 parse tree를 생성해서 연결하는 부분이다.
 *    이때 새로 생성된 parse tree의 Target에는 Dummy target과
 *    여러 DECODE_XXX target로 구성되어 있다.
 *
 * 2. Target을 validation하기 전 즉 qmsValidateTarget 함수가 호출된기
 *    전에 Dummy target을 Expand하는 부분이다.
 *    이때 전체 Table의 Column중 Pivot 구문에 쓰이지 않은 Column은
 *    Target과 Group by의 Node에 추가된다.
 *
 *  Pivot 구문은 예를 들면 다음과 같이 변환된다.
 *  create table emp(
 *     empno     number(4) constraint pk_emp primary key,
 *     ename     varchar(10),
 *     job       varchar(9),
 *     mgr       number(4),
 *     hiredate  date,
 *     sal       number(7,2),
 *     comm      number(7,2),
 *     deptno    number(2)
 *    );
 *  select * from emp pivot ( sum(sal) for deptno in (10,20,30));
 *
 *  select * from ( select empno, ename, job, mgr, hiredate, comm,
 *                         NTH_ELEMENT("$$PIVOT1",1) '10', 
 *                         NTH_ELEMENT("$$PIVOT1",2) '20', 
 *                         NTH_ELEMENT("$$PIVOT1",3) '30'
 *                  from ( select empno, ename, job, mgr, hiredate, comm,
 *                         DECODE_SUM_LIST(sal, deptno, (10,20,30)) "$$PIVOT1"
 *                         from emp
 *                         group by empno, ename, job, mgr, hiredate, comm
 *                       )
 *                );
 *
 **********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qmvOrderBy.h>
#include <qmvPivotTransform.h>
#include <qmvQuerySet.h>
#include <qmv.h>
#include <qmo.h>
#include <qmx.h>

extern mtfModule mtfList;
extern mtfModule mtfNth_element;

IDE_RC qmvPivotTransform::createFirstAlias( qcStatement     * aStatement,
                                            qmsNamePosition * aAlias,
                                            UInt              aOrder )
{
    IDU_FIT_POINT_FATAL( "qmvPivotTransform::createFirstAlias::__FT__" );

    /* Alloc Memeory */
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                      SChar,
                                      QC_MAX_OBJECT_NAME_LEN + 1,
                                      &(aAlias->name) )
              != IDE_SUCCESS );

    idlOS::snprintf( aAlias->name,
                     QC_MAX_OBJECT_NAME_LEN + 1,
                     "$$PIVOT%"ID_UINT32_FMT,
                     aOrder );

    aAlias->size = idlOS::strlen( aAlias->name );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Make Pivot Column Name
 *
 * Pivot Clause Structure
 * select ... from [Table] pivot (     [Aggr] as Alias, ...
 *                                 FOR [column]
 *                                 IN  ( [InValues] as Alias, ... )
 *                               )
 *
 *  Pivot Column Name makeing rules
 *  |-----------------------------------------------------------------
 *  |Aggr aliasing | In value alising | Pivot Column Name
 *  |-----------------------------------------------------------------
 *  | No           | No               | Pivot_in value
 *  |-----------------------------------------------------------------
 *  | Yes          | Yes              | Pivot_in alias + '_'
 *  |              |                  |   + Pivot_aggr alias
 *  |-----------------------------------------------------------------
 *  | No           | Yes              | Pivot_in alias
 *  |-----------------------------------------------------------------
 *  | Yes          | No               | Pivot_in value + '_'
 *  |              |                  |   + pivot_aggr alias
 *  |-----------------------------------------------------------------
 *
 *  1. calculate total pivot alias size.
 *  2. alloc memory.
 *  3. memcpy relative alias or value.
 *
 * @param aStatement Statement pointer
 * @param aAlias     A pointer that maked pivot column name.
 * @param aAggr      Pivot AggregationNode Pointer
 * @param aValueNode Pivot In Node Pointer
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::createSecondAlias( qcStatement     * aStatement,
                                             qmsNamePosition * aAlias,
                                             qmsPivotAggr    * aAggr,
                                             qtcNode         * aValueNode )
{
    UInt  sSize;
    UInt  sPos;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::createSecondAlias::__FT__" );

    if ( QC_IS_NULL_NAME( aValueNode->userName ) == ID_TRUE )
    {
        sSize = aValueNode->columnName.size;
    }
    else
    {
        sSize = aValueNode->userName.size;
    }

    if ( QC_IS_NULL_NAME( aAggr->aliasName ) == ID_FALSE )
    {
        ++sSize;
        sSize += aAggr->aliasName.size;
    }
    else
    {
        /* Nothing to do */
    }

    /* Alloc Memeory */
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                      SChar,
                                      sSize + 1,
                                      &(aAlias->name) )
              != IDE_SUCCESS );
    
    aAlias->size = sSize;
    aAlias->name[sSize] = '\0';

    if ( QC_IS_NULL_NAME( aValueNode->userName ) == ID_TRUE )
    {
        QC_STR_COPY( aAlias->name, aValueNode->columnName );
        sPos = aValueNode->columnName.size;
    }
    else
    {
        QC_STR_COPY( aAlias->name, aValueNode->userName );
        sPos = aValueNode->userName.size;
    }

    if ( QC_IS_NULL_NAME( aAggr->aliasName ) == ID_FALSE )
    {
        aAlias->name[sPos] = '_';
        ++sPos;

        QC_STR_COPY( aAlias->name + sPos, aAggr->aliasName );
        sPos += aAggr->aliasName.size;
    }
    else
    {
        /* Nothing to do */
    }

    // BUG-40977
    if ( aAlias->size > QC_MAX_OBJECT_NAME_LEN )
    {
        aAlias->size = QC_MAX_OBJECT_NAME_LEN;
        aAlias->name[QC_MAX_OBJECT_NAME_LEN] = '\0';
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Check pivot syntax.
 *
 *  1. Pivot Aggregation is possible Only 7 Functions.
 *     - COUNT, MIN, SUM, AVG, MAX, STDDEV, VARIANCE
 *
 *  2. Only COUNT function allow asterisk.
 *     - PIVOT COUNT(*) FOR ...
 *
 *  3. Pivot Aggregation alias dose not allow duplicate
 *
 *  4. Pivot in alias does not allow duplicate.
 *
 *  5. A number of for element is same a number of in element.
 *
 * @param aStatement  Pointer of Statement
 * @param aPivot      Pointer of pivot structure
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::checkPivotSyntax( qcStatement * aStatement,
                                            qmsPivot    * aPivot )
{
    qmsPivotAggr    * sAggrNodes  = NULL;
    qmsPivotAggr    * sAggrTmp    = NULL;
    qtcNode         * sValueNode  = NULL;
    qtcNode         * sValueTmp   = NULL;
    qcNamePosition    sName;
    qcNamePosition    sName2;
    const mtfModule * sModule     = NULL;
    qtcNode         * sNode       = NULL;
    SChar             sTextTmp[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar             sText[QC_MAX_OBJECT_NAME_LEN + 1];
    qcNamePosition    sPosition;
    idBool            sExist;
    qcuSqlSourceInfo  sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::checkPivotSyntax::__FT__" );

    for ( sAggrNodes = aPivot->aggrNodes;
          sAggrNodes != NULL;
          sAggrNodes = sAggrNodes->next )
    {
        sName = sAggrNodes->aggrFunc;

        QC_STR_COPY( sTextTmp, sName );
        
        idlOS::snprintf( sText,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "DECODE_%s_LIST",
                         sTextTmp );
        
        if ( idlOS::strMatch( sName.stmtText + sName.offset,
                              sName.size,
                              "COUNT",
                              5 ) == 0 )
        {
            sNode = sAggrNodes->node;
            
            if ( idlOS::strMatch( sNode->columnName.stmtText + sNode->columnName.offset,
                                  sNode->columnName.size,
                                  "*",
                                  1 ) == 0 )
            {
                if ( ( sNode->node.next == NULL ) &&
                     ( sNode->node.arguments == NULL ) )
                {
                    SET_EMPTY_POSITION(sPosition);
                    
                    IDE_TEST(qtc::makeValue( aStatement,
                                             &sAggrNodes->node,
                                             (const UChar *)"SMALLINT",
                                             8,
                                             &sPosition,
                                             (const UChar *)"1",
                                             1,
                                             MTC_COLUMN_NORMAL_LITERAL )
                             != IDE_SUCCESS);
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
            /* Nothing to do */
        }

        IDE_TEST( mtf::moduleByName( &sModule,
                                     &sExist,
                                     (void *) sText,
                                     idlOS::strlen( sText ) ) != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_FALSE,
                        ERR_NOT_AGGREGATION );
        
        sName = sAggrNodes->aliasName;
        
        if ( QC_IS_NULL_NAME( sName ) == ID_FALSE )
        {
            sAggrTmp = sAggrNodes->next;
        
            for ( ; sAggrTmp != NULL; sAggrTmp = sAggrTmp->next )
            {
                sName2 = sAggrTmp->aliasName;
            
                if ( QC_IS_NULL_NAME( sName2 ) == ID_FALSE )
                {
                    if ( QC_IS_NAME_MATCHED( sName, sName2 ) == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement, &sName2 );
                        IDE_RAISE( ERR_DUPLICATE_ALIAS );
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
            /* Nothing to do */
        }
    }

    if ( aPivot->valueNode->node.module == &mtfList )
    {
        for ( sValueNode = (qtcNode*)aPivot->valueNode->node.arguments;
              sValueNode != NULL;
              sValueNode = (qtcNode*)sValueNode->node.next )
        {
            // aliasName을 userName에 기록했다.
            sName = sValueNode->userName;
            
            if ( QC_IS_NULL_NAME( sName ) == ID_FALSE )
            {
                for ( sValueTmp = (qtcNode*)sValueNode->node.next;
                      sValueTmp != NULL;
                      sValueTmp = (qtcNode*)sValueTmp->node.next )
                {
                    sName2 = sValueTmp->userName;
                
                    if ( QC_IS_NULL_NAME( sName2 ) == ID_FALSE )
                    {
                        if ( QC_IS_NAME_MATCHED( sName, sName2 ) == ID_TRUE )
                        {
                            sqlInfo.setSourceInfo( aStatement, &sName2 );
                            IDE_RAISE( ERR_DUPLICATE_ALIAS );
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
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_AGGREGATION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_PIVOT_AGGREGATION));
    }
    IDE_EXCEPTION(ERR_DUPLICATE_ALIAS);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_DUPLICATE_ALIAS,
                                 sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Create Pivot Parse Tree.
 *
 *  This parse tree is like new inline view parse tree.
 *  If aTableRef has already inline view,
 *    tableRef of new parse tree has existed inline view parse tree.
 *
 * @param aStatement Pointer of Statement
 * @param aTableRef  Pointer of qmsTablfRef structure
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::createPivotParseTree( qcStatement * aStatement,
                                                qmsTableRef * aTableRef )
{
    qcStatement  * sStatement = NULL;
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsSFWGH     * sSFWGH     = NULL;
    qmsFrom      * sFrom      = NULL;
    qmsTableRef  * sTableRef  = NULL;
    idBool         sNoMergeHint = ID_FALSE;
    qcNamePosition sNullPosition;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::createPivotParseTree::__FT__" );

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

    /* PIVOT flag setting */
    sSFWGH->lflag &= ~QMV_SFWGH_PIVOT_MASK;
    sSFWGH->lflag |= QMV_SFWGH_PIVOT_TRUE;

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

    sNoMergeHint = aTableRef->noMergeHint;

    /* Copy existed TableRef to new TableRef except pivot and alias */
    idlOS::memcpy( sTableRef, aTableRef, ID_SIZEOF( qmsTableRef ) );
    QCP_SET_INIT_QMS_TABLE_REF(aTableRef);

    /* Set alias */
    SET_POSITION(aTableRef->aliasName, sTableRef->aliasName);

    /* BUG-48544 Pivot 구문 집계 함수에 Inline View Alias, With Alias 인 Coulmn 또는 Subquery 를 사용하는 경우 오류가 발생합니다. */
    if ( sTableRef->view == NULL )
    {
        SET_EMPTY_POSITION( sTableRef->aliasName );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2415 Grouping Sets Clause
    aTableRef->noMergeHint = sNoMergeHint;

    /* Set pivot */
    aTableRef->pivot = sTableRef->pivot;
    sTableRef->pivot = NULL;

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

    /* Set transformed inline view */
    aTableRef->view = sStatement;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Serarch Column in Pivot Aggregation Expression.
 *
 * @param aStatement Pointer of Statement
 * @param aPrevMtc   Previous Column list
 * @param aMtcNode   Current Search Column
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::searchPivotAggrColumn( qcStatement * aStatement,
                                                 mtcNode     * aPrevMtc,
                                                 mtcNode     * aMtcNode )
{
    qtcNode      * sNode       = NULL;
    qtcNode      * sNodeTmp    = NULL;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::searchPivotAggrColumn::__FT__" );

    if ( aMtcNode != NULL )
    {
        if ( aMtcNode->module == &(qtc::columnModule) )
        {
            sNode = (qtcNode *)aMtcNode;
            IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                   qtcNode,
                                   &sNodeTmp )
                     != IDE_SUCCESS );
            idlOS::memcpy( sNodeTmp, sNode, ID_SIZEOF( qtcNode ) );
            sNodeTmp->node.next = NULL;
            sNodeTmp->node.arguments = NULL;
            aPrevMtc->next = &sNodeTmp->node;
            aPrevMtc = aPrevMtc->next;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aMtcNode->next != NULL )
        {
            IDE_TEST(searchPivotAggrColumn( aStatement,
                                            aPrevMtc,
                                            aMtcNode->next )
                     != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }
        if ( aMtcNode->arguments != NULL )
        {
            IDE_TEST(searchPivotAggrColumn( aStatement,
                                            aPrevMtc,
                                            aMtcNode->arguments )
                     != IDE_SUCCESS);
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

/**
 * Create dummy target to target of new parse tree.
 *
 *  Dummy target has many qtc column node.
 *  This columns are used to inside of pivot clause.
 *
 *  1. make dummy target.
 *  2. search column of pivot aggregation and pivot for
 *  3. if column node found, add new column list.
 *  4. maked column list add dumy target.
 *
 * @param aStatement A pointer of statement.
 * @param aQueyrSet  New qmsQuerySet of piovt
 * @param aTableRef  A pointer of existed Table Ref
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::createDummyTarget( qcStatement * aStatement,
                                             qmsQuerySet * aQuerySet,
                                             qmsTableRef * aTableRef )
{
    qmsPivot     * sPivot      = NULL;
    qmsPivotAggr * sAggrNodes  = NULL;
    qmsTarget    * sTarget     = NULL;
    mtcNode      * sMtcNode    = NULL;
    mtcNode      * sPrevMtc    = NULL;
    mtcNode        sDummy;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::createDummyTarget::__FT__" );

    IDE_DASSERT( aTableRef->pivot != NULL );

    sPivot      = aTableRef->pivot;
    sPrevMtc    = &sDummy;
    sDummy.next = NULL;

    IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                           qmsTarget,
                           &sTarget )
             != IDE_SUCCESS);
    QMS_TARGET_INIT(sTarget);

    sTarget->flag |= QMS_TARGET_DUMMY_PIVOT_TRUE;

    IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                           qtcNode,
                           &sTarget->targetColumn)
             != IDE_SUCCESS );
    QTC_NODE_INIT(sTarget->targetColumn);

    /* search column in pivot aggr expression */
    for ( sAggrNodes = sPivot->aggrNodes;
          sAggrNodes != NULL;
          sAggrNodes = sAggrNodes->next )
    {
        sMtcNode = (mtcNode*)(sAggrNodes->node);
        IDE_TEST(searchPivotAggrColumn( aStatement,
                                        sPrevMtc,
                                        sMtcNode )
                 != IDE_SUCCESS);
        while ( sPrevMtc->next != NULL )
        {
            sPrevMtc = sPrevMtc->next;
        }
    }
    
    /* search column in pivot for */
    sPrevMtc->next = (mtcNode*)(sPivot->columnNode);
    
    /* Set column list to arguments of Dummy */
    sTarget->targetColumn->node.arguments = sDummy.next;

    /* Set Dummy Target */
    aQuerySet->SFWGH->target = sTarget;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Add Transformed Pivot Target.
 *  Pivot Clause transform DECODE Aggregation Clause.
 *
 * Pivot Transform는 Pivot Aggr과 Pivot In의 조합 만큼 DECODE_XXX Target
 * 을 생성해서 Target에 추가한다.
 *  1. Pivot Aggr구문에서 DECODE_XXX 함수를 구한다.
 *  2. Pivot in 과 Pivot aggr의 조합에 의한 alias를 생성한다.
 *  3. Target을 생성해서 추가해준다.
 *  하나의 (Pivot_In) 항목 대해 (Pivot_Aggr)이 모두 나오고
 *  그 다음 (Pivot_In)값이 나오게 된다.
 *
 * @param aStatement A pointer of statement
 * @param aSFWGH     A pointer of aSFWGH
 * @param aTableRef  A pointer of qmsTableRef
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::createPivotFirstTarget( qcStatement * aStatement,
                                                  qmsSFWGH    * aSFWGH,
                                                  qmsTableRef * aTableRef )
{
    qmsPivot        * sPivot;
    qmsTarget       * sPrevTarget;
    qmsTarget       * sTarget;
    qtcNode         * sForNode;
    qtcNode         * sValueNode = NULL;
    const mtfModule * sModule;
    qmsPivotAggr    * sPivotAggrNode;
    SChar             sTextTmp[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar             sText[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt              sAggrCount;
    idBool            sExist;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::createPivotFirstTarget::__FT__" );

    IDE_DASSERT( aTableRef->pivot != NULL );
    IDE_DASSERT( aSFWGH->target != NULL );
    
    sPivot      = aTableRef->pivot;
    sPrevTarget = aSFWGH->target;

    IDE_DASSERT( ( sPrevTarget->flag & QMS_TARGET_DUMMY_PIVOT_MASK )
                 == QMS_TARGET_DUMMY_PIVOT_TRUE );
    
    for ( sPivotAggrNode = sPivot->aggrNodes, sAggrCount = 1;
          sPivotAggrNode != NULL;
          sPivotAggrNode = sPivotAggrNode->next, sAggrCount++ )
    {
        QC_STR_COPY( sTextTmp, sPivotAggrNode->aggrFunc );
        
        idlOS::snprintf( sText,
                         QC_MAX_OBJECT_NAME_LEN + 1,
                         "DECODE_%s_LIST",
                         sTextTmp );

        IDE_TEST(mtf::moduleByName( &sModule,
                                    &sExist,
                                    (void *) sText,
                                    idlOS::strlen( sText ) )
                 != IDE_SUCCESS );
        IDE_TEST_RAISE( sExist == ID_FALSE,
                        ERR_NOT_AGGREGATION );
                     
        /* Alloc Target */
        IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                               qmsTarget,
                               &sTarget )
                 != IDE_SUCCESS );
        QMS_TARGET_INIT(sTarget);

        /* Create Alias */
        IDE_TEST(createFirstAlias( aStatement,
                                   &(sTarget->aliasColumnName),
                                   sAggrCount )
                 != IDE_SUCCESS );
        sTarget->displayName = sTarget->aliasColumnName;
        
        /* Make DECODE_XXX_LIST function */
        IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                               qtcNode,
                               &sTarget->targetColumn )
                 != IDE_SUCCESS );
        QTC_NODE_INIT( sTarget->targetColumn );
        sTarget->targetColumn->columnName.stmtText = sTarget->aliasColumnName.name;
        sTarget->targetColumn->columnName.offset   = 0;
        sTarget->targetColumn->columnName.size     = sTarget->aliasColumnName.size;
        
        sTarget->targetColumn->node.lflag   = 3;
        sTarget->targetColumn->node.lflag  |= sModule->lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
        sTarget->targetColumn->node.module  = sModule;

        /* Make first argument of DECODE_XXX_LIST function */
        /* sPivotAggrNode->node를 그대로 사용한다. */

        /* Make second argument of DECODE_XXX_LIST function */
        IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                               qtcNode,
                               &sForNode )
                 != IDE_SUCCESS );
        idlOS::memcpy( sForNode,
                       sPivot->columnNode,
                       ID_SIZEOF(qtcNode) );
        
        /* Make third argument of DECODE_XXX_LIST function */
        IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM( aStatement ),
                                         sPivot->valueNode,
                                         &sValueNode,
                                         ID_FALSE,
                                         ID_TRUE,
                                         ID_TRUE,
                                         ID_TRUE )
                  != IDE_SUCCESS );
        
        /* Link arguments of DECODE_XXX_LIST function */
        sTarget->targetColumn->node.arguments = (mtcNode*)sPivotAggrNode->node;
        sTarget->targetColumn->node.arguments->next = (mtcNode*)sForNode;
        sTarget->targetColumn->node.arguments->next->next = (mtcNode*)sValueNode;

        IDE_TEST(qtc::nextColumn( QC_QMP_MEM(aStatement),
                                  sTarget->targetColumn,
                                  aStatement,
                                  QC_SHARED_TMPLATE(aStatement),
                                  MTC_TUPLE_TYPE_INTERMEDIATE,
                                  (UInt)(sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK) )
                 != IDE_SUCCESS);

        sTarget->targetColumn->position = sPivotAggrNode->aggrFunc;

        /* Append target */
        sPrevTarget->next = sTarget;
        sPrevTarget->targetColumn->node.next = &sTarget->targetColumn->node;
        sPrevTarget = sPrevTarget->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_AGGREGATION);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NOT_PIVOT_AGGREGATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * 두번째 view target을 생성한다.
 *
 * Pivot Transform는 Pivot Aggr과 Pivot In의 조합 만큼 NTH_ELEMENT Target을
 * 생성해서 Target에 추가한다.
 *
 * @param aStatement A pointer of statement
 * @param aSFWGH     A pointer of aSFWGH
 * @param aTableRef  A pointer of qmsTableRef
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::createPivotSecondTarget( qcStatement * aStatement,
                                                   qmsSFWGH    * aSFWGH,
                                                   qmsTableRef * aTableRef )
{
    qmsPivot        * sPivot;
    qmsTarget       * sPrevTarget;
    qmsTarget       * sTarget;
    qtcNode         * sDecodeNode[2];
    qtcNode         * sConstNode[2];
    qtcNode         * sPivotInNode;
    qmsNamePosition   sAlias;
    qcNamePosition    sPosition;
    qmsPivotAggr    * sPivotAggrNode;
    SChar             sValue[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt              sInCount;
    UInt              sAggrCount;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::createPivotSecondTarget::__FT__" );

    IDE_DASSERT( aTableRef->pivot != NULL );
    IDE_DASSERT( aSFWGH->target != NULL );
    
    sPivot      = aTableRef->pivot;
    sPrevTarget = aSFWGH->target;

    IDE_DASSERT( ( sPrevTarget->flag & QMS_TARGET_DUMMY_PIVOT_MASK )
                 == QMS_TARGET_DUMMY_PIVOT_TRUE );

    if ( sPivot->valueNode->node.module == &mtfList )
    {
        sPivotInNode = (qtcNode*)sPivot->valueNode->node.arguments;
    }
    else
    {
        sPivotInNode = sPivot->valueNode;
    }
    
    for ( sInCount = 1;
          sPivotInNode != NULL;
          sPivotInNode = (qtcNode*)sPivotInNode->node.next, sInCount++ )
    {
        for ( sPivotAggrNode = sPivot->aggrNodes, sAggrCount = 1;
              sPivotAggrNode != NULL;
              sPivotAggrNode = sPivotAggrNode->next, sAggrCount++ )
        {
            /* Alloc Target */
            IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                   qmsTarget,
                                   &sTarget )
                     != IDE_SUCCESS );
            QMS_TARGET_INIT(sTarget);

            /* Create Alias */
            IDE_TEST(createSecondAlias( aStatement,
                                        &(sTarget->aliasColumnName),
                                        sPivotAggrNode,
                                        sPivotInNode )
                     != IDE_SUCCESS );
            sTarget->displayName = sTarget->aliasColumnName;

            /* Make NTH_ELEMENT function */
            IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                   qtcNode,
                                   &sTarget->targetColumn )
                     != IDE_SUCCESS );
            QTC_NODE_INIT( sTarget->targetColumn );
            sTarget->targetColumn->columnName.stmtText = sTarget->aliasColumnName.name;
            sTarget->targetColumn->columnName.offset   = 0;
            sTarget->targetColumn->columnName.size     = sTarget->aliasColumnName.size;

            sTarget->targetColumn->node.lflag   = 2;
            sTarget->targetColumn->node.lflag  |= mtfNth_element.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
            sTarget->targetColumn->node.module  = &mtfNth_element;
            
            /* Make first argument of NTH_ELEMENT function */
            IDE_TEST( createFirstAlias( aStatement,
                                        & sAlias,
                                        sAggrCount )
                      != IDE_SUCCESS );

            sPosition.stmtText = sAlias.name;
            sPosition.offset   = 0;
            sPosition.size     = sAlias.size;
            
            IDE_TEST( qtc::makeColumn( aStatement,
                                       sDecodeNode,
                                       NULL,
                                       NULL,
                                       &sPosition,
                                       NULL )
                      != IDE_SUCCESS );
            
            /* Make second argument of DECODE_XXX_LIST function */
            idlOS::snprintf( sValue,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%"ID_UINT32_FMT,
                             sInCount );
            
            SET_EMPTY_POSITION( sPosition );

            IDE_TEST(qtc::makeValue( aStatement,
                                     sConstNode,
                                     (const UChar *)"INTEGER",
                                     7,
                                     &sPosition,
                                     (const UChar *)sValue,
                                     idlOS::strlen(sValue),
                                     MTC_COLUMN_NORMAL_LITERAL )
                     != IDE_SUCCESS);
            
            /* Link arguments of DECODE_XXX_LIST function */
            sTarget->targetColumn->node.arguments = (mtcNode*)sDecodeNode[0];
            sTarget->targetColumn->node.arguments->next = (mtcNode*)sConstNode[0];
            
            IDE_TEST(qtc::nextColumn( QC_QMP_MEM(aStatement),
                                      sTarget->targetColumn,
                                      aStatement,
                                      QC_SHARED_TMPLATE(aStatement),
                                      MTC_TUPLE_TYPE_INTERMEDIATE,
                                      mtfNth_element.lflag &
                                      MTC_NODE_COLUMN_COUNT_MASK )
                     != IDE_SUCCESS);
            
            sTarget->targetColumn->position = sPivotAggrNode->aggrFunc;
            
            /* Append target */
            sPrevTarget->next = sTarget;
            sPrevTarget->targetColumn->node.next = &sTarget->targetColumn->node;
            sPrevTarget = sPrevTarget->next;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Checking Pivot Target
 *
 *  Column of pivot target is not allow duplicate,
 *
 * @param aSFWGH A pointer of qmsSFWGH
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::checkPivotTarget( qmsSFWGH * aSFWGH )
{
    qmsTarget * sTarget = NULL;
    qmsTarget * sTmp    = NULL;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::checkPivotTarget::__FT__" );

    for ( sTarget = aSFWGH->target->next;
          sTarget != NULL;
          sTarget = sTarget->next )
    {
        for ( sTmp = sTarget->next; sTmp != NULL; sTmp = sTmp->next )
        {
            IDE_TEST_RAISE( idlOS::strMatch( sTarget->displayName.name,
                                             sTarget->displayName.size,
                                             sTmp->displayName.name,
                                             sTmp->displayName.size ) == 0,
                            ERR_AMBIGOUS_PIVOT_COLUMN );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_AMBIGOUS_PIVOT_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_AMBIGOUS_PIVOT_COLUMN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Expand Dummy Target.
 *
 * Parse tree내에 Pivot과 관련된 Dummy Target 이 있다면 이를 풀어놓는다.
 * 이 함수는 validateQmsTarget 전에 호출되는 함수이다.
 * Dummy target에는 Pivot과 연관된 Column 들이 qtc Node list 형태로 달려있다.
 * Dummy target을 expand한다는 의미는 전체 Table의 컬럼중 Pivot과 연관되지 않는
 * 컬럼에대해서는 Grouping을 수행한다는 의미이다.
 * 전체 Table의 컬럼을 하나씩 비교하면서 Dummy Target에 있는 Column과 관련이 없다면,
 * qmsTarget과 Grouping에 노드를 추가한다.
 *
 * @param aStatement A pointer of statement
 * @param aTarget    A pointer of Dummy target
 * @param aSFWGH     A pointer of SFWGH
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::expandPivotDummy( qcStatement * aStatement,
                                            qmsSFWGH    * aSFWGH )
{
    qmsTarget        * sTarget      = NULL;
    qmsTarget        * sNextTarget  = NULL;
    qmsTarget        * sPrevTarget  = NULL;
    qmsTarget        * sDummyTarget = NULL;
    qmsTableRef      * sTableRef    = NULL;
    qcmColumn        * sColumn      = NULL;
    qtcNode          * sNode        = NULL;
    mtcNode          * sMtcNode     = NULL;
    qmsConcatElement * sPrevGroup   = NULL;
    qmsConcatElement * sGroup       = NULL;
    SChar            * sName        = NULL;
    UInt               sIndex       = 0;
    UInt               sSize        = 0;
    UInt               sLen         = 0;
    idBool             sFound       = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::expandPivotDummy::__FT__" );

    IDE_DASSERT( aSFWGH->target != NULL );
    IDE_DASSERT( aSFWGH->from->tableRef != NULL );

    sDummyTarget = aSFWGH->target;
    sTableRef = aSFWGH->from->tableRef;

    IDE_DASSERT((sDummyTarget->flag & QMS_TARGET_DUMMY_PIVOT_MASK)
                == QMS_TARGET_DUMMY_PIVOT_TRUE );

    sNextTarget = sDummyTarget->next;
    sColumn = sTableRef->tableInfo->columns;

    for ( sIndex = 0;
          sIndex < sTableRef->tableInfo->columnCount;
          sIndex++, sColumn = sColumn->next )
    {
        // pivot function은 제외한다.
        if ( ( idlOS::strlen( sTableRef->columnsName[sIndex] ) >= 7 ) &&
             ( idlOS::strMatch( sTableRef->columnsName[sIndex],
                                7,
                                "$$PIVOT",
                                7 ) == 0 ) )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }
        
        sFound = ID_FALSE;
        for ( sMtcNode = sDummyTarget->targetColumn->node.arguments;
              sMtcNode != NULL;
              sMtcNode = sMtcNode->next )
        {
            sNode = (qtcNode *)sMtcNode;
            sName = sNode->columnName.stmtText + sNode->columnName.offset;
            sSize = sNode->columnName.size;

            if ( idlOS::strMatch( sTableRef->columnsName[sIndex],
                                  idlOS::strlen(sTableRef->columnsName[sIndex]),
                                  sName,
                                  sSize ) == 0 )
            {
                sFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        if ( sFound == ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                               qmsTarget,
                               &sTarget )
                 != IDE_SUCCESS);
        QMS_TARGET_INIT(sTarget);

        IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                               qtcNode,
                               &sTarget->targetColumn )
                 != IDE_SUCCESS );
        QTC_NODE_INIT( sTarget->targetColumn );
        IDE_TEST(qtc::makeTargetColumn( sTarget->targetColumn,
                                        sTableRef->table,
                                        (UShort)sIndex )
                 != IDE_SUCCESS);

        sLen = idlOS::strlen(sTableRef->columnsName[sIndex]);
        IDE_TEST(STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                         SChar,
                                         sLen + 1,
                                         &(sTarget->targetColumn->position.stmtText) )
                 != IDE_SUCCESS);
        idlOS::memcpy( sTarget->targetColumn->position.stmtText,
                       sTableRef->columnsName[sIndex],
                       sLen );
        sTarget->targetColumn->position.stmtText[sLen] = '\0';
        sTarget->targetColumn->position.offset = 0;
        sTarget->targetColumn->position.size = sLen;
        sTarget->targetColumn->columnName = sTarget->targetColumn->position;

        if ( sPrevTarget == NULL )
        {
            aSFWGH->target = sTarget;
        }
        else
        {
            sPrevTarget->next = sTarget;
            sPrevTarget->targetColumn->node.next = &sTarget->targetColumn->node;
        }

        sPrevTarget = sTarget;

        /* create group by element and add group by element */
        if ( ( aSFWGH->lflag & QMV_SFWGH_PIVOT_FIRST_VIEW_MASK )
             == QMV_SFWGH_PIVOT_FIRST_VIEW_TRUE )
        {
            IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                   qmsConcatElement,
                                   &sGroup )
                     != IDE_SUCCESS);
            sGroup->type      = QMS_GROUPBY_NORMAL;
            sGroup->next      = NULL;
            sGroup->arguments = NULL;
            IDE_TEST(STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                                   qtcNode,
                                   &sGroup->arithmeticOrList )
                     != IDE_SUCCESS);
            QTC_NODE_INIT( sGroup->arithmeticOrList );
            idlOS::memcpy( sGroup->arithmeticOrList,
                           sTarget->targetColumn,
                           ID_SIZEOF(qtcNode) );

            if ( sPrevGroup == NULL )
            {
                aSFWGH->group = sGroup;
                sPrevGroup = sGroup;
            }
            else
            {
                sPrevGroup->next = sGroup;
            }
        
            sPrevGroup = sGroup;
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sPrevTarget == NULL )
    {
        aSFWGH->target = sNextTarget;
    }
    else
    {
        sPrevTarget->next = sNextTarget;
        sPrevTarget->targetColumn->node.next = &sNextTarget->targetColumn->node;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * Do Pivot Transform
 *
 *  하나의 Table에 관련된 Pivot 구문에 대 Transform 을 수행한다.
 *  실제 Validatation 단계의 validateQmsTableRef 에서 호출된다.
 *
 *  1. check pivot syntax.
 *  2. create pivot parse tree.
 *  3. create dummy target.
 *  4. create pivot target.
 *  5. check complited pivot target.
 *
 * @param aStatement A pointer of statement
 * @param aFrom      A pointer of qmsTableRef
 *
 * @return IDE_SUCCESS or IDE_FAILURE
 */
IDE_RC qmvPivotTransform::doTransform( qcStatement * aStatement,
                                       qmsSFWGH    * aSFWGH,
                                       qmsTableRef * aTableRef )
{
    qmsParseTree * sFirstParseTree;
    qmsParseTree * sSecondParseTree;

    IDU_FIT_POINT_FATAL( "qmvPivotTransform::doTransform::__FT__" );

    IDE_DASSERT( aTableRef        != NULL );
    IDE_DASSERT( aTableRef->pivot != NULL );

    IDE_TEST( checkPivotSyntax( aStatement, aTableRef->pivot )
              != IDE_SUCCESS );

    //--------------------------------
    // make first view
    //--------------------------------
    
    IDE_TEST( createPivotParseTree( aStatement, aTableRef )
              != IDE_SUCCESS );

    sFirstParseTree = (qmsParseTree *)aTableRef->view->myPlan->parseTree;
    
    //--------------------------------
    // make second view
    //--------------------------------
    
    IDE_TEST( createPivotParseTree( aStatement, aTableRef )
              != IDE_SUCCESS );
    
    sSecondParseTree = (qmsParseTree *)aTableRef->view->myPlan->parseTree;
    
    //--------------------------------
    // make first view target
    //--------------------------------
    
    IDE_TEST( createDummyTarget( aStatement,
                                 sFirstParseTree->querySet,
                                 aTableRef )
              != IDE_SUCCESS);
    
    IDE_TEST( createPivotFirstTarget( aStatement,
                                      sFirstParseTree->querySet->SFWGH,
                                      aTableRef )
              != IDE_SUCCESS);
    
    // 첫번째 view임을 표시
    sFirstParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_PIVOT_FIRST_VIEW_MASK;
    sFirstParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_PIVOT_FIRST_VIEW_TRUE;
    sFirstParseTree->querySet->SFWGH->hints = aSFWGH->hints;

    //--------------------------------
    // make second view target
    //--------------------------------
    
    IDE_TEST( createDummyTarget( aStatement,
                                 sSecondParseTree->querySet,
                                 aTableRef )
              != IDE_SUCCESS);
    
    IDE_TEST( createPivotSecondTarget( aStatement,
                                       sSecondParseTree->querySet->SFWGH,
                                       aTableRef )
              != IDE_SUCCESS);
    
    // check second view target
    IDE_TEST( checkPivotTarget( sSecondParseTree->querySet->SFWGH )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

