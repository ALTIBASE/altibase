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
 * BUG-41311 table function
 * O사의 pipelined table function에 대한 work-around로
 * array type table function 기능을 제공한다.
 *
 * create typeset type1
 * as
 *   type rec1 is record (c1 integer, c2 varchar(10));
 *   type arr1 is table of rec1 index by integer;
 * end;
 * /
 * 
 * create function func1(i1 integer)
 * return type1.arr1
 * as
 *   v1 type1.arr1;
 * begin
 *   ...
 *   return v1;
 * end;
 * /
 *
 * select * from table( func1(10) );
 *
 * 위와 같이 arr1 type을 반환하는 함수 func1을 table function을 사용하면
 * 다음과 같이 변환된다.
 *
 * select * from (
 *     select table_function_element(loop_value, loop_level, 1) c1,
 *            table_function_element(loop_value, loop_level, 2) c2
 *     from x$dual
 *     loop func1(10)
 * );
 *
 * table function transform은 다음 두 단계를 거쳐 완성한다.
 *
 * 1. loop clause와 table_function_element로 구성된 view를 생성하며
 * 2. target절의 column과 alias는 loop절의 table_function_element 함수
 *    estimate가 끝난 후 validateQmsTarget전에 생성한다.
 *
 **********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qtc.h>
#include <qmvTableFuncTransform.h>
#include <qmvQuerySet.h>
#include <qmv.h>
#include <qmo.h>
#include <qmx.h>

extern mtfModule qsfTableFuncElementModule;

