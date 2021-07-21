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
 * $Id: qcpUtil.cpp 82630 2018-03-27 00:23:41Z kclee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qdParseTree.h>
#include <qcpManager.h>
#include <qcuSqlSourceInfo.h>
#include <qcpUtil.h>

#include "qcpll.h"

// BUG-44700 reserved words error msg
// 예약어 추가시 아래의 항목에 추가 해야 합니다.
static qcpUtilReservedWordTables reservedWordTables[] =
{
    {"AT", 2, 1},
    {"IF", 2, 1},
    {"END", 3, 1},
    {"KEY", 3, 1},
    {"AGER", 4, 1},
    {"BODY", 4, 1},
    {"CAST", 4, 1},
    {"CUBE", 4, 1},
    {"EACH", 4, 1},
    {"EXEC", 4, 1},
    {"EXIT", 4, 1},
    {"FIFO", 4, 1},
    {"FULL", 4, 1},
    {"GOTO", 4, 1},
    {"JOIN", 4, 1},
    {"LEFT", 4, 1},
    {"LESS", 4, 1},
    {"LIFO", 4, 1},
    {"AFTER", 5, 1},
    {"AUDIT", 5, 1},
    {"BEGIN", 5, 1},
    {"CLOSE", 5, 1},
    {"CYCLE", 5, 1},
    {"ELSIF", 5, 1},
    {"FETCH", 5, 1},
    {"FIXED", 5, 1},
    {"FLUSH", 5, 1},
    {"INNER", 5, 1},
    {"LIMIT", 5, 1},
    {"ACCESS", 6, 1},
    {"BACKUP", 6, 1},
    {"BEFORE", 6, 1},
    {"COMMIT", 6, 1},
    {"CURSOR", 6, 1},
    {"ELSEIF", 6, 1},
    {"ENABLE", 6, 1},
    {"ESCAPE", 6, 1},
    {"EXTENT", 6, 1},
    {"ARCHIVE", 7, 1},
    {"CASCADE", 7, 1},
    {"COMMENT", 7, 1},
    {"COMPILE", 7, 1},
    {"CONJOIN", 7, 1},
    {"DECLARE", 7, 1},
    {"DECRYPT", 7, 1},
    {"DEQUEUE", 7, 1},
    {"DISABLE", 7, 1},
    {"DISJOIN", 7, 1},
    {"ENQUEUE", 7, 1},
    {"EXECUTE", 7, 1},
    {"FLUSHER", 7, 1},
    {"LIBRARY", 7, 1},
    {"COALESCE", 8, 1},
    {"CONTINUE", 8, 1},
    {"DATABASE", 8, 1},
    {"DELAUDIT", 8, 1},
    {"DISASTER", 8, 1},
    {"FUNCTION", 8, 1},
    {"INITRANS", 8, 1},
    {"LANGUAGE", 8, 1},
    {"DIRECTORY", 9, 1},
    {"FLASHBACK", 9, 1},
    {"FOLLOWING", 9, 1},
    {"ISOLATION", 9, 1},
    {"ARCHIVELOG", 10, 1},
    {"AUTOEXTEND", 10, 1},
    {"CHECKPOINT", 10, 1},
    {"COMPRESSED", 10, 1},
    {"DISCONNECT", 10, 1},
    {"EXTENTSIZE", 10, 1},
    {"CONSTRAINTS", 11, 1},
    {"DETERMINISTIC", 13, 1},
    {"AUTHID", 6, 1},
    {"CURRENT_USER", 12, 1},
    {"DEFINER", 7, 1},
    {"AS", 2, 0},
    {"BY", 2, 0},
    {"IN", 2, 0},
    {"IS", 2, 0},
    {"OF", 2, 0},
    {"ON", 2, 0},
    {"OR", 2, 0},
    {"TO", 2, 0},
    {"ADD", 3, 0},
    {"ALL", 3, 0},
    {"AND", 3, 0},
    {"ANY", 3, 0},
    {"ASC", 3, 0},
    {"FOR", 3, 0},
    {"NEW", 3, 0},
    {"NOT", 3, 0},
    {"OLD", 3, 0},
    {"OUT", 3, 0},
    {"ROW", 3, 0},
    {"SET", 3, 0},
    {"TOP", 3, 0},
    {"LOB", 3, 0},
    {"OFF", 3, 0},
    {"LINK", 4 ,0},
    {"LOOP", 4 ,0},
    {"MOVE", 4, 0},
    {"OPEN", 4, 0},
    {"OVER", 4, 0},
    {"READ", 4, 0},
    {"STEP", 4, 0},
    {"THAN", 4, 0},
    {"TYPE", 4, 0},
    {"WAIT", 4, 0},
    {"WORK", 4, 0},
    {"BULK", 4, 0},
    {"CASE", 4, 0},
    {"DESC", 4, 0},
    {"DROP", 4, 0},
    {"ELSE", 4, 0},
    {"FROM", 4, 0},
    {"INTO", 4, 0},
    {"LIKE", 4, 0},
    {"LOCK", 4, 0},
    {"MODE", 4, 0},
    {"NULL", 4, 0},
    {"SOME", 4, 0},
    {"THEN", 4, 0},
    {"TRUE", 4, 0},
    {"VIEW", 4, 0},
    {"WHEN", 4, 0},
    {"WITH", 4, 0},
    {"ALTER", 5, 0},
    {"APPLY", 5, 0},
    {"CROSS", 5, 0},
    {"FALSE", 5, 0},
    {"GRANT", 5, 0},
    {"GROUP", 5, 0},
    {"INDEX", 5, 0},
    {"LEVEL", 5, 0},
    {"MINUS", 5, 0},
    {"ORDER", 5, 0},
    {"PRIOR", 5, 0},
    {"START", 5, 0},
    {"TABLE", 5, 0},
    {"UNION", 5, 0},
    {"WHERE", 5, 0},
    {"LOCAL", 5, 0},
    {"MERGE", 5, 0},
    {"NULLS", 5, 0},
    {"OUTER", 5, 0},
    {"PIVOT", 5, 0},
    {"PURGE", 5, 0},
    {"QUEUE", 5, 0},
    {"RAISE", 5, 0},
    {"RIGHT", 5, 0},
    {"SHARD", 5, 0},
    {"SPLIT", 5, 0},
    {"STORE", 5, 0},
    {"UNTIL", 5, 0},
    {"USING", 5, 0},
    {"WHILE", 5, 0},
    {"WRITE", 5, 0},
    {"LINKER", 6, 0},
    {"MODIFY", 6, 0},
    {"ONLINE", 6, 0},
    {"OTHERS", 6, 0},
    {"REMOVE", 6, 0},
    {"RETURN", 6, 0},
    {"REVOKE", 6, 0},
    {"ROLLUP", 6, 0},
    {"COLUMN", 6, 0},
    {"CREATE", 6, 0},
    {"DELETE", 6, 0},
    {"EXISTS", 6, 0},
    {"HAVING", 6, 0},
    {"INSERT", 6, 0},
    {"NOCOPY", 6, 0},
    {"RENAME", 6, 0},
    {"ROWNUM", 6, 0},
    {"SELECT", 6, 0},
    {"UNIQUE", 6, 0},
    {"UNLOCK", 6, 0},
    {"UPDATE", 6, 0},
    {"VALUES", 6, 0},
    {"WITHIN", 6, 0},
    {"BETWEEN", 7, 0},
    {"CONNECT", 7, 0},
    {"DEFAULT", 7, 0},
    {"FOREIGN", 7, 0},
    {"INSTEAD", 7, 0},
    {"LATERAL", 7, 0},
    {"PRIMARY", 7, 0},
    {"SESSION", 7, 0},
    {"SQLCODE", 7, 0},
    {"SQLERRM", 7, 0},
    {"TRIGGER", 7, 0},
    {"VC2COLL", 7, 0},
    {"WRAPPED", 7, 0},
    {"_PROWID", 7, 0},
    {"LOGGING", 7, 0},
    {"MAXROWS", 7, 0},
    {"NOAUDIT", 7, 0},
    {"NOCYCLE", 7, 0},
    {"OFFLINE", 7, 0},
    {"PACKAGE", 7, 0},
    {"REBUILD", 7, 0},
    {"RECOVER", 7, 0},
    {"REPLACE", 7, 0},
    {"ROWTYPE", 7, 0},
    {"SEGMENT", 7, 0},
    {"STORAGE", 7, 0},
    {"SYNONYM", 7, 0},
    {"TYPESET", 7, 0},
    {"UNPIVOT", 7, 0},
    {"MAXTRANS", 8, 0},
    {"MOVEMENT", 8, 0},
    {"PARALLEL", 8, 0},
    {"ROLLBACK", 8, 0},
    {"ROWCOUNT", 8, 0},
    {"SEQUENCE", 8, 0},
    {"TRUNCATE", 8, 0},
    {"VARIABLE", 8, 0},
    {"VOLATILE", 8, 0},
    {"WHENEVER", 8, 0},
    {"COMPRESS", 8, 0},
    {"CONSTANT", 8, 0},
    {"DISTINCT", 8, 0},
    {"EXCEPTION", 9, 0},
    {"INTERSECT", 9, 0},
    {"PARTITION", 9, 0},
    {"RETURNING", 9, 0},
    {"LOGANCHOR", 9, 0},
    {"NOLOGGING", 9, 0},
    {"PRECEDING", 9, 0},
    {"PROCEDURE", 9, 0},
    {"SAVEPOINT", 9, 0},
    {"STATEMENT", 9, 0},
    {"TEMPORARY", 9, 0},
    {"CONSTRAINT", 10, 0},
    {"IDENTIFIED", 10, 0},
    {"PRIVILEGES", 10, 0},
    {"REORGANIZE", 10, 0},
    {"TABLESPACE", 10, 0},
    {"NOPARALLEL", 10, 0},
    {"PARAMETERS", 10, 0},
    {"PARTITIONS", 10, 0},
    {"REFERENCES", 10, 0},
    {"REFERENCING", 11, 0},
    {"LOCALUNIQUE", 11, 0},
    {"REPLICATION", 11, 0},
    {"SUPPLEMENTAL", 12, 0},
    {"MATERIALIZED", 12, 0},
    {"NOARCHIVELOG", 12, 0},
    {"REMOTE_TABLE", 12, 0},
    {"UNCOMPRESSED", 12, 0},
    {"SPECIFICATION", 13, 0},
    {"VARIABLE_LARGE", 14, 0},
    {"SHRINK_MEMPOOL", 14, 0},
    {"CONNECT_BY_ROOT", 15, 0},
    {"DUMP_CALLSTACKS", 15, 0},
    {"REMOTE_TABLE_STORE", 18, 0},
    {"KEEP", 4, 0},
    {NULL, 0, 0}
};

