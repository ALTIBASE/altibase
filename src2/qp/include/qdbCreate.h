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
 * $Id: qdbCreate.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef  _O_QDB_CREATE_H_
#define  _O_QDB_CREATE_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>

class qdbCreate
{
public:
    // parse
    static IDE_RC parseCreateTableAsSelect( qcStatement * aStatement );

    // validation
    static IDE_RC validateCreateTable( qcStatement * aStatement );

    static IDE_RC validateCreateTableAsSelect( qcStatement * aStatement );

    static IDE_RC validateTableSpace(qcStatement * aStatement);

    static IDE_RC validateTargetAndMakeColumnList( qcStatement * aStatement );
	
    // optimization for create as select.
    static IDE_RC optimize( qcStatement * aStatement ); 
    
    // execution
    static IDE_RC executeCreateTable( qcStatement * aStatement );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC executeCreateTablePartition( qcStatement  * aStatement,
                                               UInt           aTableID,
                                               qcmTableInfo * aTableInfo );

    static IDE_RC executeCreateTableAsSelect( qcStatement * aStatement );

    // PROJ-1407 Temporary Table
    static IDE_RC validateTemporaryTable( qcStatement      * aStatement,
                                          qdTableParseTree * aParseTree );

    static void fillColumnID(
        qcmColumn           * columns,
        qdConstraintSpec    * constraints);

    //  Table의 Attribute Flag List로부터 32bit Flag값을 계산
    static IDE_RC calculateTableAttrFlag( qcStatement      * aStatement,
                                          qdTableParseTree * aCreateTable );

private:
    // Volatile Tablespace에 Table생성중 수행하는 에러처리
    static IDE_RC checkError4CreateVolatileTable(
                      qdTableParseTree * aCreateTable );

};


#endif // _O_QDB_CREATE_H_
