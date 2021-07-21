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
 * $Id: qmvUnpivotTransform.cpp 90008 2021-02-17 06:03:16Z andrew.shin $
 *
 * PROJ-2523 Unpivot clause
 *
 * [ Unpivot Transformation은 2단계로 나뉜다. ]
 *
 *   1. validateQmsTableRef 초기 단계에서 unpivot 구문을 해석하여 TableRef->view를
 *      unpivot transformed view로 생성 또는 변경하는 부분이다.
 *      새로 생성되는 unpivot transformed view에는 loop clause가 추가 되는데,
 *      이 loop clause를 통해 열로 변환되는 행의 개수만큼 record의 수가 복제된다.
 *      이 때, unpivot transformed view의 target은 unpivot 구문에 정의된 column들을 인자로 하는
 *      DECODE_OFFSET 함수들로 구성되며, loop_level pseudo column을 통해 해당하는 순번의 column이 구해지게 된다.
 *
 *   2. Target을 validation하기 전 즉 qmsValidateTarget 함수가 호출되기 전에
 *      위의 1. 번 transform 수행 시 DECODE_OFFSET target 생성과정에
 *      인자로 사용 되지 않은 column들 을 expand한다.
 *
 * [ Unpivot transformation ]
 *
 *   < User query >
 *
 *     select *
 *       from t1
 *    unpivot exclude nulls ( value_col_name1 for measure_col_name1 in ( c1 as 'a',
 *                                                                       c2 as 'b',
 *                                                                       c5 as 'c' ) ) u1,
 *            t2
 *    unpivot exclude nulls ( value_col_name2 for measure_col_name2 in ( c1 as 'a',
 *                                                                       c2 as 'b',
 *                                                                       c5 as 'c' ) ) u2
 *      where u1.3 = u2.i3;
 *
 *   < Transformed query >
 *
 *     select *
 *       from ( select c3, c4, c6,
 *                     decode_offset( loop_level, 'a', 'b', 'c' ) measure_col_name1,
 *                     decode_offset( loop_level, c1, c2, c3 ) value_col_name1
 *                from t1
 *                loop 3 ) u1,
 *            ( select c3, c4, c6,
 *                     decode_offset( loop_level, 'a', 'b', 'c' ) measure_col_name2,
 *                     decode_offset( loop_level, c1, c2, c3 ) value_col_name2
 *                from t2
 *                loop 3 ) u2
 *      where ( u1.3 = u2.i3 ) and ( value_col_name1 is not null or value_col_name2 is not null  );
 *
 **********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qtc.h>
#include <qmvQTC.h>
#include <qmvUnpivotTransform.h>
#include <qmvQuerySet.h>
#include <qmv.h>
#include <qmo.h>
#include <qmx.h>

#define LOOP_MAX_LENGTH 16

extern mtfModule mtfDecodeOffset;
extern mtfModule mtfIsNotNull;
extern mtfModule mtfOr;
extern mtfModule mtfAnd;

