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
 * $Id: qcpManager.h 88025 2020-07-13 05:35:50Z andrew.shin $
 **********************************************************************/

#ifndef _O_QCP_MANAGER_H_
#define _O_QCP_MANAGER_H_  1

#define QCP_TEST(test) if( test ) YYABORT;

#define QCP_STRUCT_ALLOC(_node_, _structType_)                                  \
    QCP_TEST(STRUCT_ALLOC(MEMORY, _structType_, &_node_) != IDE_SUCCESS)

#define QCP_STRUCT_CRALLOC(_node_, _structType_)                                \
    QCP_TEST(STRUCT_CRALLOC(MEMORY, _structType_, &_node_) != IDE_SUCCESS)

#include <iduMemory.h>
#include <qc.h>

class qcplLexer;

class qcpManager
{
public:
    static IDE_RC parseIt( qcStatement * aStatement );

    // PROJ-1988 merge query
    // stmtText에서 일부분을 parsing 한다. subquery를 parse tree를
    // 복사하는데 사용되며, lexer에서 항상 첫번째 token으로 TR_RETURN을
    // 반환하여 get_condition_statement rule로 parsing된다.
    static IDE_RC parsePartialForSubquery( qcStatement * aStatement,
                                           SChar       * aText,
                                           SInt          aStart,
                                           SInt          aSize );
    
    // PROJ-2415 Grouping Sets Clause
    // stmtText에서 일부분을 parsing 한다.
    // Grouping Sets의 Transform 시 querySet 의 복제를 위해 사용되며,
    // lexer에서 항상 첫번째 token으로 TR_MODIFY를 반환하여
    // get_queryset_statement rule로 parsing된다. 
    static IDE_RC parsePartialForQuerySet( qcStatement * aStatement,
                                           SChar       * aText,
                                           SInt          aStart,
                                           SInt          aSize );

    // PROJ-2415 Grouping Sets Clause
    // stmtText에서 일부분을 parsing 한다.
    // Grouping Sets의 Transform시 OrderBy의 Node를 Target Node로 변환 하는데 사용되며,
    // lexer에서 항상 첫번째 token으로 TR_BACKUP를 반환하여
    // get_target_list_statement rule로 parsing된다.     
    static IDE_RC parsePartialForOrderBy( qcStatement * aStatement,
                                          SChar       * aText,
                                          SInt          aStart,
                                          SInt          aSize );

    // PROJ-2638 shard table
    static IDE_RC parsePartialForAnalyze( qcStatement * aStatement,
                                          SChar       * aText,
                                          SInt          aStart,
                                          SInt          aSize );

    /* TASK-7219 */
    static IDE_RC parsePartialForWhere( qcStatement * aStatement,
                                        SChar       * aText,
                                        SInt          aStart,
                                        SInt          aSize );

private:
    static IDE_RC parseInternal( qcStatement * aStatement,
                                 qcplLexer   * aLexer );
};

#endif  /* _O_QCP_MANAGER_H_ */