static iduFixedTableColDesc gReservedWordColDesc[] =
{
    {
        (SChar *)"KEYWORD",
        IDU_FT_OFFSETOF(qcpUtilReservedWordTables, mWord),
        40,    // RESERVED WORD MAX LENGTH
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LENGTH",
        IDU_FT_OFFSETOF(qcpUtilReservedWordTables, mLen),
        IDU_FT_SIZEOF(qcpUtilReservedWordTables, mLen),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"RESERVED_TYPE",
        IDU_FT_OFFSETOF(qcpUtilReservedWordTables, mReservedType),
        IDU_FT_SIZEOF(qcpUtilReservedWordTables, mReservedType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

IDE_RC qcpUtil::buildRecordForReservedWord( idvSQL              * /* aStatistics */,
                                            void                * aHeader,
                                            void                * /* aDumpObj */,
                                            iduFixedTableMemory * aMemory )
{
    UInt i = 0;

    for ( i = 0; reservedWordTables[i].mWord != NULL; i++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&(reservedWordTables[i]) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gReservedTableDesc =
{
    (SChar *)"X$RESERVED_WORDS",
    qcpUtil::buildRecordForReservedWord,
    gReservedWordColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

IDE_RC qcpUtil::makeColumns4Queue(
    qcStatement         * aStatement,
    qcNamePosition      * aQueueSize,
    UInt                  aMessageColFlag,     // fix BUG-14642
    UInt                  aMessageColInRowLen, // fix BUG-14642
    qdTableParseTree    * aParseTree)
{
#define IDE_FN "qcpUtil::makeColumns4Queue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qcpUtil::makeColumns4Queue"));

    //  CREATE QUEUE queue_name ( queue_size )
    // =>
    //  CREATE TABLE queue_name
    //  (
    //      MSGID         BIGINT                PRIMARY KEY,
    //      MESSAGE       VACHAR( queue_size ),
    //      CORRID        SMALLINT,
    //      ENQUEUE_TIME  DATE
    //  )

    qcmColumn         * sColumn;
    qdConstraintSpec  * sConstr;
    SInt                sPrecision;

    IDE_TEST_RAISE(aParseTree->tableName.size > QC_MAX_OBJECT_NAME_LEN - 12,
                   ERR_QUEUE_NAME_LENGTH);

    //----------------------------------------------------------
    // make MSGID column
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
             != IDE_SUCCESS);

    // BUG-16233
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &(sColumn->basicInfo))
             != IDE_SUCCESS);

    //  Message ID column의 basicInfo 초기화
    // : dataType은 bigint, language는 session의 language로 설정
    IDE_TEST( mtc::initializeColumn(
                  sColumn->basicInfo,
                  MTD_BIGINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    sColumn->basicInfo->flag |= MTC_COLUMN_QUEUE_MSGID_TRUE;


    sColumn->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sColumn->name, QC_MAX_OBJECT_NAME_LEN + 1, "MSGID" );
    sColumn->namePos.stmtText = NULL;
    sColumn->namePos.offset  = 0;
    sColumn->namePos.size    = 0;

    sColumn->defaultValue = NULL;

    sColumn->next            = NULL;
    sColumn->defaultValueStr = NULL;

    // PROJ-1557 varchar32k
    sColumn->inRowLength = ID_UINT_MAX;

    // set first column
    aParseTree->columns     = sColumn;

    //----------------------------------------------------------
    // make MSGID PRIMARY KEY constraint
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),qdConstraintSpec, &sConstr)
             != IDE_SUCCESS);
    QD_SET_INIT_CONSTRAINT_SPEC( sConstr );
    
    sConstr->constrType                = QD_PRIMARYKEY;

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &(sConstr->constraintColumns))
             != IDE_SUCCESS);

    sConstr->constraintColumns->basicInfo       = sColumn->basicInfo;
    sConstr->constraintColumns->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sConstr->constraintColumns->name, QC_MAX_OBJECT_NAME_LEN + 1, "MSGID" );
    sConstr->constraintColumns->namePos.stmtText = NULL;
    sConstr->constraintColumns->namePos.offset  = 0;
    sConstr->constraintColumns->namePos.size    = 0;
    sConstr->constraintColumns->defaultValue    = NULL;
    sConstr->constraintColumns->next            = NULL;
    sConstr->constraintColumns->defaultValueStr = NULL;

    aParseTree->constraints = sConstr;

    //----------------------------------------------------------
    // make MESSAGE column
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
             != IDE_SUCCESS);

    // BUG-16233
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &(sColumn->basicInfo))
             != IDE_SUCCESS);

    sPrecision = (SInt)idlOS::strToUInt(
        (UChar*)(aQueueSize->stmtText + aQueueSize->offset),
        aQueueSize->size );

    // MESSAGE column의 basicInfo 초기화
    // : dataType은 varchar, language는  session의 language로 설정
    IDE_TEST( mtc::initializeColumn(
                  sColumn->basicInfo,
                  MTD_VARCHAR_ID,
                  1,
                  sPrecision,
                  0 )
              != IDE_SUCCESS );

    sColumn->basicInfo->flag &= ~MTC_COLUMN_NOTNULL_TRUE;

    // fix BUG-14642
    // queue의 message column의 속성도 table과 동일하게 지정할 수 있다.
    // create queue q1(10);
    // create queue q1(10 fixed);
    // create queue q1(10 variable);
    sColumn->flag            = aMessageColFlag;
    idlOS::snprintf( sColumn->name, QC_MAX_OBJECT_NAME_LEN + 1, "MESSAGE" );
    sColumn->namePos.stmtText = NULL;
    sColumn->namePos.offset  = 0;
    sColumn->namePos.size    = 0;
    sColumn->defaultValue    = NULL;
    sColumn->next            = NULL;
    sColumn->defaultValueStr = NULL;
    
    // PROJ-1557 varchar32k
    // fix BUG-14642
    sColumn->inRowLength = aMessageColInRowLen;

    // set second column
    aParseTree->columns->next = sColumn;

    //----------------------------------------------------------
    // make CORRID column
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
             != IDE_SUCCESS);

    // BUG-16233
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &(sColumn->basicInfo))
             != IDE_SUCCESS);

    // CORRID column의 basicInfo 초기화
    // : dataType은 smallint, language는  session의 language로 설정
    IDE_TEST( mtc::initializeColumn(
                  sColumn->basicInfo,
                  MTD_INTEGER_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    sColumn->basicInfo->flag &= ~MTC_COLUMN_NOTNULL_TRUE;

    sColumn->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sColumn->name, QC_MAX_OBJECT_NAME_LEN + 1, "CORRID" );
    sColumn->namePos.stmtText = NULL;
    sColumn->namePos.offset  = 0;
    sColumn->namePos.size    = 0;
    sColumn->defaultValue    = NULL;
    sColumn->next            = NULL;
    sColumn->defaultValueStr = NULL;

    // PROJ-1557 varchar32k
    sColumn->inRowLength = ID_UINT_MAX;

    // set third column
    aParseTree->columns->next->next = sColumn;

    //----------------------------------------------------------
    // make ENQUEUE_TIME column
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
             != IDE_SUCCESS);

    // BUG-16233
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &(sColumn->basicInfo))
             != IDE_SUCCESS);

    // CORRID column? basicInfo ???
    // : dataType? smallint, language?  session? language? ??
    IDE_TEST( mtc::initializeColumn( sColumn->basicInfo,
                                     MTD_DATE_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    sColumn->basicInfo->flag &= ~MTC_COLUMN_NOTNULL_TRUE;

    sColumn->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sColumn->name, QC_MAX_OBJECT_NAME_LEN + 1, "ENQUEUE_TIME" );
    sColumn->namePos.stmtText = NULL;
    sColumn->namePos.offset  = 0;
    sColumn->namePos.size    = 0;
    sColumn->defaultValue    = NULL;
    sColumn->next            = NULL;
    sColumn->defaultValueStr = NULL;
    
    // PROJ-1557 varchar32k
    sColumn->inRowLength = ID_UINT_MAX;

    // set fourth column
    aParseTree->columns->next->next->next = sColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_QUEUE_NAME_LENGTH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_NAME_TOO_LONG));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qcpUtil::makeColumns4StructQueue(
    qcStatement         * aStatement,
    qcmColumn           * aColumns,
    qdTableParseTree    * aParseTree)
{
    //  CREATE QUEUE queue_name ( m1 varchar(10), m2 varchar(20), v1 integer )
    // =>
    //  CREATE TABLE queue_name
    //  (
    //      MSGID         BIGINT      PRIMARY KEY,
    //      ENQUEUE_TIME  DATE,
    //      M1            VARCHAR(10),
    //      M2            VARCHAR(20),
    //      V1            INTEGER
    //  )
    
    qcmColumn         * sColumn;
    qdConstraintSpec  * sConstr;
    qcuSqlSourceInfo    sqlInfo;

    IDE_TEST_RAISE(aParseTree->tableName.size > QC_MAX_OBJECT_NAME_LEN - 12,
                   ERR_QUEUE_NAME_LENGTH);

    //----------------------------------------------------------
    // make MSGID column
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
             != IDE_SUCCESS);

    // BUG-16233
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &(sColumn->basicInfo))
             != IDE_SUCCESS);

    //  Message ID column의 basicInfo 초기화
    // : dataType은 bigint, language는 session의 language로 설정
    IDE_TEST( mtc::initializeColumn(
                  sColumn->basicInfo,
                  MTD_BIGINT_ID,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    sColumn->basicInfo->flag |= MTC_COLUMN_QUEUE_MSGID_TRUE;

    sColumn->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sColumn->name, QC_MAX_OBJECT_NAME_LEN + 1, "MSGID" );
    sColumn->namePos.stmtText = NULL;
    sColumn->namePos.offset  = 0;
    sColumn->namePos.size    = 0;

    sColumn->defaultValue = NULL;

    sColumn->next            = NULL;
    sColumn->defaultValueStr = NULL;

    // PROJ-1557 varchar32k
    sColumn->inRowLength = ID_UINT_MAX;

    // set first column
    aParseTree->columns = sColumn;

    //----------------------------------------------------------
    // make MSGID PRIMARY KEY constraint
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),qdConstraintSpec, &sConstr)
             != IDE_SUCCESS);
    QD_SET_INIT_CONSTRAINT_SPEC( sConstr );
    
    sConstr->constrType = QD_PRIMARYKEY;

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &(sConstr->constraintColumns))
             != IDE_SUCCESS);

    sConstr->constraintColumns->basicInfo       = sColumn->basicInfo;
    sConstr->constraintColumns->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sConstr->constraintColumns->name, QC_MAX_OBJECT_NAME_LEN + 1, "MSGID" );
    sConstr->constraintColumns->namePos.stmtText = NULL;
    sConstr->constraintColumns->namePos.offset  = 0;
    sConstr->constraintColumns->namePos.size    = 0;
    sConstr->constraintColumns->defaultValue    = NULL;
    sConstr->constraintColumns->next            = NULL;
    sConstr->constraintColumns->defaultValueStr = NULL;

    aParseTree->constraints = sConstr;

    //----------------------------------------------------------
    // make ENQUEUE_TIME column
    //----------------------------------------------------------
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcmColumn, &sColumn)
             != IDE_SUCCESS);

    // BUG-16233
    IDE_TEST(STRUCT_CRALLOC(QC_QMP_MEM(aStatement), mtcColumn, &(sColumn->basicInfo))
             != IDE_SUCCESS);

    IDE_TEST( mtc::initializeColumn( sColumn->basicInfo,
                                     MTD_DATE_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    sColumn->basicInfo->flag &= ~MTC_COLUMN_NOTNULL_TRUE;

    sColumn->flag            = QCM_COLUMN_TYPE_FIXED;
    idlOS::snprintf( sColumn->name, QC_MAX_OBJECT_NAME_LEN + 1, "ENQUEUE_TIME" );
    sColumn->namePos.stmtText = NULL;
    sColumn->namePos.offset  = 0;
    sColumn->namePos.size    = 0;
    sColumn->defaultValue    = NULL;
    sColumn->next            = NULL;
    sColumn->defaultValueStr = NULL;
    
    // PROJ-1557 varchar32k
    sColumn->inRowLength = ID_UINT_MAX;

    // set second column
    aParseTree->columns->next = sColumn;

    //----------------------------------------------------------
    // add user defined column
    //----------------------------------------------------------

    for ( sColumn = aColumns; sColumn != NULL; sColumn = sColumn->next )
    {
        // BUG-41688
        IDE_TEST_RAISE( sColumn->basicInfo->module == NULL,
                        ERR_NOT_DATA_TYPE );
        
        // 중복 컬럼명 불가
        if ( ( idlOS::strMatch( sColumn->namePos.stmtText + sColumn->namePos.offset,
                                sColumn->namePos.size,
                                "MSGID", 5 ) == 0 ) ||
             ( idlOS::strMatch( sColumn->namePos.stmtText + sColumn->namePos.offset,
                                sColumn->namePos.size,
                                "ENQUEUE_TIME", 12 ) == 0 ) )
        {
            sqlInfo.setSourceInfo( aStatement, &(sColumn->namePos) );
            IDE_RAISE( ERR_DUP_COLUMN_NAME );
        }
        else
        {
            // Nothing to do.
        }
        
        // 암호컬럼은 불가
        IDE_TEST_RAISE( ( sColumn->basicInfo->module->flag &
                          MTD_ENCRYPT_TYPE_MASK )
                        == MTD_ENCRYPT_TYPE_TRUE,
                        ERR_EXIST_ENCRYPTED_COLUMN );
    }
    
    // set third columns
    aParseTree->columns->next->next = aColumns;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_QUEUE_NAME_LENGTH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_QUEUE_NAME_TOO_LONG));
    }
    IDE_EXCEPTION(ERR_NOT_DATA_TYPE);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NO_HAVE_DATATYPE_IN_CRT_TBL));
    }
    IDE_EXCEPTION(ERR_EXIST_ENCRYPTED_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_EXIST_ENCRYPTED_COLUMN));
    }
    IDE_EXCEPTION(ERR_DUP_COLUMN_NAME);
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_DUPLICATE_COLUMN_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpUtil::makeDefaultExpression(
    qcStatement         * aStatement,
    qtcNode            ** aNode,
    SChar               * aString,
    SInt                  aStrlen)
{
#define IDE_FN "qcpUtil::makeDefaultExpression"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qcpUtil::makeDefaultExpression"));

    UChar                  * sDefaultValueStr;
    qdDefaultParseTree     * sDefaultParseTree;
    qcStatement            * sDefaultStatement;
    UInt                     sOriInsOrUptStmtCount;
    UInt                   * sOriInsOrUptRowValueCount;
    smiValue              ** sOriInsOrUptRow;
    SChar                  * sTmpStr;

    // The value is DEFAULT value
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qcStatement, &sDefaultStatement)
             != IDE_SUCCESS);

    QC_SET_STATEMENT(sDefaultStatement, aStatement, NULL);

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( aStrlen + 8 , (void**)&sDefaultValueStr)
             != IDE_SUCCESS);

    idlOS::memset( sDefaultValueStr, 0x00, aStrlen + 8 );

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc( aStrlen + 1, (void**)&sTmpStr )
             != IDE_SUCCESS);

    idlOS::memset( sTmpStr, 0x00, aStrlen + 1 );

    idlOS::memcpy(sTmpStr, aString, aStrlen);
    sTmpStr[aStrlen] = '\0';

    idlOS::snprintf( (SChar*)sDefaultValueStr, aStrlen + 8,
                     "RETURN %s",
                     sTmpStr );

    sDefaultStatement->myPlan->stmtText = (SChar*)sDefaultValueStr;
    sDefaultStatement->myPlan->stmtTextLen = idlOS::strlen((SChar*)sDefaultValueStr);

    // preserve insOrUptRow
    sOriInsOrUptStmtCount = QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptStmtCount;
    sOriInsOrUptRowValueCount =
        QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRowValueCount;
    sOriInsOrUptRow = QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRow;
    
    IDE_TEST(qcpManager::parseIt(sDefaultStatement) != IDE_SUCCESS);

    // restore insOrUptRow
    QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptStmtCount  = sOriInsOrUptStmtCount;
    QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRowValueCount =
        sOriInsOrUptRowValueCount;
    QC_SHARED_TMPLATE(sDefaultStatement)->insOrUptRow = sOriInsOrUptRow;

    sDefaultParseTree = (qdDefaultParseTree*) sDefaultStatement->myPlan->parseTree;

    aNode[0] = sDefaultParseTree->defaultValue;
    aNode[1] = sDefaultParseTree->lastNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qcpUtil::makeConstraintColumnsFromExpression(
    qcStatement         * aStatement,
    qcmColumn          ** aConstraintColumns,
    qtcNode             * aNode )
{
/***********************************************************************
 *
 * Description :
 *  Parsing 단계의 expression으로 Constraint Column List를 만든다.
 *      - subquery를 지원하지 않는다.
 *
 * Implementation :
 *  (1) qtc::makeColumn()에서 지정한 항목으로 Column인지 확인하고,
 *      Column Name 중복이 없는 Constraint Column List를 구성한다.
 *      - Constraint Column에 User Name, Table Name을 지정할 수 없다.
 *  (2) arguments를 Recursive Call
 *  (3) next를 Recursive Call
 *
 ***********************************************************************/

    qcmColumn           * sColumn;
    qcmColumn           * sLastColumn = NULL;
    qcmColumn           * sNewColumn  = NULL;
    qcuSqlSourceInfo      sqlInfo;

    /* qtc::makeColumn()에서 지정한 항목으로 Column인지 확인하고,
     * Column Name 중복이 없는 Constraint Column List를 구성한다.
     */
    if ( (QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE) &&
         (aNode->node.module == &qtc::columnModule) )
    {
        /* Constraint Column에 User Name, Table Name을 지정할 수 없다. */
        if ( QC_IS_NULL_NAME( aNode->userName ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aNode->userName) );
            IDE_RAISE( ERR_SET_USER_NAME_OR_TABLE_NAME_TO_CONSTRAINT_COLUMN );
        }
        else if ( QC_IS_NULL_NAME( aNode->tableName ) == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(aNode->tableName) );
            IDE_RAISE( ERR_SET_USER_NAME_OR_TABLE_NAME_TO_CONSTRAINT_COLUMN );
        }
        else
        {
            /* Nothing to do */
        }

        for ( sColumn = *aConstraintColumns;
              sColumn != NULL;
              sColumn = sColumn->next )
        {
            if ( QC_IS_NAME_MATCHED( sColumn->namePos, aNode->columnName ) )
            {
                break;
            }
            else
            {
                sLastColumn = sColumn;
            }
        }

        if ( sColumn == NULL )
        {
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement), qcmColumn, &sNewColumn )
                      != IDE_SUCCESS );
            QCM_COLUMN_INIT( sNewColumn );
            SET_POSITION( sNewColumn->namePos, aNode->columnName );

            if ( *aConstraintColumns == NULL )
            {
                *aConstraintColumns = sNewColumn;
            }
            else
            {
                sLastColumn->next = sNewColumn;
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

    /* BUG-35385
     * user-defined function인 경우 user name을 명시해야한다.
     * (user.func1()은 tableName.columnName으로 parsing된다.)
     */
    if ( (QC_IS_NULL_NAME( aNode->columnName ) == ID_FALSE) &&
         (QC_IS_NULL_NAME( aNode->tableName ) == ID_TRUE) &&
         (aNode->node.module == &qtc::spFunctionCallModule) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(aNode->columnName) );
        IDE_RAISE( ERR_REQUIRE_OWNER_NAME );
    }
    else
    {
        /* Nothing to do */
    }
    
    /* arguments를 Recursive Call */
    if ( aNode->node.arguments != NULL )
    {
        IDE_TEST( makeConstraintColumnsFromExpression(
                        aStatement,
                        aConstraintColumns,
                        (qtcNode *)aNode->node.arguments )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* next를 Recursive Call */
    if ( aNode->node.next != NULL )
    {
        IDE_TEST( makeConstraintColumnsFromExpression(
                        aStatement,
                        aConstraintColumns,
                        (qtcNode *)aNode->node.next )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_USER_NAME_OR_TABLE_NAME_TO_CONSTRAINT_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_SET_USER_NAME_OR_TABLE_NAME_TO_CONSTRAINT_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_REQUIRE_OWNER_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_REQUIRE_OWNER_NAME_IN_CHECK_EXPR,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcpUtil::resetOffset(
    qtcNode             * aNode,
    SChar               * aStmtText,
    SInt                  aOffset)
{
#define IDE_FN "qcpUtil::resetOffset"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qcpUtil::resetOffset"));

    qtcNode * sNode;
    SInt      sOffset = aOffset;

    for (sNode = (qtcNode *)aNode->node.arguments;
         sNode != NULL;
         sNode = (qtcNode *)sNode->node.next)
    {
        IDE_TEST(resetOffset(sNode, aStmtText, aOffset) != IDE_SUCCESS);
    }

    aNode->position.stmtText = aStmtText;
    aNode->position.offset += sOffset;

    if (aNode->userName.size > 0)
    {
        aNode->userName.stmtText = aStmtText;
        aNode->userName.offset += sOffset;
    }

    if (aNode->tableName.size > 0)
    {
        aNode->tableName.stmtText = aStmtText;
        aNode->tableName.offset += sOffset;
    }

    if (aNode->columnName.size > 0)
    {
        aNode->columnName.stmtText = aStmtText;
        aNode->columnName.offset += sOffset;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qcpUtil::makeHiddenColumnNameAndPosition(
    qcStatement           * aStatement,
    qcNamePosition          aIndexName,
    qcNamePosition          aStartPos,
    qcNamePosition          aEndPos,
    qdColumnWithPosition  * aColumn )
{
/***********************************************************************
 *
 * Description :
 *  create index의 parsing시 expression의 정확한 position을 계산하고
 *  expression의 hidden column name을 생성한다.
 *
 *  ex) create index abc on t1(i1+1, i2 asc, func(i3) desc)
 *                             ----  --      --------
 *                              ^    ^          ^
 *                              |    |          |
 *                          +---+ +--+   +------+
 *                          |     |      |
 *      key columns => (abc$idx1, i2, abc$idx2)
 *
 * Implementation :
 *
 ***********************************************************************/

    qdColumnWithPosition  * sCurrColumn;
    UInt                    sColumnNumber = 0;
    qcNamePosition          sPosition;
    qcuSqlSourceInfo        sqlInfo;
    
    /* 처음의 '('를, Expression 위치 조사를 위해 지정한다. */
    SET_POSITION( aColumn->beforePosition, aStartPos );

    /* Expression의 뒷 부분 위치가 없으면, 다음 Expression의 첫 부분 위치로 지정한다. */
    for ( sCurrColumn = aColumn;
          sCurrColumn->next != NULL;
          sCurrColumn = sCurrColumn->next )
    {
        if ( QC_IS_NULL_NAME( sCurrColumn->afterPosition ) == ID_TRUE )
        {
            SET_POSITION( sCurrColumn->afterPosition,
                          sCurrColumn->next->beforePosition );
        }
        else
        {
            /* Expression의 뒷 부분 위치는 ASC 또는 DESC 이다. */
        }
    }

    /* 마지막 ')'를, Expression 위치 조사를 위해 지정한다. */
    if ( QC_IS_NULL_NAME( sCurrColumn->afterPosition ) == ID_TRUE )
    {
        SET_POSITION( sCurrColumn->afterPosition, aEndPos );
    }
    else
    {
        /* Expression의 뒷 부분 위치는 ASC 또는 DESC 이다. */
    }

    /* PROJ-1090 Function-based Index */
    for ( sCurrColumn = aColumn;
          sCurrColumn != NULL;
          sCurrColumn = sCurrColumn->next )
    {
        if ( (sCurrColumn->column->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
             == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
        {
            /* Hidden Column의 Expression 위치를 지정한다. */
            sCurrColumn->column->defaultValue->position.offset =
                sCurrColumn->beforePosition.offset +
                sCurrColumn->beforePosition.size;
            sCurrColumn->column->defaultValue->position.size =
                sCurrColumn->afterPosition.offset -
                sCurrColumn->beforePosition.offset -
                sCurrColumn->beforePosition.size;

            /* Expression 위치의 앞뒤 공백을 제거한다. */
            SET_POSITION( sPosition,
                          sCurrColumn->column->defaultValue->position );
            while ( ( sPosition.stmtText[sPosition.offset] == ' ' ) ||
                    ( sPosition.stmtText[sPosition.offset] == '\t' ) ||
                    ( sPosition.stmtText[sPosition.offset] == '\r' ) ||
                    ( sPosition.stmtText[sPosition.offset] == '\n' ) )
            {
                sPosition.offset++;
                sPosition.size--;
            }
            while ( ( sPosition.stmtText[sPosition.offset + sPosition.size - 1] == ' ' ) ||
                    ( sPosition.stmtText[sPosition.offset + sPosition.size - 1] == '\t' ) ||
                    ( sPosition.stmtText[sPosition.offset + sPosition.size - 1] == '\r' ) ||
                    ( sPosition.stmtText[sPosition.offset + sPosition.size - 1] == '\n' ) )
            {
                sPosition.size--;
            }
            SET_POSITION( sCurrColumn->column->defaultValue->position,
                          sPosition );

            /* Hidden Column 길이가 40을 초과하는지 검사한다. */
            if ( aIndexName.size > QC_MAX_FUNCTION_BASED_INDEX_NAME_LEN )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & aIndexName );
                IDE_RAISE( ERR_TOO_LONG_NAME );
            }
            else
            {
                /* Nothing to do */
            }

            /* Hidden Column의 Name을 생성한다.
             *    Index Name + $ + IDX + Number
             */
            QC_STR_COPY( sCurrColumn->column->name, aIndexName );
            (void)idlVA::appendFormat( sCurrColumn->column->name,
                                       QC_MAX_OBJECT_NAME_LEN + 1,
                                       "$IDX%"ID_UINT32_FMT,
                                       ++sColumnNumber ); // 최대 32
            
            sCurrColumn->column->namePos.stmtText = sCurrColumn->column->name;
            sCurrColumn->column->namePos.offset   = 0;
            sCurrColumn->column->namePos.size     =
                idlOS::strlen( sCurrColumn->column->name );
        }
        else
        {
            /* Nothing to do */
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

void qcpUtil::setLastTokenPosition(
    void            * aLexer,
    qcNamePosition  * aPosition )
{
    ((qcplLexer*)aLexer)->setLastTokenPosition( aPosition );
}

void qcpUtil::setEndTokenPosition(
    void            * aLexer,
    qcNamePosition  * aPosition )
{
    ((qcplLexer*)aLexer)->setEndTokenPosition( aPosition );
}

// BUG-44700 reserved words error msg
void qcpUtil::checkReservedWord( const SChar   * aWord,
                                       UInt      aLen,
                                       idBool  * aIsReservedWord )
{
    UInt i;

    *aIsReservedWord = ID_FALSE;
    
    for ( i = 0; reservedWordTables[i].mWord != NULL ; i++ )
    {
        if ( idlOS::strCaselessMatch( aWord,
                                      aLen,
                                      reservedWordTables[i].mWord,
                                      reservedWordTables[i].mLen ) == 0 )
        {
            *aIsReservedWord = ID_TRUE;

            break;
        }
        else
        {
            // nothing to do
        }
    }
}