IDE_RC qmvUnpivotTransform::checkUnpivotSyntax( qmsUnpivot * aUnpivot )
{
/***********************************************************************
 *
 * Description :
 *     Unpivot 구문에 대한 validation 검사를 수행한다.
 *
 *     unpivot_value_column, unpivot_measure_column, unpivot_in_column, unpivot_in_alias 에 정의된
 *     column의 개수는 동일해야 한다.
 *
 * Implementation :
 *     1. Unpivot value column의 개수를 구한다.
 *     2. Unpivot measure column의 개수를 구하고, value columns의 개수와 다르면 에러
 *     3. Unpivot_in_clause의 column개수와 alias 개수를 구하고, value column의 개수와 다르면 에러
 *
 * Arguments :
 *
 ***********************************************************************/
    qmsUnpivotColName   * sValueColumnName = NULL;
    qmsUnpivotColName   * sMeasureColumnName = NULL;

    qmsUnpivotInColInfo * sInColInfo = NULL;

    qmsUnpivotInNode    * sInColumn = NULL;
    qmsUnpivotInNode    * sInAlias = NULL;

    UInt sValueColumnCnt = 0;
    UInt sMeasureColumnCnt = 0;

    UInt sInColumnCnt = 0;
    UInt sInAliasCnt = 0;

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::checkUnpivotSyntax::__FT__" );

    for ( sValueColumnName  = aUnpivot->valueColName, sValueColumnCnt = 0;
          sValueColumnName != NULL;
          sValueColumnName  = sValueColumnName->next, sValueColumnCnt++ )
    {
        /* Nothing to do */
    }

    for ( sMeasureColumnName  = aUnpivot->measureColName, sMeasureColumnCnt = 0;
          sMeasureColumnName != NULL;
          sMeasureColumnName  = sMeasureColumnName->next, sMeasureColumnCnt++ )
    {
        /* Nothing to do */
    }

    /* Value_column의 count와 measure column의 count가 다르면 안된다. */
    IDE_TEST_RAISE( sValueColumnCnt != sMeasureColumnCnt, ERR_INVALID_UNPIVOT_ARGUMENT );

    for ( sInColInfo  = aUnpivot->inColInfo;
          sInColInfo != NULL;
          sInColInfo  = sInColInfo->next )
    {
        for ( sInColumn  = sInColInfo->inColumn, sInColumnCnt = 0;
              sInColumn != NULL;
              sInColumn  = sInColumn->next, sInColumnCnt++ )
        {
            /* Nothing to do */
        }

        /* Value_column의 count와 in_clause의 column count가 다르면 안된다. */
        IDE_TEST_RAISE( sValueColumnCnt != sInColumnCnt, ERR_INVALID_UNPIVOT_ARGUMENT );

        for ( sInAlias  = sInColInfo->inAlias, sInAliasCnt = 0;
              sInAlias != NULL;
              sInAlias  = sInAlias->next, sInAliasCnt++ )
        {
            /* Nothing to do */
        }

        /* Value_column의 count와 in_clause의 alias count가 다르면 안된다. */
        IDE_TEST_RAISE( sValueColumnCnt != sInAliasCnt, ERR_INVALID_UNPIVOT_ARGUMENT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_UNPIVOT_ARGUMENT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_INVALID_UNPIVOT_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvUnpivotTransform::makeViewForUnpivot( qcStatement * aStatement,
                                                qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 * Description :
 *     Unpivot 대상 table( 또는 view )에 대해서
 *     Parse tree를 변경하는 Transformation을 수행한다.
 *
 * Implementation :
 *     1. Transformed view statement를 생성
 *     2. Transformed view parse tree 생성
 *     3. Transformed view query set 생성
 *     4. Transformed view SFWGH 생성
 *     5. Transformed view from 생성
 *     7. Original tableRef의 table ( 또는 view )를 생성 된 transformed view from에 연결
 *     8. Original tableRef에 transformed view statement를 in-line view로 연결
 *
 * Arguments :
 *
 ***********************************************************************/

    qcStatement  * sStatement = NULL;
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsSFWGH     * sSFWGH     = NULL;
    qmsFrom      * sFrom      = NULL;
    qmsTableRef  * sTableRef  = NULL;
    qcNamePosition sNullPosition;

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::makeViewForUnpivot::__FT__" );

    SET_EMPTY_POSITION( sNullPosition );

    /* Allocation & initialization for transformed view */

    IDU_FIT_POINT( "qmvUnpivotTransform::makeViewForUnpivot::STRUCT_ALLOC::sStatement",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qcStatement,
                            &sStatement )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "qmvUnpivotTransform::makeViewForUnpivot::STRUCT_ALLOC::sParseTree",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsParseTree,
                            &sParseTree )
              != IDE_SUCCESS );

    QC_SET_INIT_PARSE_TREE( sParseTree, sNullPosition );

    IDU_FIT_POINT( "qmvUnpivotTransform::makeViewForUnpivot::STRUCT_ALLOC::sQuerySet",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsQuerySet,
                            &sQuerySet )
              != IDE_SUCCESS );

    QCP_SET_INIT_QMS_QUERY_SET( sQuerySet );

    IDU_FIT_POINT( "qmvUnpivotTransform::makeViewForUnpivot::STRUCT_ALLOC::sSFWGH",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsSFWGH,
                            &sSFWGH )
              != IDE_SUCCESS );
    
    QCP_SET_INIT_QMS_SFWGH( sSFWGH );

    /* UNPIVOT flag setting for expanding target */
    sSFWGH->lflag &= ~QMV_SFWGH_UNPIVOT_MASK;
    sSFWGH->lflag |= QMV_SFWGH_UNPIVOT_TRUE;

    IDU_FIT_POINT( "qmvUnpivotTransform::makeViewForUnpivot::STRUCT_ALLOC::sFrom",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsFrom,
                            &sFrom )
              != IDE_SUCCESS );

    QCP_SET_INIT_QMS_FROM( sFrom );

    IDU_FIT_POINT( "qmvUnpivotTransform::makeViewForUnpivot::STRUCT_ALLOC::sTableRef",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                            qmsTableRef,
                            &sTableRef )
              != IDE_SUCCESS );

    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );

    /* Copy original tableRef info to new tableRef except unpivot, noMergeHint and alias */
    idlOS::memcpy( sTableRef, aTableRef, ID_SIZEOF( qmsTableRef ) );
    QCP_SET_INIT_QMS_TABLE_REF( aTableRef );

    /* Set alias */
    SET_POSITION( aTableRef->aliasName, sTableRef->aliasName );

    /* BUG-48544 Pivot 구문 집계 함수에 Inline View Alias, With Alias 인 Coulmn 또는 Subquery 를 사용하는 경우 오류가 발생합니다. */
    if ( sTableRef->view == NULL )
    {
        SET_EMPTY_POSITION( sTableRef->aliasName );
    }
    else
    {
        /* Nothing to do */
    }

    /* TASK-7219 */
    SET_POSITION( aTableRef->position, sTableRef->position );
    SET_EMPTY_POSITION( sTableRef->position );    

    /* Set noMergeHint */
    aTableRef->noMergeHint = sTableRef->noMergeHint;

    /* Set unpivot */
    aTableRef->unpivot = sTableRef->unpivot;
    sTableRef->unpivot = NULL;

    /* Set transformed view query set */ 
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    /* Set transformed view parse tree */
    sParseTree->withClause         = NULL;
    sParseTree->querySet           = sQuerySet;
    sParseTree->orderBy            = NULL;
    sParseTree->limit              = NULL;
    sParseTree->loopNode           = NULL;
    sParseTree->forUpdate          = NULL;
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

    /* Set transformed view statement */
    QC_SET_STATEMENT( sStatement, aStatement, sParseTree );
    sStatement->myPlan->parseTree->stmtKind = QCI_STMT_SELECT;

    /* Link transformed view statement on the original tableRef as a in-line view */
    aTableRef->view = sStatement;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC qmvUnpivotTransform::makeArgsForDecodeOffset( qcStatement         * aStatement,
                                                     qtcNode             * aNode,
                                                     qmsUnpivotInColInfo * aUnpivotIn,
                                                     idBool                aIsValueColumn,
                                                     ULong                 aOrder )
{
/***********************************************************************
 *
 * Description :
 *     Unpivot transfored view의 target인 DECODE_OFFSET 함수의 인자들을 생성한다.
 *
 * Implementation :
 *     1. LOOP_LEVEL pseudo column 생성
 *     2. DECODE_OFFSET 함수의 첫 번 째 인자로 LOOP_LEVEL pseudo column을 등록
 *     3. unpivot_in_clause의 column 또는 alias의 nodes를
 *        DECODE_OFFSET 함수의 search parameter로 등록
 *     4. DECODE_OFFSET 함수의 인자 개수를 flag에 세팅
 *
 * Arguments :
 *     aIsValueColumn - 이 함수는 unpivot_value_column에 대해 수행 될 때( ID_TRUE )는
 *                      unpivot_in_clause의 column nodes를 DECODE_OFFSET 함수의 search parameter로 등록하고,
 *                      unpivot_measure_column에 대해 수행 될 때( ID_FALSE )는
 *                      unpivot_in_clause의 alias nodes를 search parameter로 등록한다.
 * 
 *     aOrder - unpivot이 multiple unpivotted columns를 생성 할 경우
 *
 *             ex ) unpivot (    value1,   value2,   value3
 *                      for    measure1, measure2, measure3
 *                       in ( ( column1,  column2,  column3 )
 *                         as (  alias1,   alias2,   alias3 ),
 *                            ( column4,  column5,  column6 )
 *                         as (  alias4,   alias5,   alias6 )
 *                         ... ) )
 *
 *               value1에 대해서 수행 될때는 column1, column 4
 *               measure2에 대해서 수행 될 때는 alias2, alias5 가
 *               DECODE_OFFSET의 search parameter로 등록된다.
 *               자신에게 해당하는 순번의 nodes를 등록하기 위해 aOrder를 전달 받는다.
 *
 **********************************************************************/

    qtcNode             * sLoopNode[2];
    static SChar        * sLoopLevelName = (SChar*)"LOOP_LEVEL";
    qcNamePosition        sNamePosition;

    qmsUnpivotInColInfo * sUnpivotIn = NULL;
    qmsUnpivotInNode    * sInColumn  = NULL;
    mtcNode             * sPrevNode  = NULL;
    qmsUnpivotInNode    * sInAlias   = NULL;

    UInt                 sCount = 0;
    UInt                 sArgCnt = 0;

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::makeViewForUnpivot::__FT__" );

    sNamePosition.stmtText = sLoopLevelName;
    sNamePosition.offset   = 0;
    sNamePosition.size     = idlOS::strlen( sLoopLevelName );

    /* Make LOOP_LEVEL pseudo column for the offset of DECODE_OFFSET function */
    IDE_TEST( qtc::makeColumn( aStatement,
                               sLoopNode,
                               NULL,
                               NULL,
                               &sNamePosition,
                               NULL )
              != IDE_SUCCESS );

    /* Add LOOP_LEVEL pseudo column node on DECODE_OFFSET as the first argument */
    aNode->node.arguments  = &(sLoopNode[0]->node );
    sPrevNode = aNode->node.arguments;

    /* For setting the argument count of decodeOffset */
    sArgCnt++;

    /* Make arguments for search parameters of DECODE_OFFSET function */
    for ( sUnpivotIn  = aUnpivotIn;
          sUnpivotIn != NULL;
          sUnpivotIn  = sUnpivotIn->next )
    {
        if ( aIsValueColumn == ID_TRUE )
        {
            /* For value column */
            for ( sInColumn  = sUnpivotIn->inColumn, sCount = 0;
                  ( sInColumn != NULL ) && ( sCount < aOrder );
                  sInColumn  = sInColumn->next, sCount++ )
            {
                /* Nothing to do */
            }

            IDE_TEST_RAISE( sInColumn == NULL, ERR_UNEXPECTED_ERROR );

            sPrevNode->next = &( sInColumn->inNode->node );
            sPrevNode = &( sInColumn->inNode->node );
            sArgCnt++;
        }
        else
        {
            /* For value measure */
            for ( sInAlias  = sUnpivotIn->inAlias, sCount = 0;
                  ( sInAlias != NULL ) && ( sCount < aOrder );
                  sInAlias  = sInAlias->next, sCount++ )
            {
                /* Nothing to do */
            }

            IDE_TEST_RAISE( sInAlias == NULL, ERR_UNEXPECTED_ERROR );

            sPrevNode->next = &( sInAlias->inNode->node );
            sPrevNode = &( sInAlias->inNode->node );
            sArgCnt++;
        }
    }

    /* Set arugment count of DECODE_OFFSET function. */
    aNode->node.lflag  = sArgCnt;
    aNode->node.lflag |= mtfDecodeOffset.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvUnpivotTransform::makeArgsForDecodeOffset",
                                  "Invalid Unpivot Target" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvUnpivotTransform::makeUnpivotTarget( qcStatement * aStatement,
                                               qmsSFWGH    * aSFWGH,
                                               qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 * Description :
 *     Unpivot transfored view의 target으로 DECODE_OFFSET 함수들을 생성한다.
 *     unpivot_value_column과 unpivot_measure_column 하나당 하나씩
 *     DECODE_OFFSET 함수가 생성된다.
 *
 * Implementation :
 *     1. unpivot_measure_column에 대한 target을 생성한다.
 *        1-a. qmsTarget 생성
 *        1-b. target expand시 제외할 것 을 flag( QMS_TARGET_UNPIVOT_COLUMN_TRUE ) 처리 
 *        1-c. unpivot_measure_column에 등록된 이름을 생성된 target의 alias로 지정
 *        1-d. DECODE_OFFSET function을 생성해서 targetColumn에 등록
 *        1-e. DECODE_OFFSET에 대한 INTERMEDIATE tuple을 할당
 *     2. unpivot_value_column에 대한 target을 생성한다.
 *        1-a. qmsTarget 생성
 *        1-b. target expand시 제외할 것 을 flag( QMS_TARGET_UNPIVOT_COLUMN_TRUE ) 처리 
 *        1-c. unpivot_value_column에 등록된 이름을 생성된 target의 alias로 지정
 *        1-d. DECODE_OFFSET function을 생성해서 targetColumn에 등록
 *        1-e. DECODE_OFFSET에 대한 INTERMEDIATE tuple을 할당
 *
 * Arguments :
 *
 **********************************************************************/

    qmsTarget           * sTarget         = NULL;
    qmsTarget           * sPrevTarget     = NULL;
    
    qmsUnpivot          * sUnpivot        = NULL;
    qmsUnpivotColName   * sValueColName   = NULL;
    qmsUnpivotColName   * sMeasureColName = NULL;

    UInt                  sValueColumns   = 0;
    UInt                  sMeasureColumns = 0;

    qcNamePosition        sNullName;
    qtcNode             * sDecodeOffsetNode[2];

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::makeUnpivotTarget::__FT__" );

    SET_EMPTY_POSITION( sNullName );

    sUnpivot = aTableRef->unpivot;
    
    /* Make DECODE_OFFSET for measure column(s) as a target for unpivot transformed view */
    for ( sMeasureColName  = sUnpivot->measureColName, sMeasureColumns = 0;
          sMeasureColName != NULL;
          sMeasureColName  = sMeasureColName->next, sMeasureColumns++ )
    {
        IDU_FIT_POINT( "qmvUnpivotTransform::makeUnpivotTarget::STRUCT_ALLOC::sTargetOfMeasueColName",
                       idERR_ABORT_InsufficientMemory );

        /* Alloc qmsTarget */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qtcNode,
                                &sTarget )
                  != IDE_SUCCESS );

        QMS_TARGET_INIT( sTarget );

        IDU_FIT_POINT( "qmvUnpivotTransform::makeUnpivotTarget::STRUCT_ALLOC::nameOfMeasueColName",
                       idERR_ABORT_InsufficientMemory );

        /* Set written name of measure column on the created target as an alias */
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                          SChar,
                                          sMeasureColName->colName.size + 1,
                                          &( sTarget->aliasColumnName.name ) )
                  != IDE_SUCCESS );
        
        QC_STR_COPY( sTarget->aliasColumnName.name, sMeasureColName->colName ); // null terminator added.
        sTarget->aliasColumnName.size = sMeasureColName->colName.size;

        sTarget->displayName = sTarget->aliasColumnName;
        
        IDE_TEST( qtc::makeNode( aStatement,
                                 sDecodeOffsetNode,
                                 &sNullName,
                                 &mtfDecodeOffset )
                  != IDE_SUCCESS );

        sTarget->targetColumn = sDecodeOffsetNode[0];

        /* Make the arguments of DECODE_OFFSET function */
        IDE_TEST( makeArgsForDecodeOffset( aStatement,
                                           sTarget->targetColumn,
                                           sUnpivot->inColInfo,
                                           ID_FALSE,
                                           sMeasureColumns )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   sTarget->targetColumn,
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   (UInt)(mtfDecodeOffset.lflag & MTC_NODE_COLUMN_COUNT_MASK) )
                  != IDE_SUCCESS );

        if ( sMeasureColumns == 0 )
        {
            aSFWGH->target = sTarget;
            sPrevTarget    = sTarget;
        }
        else
        {
            sPrevTarget->next = sTarget;
            sPrevTarget = sTarget;            
        }
    }

    /* Make DECODE_OFFSET for value column(s) as a target for unpivot transformed view */
    for ( sValueColName  = sUnpivot->valueColName, sValueColumns = 0;
          sValueColName != NULL;
          sValueColName  = sValueColName->next, sValueColumns++ )
    {
        IDU_FIT_POINT( "qmvUnpivotTransform::makeUnpivotTarget::STRUCT_ALLOC::sTargetOfValueColName",
                       idERR_ABORT_InsufficientMemory );

        /* Alloc qmsTarget */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qtcNode,
                                &sTarget )
                  != IDE_SUCCESS );

        QMS_TARGET_INIT( sTarget ) ;
        
        /* For expanding unpivot target */
        sTarget->flag &= ~QMS_TARGET_UNPIVOT_COLUMN_MASK;
        sTarget->flag |=  QMS_TARGET_UNPIVOT_COLUMN_TRUE;

        IDU_FIT_POINT( "qmvUnpivotTransform::makeUnpivotTarget::STRUCT_ALLOC::nameOfValueColName",
                       idERR_ABORT_InsufficientMemory );

        /* Set written name of value column on the created target as an alias */
        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                          SChar,
                                          sValueColName->colName.size + 1,
                                          &( sTarget->aliasColumnName.name ) )
                  != IDE_SUCCESS );

        QC_STR_COPY( sTarget->aliasColumnName.name, sValueColName->colName ); // null terminator added.
        sTarget->aliasColumnName.size = sValueColName->colName.size;

        sTarget->displayName = sTarget->aliasColumnName;

        IDE_TEST( qtc::makeNode( aStatement,
                                 sDecodeOffsetNode,
                                 &sNullName,
                                 &mtfDecodeOffset )
                  != IDE_SUCCESS );

        sTarget->targetColumn = sDecodeOffsetNode[0];

        /* Make the arguments of DECODE_OFFSET function */
        IDE_TEST( makeArgsForDecodeOffset( aStatement,
                                           sTarget->targetColumn,
                                           sUnpivot->inColInfo,
                                           ID_TRUE,
                                           sValueColumns )
                  != IDE_SUCCESS );

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   sTarget->targetColumn,
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   (UInt)(mtfDecodeOffset.lflag & MTC_NODE_COLUMN_COUNT_MASK) )
                  != IDE_SUCCESS );

        // To remove a codesonar warning.
        IDE_TEST_RAISE( sPrevTarget == NULL, ERR_UNEXPECTED_ERROR );

        sPrevTarget->next = sTarget;
        sPrevTarget = sTarget;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvUnpivotTransform::makeUnpivotTarget",
                                  "Invalid Unpivot Clause" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvUnpivotTransform::makeLoopClause( qcStatement         * aStatement,
                                            qmsParseTree        * aParseTree,
                                            qmsUnpivotInColInfo * aInColInfo )
{
/***********************************************************************
 *
 * Description :
 *     Unpivot transfored view의 parse tree에 loop node를 추가한다.
 *
 * Implementation :
 *
 * Arguments :
 *     aInColInfo - unpivot_in의 element개수
 *
 **********************************************************************/
    qtcNode             * sLoopNode[2];
    SChar                 sValue[LOOP_MAX_LENGTH + 1];
    qcNamePosition        sPosition;
    UInt                  sLoopLevelCount = 0;
    qmsUnpivotInColInfo * sUnpivotIn = NULL;

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::makeLoopClause::__FT__" );

    for ( sUnpivotIn  = aInColInfo, sLoopLevelCount = 0;
          sUnpivotIn != NULL;
          sUnpivotIn  = sUnpivotIn->next, sLoopLevelCount++ )
    {
        /* nothing to do */
    }

    idlOS::snprintf( sValue,
                     LOOP_MAX_LENGTH + 1,
                     "%"ID_UINT32_FMT,
                     sLoopLevelCount );

    SET_EMPTY_POSITION( sPosition );
    
    IDE_TEST( qtc::makeValue( aStatement,
                              sLoopNode,
                              (const UChar*)"INTEGER",
                              7,
                              &sPosition,
                              (const UChar *)sValue,
                              (UInt)idlOS::strlen(sValue),
                              MTC_COLUMN_NORMAL_LITERAL )
              != IDE_SUCCESS );

    aParseTree->loopNode = sLoopNode[0];

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvUnpivotTransform::expandUnpivotTarget( qcStatement * aStatement,
                                                 qmsSFWGH    * aSFWGH)
{
/***********************************************************************
 *
 * Description :
 *     Validate phase의 validateQmsTarget 직전에 호출 되어
 *     Unpivot clause에 사용되지 않은 column들을 target에 확장한다.
 *     이 때, 앞 서 transformation 단계에서 생성된 DECODE_OFFSET target보다 앞 쪽 target으로 등록된다.
 *     ex ) t1 := c1, c2, c3, c4, c5
 *
 *     select *
 *       from ( select decode_offset( loop_level, c1, c2, c4 ),...
 *              ...
 *            );
 *
 *                                |
 *                                v
 *
 *     select *
 *       from ( select c3, c5, decode_offset( loop_level, c1, c2, c4 ),...
 *              ...
 *            );
 *
 * Implementation :
 *     1. TableRef의 columnCount만큼 반복하며, tableInfo에 있는 column들을 target에 등록한다.
 *        이 때,  TableInfo의 column중에 앞선 transformation 단계에서 DECODE_OFFSET 함수의 인자로 사용 된
 *        column들은 expand하지 않는다.
 *
 * Arguments :
 *
 ***********************************************************************/
    qmsTarget   * sUnpivotTarget      = NULL;
    qmsTarget   * sDecodeOffsetTarget = NULL; 

    qmsTableRef * sUnpivotTableRef = NULL;
    qcmColumn   * sColumn          = NULL;
    mtcNode     * sMtcNode         = NULL;

    UInt          sIndex           = 0;
    idBool        sIsFound         = ID_FALSE;
    UInt          sLen             = 0;
    UInt          sSize            = 0;
    SChar       * sName            = NULL;

    qmsTarget   * sTarget          = NULL;
    qmsTarget   * sPrevTarget      = NULL;

    qtcNode     * sNode            = NULL;

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::expandUnpivotTarget::__FT__" );

    IDE_DASSERT( aSFWGH->target != NULL );
    IDE_DASSERT( aSFWGH->from->tableRef != NULL );

    sUnpivotTableRef    = aSFWGH->from->tableRef;
    sDecodeOffsetTarget = aSFWGH->target;

    sColumn = sUnpivotTableRef->tableInfo->columns;

    for ( sIndex = 0;
          sIndex < sUnpivotTableRef->tableInfo->columnCount;
          sIndex++, sColumn = sColumn->next )
    {
        sIsFound = ID_FALSE;

        /* 해당 column을 unpivot transformed view target에 사용 되었는지 확인 */
        for ( sUnpivotTarget  = sDecodeOffsetTarget;
              sUnpivotTarget != NULL;
              sUnpivotTarget  = sUnpivotTarget->next )
        {
            if ( ( sUnpivotTarget->flag & QMS_TARGET_UNPIVOT_COLUMN_MASK )
                 == QMS_TARGET_UNPIVOT_COLUMN_TRUE )
            {
                IDE_DASSERT( sUnpivotTarget->targetColumn->node.module == &mtfDecodeOffset );

                /* DECODE_OFFSET의 search parameter인 2번 째 argument부터 조사한다. */
                for ( sMtcNode  = sUnpivotTarget->targetColumn->node.arguments->next;
                      sMtcNode != NULL;
                      sMtcNode  = sMtcNode->next )
                {
                    sNode = ( qtcNode *)sMtcNode;
                    sName = sNode->columnName.stmtText + sNode->columnName.offset;
                    sSize = sNode->columnName.size;

                    if ( idlOS::strMatch( sUnpivotTableRef->columnsName[ sIndex ],
                                          idlOS::strlen( sUnpivotTableRef->columnsName[ sIndex ] ),
                                          sName,
                                          sSize ) == 0 )
                    {
                        /* Unpivot transformed view target에 사용 된 column이다. */
                        sIsFound = ID_TRUE;
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
                /* Nohting to do */
            }

            if ( sIsFound == ID_TRUE )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* Unpivot 에 사용된 column은 target expand에서 제외한다. */
        if ( sIsFound == ID_TRUE )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDU_FIT_POINT( "qmvUnpivotTransform::expandUnpivotTarget::STRUCT_ALLOC::sTarget",
                       idERR_ABORT_InsufficientMemory );

        /* Alloc & initialize qmsTarget */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qmsTarget,
                                &sTarget )
                  != IDE_SUCCESS );
        
        QMS_TARGET_INIT( sTarget );

        IDU_FIT_POINT( "qmvUnpivotTransform::expandUnpivotTarget::STRUCT_ALLOC::targetColumn",
                       idERR_ABORT_InsufficientMemory );

        /* Alloc & initialize target column */
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qtcNode,
                                &( sTarget->targetColumn ) )
                  != IDE_SUCCESS );

        QTC_NODE_INIT( sTarget->targetColumn );

        sTarget->targetColumn->node.module = &qtc::columnModule;
        sTarget->targetColumn->node.lflag = qtc::columnModule.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;

        /* Set column name */
        sLen = idlOS::strlen( sUnpivotTableRef->columnsName[ sIndex ] );

        IDU_FIT_POINT( "qmvUnpivotTransform::expandUnpivotTarget::STRUCT_ALLOC::stmtText",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                          SChar,
                                          sLen + 1,
                                          &( sTarget->targetColumn->position.stmtText ) )
                  != IDE_SUCCESS );

        idlOS::memcpy( sTarget->targetColumn->position.stmtText,
                       sUnpivotTableRef->columnsName[ sIndex ],
                       sLen );

        sTarget->targetColumn->position.stmtText[ sLen ]= '\0';
        sTarget->targetColumn->position.offset = 0;
        sTarget->targetColumn->position.size = sLen;
        sTarget->targetColumn->columnName = sTarget->targetColumn->position;

        /* Add the created target to the target list */
        if ( sPrevTarget == NULL )
        {
            aSFWGH->target = sTarget;
        }
        else
        {
            sPrevTarget->next = sTarget;
            sPrevTarget->targetColumn->node.next = &( sTarget->targetColumn->node );
        }

        sPrevTarget = sTarget;
    }

    /* Link the transformed target( DECODE_OFFSET ) to the expanded target next */
    if ( sPrevTarget == NULL )
    {
        aSFWGH->target = sDecodeOffsetTarget;
    }
    else
    {
        sPrevTarget->next = sDecodeOffsetTarget;
        sPrevTarget->targetColumn->node.next = &( sDecodeOffsetTarget->targetColumn->node );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC qmvUnpivotTransform::makeNotNullPredicate( qcStatement * aStatement,
                                                  qmsSFWGH    * aSFWGH,
                                                  qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 * Description :
 *     Unpivot clause에 EXCLUDE NULLS option(DEFAULT)이 사용 되었을 경우,
 *     Original query set의 WHERE predicate에
 *     value_column의 not null predicate들을 추가 해 준다.
 *
 *     ( rownum의 경우 COUNTER나 STOPKEY operator로 다른 predicate이후에
 *       수행 되기 때문에 정상적으로  동작한다. )
 *
 * Implementation :
 *     1. Unpivot_value_column 을 복제한다.
 *     2. Not null predicate node를 생성한다.
 *     3. Not null predicate node에 복제한 unpivot_value_column을 argument로 등록한다.
 *     4. 이전에 생성된 not null predicate가 있으면 Or node를 생성 해 연결하여
 *        Predicate tree를 구성한다.
 *     5. Not null predicate tree의 구성이 끝나고, original query set의 where에
 *        다른 predicate이 없으면, where에 not null predicate tree를 연결 해 주고,
 *        다른 predicate이 있으면, And node를 추가로 생성해
 *        추가로 생성한 not null predicate tree와 기존 predicate들을 And로 묶어
 *        Where에 그 And node를 연결한다.
 *
 * Arguments :
 *
 ***********************************************************************/

    qtcNode * sNotNullHead = NULL;

    qcNamePosition sNullName;

    qmsUnpivotColName * sValueColumn = NULL;

    qtcNode * sNode[2];
    qtcNode * sNotNullPredicate[2];

    qtcNode * sOrNode[2] = { NULL, NULL };
    qtcNode * sAndNode[2];

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::makeNotNullPredicate::__FT__" );

    /* EXCLUDE NULLS Option exists. */
    if ( aTableRef->unpivot->isIncludeNulls == ID_FALSE )
    {
        sValueColumn = aTableRef->unpivot->valueColName;

        SET_EMPTY_POSITION( sNullName );
        /*************************************************************
         * 1. Make not null predicate list
         *
         *  (    or     )                              
         *        |  
         * ( is not null )->( is not null )->( is not null )   ...
         *        |                 |              |
         * ( valueColumn1 ) ( valueColumn2 ) ( valueColumn3 )  ...
         **************************************************************/
        for ( sValueColumn  = aTableRef->unpivot->valueColName;
              sValueColumn != NULL;
              sValueColumn  = sValueColumn->next )
        {
            // BUG-41878
            // Unpivotted table에 tuple variable이 있을 경우, 해당 tuple variable을 지정하여 생성한다.
            IDE_TEST( qtc::makeColumn( aStatement,
                                       sNode,
                                       NULL,
                                       &( aTableRef->aliasName ),
                                       &( sValueColumn->colName ),
                                       NULL )
                      != IDE_SUCCESS );

            /* Make not null predicate node */
            IDE_TEST( qtc::makeNode( aStatement,
                                     sNotNullPredicate,
                                     &sNullName,
                                     &mtfIsNotNull )
                      != IDE_SUCCESS );

            /* Add the column node on the not null node as an argument */
            IDE_TEST( qtc::addArgument( sNotNullPredicate, sNode )
                      != IDE_SUCCESS );

            if ( sValueColumn == aTableRef->unpivot->valueColName )
            {
                sNotNullHead = sNotNullPredicate[0];
            }
            else
            {
                if ( sOrNode[0] == NULL )
                {
                    /* Make OR node for making a not null predicate tree */
                    IDE_TEST(qtc::makeNode(aStatement,
                                           sOrNode,
                                           &sNullName,
                                           &mtfOr )
                             != IDE_SUCCESS);

                    IDE_TEST( qtc::addArgument( sOrNode, &sNotNullHead ) != IDE_SUCCESS );

                    sNotNullHead = sOrNode[0];
                }
                else
                {
                    // Nothing to do.
                }

                IDE_TEST( qtc::addArgument( sOrNode, sNotNullPredicate ) != IDE_SUCCESS );
            }
        }

        if ( aSFWGH->where == NULL )
        {
            /* Original querySet에 다른 predicate가 없는 경우 */
            aSFWGH->where = sNotNullHead;
        }
        else
        {
            /* Original querySet에 다른 predicate가 있는 경우 */
            /* 다른 predicate와 위에서 생성한 not null predicate들을 and로 연결 해 준다. */
            IDE_TEST(qtc::makeNode(aStatement,
                                   sAndNode,
                                   &sNullName,
                                   &mtfAnd )
                     != IDE_SUCCESS);

            IDE_TEST( qtc::addAndArgument( aStatement,
                                           sAndNode, &sNotNullHead, &aSFWGH->where )
                      != IDE_SUCCESS );

            aSFWGH->where = sAndNode[0];
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

IDE_RC qmvUnpivotTransform::doTransform( qcStatement * aStatement,
                                         qmsSFWGH    * aSFWGH,
                                         qmsTableRef * aTableRef )
{
/***********************************************************************
 *
 * Description :
 *     하나의 TableRef에 관련된 Unpivot 구문에 대해 Transformation을 수행한다.
 *     Validation Phase인 validateQmsTableRef에서 호출 된다.
 *
 * Implementation :
 *     1. Check unpivot syntax.
 *     2. Create unpivot parse tree. ( making the transformed view for unpivot )
 *     3. Create target for unpivot transformed view.
 *     4. Create loop clause for unpivot transformed view.
 *     5. Create not null predicates for unpivot transformed view,
 *        If there is EXCLUDE NULLS option on unpivot clause.
 *
 * Arguments :
 *
 ***********************************************************************/

    qmsParseTree        * sTransformedViewParseTree = NULL;

    IDU_FIT_POINT_FATAL( "qmvUnpivotTransform::makeNotNullPredicate::__FT__" );

    /**********************************
     * Check valid unpivot clause
     **********************************/
    IDE_TEST( checkUnpivotSyntax( aTableRef->unpivot )
              != IDE_SUCCESS );

    /**********************************
     * Make unpivot parse tree
     **********************************/
    IDE_TEST( makeViewForUnpivot( aStatement,
                                  aTableRef )
              != IDE_SUCCESS );

    sTransformedViewParseTree = ( qmsParseTree * )aTableRef->view->myPlan->parseTree;    

    /**********************************
     * Make unpivot view target
     **********************************/
    IDE_TEST( makeUnpivotTarget( aStatement,
                                 sTransformedViewParseTree->querySet->SFWGH,
                                 aTableRef )
              != IDE_SUCCESS );

    /**********************************
     * Add loop clause
     **********************************/
    
    IDE_TEST( makeLoopClause( aStatement,
                              sTransformedViewParseTree,
                              aTableRef->unpivot->inColInfo )
              != IDE_SUCCESS );
    
    /**********************************
     * Add not null predicate
     **********************************/
    IDE_TEST( makeNotNullPredicate( aStatement,
                                    aSFWGH,
                                    aTableRef )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}