IDE_RC qmvTableFuncTransform::createParseTree( qcStatement * aStatement,
                                               qmsTableRef * aTableRef )
{
    static SChar * sDualTable = (SChar*)"X$DUAL";
    qcStatement  * sStatement = NULL;
    qmsParseTree * sParseTree = NULL;
    qmsQuerySet  * sQuerySet  = NULL;
    qmsSFWGH     * sSFWGH     = NULL;
    qmsFrom      * sFrom      = NULL;
    qmsTableRef  * sTableRef  = NULL;
    qtcNode      * sFuncNode  = NULL;
    idBool         sNoMergeHint = ID_FALSE;
    qcNamePosition sNullPosition;

    IDU_FIT_POINT_FATAL( "qmvTableFuncTransform::createParseTree::__FT__" );

    SET_EMPTY_POSITION( sNullPosition );

    sFuncNode  = aTableRef->funcNode;

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

    sNoMergeHint = aTableRef->noMergeHint;

    /* Copy existed TableRef to new TableRef except funcNode and alias */
    idlOS::memcpy( sTableRef, aTableRef, ID_SIZEOF( qmsTableRef ) );
    QCP_SET_INIT_QMS_TABLE_REF(aTableRef);

    /* Set alias */
    SET_POSITION(aTableRef->aliasName, sTableRef->aliasName);
    SET_EMPTY_POSITION(sTableRef->aliasName);

    // PROJ-2415 Grouping Sets Clause
    aTableRef->noMergeHint = sNoMergeHint;

    // set funcNode
    sTableRef->funcNode = NULL;

    // set dual table
    sTableRef->tableName.stmtText = sDualTable;
    sTableRef->tableName.offset   = 0;
	sTableRef->tableName.size     = idlOS::strlen( sDualTable );
    
    sFrom->tableRef      = sTableRef;
    sSFWGH->from         = sFrom;
    sSFWGH->thisQuerySet = sQuerySet;
    sQuerySet->SFWGH     = sSFWGH;

    sParseTree->withClause         = NULL;
    sParseTree->querySet           = sQuerySet;
    sParseTree->orderBy            = NULL;
    sParseTree->limit              = NULL;
    sParseTree->loopNode           = sFuncNode;  // set funcNode
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

IDE_RC qmvTableFuncTransform::expandTarget( qcStatement * aStatement,
                                            qmsSFWGH    * aSFWGH )
{
    static SChar      * sListColName   = (SChar*)"COLUMN_VALUE";
    static SChar      * sLoopValueName = (SChar*)"LOOP_VALUE";
    static SChar      * sLoopLevelName = (SChar*)"LOOP_LEVEL";
    qtcNode           * sLoopNode;
    mtcColumn         * sColumn;
    qtcModule         * sQtcModule;
    qsTypes           * sTypeInfo;
    qcmColumn         * sQcmColumn;
    qmsTarget         * sTarget;
    qtcNode           * sNode[2];
    UInt                sTargetCount;
    qcNamePosition      sNamePos;
    qcuSqlSourceInfo    sqlInfo;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmvTableFuncTransform::expandTarget::__FT__" );

    sLoopNode = aSFWGH->thisQuerySet->loopNode;
    
    IDE_TEST_RAISE( sLoopNode == NULL, ERR_NOT_FOUND_LOOP_NODE );

    //--------------------------------------
    // 생성할 target column의 갯수와 타입을 결정
    //--------------------------------------
    
    sColumn = aSFWGH->thisQuerySet->loopStack.column;
    
    if ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID )
    {
        sQtcModule = (qtcModule*) sColumn->module;
        sTypeInfo = sQtcModule->typeInfo;
        
        sQcmColumn = sTypeInfo->columns->next;  // element type
        sColumn    = sQcmColumn->basicInfo;
        
        if ( ( sColumn->module->id >= MTD_UDT_ID_MIN ) &&
             ( sColumn->module->id <= MTD_UDT_ID_MAX ) )
        {
            // UDT중 record type만 가능
            IDE_TEST_RAISE( ( sColumn->module->id != MTD_ROWTYPE_ID ) &&
                            ( sColumn->module->id != MTD_RECORDTYPE_ID ),
                            ERR_NOT_APPLICABLE_TABLE_FUNCTION );

            sQtcModule = (qtcModule*) sColumn->module;
            sTypeInfo = sQtcModule->typeInfo;

            sTargetCount = sTypeInfo->columnCount;
            sQcmColumn   = sTypeInfo->columns;
        }
        else
        {
            sTargetCount = 1;
            sQcmColumn   = NULL;
        }
    }
    else if ( ( sColumn->module->id == MTD_RECORDTYPE_ID ) ||
              ( sColumn->module->id == MTD_ROWTYPE_ID ) )
    {
        sQtcModule = (qtcModule*) sColumn->module;
        sTypeInfo = sQtcModule->typeInfo;

        sTargetCount = sTypeInfo->columnCount;
        sQcmColumn   = sTypeInfo->columns;
    }
    else if ( sColumn->module->id == MTD_LIST_ID )
    {
        sTargetCount = 1;
        sQcmColumn   = NULL;
    }
    else
    {
        IDE_RAISE( ERR_NOT_APPLICABLE_TABLE_FUNCTION );
    }

    //--------------------------------------
    // target column의 생성
    //--------------------------------------

    // n개의 target 생성
    IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                       qmsTarget,
                                       sTargetCount,
                                       & sTarget )
              != IDE_SUCCESS );

    for ( i = 0; i < sTargetCount; i++ )
    {
        QMS_TARGET_INIT( &(sTarget[i]) );

        // make alias
        if ( sQcmColumn != NULL )
        {
            sTarget[i].aliasColumnName.name = sQcmColumn->name;
            sTarget[i].aliasColumnName.size = idlOS::strlen( sQcmColumn->name );
        }
        else
        {
            sTarget[i].aliasColumnName.name = sListColName;
            sTarget[i].aliasColumnName.size = idlOS::strlen( sListColName );
        }
        
        // targetColumn 생성
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                qtcNode,
                                & (sTarget[i].targetColumn) )
                  != IDE_SUCCESS );

        QTC_NODE_INIT( sTarget[i].targetColumn );

        // table_function_element 함수 생성
        sTarget[i].targetColumn->node.lflag   = 2;
        sTarget[i].targetColumn->node.lflag  |=
            qsfTableFuncElementModule.lflag & ~MTC_NODE_COLUMN_COUNT_MASK;
        sTarget[i].targetColumn->node.module  = & qsfTableFuncElementModule;
        sTarget[i].targetColumn->node.info    = i + 1;  // column order

        IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                                   sTarget[i].targetColumn,
                                   aStatement,
                                   QC_SHARED_TMPLATE(aStatement),
                                   MTC_TUPLE_TYPE_INTERMEDIATE,
                                   qsfTableFuncElementModule.lflag &
                                   MTC_NODE_COLUMN_COUNT_MASK )
                  != IDE_SUCCESS );
        
        // loop_value pseudo column 생성 & 연결
        sNamePos.stmtText = sLoopValueName;
        sNamePos.offset = 0;
        sNamePos.size = idlOS::strlen( sLoopValueName );
    
        IDE_TEST( qtc::makeColumn( aStatement,
                                   sNode,
                                   NULL,
                                   NULL,
                                   & sNamePos,
                                   NULL )
                  != IDE_SUCCESS );

        sTarget[i].targetColumn->node.arguments = (mtcNode*) sNode[0];

        // loop_level pseudo column 생성 & 연결
        sNamePos.stmtText = sLoopLevelName;
        sNamePos.offset = 0;
        sNamePos.size = idlOS::strlen( sLoopLevelName );
    
        IDE_TEST( qtc::makeColumn( aStatement,
                                   sNode,
                                   NULL,
                                   NULL,
                                   & sNamePos,
                                   NULL )
                  != IDE_SUCCESS );
        
        sTarget[i].targetColumn->node.arguments->next = (mtcNode*) sNode[0];
        
        if ( sQcmColumn != NULL )
        {
            sQcmColumn = sQcmColumn->next;
        }
        else
        {
            // Nothing to do.
        }
        
        sTarget[i].next = &(sTarget[i + 1]);
    }
    
    sTarget[sTargetCount - 1].next = NULL;

    //--------------------------------------
    // target column의 연결
    //--------------------------------------

    aSFWGH->target = sTarget;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_APPLICABLE_TABLE_FUNCTION )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sLoopNode->position );

        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QMV_NOT_APPLICABLE_TABLE_FUNCTION,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_LOOP_NODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmvTableFuncTransform::expandTarget",
                                  "loop node not found" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvTableFuncTransform::doTransform( qcStatement * aStatement,
                                           qmsTableRef * aTableRef )
{
    qmsParseTree * sParseTree;

    IDU_FIT_POINT_FATAL( "qmvTableFuncTransform::doTransform::__FT__" );

    IDE_DASSERT( aTableRef != NULL );
    IDE_DASSERT( aTableRef->funcNode != NULL );

    IDE_TEST( createParseTree( aStatement, aTableRef )
              != IDE_SUCCESS );

    sParseTree = (qmsParseTree *)aTableRef->view->myPlan->parseTree;
    
    sParseTree->querySet->SFWGH->lflag &= ~QMV_SFWGH_TABLE_FUNCTION_VIEW_MASK;
    sParseTree->querySet->SFWGH->lflag |= QMV_SFWGH_TABLE_FUNCTION_VIEW_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

