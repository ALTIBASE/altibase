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
 * $Id: qcmCreate.cpp 90270 2021-03-21 23:20:18Z bethy $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <mtuProperty.h>
#include <qcmCreate.h>
#include <qcg.h>
#include <qcpManager.h>
#include <qcmProc.h>
#include <qcmPriv.h>
#include <qcmTrigger.h>
#include <qcmDirectory.h>
#include <qcmXA.h>
#include <qdm.h>
#include <qsx.h>
#include <qcmDatabase.h>

IDE_RC qcmCreate::createDB( idvSQL * aStatistics,
                            SChar  * aDBName, 
                            SChar  * aOwnerDN, 
                            UInt     aOwnerDNLen)
{
    mtcColumn       sTblColumn[QCM_TABLES_COL_CNT];
    mtcColumn       sColColumn[QCM_COLUMNS_COL_CNT];
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement  * sDummySmiStmt;
    UInt            sSmiStmtFlag  = 0;
    iduMemory       sIduMem;

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    // make smiTrans
    sIduMem.init(IDU_MEM_QCM);
    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);

    IDE_TEST(sTrans.begin(&sDummySmiStmt,
                          aStatistics,
                          (SMI_ISOLATION_NO_PHANTOM |
                           SMI_TRANSACTION_NORMAL   |
                           SMI_TRANSACTION_REPL_DEFAULT |
                           SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag)
             != IDE_SUCCESS);
    IDE_TEST_RAISE( qcm::check(&sSmiStmt) == IDE_SUCCESS,
                    ERR_META_ALEADY_EXIST );

    IDE_TEST(createQcmTables( aStatistics, &sSmiStmt, sTblColumn) != IDE_SUCCESS);

    IDE_TEST(createQcmColumns( aStatistics, &sSmiStmt, sTblColumn, sColColumn)
             != IDE_SUCCESS);

    IDE_TEST(createTableIDSequence(&sSmiStmt, sTblColumn)
             != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    // create sequence, create other meta, create index.
    IDE_TEST
        (runDDLforMETA( aStatistics, &sTrans, aDBName, aOwnerDN, aOwnerDNLen)
         != IDE_SUCCESS);

    IDE_TEST(sTrans.begin(&sDummySmiStmt,
                          aStatistics,
                          (SMI_ISOLATION_NO_PHANTOM     |
                           SMI_TRANSACTION_NORMAL       |
                           SMI_TRANSACTION_REPL_DEFAULT |
                           SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
             != IDE_SUCCESS);

    IDE_TEST(makeSignature(&sSmiStmt) != IDE_SUCCESS);

    IDE_TEST(qsx::unloadAllProc( &sSmiStmt, &sIduMem ) != IDE_SUCCESS);

    // PROJ-1073 Package
    IDE_TEST(qsx::unloadAllPkg( &sSmiStmt, &sIduMem ) != IDE_SUCCESS);

    IDE_TEST(qcm::finiMetaCaches(&sSmiStmt) != IDE_SUCCESS);

    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);

    IDE_TEST(sTrans.destroy( aStatistics ) != IDE_SUCCESS);

    sIduMem.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_ALEADY_EXIST );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_ALEADY_EXIST));
    }
    IDE_EXCEPTION_END;

    sIduMem.destroy();
    return IDE_FAILURE;
}

IDE_RC qcmCreate::createQcmTables( idvSQL          * aStatistics,
                                   smiStatement    * aSmiStmt,
                                   mtcColumn       * aTblColumn )
{
/***********************************************************************
 *
 * Description :
 *    메타 테이블 SYS_TABLES_ 생성
 *
 * Implementation :
 *    1. SYS_TABLES_ 테이블의 각 컬럼의 type,offset,id 부여
 *    2. actualSize 를 구한 다음 smiColumnList 를 만든다.
 *    3. create SM table => smiTable::createTable
 *    4. make TempInfo => createTableInfoForCreateDB
 *
 *    PROJ-1705 디스크레코드최적화프로젝트 수행으로
 *    sm에 레코드 전달을 위해 smiValue 자료구조 정보 구성시
 *    아래 함수를 사용해서 memory/disk table의 레코드를
 *    적절히 처리하도록 수정되었으나,
 *    메타테이블은 memory table이므로 기존 코드를 수정하지 않음.
 *    
 ***********************************************************************/

    /*---------------------------------------------------------------*
    CREATE TABLE SYS_TABLES_
    (
        USER_ID                    INTEGER,
        TABLE_ID                   INTEGER,
        TABLE_OID                  BIGINT,
        COLUMN_COUNT               INTEGER,
        TABLE_NAME                 VARCHAR(128),
        TABLE_TYPE                 CHAR(1),    // T : table, S : sequence, V : view
        REPLICATION_COUNT          INTEGER,
        REPLICATION_RECOVERY_COUNT INTEGER,    // PROJ-1608 RECOVERY FROM REPLICATION
        MAXROW                     BIGINT,
        TBS_ID                     INTEGER,
        TBS_NAME                   VARCHAR(128),// PROJ-2375 Global Meta
        PCTFREE                    INTEGER,
        PCTUSED                    INTEGER,
        INIT_TRANS                 INTEGER,
        MAX_TRANS                  INTEGER,
        INITEXTENTS                BIGINT,
        NEXTEXTENTS                BIGINT,
        MINEXTENTS                 BIGINT,
        MAXEXTENTS                 BIGINT,
        IS_PARTITIONED             CHAR(1),  // PROJ-1502 PARTITIONED DISK TABLE
        TEMPORARY                  CHAR(1),  // PROJ-1407 Temporary Table
        HIDDEN                     CHAR(1),  // PROJ-1624 Global Non-partitioned Index      
        ACCESS                     CHAR(1),  // PROJ-2359 Table/Partition Access Option
        PARALLEL_DEGREE            INTEGER,  // PROJ-1071 Parallel query
        USABLE                     CHAR(1),  // TASK-7307 DML Data Consistency in Shard
        SHARD_FLAG                 INTEGER,  // TASK-7307 DML Data Consistency in Shard
        CREATED                    DATE,     // BUG-14394 객체 생성시간
        LAST_DDL_TIME              DATE,     // BUG-14394 DDL 수행시간
    )
    *---------------------------------------------------------------*/

    SInt                  i;
    smiColumnList         sSmiColumnList[QCM_TABLES_COL_CNT];
    mtcColumn           * sColumn = aTblColumn;
    UInt                  sCurrentOffset;
    SChar               * sNullRowValue = NULL;
    smiValue              sNullRow[QCM_TABLES_COL_CNT];
    UInt                  sTableID = QCM_TABLES_TBL_ID;
    qcmTableInfo        * sTableInfo        = NULL;
    smiSegAttr            sSegmentAttr;
    smiSegStorageAttr     sSegmentStoAttr;
    UShort                sMaxAlign = 0;

    SChar           * sColNames[] =
    {
        (SChar*) "USER_ID",
        (SChar*) "TABLE_ID",
        (SChar*) "TABLE_OID",
        (SChar*) "COLUMN_COUNT",
        (SChar*) "TABLE_NAME",
        (SChar*) "TABLE_TYPE",
        (SChar*) "REPLICATION_COUNT",
        (SChar*) "REPLICATION_RECOVERY_COUNT",      // PROJ-1608 RECOVERY FROM REPLICATION
        (SChar*) "MAXROW",
        (SChar*) "TBS_ID",
        (SChar*) "TBS_NAME",
        (SChar*) "PCTFREE",
        (SChar*) "PCTUSED",
        (SChar*) "INIT_TRANS",
        (SChar*) "MAX_TRANS",
        (SChar*) "INITEXTENTS",
        (SChar*) "NEXTEXTENTS",
        (SChar*) "MINEXTENTS",
        (SChar*) "MAXEXTENTS",
        (SChar*) "IS_PARTITIONED",    // PROJ-1502 PARTITIONED DISK TABLE
        (SChar*) "TEMPORARY",         // PROJ-1407 Temporary Table
        (SChar*) "HIDDEN",            // PROJ-1624 Global Non-partitioned Index        
        (SChar*) "ACCESS",            // PROJ-2359 Table/Partition Access Option
        (SChar*) "PARALLEL_DEGREE",   // PROJ-1071 Parallel query
        (SChar*) "USABLE",            // TASK-7307 DML Data Consistency in Shard
        (SChar*) "SHARD_FLAG",        // TASK-7307 DML Data Consistency in Shard
        (SChar*) "CREATED",           // BUG-14394 객체 생성시간
        (SChar*) "LAST_DDL_TIME"      // BUG-14394 DDL 수행시간
    };

    sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_MEMORY);

    //---------------------------
    // set USER_ID INTEGER
    //---------------------------

    i = 0;

    // USER_ID Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set TABLE_ID INTEGER
    //---------------------------

    i = 1;

    // TABLE_ID Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set TABLE_OID BIGINT
    //---------------------------

    i = 2;

    // TABLE_OID Column의 초기화
    // : dataType은 big int, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_BIGINT_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set COLUMN_COUNT INTEGER
    //---------------------------

    i = 3;

    // COLUMN_COUNT Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set TABLE_NAME VARCHAR(128)
    //---------------------------

    i = 4;

    // TABLE_NAME Column의 초기화
    // : dataType은 varchar, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_VARCHAR_ID,
                                     1,
                                     QC_MAX_OBJECT_NAME_LEN,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set TABLE_TYPE CHAR(1)
    //---------------------------

    i = 5;

    // TABLE_TYPE Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set REPLICATION_COUNT
    //---------------------------

    i = 6;

    // REPLICATION_COUNT Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set REPLICATION_RECOVERY_COUNT
    //---------------------------
    
    i = 7;
    
    // REPLICATION_RECOVERY_COUNT Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set MAXROW
    //---------------------------

    i = 8;

    // MAXROW Column의 초기화
    // : dataType은 bigint, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_BIGINT_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set TBS_ID INTEGER
    //---------------------------

    i = 9;

    // TBS_ID Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set TBS_NAME VARCHAR(128)
    //---------------------------
    
    i = 10;
    
    // TBS_NAME Column의 초기화
    // : dataType은 varchar, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                    MTD_VARCHAR_ID,
                                    1,
                                    QC_MAX_OBJECT_NAME_LEN,
                                    0 )
             != IDE_SUCCESS );
    
    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set PCTFREE INTEGER
    //---------------------------

    i = 11;

    // PCTFREE INTEGER Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set PCTUSED INTEGER
    //---------------------------

    i = 12;

    // PCTUSED INTEGER Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set INIT_TRANS INTEGER
    //---------------------------

    i = 13;

    // INIT_TRANS INTEGER Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set MAX_TRANS INTEGER
    //---------------------------

    i = 14;

    // MAX_TRANS INTEGER Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set INITEXTENS BIGINT
    //---------------------------

    i = 15;

    // INITEXTENS Column의 초기화
    // : dataType은 bigint, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_BIGINT_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set NEXTEXTENS BIGINT
    //---------------------------

    i = 16;

    // NEXTEXTENS Column의 초기화
    // : dataType은 bigint, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_BIGINT_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set MINEXTENS BIGINT
    //---------------------------

    i = 17;

    // MINEXTENS Column의 초기화
    // : dataType은 bigint, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_BIGINT_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set MAXEXTENS BIGINT
    //---------------------------

    i = 18;

    // MAXEXTENS Column의 초기화
    // : dataType은 bigint, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_BIGINT_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set IS_PARTITIONED CHAR(1)
    //---------------------------

    i = 19;

    // IS_PARTITIONED CHAR Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set TEMPORARY CHAR(1)
    //---------------------------

    i = 20;

    // TEMPORARY CHAR Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    
    //---------------------------
    // set HIDDEN CHAR(1)
    //---------------------------

    i = 21;

    // HIDDEN CHAR Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set ACCESS CHAR(1)
    //---------------------------
    i = 22;
    // ACCESS CHAR Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set PARALLEL_DEGREE INTEGER
    //---------------------------
    i = 23;
    // PARALLEL DEGREE Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set USABLE CHAR(1)
    //---------------------------
    i = 24;
    // USABLE CHAR Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set SHARD_FLAG INTEGER
    //---------------------------
    i = 25;
    // SHARD_FLAG Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set CREATED DATE
    //---------------------------

    i = 26;

    // CREATED DATE Column의 초기화
    // : dataType은 date, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_DATE_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set LAST_DDL_TIME DATE
    //---------------------------

    i = 27;

    // LAST_DDL_TIME DATE Column의 초기화
    // : dataType은 date, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_DATE_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //-------------------------------------
    // make NULL row
    //-------------------------------------    
    IDU_FIT_POINT( "qcmCreate::createQcmTables::malloc::sNullRowValue",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               sCurrentOffset,
                               (void**)&(sNullRowValue))
             != IDE_SUCCESS);

    for (i = 0; i < QCM_TABLES_COL_CNT; i++)
    {
        // set Tablespace ID
        sColumn[i].column.colSpace = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;

        // set NULL row

        if ( ( sColumn[i].column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_VARIABLE )
        {
            /* BUG-43287 Variable Colum 들의 Align 중 가장 큰 값을 구한다. */
            sMaxAlign = IDL_MAX( sMaxAlign, (UShort)sColumn[i].module->align );
        }
        else
        {
            // Nothing to do.
        }
        
        // PROJ-1391 Variable Null
        // Variable column에 대한 null 처리는
        // smiValue.value에 NULL을 할당한다.
        // fix BUG-14665
        if( ( sColumn[i].column.flag & SMI_COLUMN_TYPE_MASK )
            != SMI_COLUMN_TYPE_FIXED )
        {
            sNullRow[i].value = NULL;
            sNullRow[i].length = 0;
        }
        else
        {
            sNullRow[i].value = sNullRowValue + sColumn[i].column.offset;

            sColumn[i].module->null( sColumn + i,
                                     (void *) sNullRow[i].value );

            sNullRow[i].length =
                sColumn[i].module->actualSize( sColumn + i,
                                               sNullRow[i].value );
        }

        // BUG-44814 smiTable::createTable 을 호출하기전에 통계정보를 clear 해야 한다.
        idlOS::memset( &sColumn[i].column.mStat, 0x00, ID_SIZEOF(smiColumnStat) );

        // make smiColumnList
        sSmiColumnList[i].column = &(sColumn[i].column);
        
        //BUG-43117 : smiColumn에 align값 입력(SYS_TABLES_) 
        sColumn[i].column.align = sColumn[i].module->align;

        if (i == QCM_TABLES_COL_CNT - 1)
        {
            sSmiColumnList[i].next = NULL;
        }
        else
        {
            sSmiColumnList[i].next = &sSmiColumnList[i+1];
        }
    }

    /* BUG-43287 Variable column 들의 Align 중 가장 큰 값을 기록해 둔다. */
    for ( i = 0; i < QCM_TABLES_COL_CNT; i++ )
    {
        sColumn[i].column.maxAlign = sMaxAlign;
    }

    // Memory Table 은 사용하지 않는 속성이지만, 설정하여 전달한다.
    sSegmentAttr.mPctFree =
                  QD_MEMORY_TABLE_DEFAULT_PCTFREE;  // PCTFREE
    sSegmentAttr.mPctUsed =
                  QD_MEMORY_TABLE_DEFAULT_PCTUSED;  // PCTUSED
    sSegmentAttr.mInitTrans =
                  QD_MEMORY_TABLE_DEFAULT_INITRANS; // initial ttl size
    sSegmentAttr.mMaxTrans =
                  QD_MEMORY_TABLE_DEFAULT_MAXTRANS; // maximum ttl size
    sSegmentStoAttr.mInitExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;  // initextents
    sSegmentStoAttr.mNextExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;  // nextextents
    sSegmentStoAttr.mMinExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;  // minextents
    sSegmentStoAttr.mMaxExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;  // maxextents

    // create SM table
    IDE_TEST(
        smiTable::createTable(
            aStatistics,
            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, // tableSpaceID
            sTableID,                            /* FOR XDB */
            aSmiStmt,
            sSmiColumnList,
            ID_SIZEOF(mtcColumn),                // aColumnSize
            NULL,                                // aInfo
            0,                                   // aInfoSize
            sNullRow,                            // aNullRow
            SMI_TABLE_META,                      // aFlag
            0,                                   // aMaxRow
            sSegmentAttr,                        // Segment Attribute
            sSegmentStoAttr,                     // Segment Storage Attribute
            1,                                   // Parallel Degree
            &gQcmTables)                         // table handle
        != IDE_SUCCESS);

    IDU_FIT_POINT( "qcmCreate::createQcmTables::malloc::sTableInfo",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF(qcmTableInfo),
                               (void**)&sTableInfo)
             != IDE_SUCCESS);

    // make TempInfo
    IDE_TEST(createTableInfoForCreateDB(sTableInfo,
                                        sColumn,
                                        QCM_TABLES_COL_CNT,
                                        QCM_TABLES_TBL_ID,
                                        gQcmTables,
                                        (SChar*)"SYS_TABLES_",
                                        sColNames)
             != IDE_SUCCESS);

    smiSetTableTempInfo( gQcmTables, (void*) sTableInfo);

    // To Fix PR-12539
    // 메모리 해제가 두번 될 수 있음.
    IDE_TEST(iduMemMgr::free(sNullRowValue) 
             != IDE_SUCCESS);
    sNullRowValue = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sNullRowValue != NULL )
    {
        (void)iduMemMgr::free(sNullRowValue);
        sNullRowValue = NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( sTableInfo != NULL )
    {
        (void)iduMemMgr::free(sTableInfo);
        sTableInfo = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::createQcmColumns( idvSQL          * aStatistics,
                                    smiStatement    * aSmiStmt,
                                    mtcColumn       * aTblColumn,
                                    mtcColumn       * aColColumn )
{
/***********************************************************************
 *
 * Description :
 *    메타 테이블 SYS_COLUMNS_ 생성
 *
 * Implementation :
 *    1. SYS_COLUMNS_ 테이블의 각 컬럼의 type,offset,id 부여
 *    2. actualSize 를 구한 다음 smiColumnList 를 만든다.
 *    3. create SM table => smiTable::createTable
 *    4. make TempInfo => createTableInfoForCreateDB
 *
 *    PROJ-1705 디스크레코드최적화프로젝트 수행으로
 *    sm에 레코드 전달을 위해 smiValue 자료구조 정보 구성시
 *    아래 함수를 사용해서 memory/disk table의 레코드를
 *    적절히 처리하도록 수정되었으나,
 *    메타테이블은 memory table이므로 기존 코드를 수정하지 않음.
 *
 ***********************************************************************/

    /*---------------------------------------------------------------*
      CREATE TABLE SYS_COLUMNS_
      (
      COLUMN_ID               INTEGER,
      DATA_TYPE               INTEGER,
      LANG_ID                 INTEGER,
      OFFSET                  BIGINT,    // PROJ-1362
      SIZE                    BIGINT,    // PROJ-1362
      USER_ID                 INTEGER,
      TABLE_ID                INTEGER,
      PRECISION               INTEGER,
      SCALE                   INTEGER,
      COLUMN_ORDER            INTEGER,
      COLUMN_NAME             VARCHAR(128),
      IS_NULLABLE             CHAR(1),   // F : not include NULL,
      // T : include NULL
      DEFAULT_VAL             VARCHAR(4000),
      STORE_TYPE              CHAR(1),
      IN_ROW_SIZE             INTEGER,   // PROJ-1557
      REPL_CONDITION          INTEGER,   // PROJ-1638
      IS_HIDDEN               CHAR(1)    // PROJ-1090 Function-based Index
      IS_KEY_PRESERVED        CHAR(1)    // PROJ-2204 Join Update, Delete
      )
      *---------------------------------------------------------------*/

    SInt                 i;
    smiColumnList        sSmiColumnList[QCM_COLUMNS_COL_CNT];
    mtcColumn          * sColumn = aColColumn;
    UInt                 sCurrentOffset;
    SChar              * sNullRowValue = NULL;
    smiValue             sNullRow[QCM_COLUMNS_COL_CNT];
    UInt                 sTableID = QCM_COLUMNS_TBL_ID;
    qcmTableInfo         *sTableInfo         = NULL;
    smiSegAttr           sSegmentAttr;
    smiSegStorageAttr    sSegmentStoAttr;
    UShort               sMaxAlign = 0;


    SChar             *sColNames[] =
        {
            (SChar*) "COLUMN_ID",       // 0
            (SChar*) "DATA_TYPE",       // 1
            (SChar*) "LANG_ID",         // 2
            (SChar*) "OFFSET",          // 3
            (SChar*) "SIZE",            // 4
            (SChar*) "USER_ID",         // 5
            (SChar*) "TABLE_ID",        // 6
            (SChar*) "PRECISION",       // 7
            (SChar*) "SCALE",           // 8
            (SChar*) "COLUMN_ORDER",    // 9
            (SChar*) "COLUMN_NAME",     // 10
            (SChar*) "IS_NULLABLE",     // 11
            (SChar*) "DEFAULT_VAL",     // 12
            (SChar*) "STORE_TYPE",      // 13
            (SChar*) "IN_ROW_SIZE",     // 14
            (SChar*) "REPL_CONDITION",  // 15
            (SChar*) "IS_HIDDEN",       // 16
            (SChar*) "IS_KEY_PRESERVED"  // 17
        };

    sCurrentOffset = smiGetRowHeaderSize(SMI_TABLE_MEMORY);

    //---------------------------
    // set COLUMN_ID INTEGER
    // set DATA_TYPE INTEGER
    // set LANG_ID   INTEGER
    //---------------------------

    for (i = 0; i < 3; i++)
    {
        // 각 Column의 초기화
        // : dataType은 integer, language는 default로 설정
        IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                         MTD_INTEGER_ID,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        // offset 설정
        sCurrentOffset =
            idlOS::align(sCurrentOffset, sColumn[i].module->align);
        sColumn[i].column.offset = sCurrentOffset;
        sCurrentOffset += sColumn[i].column.size;
        sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
        // PROJ-1557 vcInOutBaseSize를 0으로 설정
        sColumn[i].column.vcInOutBaseSize = 0;
        sColumn[i].column.value = NULL;
    }

    //---------------------------
    // set OFFSET    BIGINT
    // set SIZE      BIGINT
    //---------------------------

    for ( ; i < 5; i++)
    {
        // 각 Column의 초기화
        // : dataType은 bigint, language는 default로 설정
        IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                         MTD_BIGINT_ID,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        // offset 설정
        sCurrentOffset =
            idlOS::align(sCurrentOffset, sColumn[i].module->align);
        sColumn[i].column.offset = sCurrentOffset;
        sCurrentOffset += sColumn[i].column.size;
        sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
        // PROJ-1557 vcInOutBaseSize를 0으로 설정
        sColumn[i].column.vcInOutBaseSize = 0;
        sColumn[i].column.value = NULL;
    }

    //---------------------------
    // set USER_ID      INTEGER
    // set TABLE_ID     INTEGER
    // set PRECISION    INTEGER
    // set SCALE        INTEGER
    // set COLUMN_ORDER INTEGER
    //---------------------------

    for ( ; i < 10; i++)
    {
        // 각 Column의 초기화
        // : dataType은 integer, language는 default로 설정
        IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                         MTD_INTEGER_ID,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );

        // offset 설정
        sCurrentOffset =
            idlOS::align(sCurrentOffset, sColumn[i].module->align);
        sColumn[i].column.offset = sCurrentOffset;
        sCurrentOffset += sColumn[i].column.size;
        sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
        // PROJ-1557 vcInOutBaseSize를 0으로 설정
        sColumn[i].column.vcInOutBaseSize = 0;
        sColumn[i].column.value = NULL;
    }

    //---------------------------
    // set COLUMN_NAME VARCHAR(128)
    //---------------------------

    i = 10;

    // COLUMN_NAME Column의 초기화
    // : dataType은 varchar, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_VARCHAR_ID,
                                     1,
                                     QC_MAX_OBJECT_NAME_LEN,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // set IS_NULLABLE CHAR(1)
    //---------------------------

    i = 11;

    // IS_NULLABLE Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // To Fix PR-5795
    // Variable Column으로 지정함.
    // set DEFAULT_VAL VARCHAR(4000)
    //---------------------------

    i = 12;

    // DEFAULT_VAL Column의 초기화
    // : dataType은 varchar, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_VARCHAR_ID,
                                     1,
                                     4000,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sColumn[i].column.flag = SMI_COLUMN_TYPE_VARIABLE;
    sColumn[i].column.offset = 0;               /* 32000 이하의 variable column offset 은 항상 0 */
    sColumn[i].column.varOrder = 0;             /* variable column의 순서, 여기서는 1개뿐이니까 0 */
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // set STORE_TYPE CHAR(1)
    //---------------------------

    i = 13;

    // STORE_TYPE Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;


    //---------------------------
    // PROJ-1557 varchar32k
    // set IN_ROW_SIZE INTEGER
    //---------------------------

    i = 14;

    // IN_ROW_SIZE Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset =
        idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // PROJ-1638 selectionRP
    // set REPLI_CONDITION
    //---------------------------

    i = 15;

    // REPL_CONDITION Column의 초기화
    // : dataType은 integer, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_INTEGER_ID,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // PROJ-1090 Function-based Index
    // set IS_HIDDEN CHAR(1)
    //---------------------------

    i = 16;

    // HIDDEN_COLUMN Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    //---------------------------
    // PROJ-2204 Join Update, Delete
    // set IS_KEY_PRESERVED CHAR(1)
    //---------------------------

    i = 17;

    // HIDDEN_COLUMN Column의 초기화
    // : dataType은 char, language는 default로 설정
    IDE_TEST( mtc::initializeColumn( & sColumn[i],
                                     MTD_CHAR_ID,
                                     1,
                                     1,
                                     0 )
              != IDE_SUCCESS );

    // offset 설정
    sCurrentOffset = idlOS::align(sCurrentOffset, sColumn[i].module->align);
    sColumn[i].column.offset = sCurrentOffset;
    sCurrentOffset += sColumn[i].column.size;
    sColumn[i].column.id = (sTableID * SMI_COLUMN_ID_MAXIMUM) + i;
    // PROJ-1557 vcInOutBaseSize를 0으로 설정
    sColumn[i].column.vcInOutBaseSize = 0;
    sColumn[i].column.value = NULL;

    // make NULL row
    IDU_LIMITPOINT("qcmCreate::createQcmColumns::malloc1");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               sCurrentOffset,
                               (void**)&sNullRowValue)
             != IDE_SUCCESS);

    for (i = 0; i < QCM_COLUMNS_COL_CNT; i++)
    {
        // set Tablespace ID
        sColumn[i].column.colSpace = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;

        // set NULL row

        // PROJ-1391 Variable Null
        // Variable column에 대한 null 처리는
        // smiValue.value에 NULL을 할당한다.
        // fix BUG-14665
        if( ( sColumn[i].column.flag & SMI_COLUMN_TYPE_MASK )
            == SMI_COLUMN_TYPE_VARIABLE )
        {
            sNullRow[i].value = NULL;
            sNullRow[i].length = 0;
            /* BUG-43287 Variable Colum 들의 Align 중 가장 큰 값을 구한다. */
            sMaxAlign = ( sMaxAlign > (UShort)sColumn[i].module->align ) ? sMaxAlign : (UShort)sColumn[i].module->align;
        }
        else
        {
            sNullRow[i].value = sNullRowValue + sColumn[i].column.offset;

            // To Fix PR-5709
            // Variable Column을 처리할 수 없음.
            // sColumn[i].module->null( sColumn + i,
            //                          sNullRowValue,
            //                          MTD_OFFSET_USE );
            sColumn[i].module->null( sColumn + i,
                                     (void *) sNullRow[i].value );

            sNullRow[i].length =
                sColumn[i].module->actualSize(sColumn + i,
                                              sNullRow[i].value );
        }

        // make smiColumnList
        // BUG-43117 : smiColumn에 align값 입력(SYS_COLUMNS_)
        sColumn[i].column.align = sColumn[i].module->align;

        // BUG-44814 smiTable::createTable 을 호출하기전에 통계정보를 clear 해야 한다.
        idlOS::memset( &sColumn[i].column.mStat, 0x00, ID_SIZEOF(smiColumnStat) );
        
        sSmiColumnList[i].column = &(sColumn[i].column);

        if (i == QCM_COLUMNS_COL_CNT - 1)
        {
            sSmiColumnList[i].next = NULL;
        }
        else
        {
            sSmiColumnList[i].next = &sSmiColumnList[i+1];
        }
    }

    /* BUG-43287 Variable column 들의 Align 중 가장 큰 값을 기록해 둔다. */
    for ( i = 0; i < QCM_COLUMNS_COL_CNT; i++ )
    {
        sColumn[i].column.maxAlign = sMaxAlign;
    }

    // Memory Table 은 사용하지 않는 속성이지만, 설정하여 전달한다.
    sSegmentAttr.mPctFree = 
                  QD_MEMORY_TABLE_DEFAULT_PCTFREE;  // PCTFREE
    sSegmentAttr.mPctUsed = 
                  QD_MEMORY_TABLE_DEFAULT_PCTUSED;  // PCTUSED
    sSegmentAttr.mInitTrans = 
                  QD_MEMORY_TABLE_DEFAULT_INITRANS;  // initial ttl size
    sSegmentAttr.mMaxTrans = 
                  QD_MEMORY_TABLE_DEFAULT_MAXTRANS;  // maximum ttl size
    sSegmentStoAttr.mInitExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;  // initextents
    sSegmentStoAttr.mNextExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;  // nextextents
    sSegmentStoAttr.mMinExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;  // minextents
    sSegmentStoAttr.mMaxExtCnt =
                  QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;  // maxextents

    // create SM table
    IDE_TEST( smiTable::createTable(
                  aStatistics,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, // tableSpaceID
                  sTableID,                            /* FOR XDB */
                  aSmiStmt,
                  sSmiColumnList,
                  ID_SIZEOF(mtcColumn),                // aColumnSize
                  NULL,                                // aInfo
                  0,                                   // aInfoSize
                  sNullRow,                            // aNullRow
                  SMI_TABLE_META,                      // aFlag
                  0,                                   // aMaxRow
                  sSegmentAttr,                        // Segment Attribute
                  sSegmentStoAttr,                     // Segment Storage Attribute
                  1,                                   // Parallel Degree 
                  &gQcmColumns)                        // table handle
              != IDE_SUCCESS);

    IDU_LIMITPOINT("qcmCreate::createQcmColumns::malloc2");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF(qcmTableInfo),
                               (void**)&sTableInfo)
             != IDE_SUCCESS);

    // insert into table(qcm_tables_, qcm_columns_) values
    IDE_TEST( insertIntoQcmTables( aSmiStmt,
                                   aTblColumn )
              != IDE_SUCCESS);

    IDE_TEST(insertIntoQcmColumns(aSmiStmt, aTblColumn, sColumn)
             != IDE_SUCCESS);

    // make TempInfo
    IDE_TEST(createTableInfoForCreateDB(sTableInfo,
                                        sColumn,
                                        QCM_COLUMNS_COL_CNT,
                                        sTableID,
                                        gQcmColumns,
                                        (SChar*)"SYS_COLUMNS_",
                                        sColNames)
             != IDE_SUCCESS);

    smiSetTableTempInfo( gQcmColumns, (void*)sTableInfo );

    // To Fix PR-12539
    IDE_TEST(iduMemMgr::free(sNullRowValue) != IDE_SUCCESS);
    sNullRowValue = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sNullRowValue != NULL )
    {
        (void)iduMemMgr::free(sNullRowValue);
        sNullRowValue = NULL;
    }
    else
    {
        // Nothing to do.
    }

    if ( sTableInfo != NULL )
    {
        (void)iduMemMgr::free(sTableInfo);
        sTableInfo = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::createTableIDSequence( smiStatement *aSmiStmt,
                                         mtcColumn    *aColumn )
{
/*********************************************************************
 *    PROJ-1705 디스크레코드최적화프로젝트 수행으로
 *    sm에 레코드 전달을 위해 smiValue 자료구조 정보 구성시
 *    아래 함수를 사용해서 memory/disk table의 레코드를
 *    적절히 처리하도록 수정되었으나,
 *    메타테이블은 memory table이므로 기존 코드를 수정하지 않음.
 **********************************************************************/

    const void       * sTableIDSeqHandle;
    smiTableCursor     sCursor;
    smiValue           sValues[QCM_TABLES_COL_CNT];
    UInt               sUserID = QC_SYSTEM_USER_ID;
    UInt               sTableID = QCM_TABLEID_SEQ_TBL_ID;
    smOID              sQcmSequenceOID;
    ULong              sSequenceOIDULong;

    UInt               sColumnCount = QCM_SEQUENCE_COL_COUNT;

    qcNameCharBuffer   sTblNameBuffer;
    mtdCharType      * sTblName = (mtdCharType *) & sTblNameBuffer;

    qcNameCharBuffer   sTBSNameBuffer;
    mtdCharType      * sTBSName = ( mtdCharType * ) & sTBSNameBuffer;

    qcNameCharBuffer   sTypeBuffer;
    mtdCharType      * sType    = (mtdCharType *) & sTypeBuffer;

    mtdDateType        sSysDate;
    void             * sDummyRow;
    scGRID             sDummyGRID;

    // PROJ-1502 PARTITIONED DISK TABLE
    qcNameCharBuffer   sIsPartitionedBuffer;
    mtdCharType      * sIsPartitioned
        = ( mtdCharType * ) & sIsPartitionedBuffer;

    // PROJ-1407 Temporary Table
    qcNameCharBuffer   sTemporaryBuffer;
    mtdCharType      * sTemporary
        = ( mtdCharType * ) & sTemporaryBuffer;

    // PROJ-1624 Global Non-partitioned Index
    qcNameCharBuffer   sHiddenBuffer;
    mtdCharType      * sHidden
        = ( mtdCharType * ) & sHiddenBuffer;

    // PROJ-1071 Parallel query
    UInt               sParallelDegree = 1;
    
    /* PROJ-2359 Table/Partition Access Option */
    qcNameCharBuffer   sAccessBuffer;
    mtdCharType      * sAccess
        = ( mtdCharType * ) & sAccessBuffer;

    /* TASK-7307 DML Data Consistency in Shard */
    qcNameCharBuffer   sUsableBuffer;
    mtdCharType      * sUsable
        = ( mtdCharType * ) & sUsableBuffer;
    UInt               sShardFlag = 0;

    UInt               sReplCount = 0;
    UInt               sReplRecoveryCount = 0;
    ULong              sMaxRows   = 0;
    mtdIntegerType     sIntDataOfTBSID
        = (mtdIntegerType) SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
    UInt               sPctFree
        = QD_MEMORY_TABLE_DEFAULT_PCTFREE;
    UInt               sPctUsed
        = QD_MEMORY_TABLE_DEFAULT_PCTUSED;
    UInt               sInitTrans
        = QD_MEMORY_TABLE_DEFAULT_INITRANS;
    UInt               sMaxTrans
        = QD_MEMORY_TABLE_DEFAULT_MAXTRANS;
    SInt               i;
    UInt               sStage = 0;
    UInt               sInitExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;
    UInt               sNextExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;
    UInt               sMinExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;
    UInt               sMaxExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;

    sCursor.initialize();

    IDE_TEST(smiTable::createSequence(aSmiStmt,
                                      QCM_TABLES_SEQ_STARTVALUE,
                                      QCM_TABLES_SEQ_INCREMETVALUE,
                                      QCM_TABLES_SEQ_CACHEVALUE,
                                      QCM_TABLES_SEQ_MAXVALUE,
                                      QCM_TABLES_SEQ_MINVALUE,
                                      SMI_SEQUENCE_CIRCULAR_ENABLE,
                                      &sTableIDSeqHandle)
             != IDE_SUCCESS);

    sQcmSequenceOID = smiGetTableId(sTableIDSeqHandle);
    sSequenceOIDULong = sQcmSequenceOID;

    IDE_TEST( sCursor.open(aSmiStmt,
                           gQcmTables,
                           NULL,       // aIndex
                           smiGetRowSCN(gQcmTables),
                           NULL,       // aColumns
                           smiGetDefaultKeyRange(),
                           smiGetDefaultKeyRange(),
                           smiGetDefaultFilter(),
                           SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                           SMI_INSERT_CURSOR,
                           &gMetaDefaultCursorProperty)
              != IDE_SUCCESS );

    sStage = 1;

    // USER_ID
    sValues[0].length = ID_SIZEOF(sUserID);
    sValues[0].value  = &sUserID;

    // TABLE_ID
    sValues[1].length = ID_SIZEOF(sTableID);
    sValues[1].value  = &sTableID;

    // TABLE_OID
    sValues[2].length = ID_SIZEOF(sSequenceOIDULong);
    sValues[2].value  = &sSequenceOIDULong;

    // COLUMN_COUNT
    sValues[3].length = ID_SIZEOF(sColumnCount);
    sValues[3].value  = &sColumnCount;

    // TABLE_NAME
    i = 4;
    qtc::setVarcharValue( sTblName,
                          &(aColumn[i]),
                          gDBSequenceName[QCM_DB_SEQUENCE_TABLEID],
                          0 );
    sValues[i].value = (void *) sTblName;
    sValues[i].length =
        aColumn[i].module->actualSize( aColumn + i,
                                       sValues[i].value );

    // TABLE_TYPE
    i = 5;
    qtc::setCharValue( sType,
                       &(aColumn[i]),
                       (SChar*) "S" );
    sValues[i].value = (void *) sType;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // REPLICATION_COUNT
    i = 6;
    sValues[i].length = ID_SIZEOF(sReplCount);
    sValues[i].value = &sReplCount;

    // REPLICATION_RECOVERY_COUNT proj-1608 recovery from replication
    i = 7;
    sValues[i].length = ID_SIZEOF(sReplRecoveryCount);
    sValues[i].value = &sReplRecoveryCount;

    // MAXROW
    i = 8;
    sValues[i].length = ID_SIZEOF(sMaxRows);
    sValues[i].value = &sMaxRows;

    // TBS_ID
    i = 9;
    sValues[i].length = ID_SIZEOF(sIntDataOfTBSID);
    sValues[i].value = &sIntDataOfTBSID;

    // TBS_NAME
    i = 10;
    qtc::setVarcharValue( sTBSName,
                         &(aColumn[i]),
                         (SChar*) SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC,
                         0);
    sValues[i].value = (void *) sTBSName;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                      sValues[i].value );
    // PCTFREE
    i = 11;
    sValues[i].length = ID_SIZEOF(sPctFree);
    sValues[i].value = &sPctFree;

    // PCTUSED
    i = 12;
    sValues[i].length = ID_SIZEOF(sPctUsed);
    sValues[i].value = &sPctUsed;

    // INIT_TRANS
    i = 13;
    sValues[i].length = ID_SIZEOF(sInitTrans);
    sValues[i].value = &sInitTrans;
    
    // INIT_TRANS
    i = 14;
    sValues[i].length = ID_SIZEOF(sMaxTrans);
    sValues[i].value = &sMaxTrans;
    
    // INITEXTENTS
    i = 15;
    sValues[i].length = ID_SIZEOF(sInitExtents);
    sValues[i].value = &sInitExtents;

    // NEXTEXTENTS
    i = 16;
    sValues[i].length = ID_SIZEOF(sNextExtents);
    sValues[i].value = &sNextExtents;

    // MINEXTENTS
    i = 17;
    sValues[i].length = ID_SIZEOF(sMinExtents);
    sValues[i].value = &sMinExtents;

    // MAXEXTENTS
    i = 18;
    sValues[i].length = ID_SIZEOF(sMaxExtents);
    sValues[i].value = &sMaxExtents;

    // PROJ-1502 PARTITIONED DISK TABLE
    // IS_PARTITIONED
    i = 19;
    qtc::setCharValue( sIsPartitioned, &(aColumn[i]), (SChar*)"F");
    sValues[i].value = (void *) sIsPartitioned;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );
    
    // PROJ-1407 Temporary Table
    // TEMPORARY
    i = 20;
    qtc::setCharValue( sTemporary, &(aColumn[i]), (SChar*)"N");
    sValues[i].value = (void *) sTemporary;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // PROJ-1624 Global Non-partitioned Index
    // HIDDEN
    i = 21;
    qtc::setCharValue( sHidden, &(aColumn[i]), (SChar*)"N");
    sValues[i].value = (void *) sHidden;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    /* PROJ-2359 Table/Partition Access Option */
    // ACCESS
    i = 22;
    qtc::setCharValue( sAccess, &(aColumn[i]), (SChar*)"W");
    sValues[i].value = (void *) sAccess;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // PROJ-1071 Parallel query
    // PARALLEL_DEGREE
    i = 23;
    sValues[i].length = ID_SIZEOF(sParallelDegree);
    sValues[i].value = &sParallelDegree;

    /* TASK-7307 DML Data Consistency in Shard */
    // USABLE
    i = 24;
    qtc::setCharValue( sUsable, &(aColumn[i]), (SChar*)"Y");
    sValues[i].value = (void *) sUsable;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );
    // SHARD_FLAG
    i = 25;
    sValues[i].length = ID_SIZEOF(sShardFlag);
    sValues[i].value = &sShardFlag;

    // BUG-14394
    qtc::sysdate( &sSysDate );

    // CREATED
    i = 26;
    sValues[i].value = (void *)&sSysDate;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // BUG-14394
    // LAST_DDL_TIME
    i = 27;
    sValues[i].value = (void *)&sSysDate;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );
    // insert record
    IDE_TEST(sCursor.insertRow( sValues,
                                &sDummyRow,
                                &sDummyGRID )
             != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::createTableInfoForCreateDB( qcmTableInfo  * aTableInfo,
                                              mtcColumn     * aMtcColumn,
                                              UInt            aColCount,
                                              UInt            aTableID,
                                              const void    * aTableHandle,
                                              SChar         * aTableName,
                                              SChar         * aColumnNames[])
{
    qcmColumn * sQcmColumn;
    UInt        i;
    UInt        sState = 0;

    IDU_LIMITPOINT("qcmCreate::createTableInfoForCreateDB::malloc");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QCM,
                               ID_SIZEOF(qcmColumn) * aColCount,
                               (void**)&sQcmColumn)
             != IDE_SUCCESS);
    sState = 1;

    aTableInfo->tableOwnerID     = QC_SYSTEM_USER_ID;
    aTableInfo->columnCount      = aColCount;
    aTableInfo->indexCount       = 0;
    aTableInfo->indices          = NULL;
    aTableInfo->primaryKey       = NULL;
    aTableInfo->foreignKeyCount  = 0;
    aTableInfo->foreignKeys      = NULL;
    aTableInfo->columns          = sQcmColumn;
    aTableInfo->uniqueKeyCount   = 0;
    aTableInfo->uniqueKeys       = NULL;
    aTableInfo->privilegeCount   = 0;
    aTableInfo->privilegeInfo    = NULL;
    aTableInfo->replicationCount = 0;
    aTableInfo->replicationRecoveryCount = 0; //PROJ-1608
    aTableInfo->notNullCount     = 0;
    aTableInfo->notNulls         = NULL;
    aTableInfo->checkCount       = 0;       /* PROJ-1107 Check Constraint 지원 */
    aTableInfo->checks           = NULL;    /* PROJ-1107 Check Constraint 지원 */
    aTableInfo->tableType        = QCM_META_TABLE;
    aTableInfo->mPVType          = QCM_PV_TYPE_NONE;    /* BUG-45646 */
    aTableInfo->maxrows          = 0;
    aTableInfo->TBSID            = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
    aTableInfo->TBSType          = SMI_MEMORY_SYSTEM_DICTIONARY;
    aTableInfo->segAttr.mPctFree
                                 = QD_MEMORY_TABLE_DEFAULT_PCTFREE;
    aTableInfo->segAttr.mPctUsed
                                 = QD_MEMORY_TABLE_DEFAULT_PCTUSED;
    aTableInfo->segAttr.mInitTrans
                                 = QD_MEMORY_TABLE_DEFAULT_INITRANS;
    aTableInfo->segAttr.mMaxTrans
                                 = QD_MEMORY_TABLE_DEFAULT_MAXTRANS;
    aTableInfo->segStoAttr.mInitExtCnt
                                 = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;
    aTableInfo->segStoAttr.mNextExtCnt
                                 = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;
    aTableInfo->segStoAttr.mMinExtCnt
                                 = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;
    aTableInfo->segStoAttr.mMaxExtCnt
                                 = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;
    aTableInfo->triggerCount     = 0;
    aTableInfo->triggerInfo      = NULL;
    aTableInfo->lobColumnCount   = 0;   // PROJ-1362
    
    // PROJ-1502 PARTITIONED DISK TABLE
    aTableInfo->partitionMethod  = QCM_PARTITION_METHOD_NONE;
    aTableInfo->tablePartitionType
                                 = QCM_NONE_PARTITIONED_TABLE;

    // PROJ-1407 Temporary Table
    aTableInfo->temporaryInfo.type = QCM_TEMPORARY_ON_COMMIT_NONE;

    // PROJ-1624 Global Non-partitioned Index
    aTableInfo->hiddenType         = QCM_HIDDEN_NONE;

    /* PROJ-2359 Table/Partition Access Option */
    aTableInfo->accessOption     = QCM_ACCESS_OPTION_READ_WRITE;

    // PROJ-1071 Parallel query
    aTableInfo->parallelDegree   = 1;

    aTableInfo->tableFlag       = 0;
    aTableInfo->isDictionary    = ID_FALSE;
    aTableInfo->viewReadOnly    = QCM_VIEW_NON_READ_ONLY;

    // TASK-7307 DML Data Consistency in Shard
    aTableInfo->mIsUsable       = ID_TRUE;
    aTableInfo->mShardFlag      = QCM_SHARD_FLAG_TABLE_NONE;

    // BUG-13725
    qcm::setOperatableFlag( aTableInfo->tableType,
                            &aTableInfo->operatableFlag );

    for (i = 0; i < aColCount; i++)
    {
        sQcmColumn[i].flag = 0;
        sQcmColumn[i].basicInfo = &aMtcColumn[i];

        // mtdModule 설정
        IDE_TEST(mtd::moduleById( &(sQcmColumn[i].basicInfo->module),
                                  sQcmColumn[i].basicInfo->type.dataTypeId )
                 != IDE_SUCCESS);

        // mtlModule 설정
        IDE_TEST(mtl::moduleById( &(sQcmColumn[i].basicInfo->language),
                                  sQcmColumn[i].basicInfo->type.languageId )
                 != IDE_SUCCESS);

        idlOS::snprintf(sQcmColumn[i].name, QC_MAX_OBJECT_NAME_LEN + 1, "%s", aColumnNames[i]);

        if ( idlOS::strMatch( sQcmColumn[i].name,
                              idlOS::strlen( sQcmColumn[i].name ),
                              "DEFAULT_VAL",
                              11 ) == 0 )
        {
            sQcmColumn[i].basicInfo->flag &= ~MTC_COLUMN_NOTNULL_TRUE;
        }
        else
        {
            sQcmColumn[i].basicInfo->flag |= MTC_COLUMN_NOTNULL_TRUE;
        }

        if (i == aColCount -1)
        {
            sQcmColumn[i].next = NULL;
        }
        else
        {
            sQcmColumn[i].next = &(sQcmColumn[i+1]);
        }
        sQcmColumn[i].defaultValueStr = NULL;
    }

    idlOS::snprintf(aTableInfo->name,
                    QC_MAX_OBJECT_NAME_LEN + 1,
                    "%s",
                    aTableName);

    /* BUG-36319 Conditional jump or move depends on uninitialised value */
    idlOS::snprintf(aTableInfo->tableOwnerName,
                    QC_MAX_OBJECT_NAME_LEN + 1,
                    "%s",
                    QC_SYSTEM_USER_NAME);

    aTableInfo->tableID = aTableID;
    aTableInfo->tableHandle = (void *)aTableHandle;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free(sQcmColumn);
            /* fall through */
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::insertIntoQcmTables( smiStatement    * aSmiStmt,
                                       mtcColumn       * aColumn )
{
/***********************************************************************
 *
 * Description :
 *    insert into SYS_TABLES_ values
 *
 * Implementation :
 *    1. SYS_TABLES_ 테이블 정보를 SYS_TABLES_ 테이블에 입력
 *    2. SYS_COLUMNS_ 테이블 정보를 SYS_TABLES_ 테이블에 입력
 *
 *    PROJ-1705 디스크레코드최적화프로젝트 수행으로
 *    sm에 레코드 전달을 위해 smiValue 자료구조 정보 구성시
 *    아래 함수를 사용해서 memory/disk table의 레코드를
 *    적절히 처리하도록 수정되었으나,
 *    메타테이블은 memory table이므로 기존 코드를 수정하지 않음.
 ***********************************************************************/

    UInt               sStage = 0;
    smOID              sQcmTablesOID;
    smOID              sQcmColumnsOID;
    smiTableCursor     sCursor;
    SInt               i;
    UInt               sUserID;
    UInt               sTableID;
    ULong              sTableOIDULong;
    UInt               sColumnCount;
    smiValue           sValues[QCM_TABLES_COL_CNT];
    mtdDateType        sSysDate;

    qcNameCharBuffer   sTblNameBuffer;
    mtdCharType      * sTblName = ( mtdCharType * ) & sTblNameBuffer;
    
    qcNameCharBuffer   sTBSNameBuffer;
    mtdCharType      * sTBSName = ( mtdCharType * ) & sTBSNameBuffer;

    qcNameCharBuffer   sTypeBuffer;
    mtdCharType      * sType    = ( mtdCharType * ) & sTypeBuffer;

    void             * sDummyRow;
    scGRID             sDummyGRID;

    // PROJ-1502 PARTITIONED DISK TABLE
    qcNameCharBuffer   sIsPartitionedBuffer;
    mtdCharType      * sIsPartitioned = (mtdCharType *) &sIsPartitionedBuffer;

    // PROJ-1407 Temporary Table
    qcNameCharBuffer   sTemporaryBuffer;
    mtdCharType      * sTemporary = ( mtdCharType * ) & sTemporaryBuffer;

    // PROJ-1624 Global Non-partitoned Index
    qcNameCharBuffer   sHiddenBuffer;
    mtdCharType      * sHidden = ( mtdCharType * ) & sHiddenBuffer;

    // PROJ-1071 Parallel query
    UInt               sParallelDegree = 1;
    
    /* PROJ-2359 Table/Partition Access Option */
    qcNameCharBuffer   sAccessOptionBuffer;
    mtdCharType      * sAccessOption = (mtdCharType *)&sAccessOptionBuffer;

    /* TASK-7307 DML Data Consistency in Shard */
    qcNameCharBuffer   sUsableBuffer;
    mtdCharType      * sUsable = (mtdCharType *)&sUsableBuffer;
    UInt               sShardFlag = 0;

    UInt               sReplCount = 0;
    ULong              sMaxRows = 0;
    mtdIntegerType     sIntDataOfTBSID
        = (mtdIntegerType) SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC;
    UInt               sPctFree
        = QD_MEMORY_TABLE_DEFAULT_PCTFREE;
    UInt               sPctUsed
        = QD_MEMORY_TABLE_DEFAULT_PCTUSED;
    UInt               sInitTrans
        = QD_MEMORY_TABLE_DEFAULT_INITRANS;
    UInt               sMaxTrans
        = QD_MEMORY_TABLE_DEFAULT_MAXTRANS;
    UInt               sReplRecoveryCount = 0;
    UInt               sInitExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS;
    UInt               sNextExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS;
    UInt               sMinExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;
    UInt               sMaxExtents
        = QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;

    sQcmTablesOID  = smiGetTableId(gQcmTables);
    sQcmColumnsOID = smiGetTableId(gQcmColumns); 

    sCursor.initialize();

    //----- insert into QCM_TABLES values -----//
    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  gQcmTables,
                  NULL,       // aIndex
                  smiGetRowSCN(gQcmTables),
                  NULL,       // aColumns
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                  SMI_INSERT_CURSOR,
                  & gMetaDefaultCursorProperty )
              != IDE_SUCCESS );
    sStage = 1;

    sUserID = QC_SYSTEM_USER_ID;

    // USER_ID
    sValues[0].length = ID_SIZEOF(sUserID);
    sValues[0].value  = &sUserID;

    // TABLE_ID
    sValues[1].length = ID_SIZEOF(sTableID);
    sValues[1].value  = &sTableID;

    // TABLE_OID
    sValues[2].length = ID_SIZEOF(sTableOIDULong);
    sValues[2].value  = &sTableOIDULong;

    // COLUMN_COUNT
    sValues[3].length = ID_SIZEOF(sColumnCount);
    sValues[3].value  = &sColumnCount;

    /* (1) ( QC_SYSTEM_USER_ID,
       QCM_TABLES_TBL_ID,
       sQcmTablesOID,
       QCM_TABLES_COL_CNT,
       'SYS_TABLES_',
       'T',
       0,
       'T',
       0)               */
    sTableID     = QCM_TABLES_TBL_ID;
    sTableOIDULong = sQcmTablesOID;
    sColumnCount = QCM_TABLES_COL_CNT;

    // TABLE_NAME
    i = 4;
    qtc::setVarcharValue( sTblName,
                          &(aColumn[i]),
                          (SChar*) QCM_TABLES,
                          0);
    sValues[i].value = (void *) sTblName;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // TABLE_TYPE
    i = 5;
    qtc::setCharValue( sType, &(aColumn[i]), (SChar*)"T");
    sValues[i].value = (void *) sType;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // REPLICATION_COUNT
    i = 6;
    sValues[i].length = ID_SIZEOF(sReplCount);
    sValues[i].value = &sReplCount;

    // REPLICATION_RECOVERY_COUNT proj-1608
    i = 7;
    sValues[i].length = ID_SIZEOF(sReplRecoveryCount);
    sValues[i].value = &sReplRecoveryCount;

    // MAXROW
    i = 8;
    sValues[i].length = ID_SIZEOF(sMaxRows);
    sValues[i].value = &sMaxRows;

    // TBS_ID
    i = 9;
    sValues[i].length = ID_SIZEOF(sIntDataOfTBSID);
    sValues[i].value = &sIntDataOfTBSID;

    // TBS_NAME
    i = 10;
    qtc::setVarcharValue( sTBSName,
                         &(aColumn[i]),
                         (SChar*) SMI_TABLESPACE_NAME_SYSTEM_MEMORY_DIC,
                         0);
    sValues[i].value = (void *) sTBSName;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                      sValues[i].value );
    // PCTFREE
    i = 11;
    sValues[i].length = ID_SIZEOF(sPctFree);
    sValues[i].value = &sPctFree;

    // PCTUSED
    i = 12;
    sValues[i].length = ID_SIZEOF(sPctUsed);
    sValues[i].value = &sPctUsed;

    // INIT_TRANS
    i = 13;
    sValues[i].length = ID_SIZEOF(sInitTrans);
    sValues[i].value = &sInitTrans;

    // MAX_TRANS
    i = 14;
    sValues[i].length = ID_SIZEOF(sMaxTrans);
    sValues[i].value = &sMaxTrans;

    // INITEXTENTS
    i = 15;
    sValues[i].length = ID_SIZEOF(sInitExtents);
    sValues[i].value = &sInitExtents;

    // NEXTEXTENTS
    i = 16;
    sValues[i].length = ID_SIZEOF(sNextExtents);
    sValues[i].value = &sNextExtents;

    // MINEXTENTS
    i = 17;
    sValues[i].length = ID_SIZEOF(sMinExtents);
    sValues[i].value = &sMinExtents;

    // MAXEXTENTS
    i = 18;
    sValues[i].length = ID_SIZEOF(sMaxExtents);
    sValues[i].value = &sMaxExtents;

    // PROJ-1502 PARTITIONED DISK TABLE
    // IS_PARTITIONED
    i = 19;
    qtc::setCharValue( sIsPartitioned, &(aColumn[i]), (SChar*)"F");
    sValues[i].value = (void *) sIsPartitioned;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // PROJ-1407 Temporary Table
    // TEMPORARY
    i = 20;
    qtc::setCharValue( sTemporary, &(aColumn[i]), (SChar*)"N");
    sValues[i].value = (void *) sTemporary;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // PROJ-1624 Global Non-partitioned Index 
    // HIDDEN
    i = 21;
    qtc::setCharValue( sHidden, &(aColumn[i]), (SChar*)"N");
    sValues[i].value = (void *) sHidden;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    /* PROJ-2359 Table/Partition Access Option */
    // ACCESS
    i = 22;
    qtc::setCharValue( sAccessOption, &(aColumn[i]), (SChar*)"W");
    sValues[i].value = (void *) sAccessOption;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // PROJ-1071 Parallel query
    // PARALLEL_DEGREE
    i = 23;
    sValues[i].length = ID_SIZEOF(sParallelDegree);
    sValues[i].value = &sParallelDegree;

    /* TASK-7307 DML Data Consistency in Shard */
    // USABLE
    i = 24;
    qtc::setCharValue( sUsable, &(aColumn[i]), (SChar*)"Y");
    sValues[i].value = (void *) sUsable;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );
    // SHARD_FLAG
    i = 25;
    sValues[i].length = ID_SIZEOF(sShardFlag);
    sValues[i].value = &sShardFlag;

    // BUG-14394
    IDE_TEST( qtc::sysdate( &sSysDate ) != IDE_SUCCESS );

    // CREATED
    i = 26;
    sValues[i].value = (void *)&sSysDate;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // BUG-14394
    // LAST_DDL_TIME
    i = 27;
    sValues[i].value = (void *)&sSysDate;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );
    // insert record
    IDE_TEST(sCursor.insertRow( sValues,
                                &sDummyRow,
                                &sDummyGRID )
             != IDE_SUCCESS);

    /* (2) ( QC_SYSTEM_USER_ID,
       QCM_COLUMNS_TBL_ID,
       sQcmColumnsOID,
       QCM_COLUMNS_COL_CNT,
       'SYS_COLUMNS_',
       'T',
       0)               */
    sTableID     = QCM_COLUMNS_TBL_ID;
    sTableOIDULong    = sQcmColumnsOID;
    sColumnCount = QCM_COLUMNS_COL_CNT;

    // TABLE_NAME
    i = 4;
    qtc::setVarcharValue( sTblName, &(aColumn[i]), (SChar*)QCM_COLUMNS, 0);
    sValues[i].value = (void *) sTblName;
    sValues[i].length = aColumn[i].module->actualSize( aColumn + i,
                                                       sValues[i].value );

    // other value reuse

    IDE_TEST(sCursor.insertRow( sValues,
                                &sDummyRow,
                                &sDummyGRID )
             != IDE_SUCCESS);

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::insertIntoQcmColumns( smiStatement    * aSmiStmt,
                                        mtcColumn       * aTblColumn,
                                        mtcColumn       * aColColumn )
{
/***********************************************************************
 *
 * Description :
 *    insert into SYS_COLUMNS_ values
 *
 * Implementation :
 *    1. SYS_TABLES_ 테이블의 컬럼 정보를 SYS_COLUMNS_ 테이블에 입력
 *    2. SYS_COLUMNS_ 테이블의 컬럼 정보를 SYS_COLUMNS_ 테이블에 입력
 *
 *    PROJ-1705 디스크레코드최적화프로젝트 수행으로
 *    sm에 레코드 전달을 위해 smiValue 자료구조 정보 구성시
 *    아래 함수를 사용해서 memory/disk table의 레코드를
 *    적절히 처리하도록 수정되었으나,
 *    메타테이블은 memory table이므로 기존 코드를 수정하지 않음.
 ***********************************************************************/

    UInt               sStage = 0;
    smiTableCursor     sCursor;
    SInt               sPos_COLUMN_NAME = 10;
    SInt               sPos_IS_NULLABLE = 11;
    SInt               sPos_DEFAULT_VAL = 12;
    SInt               sPos_STORE_TYPE  = 13;
    SInt               sPos_IS_HIDDEN = 16;
    SInt               sPos_IS_KEY_PRESERVED = 17;
    SInt               sColOrder;
    UInt               sUserID = QC_SYSTEM_USER_ID;
    UInt               sReplCondition   = 0;
    smiValue           sValues[QCM_COLUMNS_COL_CNT];
    SChar              sColNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SLong              sOffset;
    SLong              sSize;

    qcNameCharBuffer   sColNameBuffer;
    mtdCharType *      sColName = ( mtdCharType * ) & sColNameBuffer;

    qcNameCharBuffer   sIsNullBuffer;
    mtdCharType *      sIsNull = ( mtdCharType * ) & sIsNullBuffer;

    qcNameCharBuffer   sStoreTypeBuffer;
    mtdCharType *      sStoreType = ( mtdCharType * ) & sStoreTypeBuffer;

    qcNameCharBuffer   sDefaultBuffer;
    mtdCharType *      sDefault = ( mtdCharType * ) & sDefaultBuffer;

    /* PROJ-1090 Function-based Index */
    qcNameCharBuffer   sHiddenBuffer;
    mtdCharType *      sHidden = ( mtdCharType * ) & sHiddenBuffer;

    qcNameCharBuffer   sKeyPrevBuffer;
    mtdCharType *      sKeyPrev = ( mtdCharType * ) & sKeyPrevBuffer;

    UInt               sTableID;
    void             * sDummyRow;
    scGRID             sDummyGRID;

    sCursor.initialize();

    //----- insert into QCM_COLUMNS -----//
    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  gQcmColumns,
                  NULL,       // aIndex
                  smiGetRowSCN(gQcmColumns),
                  NULL,       // aColumns
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                  SMI_INSERT_CURSOR,
                  & gMetaDefaultCursorProperty )
              != IDE_SUCCESS );
    sStage = 1;

    // IS_NULLABLE
    qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]), (SChar*)"F");
    sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;

    sValues[sPos_IS_NULLABLE].length
        = aColColumn[sPos_IS_NULLABLE].module->actualSize(
            &(aColColumn[sPos_IS_NULLABLE]),
            (UChar *)(sValues[sPos_IS_NULLABLE].value) );

    // STORE_TYPE
    qtc::setCharValue(sStoreType, &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
    sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

    sValues[sPos_STORE_TYPE].length
        = aColColumn[sPos_STORE_TYPE].module->actualSize(
            &(aColColumn[sPos_STORE_TYPE]),
            (UChar *)(sValues[sPos_STORE_TYPE].value) );


    // DEFAULT_VAL
    qtc::setVarcharValue(sDefault,
                         &(aColColumn[sPos_DEFAULT_VAL]),
                         (SChar*)"",
                         0 );
    sValues[sPos_DEFAULT_VAL].value = (void *)sDefault;

    sValues[sPos_DEFAULT_VAL].length
        = aColColumn[sPos_DEFAULT_VAL].module->actualSize(
            &(aColColumn[sPos_DEFAULT_VAL]),
            (UChar *)(sValues[sPos_DEFAULT_VAL].value) );

    /* PROJ-1090 Function-based Index
     *  IS_HIDDEN
     */
    qtc::setCharValue( sHidden, &(aColColumn[sPos_IS_HIDDEN]), (SChar*)"F" );
    sValues[sPos_IS_HIDDEN].value = (void *)sHidden;

    sValues[sPos_IS_HIDDEN].length
        = aColColumn[sPos_IS_HIDDEN].module->actualSize(
            &(aColColumn[sPos_IS_HIDDEN]),
            (UChar *)(sValues[sPos_IS_HIDDEN].value) );

    // IS_KEY_PRESERVED
    qtc::setCharValue( sKeyPrev, &(aColColumn[sPos_IS_KEY_PRESERVED]), (SChar*)"F" );
    sValues[sPos_IS_KEY_PRESERVED].value = (void *)sKeyPrev;

    sValues[sPos_IS_KEY_PRESERVED].length
        = aColColumn[sPos_IS_KEY_PRESERVED].module->actualSize(
            &(aColColumn[sPos_IS_KEY_PRESERVED]),
            (UChar *)(sValues[sPos_IS_KEY_PRESERVED].value) );

    //------------------- columns of qcm_tables_ -------------------------//
    sTableID = QCM_TABLES_TBL_ID;
    for (sColOrder = 0; sColOrder < QCM_TABLES_COL_CNT; sColOrder++)
    {
        /* ( aTblColumn[sColOrder].column.id
           aTblColumn[sColOrder].type.type,
           aTblColumn[sColOrder].type.language,
           aTblColumn[sColOrder].column.offset,
           aTblColumn[sColOrder].column.size,
           QC_SYSTEM_USER_ID,
           sTableID,
           aTblColumn[sColOrder].precision,
           aTblColumn[sColOrder].scale,
           sColOrder,
           column_name,
           'F',
           NULL,
           'F')              */

        // COLUMN_ID
        sValues[0].length = ID_SIZEOF(aTblColumn[sColOrder].column.id);
        sValues[0].value = &(aTblColumn[sColOrder].column.id);

        // DATA_TYPE
        sValues[1].length = ID_SIZEOF(aTblColumn[sColOrder].type.dataTypeId);
        sValues[1].value = &(aTblColumn[sColOrder].type.dataTypeId);

        // LANG_ID
        sValues[2].length = ID_SIZEOF(aTblColumn[sColOrder].type.languageId);
        sValues[2].value = &(aTblColumn[sColOrder].type.languageId);

        // OFFSET
        // PROJ-1362
        sOffset = (SLong) aTblColumn[sColOrder].column.offset;
        sValues[3].length = ID_SIZEOF(sOffset);
        sValues[3].value = &sOffset;

        // SIZE
        // PROJ-1362
        sSize = (SLong) aTblColumn[sColOrder].column.size;
        sValues[4].length = ID_SIZEOF(sSize);
        sValues[4].value = &sSize;

        // USER_ID
        sValues[5].length = ID_SIZEOF(sUserID);
        sValues[5].value = &sUserID;

        // TABLE_ID
        sValues[6].length = ID_SIZEOF(sTableID);
        sValues[6].value = &sTableID;

        // PRECISION
        sValues[7].length = ID_SIZEOF(aTblColumn[sColOrder].precision);
        sValues[7].value = &(aTblColumn[sColOrder].precision);

        // SCALE
        sValues[8].length = ID_SIZEOF(aTblColumn[sColOrder].scale);
        sValues[8].value = &(aTblColumn[sColOrder].scale);

        // COLUMN_ORDER
        sValues[9].length = ID_SIZEOF(sColOrder);
        sValues[9].value = &sColOrder;

        // PROJ-1557
        // IN_ROW_SIZE
        sValues[14].length = ID_SIZEOF(aTblColumn[sColOrder].column.vcInOutBaseSize);
        sValues[14].value = &(aTblColumn[sColOrder].column.vcInOutBaseSize);

        // PROJ-1638
        // REPL_CONDITION
        sValues[15].length = ID_SIZEOF(sReplCondition);
        sValues[15].value = &sReplCondition;

        switch (sColOrder)
        {
            case 0:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "USER_ID");
                break;
            case 1:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TABLE_ID");
                break;
            case 2:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TABLE_OID");
                break;
            case 3:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "COLUMN_COUNT");
                break;
            case 4:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TABLE_NAME");
                break;
            case 5:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TABLE_TYPE");
                break;
            case 6:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "REPLICATION_COUNT");
                break;
            case 7:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "REPLICATION_RECOVERY_COUNT");
                break;
            case 8:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "MAXROW");
                break;
            case 9:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TBS_ID");
                break;
            case 10:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TBS_NAME");
                break;
            case 11:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "PCTFREE");
                break;
            case 12:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "PCTUSED");
                break;
                // PROJ-1671 Bitmap Tablespace And Segment Space Management
            case 13:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "INIT_TRANS");
                break;
            case 14:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "MAX_TRANS");
                break;
                // PROJ-1704 MVCC Renewal
            case 15:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "INITEXTENTS");
                break;
            case 16:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "NEXTEXTENTS");
                break;
            case 17:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "MINEXTENTS");
                break;
            case 18:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "MAXEXTENTS");
                break;
                // PROJ-1502 PARTITIONED DISK TABLE
            case 19:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "IS_PARTITIONED");
                break;
                // PROJ-1407 Temporary Table
            case 20:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TEMPORARY");
                break;
                // PROJ-1624 Global Non-partitioned Index
            case 21:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "HIDDEN");
                break;                
            case 22: /* PROJ-2359 Table/Partition Access Option */
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "ACCESS");
                break;     
            case 23:
                /* PROJ-1071 Parallel Query */
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "PARALLEL_DEGREE");
                break;
            case 24: /* TASK-7307 DML Data Consistency in Shard */
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "USABLE");
                break;     
            case 25: /* TASK-7307 DML Data Consistency in Shard */
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "SHARD_FLAG");
                break;
            case 26: // BUG-14394
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "CREATED");
                break;
            case 27: // BUG-14394
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "LAST_DDL_TIME");
                break;
            default:
                IDE_RAISE( ERR_INVALID_COLUMN_ORDER_OF_SYS_TABLES );
        }

        qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                          (SChar*)"F");
        sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;


        // COLUMN_NAME
        qtc::setVarcharValue( sColName,
                              &(aColColumn[sPos_COLUMN_NAME]),
                              sColNameStr,
                              0 );
        sValues[sPos_COLUMN_NAME].value = (void *)sColName;

        sValues[sPos_COLUMN_NAME].length
            = aColColumn[sPos_COLUMN_NAME].module->actualSize(
                &(aColColumn[sPos_COLUMN_NAME]),
                (UChar *)(sValues[sPos_COLUMN_NAME].value) );

        IDE_TEST(sCursor.insertRow( sValues,
                                    &sDummyRow,
                                    &sDummyGRID )
                 != IDE_SUCCESS);
    }

    //------------------- columns of qcm_columns_ -------------------------//
    sTableID = QCM_COLUMNS_TBL_ID;
    for (sColOrder = 0; sColOrder < QCM_COLUMNS_COL_CNT; sColOrder++)
    {
        /* ( aColColumn[sColOrder].column.id
           aColColumn[sColOrder].type.type,
           aTblColumn[sColOrder].type.language,
           aColColumn[sColOrder].column.offset,
           aColColumn[sColOrder].column.size,
           QC_SYSTEM_USER_ID,
           sTableID,
           aColColumn[sColOrder].precision,
           aColColumn[sColOrder].scale,
           sColOrder,
           column_name,
           'F',
           NULL )              */
        // PROJ-1362
        sOffset = (SLong) aColColumn[sColOrder].column.offset;
        sSize = (SLong) aColColumn[sColOrder].column.size;

        sValues[0].value = &(aColColumn[sColOrder].column.id);
        sValues[1].value = &(aColColumn[sColOrder].type.dataTypeId);
        sValues[2].value = &(aColColumn[sColOrder].type.languageId);
        sValues[3].value = &sOffset;
        sValues[4].value = &sSize;
        sValues[5].value = &sUserID;
        sValues[6].value = &sTableID;
        sValues[7].value = &(aColColumn[sColOrder].precision);
        sValues[8].value = &(aColColumn[sColOrder].scale);
        sValues[9].value = &sColOrder;

        // PROJ-1557
        sValues[14].value = &(aColColumn[sColOrder].column.vcInOutBaseSize);

        switch (sColOrder)
        {
            case 0 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "COLUMN_ID");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");

                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 1 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "DATA_TYPE");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 2 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "LANG_ID");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;


                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 3 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "OFFSET");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 4 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "SIZE");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 5 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "USER_ID");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 6 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "TABLE_ID");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 7 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "PRECISION");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 8 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "SCALE");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 9 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "COLUMN_ORDER");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 10 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "COLUMN_NAME");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 11 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "IS_NULLABLE");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 12 :
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "DEFAULT_VAL");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"T");

                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"V");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 13:
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "STORE_TYPE");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"T");  // BUG-25728
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 14:          // PROJ-1557
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "IN_ROW_SIZE");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 15:          // PROJ-1638
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "REPL_CONDITION");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 16 : /* PROJ-1090 Function-based Index */
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "IS_HIDDEN");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            case 17 : // PROJ-2204 Join Update, Delete
                idlOS::snprintf(sColNameStr, QC_MAX_OBJECT_NAME_LEN + 1, "IS_KEY_PRESERVED");
                qtc::setCharValue(sIsNull, &(aColColumn[sPos_IS_NULLABLE]),
                                  (SChar*)"F");
                // To Fix PR-5795
                qtc::setCharValue(sStoreType,
                                  &(aColColumn[sPos_STORE_TYPE]), (SChar*)"F");
                sValues[sPos_STORE_TYPE].value = (void *)sStoreType;

                sValues[sPos_IS_NULLABLE].value = (void *)sIsNull;
                break;
            default:
                IDE_RAISE( ERR_INVALID_COLUMN_ORDER_OF_SYS_COLUMNS );
        }

        // COLUMN_NAME
        qtc::setVarcharValue( sColName,
                              &(aColColumn[sPos_COLUMN_NAME]),
                              sColNameStr,
                              0 );
        sValues[sPos_COLUMN_NAME].value = (void *)sColName;

        sValues[sPos_COLUMN_NAME].length
            = aColColumn[sPos_COLUMN_NAME].module->actualSize(
                &(aColColumn[sPos_COLUMN_NAME]),
                (UChar *)(sValues[sPos_COLUMN_NAME].value) );

        IDE_TEST(sCursor.insertRow( sValues,
                                    &sDummyRow,
                                    &sDummyGRID )
                 != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_COLUMN_ORDER_OF_SYS_TABLES )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmCreate::insertIntoQcmColumns",
                                  "Invalid column order of SYS_TABLES_" ));
    }
    IDE_EXCEPTION( ERR_INVALID_COLUMN_ORDER_OF_SYS_COLUMNS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcmCreate::insertIntoQcmColumns",
                                  "Invalid column order of SYS_COLUMNS_" ));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::makeSignature(smiStatement *aStatement)
{
    ULong sQcmTablesTableOIDULong;

    sQcmTablesTableOIDULong = (ULong)smiGetTableId(gQcmTables);

    IDE_TEST(smiTable::modifyTableInfo(aStatement,
                                       smiGetCatalogTable(),
                                       NULL,
                                       0,
                                       (const void*) &sQcmTablesTableOIDULong,
                                       ID_SIZEOF(ULong),
                                       SMI_TABLE_FLAG_UNCHANGE,
                                       SMI_TBSLV_DDL_DML,
                                       0, /* Parallel Degree */
                                       0,
                                       ID_TRUE) /* aIsInitRowTemplate */
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmCreate::runDDLforMETA( idvSQL   * aStatistics,
                                 smiTrans * aTrans, 
                                 SChar    * aDBName, 
                                 SChar    * aOwnerDN, 
                                 UInt       aOwnerDNLen)
{
/*
  [IMPORTANT] About sCrtMetaSql
  1. The last array item should be NULL
  2. The order of index creation should be same
  with XXX_IDX_ORDER definitions in qcm.h
*/

    SChar   * sCrtMetaSql[] = {
        (SChar*) "CREATE SEQUENCE NEXT_USER_ID MINVALUE "QCM_META_SEQ_MINVALUE_STR" MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
        (SChar*) "CREATE SEQUENCE NEXT_INDEX_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
        (SChar*) "CREATE SEQUENCE NEXT_CONSTRAINT_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
#if !defined(SMALL_FOOTPRINT)
        (SChar*) "CREATE SEQUENCE NEXT_REPL_HOST_NO MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
#endif
// PROJ-1502 PARTITIONED DISK TABLE
        (SChar*) "CREATE SEQUENCE NEXT_TABLE_PARTITION_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
// PROJ-1502 PARTITIONED DISK TABLE
        (SChar*) "CREATE SEQUENCE NEXT_INDEX_PARTITION_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
/* PROJ-1371 Directories */
        (SChar*) "CREATE SEQUENCE NEXT_DIRECTORY_ID"
        ,
        (SChar*) "CREATE SEQUENCE NEXT_MATERIALIZED_VIEW_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
// PROJ-1685
        (SChar*) "CREATE SEQUENCE NEXT_LIBRARY_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
        /* PROJ-1832 New database link */
        (SChar *) "CREATE SEQUENCE NEXT_DATABASE_LINK_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
// PROJ-2261 Dictionary table
        (SChar*) "CREATE SEQUENCE NEXT_DICTIONARY_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
/* PROJ-1438 */
        (SChar*) "CREATE SEQUENCE NEXT_JOB_ID MAXVALUE "QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
// PROJ-2689 Downgrade meta
        (SChar*) "CREATE TABLE SYS_DATABASE_ ( \
DB_NAME VARCHAR(40) FIXED, \
OWNER_DN VARCHAR(2048) FIXED, \
META_MAJOR_VER INTEGER, \
META_MINOR_VER INTEGER, \
META_PATCH_VER INTEGER,  \
PREV_META_MAJOR_VER INTEGER, \
PREV_META_MINOR_VER INTEGER, \
PREV_META_PATCH_VER INTEGER  \
)"
,
(SChar*) "CREATE TABLE DBA_USERS_ ( \
USER_ID INTEGER,  \
USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PASSWORD VARCHAR(256) FIXED, \
DEFAULT_TBS_ID INTEGER, \
TEMP_TBS_ID INTEGER, \
ACCOUNT_LOCK             CHAR(1), \
ACCOUNT_LOCK_DATE        DATE, \
PASSWORD_LIMIT_FLAG      CHAR(1), \
FAILED_LOGIN_ATTEMPTS    INTEGER, \
FAILED_LOGIN_COUNT       INTEGER, \
PASSWORD_LOCK_TIME       INTEGER, \
PASSWORD_EXPIRY_DATE     DATE, \
PASSWORD_LIFE_TIME       INTEGER, \
PASSWORD_GRACE_TIME      INTEGER, \
PASSWORD_REUSE_DATE      DATE, \
PASSWORD_REUSE_TIME      INTEGER, \
PASSWORD_REUSE_MAX       INTEGER, \
PASSWORD_REUSE_COUNT     INTEGER, \
PASSWORD_VERIFY_FUNCTION VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
USER_TYPE                CHAR(1), \
DISABLE_TCP              CHAR(1), \
CREATED                  DATE, \
LAST_DDL_TIME            DATE \
)"
,
#if !defined(SMALL_FOOTPRINT)
(SChar*) "CREATE TABLE SYS_DN_USERS_ ( \
USER_DN VARCHAR(2048) FIXED, \
USER_AID VARCHAR(2048) FIXED, \
USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REGISTERED DATE, \
START_PERIOD DATE, \
END_PERIOD DATE )"
#else
(SChar*) "CREATE TABLE SYS_DN_USERS_ ( \
USER_DN VARCHAR(2048) VARIABLE, \
USER_AID VARCHAR(2048) VARIABLE, \
USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REGISTERED DATE, \
START_PERIOD DATE, \
END_PERIOD DATE )"
#endif
,
(SChar*) "CREATE TABLE SYS_TBS_USERS_ ( \
TBS_ID INTEGER,  \
USER_ID INTEGER,  \
IS_ACCESS INTEGER)"
        ,
// PROJ-1502 PARTITIONED DISK TABLE
// IS_PARTITIONED 추가
/* PROJ-2433 Direct Key Index
 * IS_DIRECTKEY 추가 */
        (SChar*) "CREATE TABLE SYS_INDICES_ ( \
USER_ID INTEGER, \
TABLE_ID INTEGER, \
INDEX_ID INTEGER, \
INDEX_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
INDEX_TYPE INTEGER, \
IS_UNIQUE CHAR(1), \
COLUMN_CNT INTEGER, \
IS_RANGE CHAR(1), \
IS_PERS CHAR(1), \
IS_DIRECTKEY CHAR(1), \
TBS_ID INTEGER, \
IS_PARTITIONED CHAR(1), \
INDEX_TABLE_ID INTEGER, \
CREATED DATE, \
LAST_DDL_TIME DATE )"
,
(SChar*) "CREATE TABLE SYS_INDEX_COLUMNS_ ( \
USER_ID INTEGER, \
INDEX_ID INTEGER, \
COLUMN_ID INTEGER, \
INDEX_COL_ORDER INTEGER, \
SORT_ORDER CHAR(1), \
TABLE_ID INTEGER)"
        ,

        (SChar*) "CREATE TABLE SYS_CONSTRAINTS_ ( \
USER_ID INTEGER, \
TABLE_ID INTEGER, \
CONSTRAINT_ID INTEGER, \
CONSTRAINT_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
CONSTRAINT_TYPE INTEGER, \
INDEX_ID INTEGER, \
COLUMN_CNT INTEGER, \
REFERENCED_TABLE_ID INTEGER, \
REFERENCED_INDEX_ID INTEGER, \
DELETE_RULE INTEGER, \
CHECK_CONDITION VARCHAR(4000) VARIABLE, \
VALIDATED CHAR(1) DEFAULT 'T' )"
        ,
        (SChar*) "CREATE TABLE SYS_CONSTRAINT_COLUMNS_ ( \
USER_ID  INTEGER, \
TABLE_ID INTEGER, \
CONSTRAINT_ID INTEGER, \
CONSTRAINT_COL_ORDER INTEGER, \
COLUMN_ID INTEGER)"
        ,
        (SChar*) "CREATE TABLE SYS_PROCEDURES_ ( \
USER_ID                       INTEGER, \
PROC_OID                      BIGINT, \
PROC_NAME                     VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
OBJECT_TYPE                   INTEGER, \
STATUS                        INTEGER, \
PARA_NUM                      INTEGER, \
RETURN_DATA_TYPE              INTEGER, \
RETURN_LANG_ID                INTEGER, \
RETURN_SIZE                   INTEGER, \
RETURN_PRECISION              INTEGER, \
RETURN_SCALE                  INTEGER, \
PARSE_NO                      INTEGER, \
PARSE_LEN                     INTEGER, \
CREATED                       DATE, \
LAST_DDL_TIME                 DATE, \
AUTHID                        INTEGER )"
,
#if !defined(SMALL_FOOTPRINT)
(SChar*) "CREATE TABLE SYS_PROC_PARAS_ ( \
USER_ID                       INTEGER, \
PROC_OID                      BIGINT, \
PARA_NAME                     VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARA_ORDER                    INTEGER, \
INOUT_TYPE                    INTEGER, \
DATA_TYPE                     INTEGER, \
LANG_ID                       INTEGER, \
SIZE                          INTEGER, \
PRECISION                     INTEGER, \
SCALE                         INTEGER, \
DEFAULT_VAL                   VARCHAR("QCM_MAX_DEFAULT_VAL_STR") FIXED )"
#else
(SChar*) "CREATE TABLE SYS_PROC_PARAS_ ( \
USER_ID                       INTEGER, \
PROC_OID                      BIGINT, \
PARA_NAME                     VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARA_ORDER                    INTEGER, \
INOUT_TYPE                    INTEGER, \
DATA_TYPE                     INTEGER, \
LANG_ID                       INTEGER, \
SIZE                          INTEGER, \
PRECISION                     INTEGER, \
SCALE                         INTEGER, \
DEFAULT_VAL                   VARCHAR("QCM_MAX_DEFAULT_VAL_STR") VARIABLE )"
#endif
        ,
        (SChar*) "CREATE TABLE SYS_PROC_PARSE_ ( \
USER_ID                       INTEGER, \
PROC_OID                      BIGINT, \
SEQ_NO                        INTEGER, \
PARSE                         VARCHAR("QCM_MAX_PROC_LEN_STR") FIXED )"
        ,
        (SChar*) "CREATE TABLE SYS_PROC_RELATED_ ( \
USER_ID                        INTEGER, \
PROC_OID                       BIGINT, \
RELATED_USER_ID                INTEGER, \
RELATED_OBJECT_NAME            VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
RELATED_OBJECT_TYPE            INTEGER )"
        ,
// PROJ-1073 Package
        (SChar*) "CREATE TABLE SYS_PACKAGES_ ( \
USER_ID                        INTEGER, \
PACKAGE_OID                    BIGINT, \
PACKAGE_NAME                   VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PACKAGE_TYPE                   INTEGER, \
STATUS                         INTEGER, \
CREATED                        DATE, \
LAST_DDL_TIME                  DATE, \
AUTHID                         INTEGER )"
        ,
        (SChar*) "CREATE TABLE SYS_PACKAGE_PARAS_ ( \
USER_ID                        INTEGER, \
OBJECT_NAME                    VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED,\
PACKAGE_NAME                   VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PACKAGE_OID                    BIGINT, \
SUB_ID                         INTEGER, \
SUB_TYPE                       INTEGER, \
PARA_NAME                      VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARA_ORDER                     INTEGER, \
INOUT_TYPE                     INTEGER, \
DATA_TYPE                      INTEGER, \
LANG_ID                        INTEGER, \
SIZE                           INTEGER, \
PRECISION                      INTEGER, \
SCALE                          INTEGER, \
DEFAULT_VAL                    VARCHAR("QCM_MAX_DEFAULT_VAL_STR") VARIABLE )"
        ,
        (SChar*) "CREATE TABLE SYS_PACKAGE_PARSE_ ( \
USER_ID                        INTEGER, \
PACKAGE_OID                    BIGINT, \
PACKAGE_TYPE                   INTEGER, \
SEQ_NO                         INTEGER, \
PARSE                          VARCHAR("QCM_MAX_PROC_LEN_STR") FIXED)"
        ,
        (SChar*) "CREATE TABLE SYS_PACKAGE_RELATED_ ( \
USER_ID                        INTEGER, \
PACKAGE_OID                    BIGINT, \
RELATED_USER_ID                INTEGER, \
RELATED_OBJECT_NAME            VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
RELATED_OBJECT_TYPE            INTEGER )"
        ,
//=========================================================
// [PROJ-1359] Trigger를 위한 Meta Table 구조
// SYS_TRIGGERS_
// USER_ID          : User ID
// USER_NAME        : User Name
// TRIGGER_OID      : Trigger Object ID
// TRIGGER_NAME     : Trigger Name
// TABLE_ID         : Trigger가 참조하는 Table의 ID
// IS_ENABLE        : 수행 여부의 지정
// EVENT_TIME       : BEFORE or AFTER Event
// EVENT_TYPE       : Trigger Event의 종류(INSERT, DELETE, UPDATE)
// UPDATE_COLUMN_CNT: Update Event에 포함된 Column의 개수
// GRANULARITY      : Action Granularity
// REF_ROW_CNT      : Referencing Row 의 개수
// SUBSTING_CNT     : Trigger 생성문이 분할된 개수
// STRING_LENGTH    : Trigger 생성문의 총 길이
//=========================================================

        (SChar*) "CREATE TABLE SYS_TRIGGERS_ (                   \
USER_ID                        INTEGER,                  \
USER_NAME                      VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED,        \
TRIGGER_OID                    BIGINT,                   \
TRIGGER_NAME                   VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED,        \
TABLE_ID                       INTEGER,                  \
IS_ENABLE                      INTEGER,                  \
EVENT_TIME                     INTEGER,                  \
EVENT_TYPE                     INTEGER,                  \
UPDATE_COLUMN_CNT              INTEGER,                  \
GRANULARITY                    INTEGER,                  \
REF_ROW_CNT                    INTEGER,                  \
SUBSTRING_CNT                  INTEGER,                  \
STRING_LENGTH                  INTEGER,                  \
CREATED                        DATE,                     \
LAST_DDL_TIME                  DATE )"
,

//=========================================================
// [PROJ-1359] 분할된 Trigger 구문의 관리
// SYS_TRIGGER_STRINGS_
// TABLE_ID      : Table ID
// TRIGGER_ID    : Trigger의 ID
// SEQNO         : Trigger 생성 구문의 Substring 순서
// SUBSTRING     : Trigger 생성 구문의 실제 Sub-String
//=========================================================

        (SChar*) "CREATE TABLE SYS_TRIGGER_STRINGS_ (            \
TABLE_ID              INTEGER,                           \
TRIGGER_OID           BIGINT,                            \
SEQNO                 INTEGER,                           \
SUBSTRING             VARCHAR("QCM_TRIGGER_SUBSTRING_LEN_STR") FIXED )"
        ,

//=========================================================
// [PROJ-1359] UPDATE Event의 OF 구문에 존재하는 Column을 관리
// SYS_TRIGGER_UPDATE_COLUMNS_
// TABLE_ID      : Table ID
// TRIGGER_OID    : Trigger의 ID
// COLUMN_ID      : OF 구문의 Column ID
//=========================================================

        (SChar*) "CREATE TABLE SYS_TRIGGER_UPDATE_COLUMNS_ (      \
TABLE_ID               INTEGER,                           \
TRIGGER_OID            BIGINT,                            \
COLUMN_ID              INTEGER )"
        ,

//=========================================================
// [PROJ-1359] Action Body가 DML로 접근하는 테이블을 관리
// SYS_TRIGGER_DML_TABLES_
// TABLE_ID          : Table ID
// TRIGGER_ID        : Trigger의 ID
// DML_TABLE_ID      : DML로 접근하는 Table의 ID
// STMT_TYPE         : 해당 Table에 접근하는 STMT의 종류
//=========================================================

        (SChar*) "CREATE TABLE SYS_TRIGGER_DML_TABLES_ (         \
TABLE_ID              INTEGER,                           \
TRIGGER_OID           BIGINT,                            \
DML_TABLE_ID          INTEGER,                           \
STMT_TYPE             INTEGER )"
        ,

//=========================================================
// [PROJ-1362] LOB을 위한 Meta Table 구조
// SYS_LOBS_
// USER_ID           : User ID
// TABLE_ID          : Table ID
// COLUMN_ID         : Column ID
// TBS_ID            : LOB Tablespace ID
// LOGGING           : LOB Logging Flag
// BUFFER            : LOB Buffer Flag
// TBS_PRIORITY      : LOB Tablespace Priority
//=========================================================

        (SChar*) "CREATE TABLE SYS_LOBS_ (                       \
USER_ID               INTEGER,                           \
TABLE_ID              INTEGER,                           \
COLUMN_ID             INTEGER,                           \
TBS_ID                INTEGER,                           \
LOGGING               CHAR(1),                           \
BUFFER                CHAR(1),                           \
IS_DEFAULT_TBS        CHAR(1) )"
        ,

#if !defined(SMALL_FOOTPRINT)
        (SChar*) "CREATE TABLE SYS_REPLICATIONS_ ( \
REPLICATION_NAME            VARCHAR("QCM_META_NAME_LEN") FIXED, \
LAST_USED_HOST_NO           INTEGER, \
HOST_COUNT                  INTEGER, \
IS_STARTED                  INTEGER, \
XSN                         BIGINT, \
ITEM_COUNT                  INTEGER, \
CONFLICT_RESOLUTION         INTEGER, \
REPL_MODE                   INTEGER, \
ROLE                        INTEGER, \
OPTIONS                     INTEGER, \
INVALID_RECOVERY            INTEGER, \
REMOTE_FAULT_DETECT_TIME    DATE, \
GIVE_UP_TIME                DATE, \
GIVE_UP_XSN                 BIGINT, \
PARALLEL_APPLIER_COUNT      INTEGER,\
APPLIER_INIT_BUFFER_SIZE    BIGINT, \
REMOTE_XSN                  BIGINT, \
PEER_REPLICATION_NAME       VARCHAR("QCM_META_NAME_LEN") FIXED, \
REMOTE_LAST_DDL_XSN         BIGINT, \
CURRENT_READ_XLOGFILE_NO    INTEGER, \
CURRENT_READ_XLOGFILE_OFFSET INTEGER )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_RECEIVER_ ( \
REPLICATION_NAME            VARCHAR("QCM_META_NAME_LEN") FIXED, \
REMOTE_XSN                  BIGINT, \
CURRENT_READ_XLOGFILE_NO    INTEGER, \
CURRENT_READ_XLOGFILE_OFFSET INTEGER )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_HOSTS_( \
HOST_NO INTEGER, \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
HOST_IP VARCHAR(64) FIXED, \
PORT_NO INTEGER, \
CONN_TYPE VARCHAR(20) FIXED, \
IB_LATENCY VARCHAR(10) FIXED )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_ITEMS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
LOCAL_USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
LOCAL_TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
LOCAL_PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REMOTE_USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REMOTE_TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REMOTE_PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
IS_PARTITION CHAR(1), \
INVALID_MAX_SN BIGINT, \
CONDITION VARCHAR(1000) VARIABLE, \
REPLICATION_UNIT CHAR(1),\
IS_CONDITION_SYNCED INTEGER )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_ITEMS_HISTORY_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
OLD_OID BIGINT, \
NEW_OID BIGINT )"
        ,
/* PROJ-1442 Replication Online 중 DDL 허용 */
        (SChar*) "CREATE TABLE SYS_REPL_OLD_ITEMS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PRIMARY_KEY_INDEX_ID INTEGER, \
REMOTE_USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REMOTE_TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
REMOTE_PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARTITION_ORDER       INTEGER, \
PARTITION_MIN_VALUE   VARCHAR(4000) VARIABLE, \
PARTITION_MAX_VALUE   VARCHAR(4000) VARIABLE, \
INVALID_MAX_SN BIGINT, \
TABLE_ID                    INTEGER, \
TABLE_PARTITION_TYPE        INTEGER, \
IS_PARTITION                CHAR(1), \
REPLICATION_UNIT            CHAR(1), \
TBS_TYPE                    INTEGER, \
PARTITION_METHOD            INTEGER, \
PARTITION_COUNT             INTEGER  )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_OLD_COLUMNS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
COLUMN_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
MT_DATATYPE_ID INTEGER, \
MT_LANGUAGE_ID INTEGER, \
MT_FLAG INTEGER, \
MT_PRECISION INTEGER, \
MT_SCALE INTEGER, \
MT_ENCRYPT_PRECISION INTEGER, \
MT_POLICY_NAME VARCHAR(16) FIXED, \
MT_SRID INTEGER  FIXED, \
SM_ID INTEGER, \
SM_FLAG INTEGER, \
SM_OFFSET INTEGER, \
SM_VARORDER INTEGER, \
SM_SIZE INTEGER, \
SM_DIC_TABLE_OID BIGINT, \
SM_COL_SPACE INTEGER, \
QP_FLAG INTEGER, \
DEFAULT_VAL VARCHAR("QCM_MAX_DEFAULT_VAL_STR") VARIABLE )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_OLD_INDICES_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
INDEX_ID INTEGER, \
INDEX_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
TYPE_ID INTEGER, \
IS_UNIQUE CHAR(1), \
IS_LOCAL_UNIQUE CHAR(1), \
IS_RANGE CHAR(1) )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_OLD_INDEX_COLUMNS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
INDEX_ID INTEGER, \
KEY_COLUMN_ID INTEGER, \
KEY_COLUMN_FLAG INTEGER, \
COMPOSITE_ORDER INTEGER )"
        ,
        (SChar*) "CREATE TABLE SYS_REPL_OLD_CHECKS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
CONSTRAINT_ID INTEGER, \
CHECK_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
CONDITION VARCHAR(4000) VARIABLE ) "
        ,
        (SChar*) "CREATE TABLE SYS_REPL_OLD_CHECK_COLUMNS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
TABLE_OID BIGINT, \
CONSTRAINT_ID INTEGER, \
COLUMN_ID INTEGER ) "
        ,
        (SChar*) "CREATE TABLE SYS_REPL_RECOVERY_INFOS_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
MASTER_BEGIN_SN BIGINT,\
MASTER_COMMIT_SN BIGINT,\
REPLICATED_BEGIN_SN BIGINT,\
REPLICATED_COMMIT_SN BIGINT )"
        ,
/* PROJ-1915 */        
        (SChar*) "CREATE TABLE SYS_REPL_OFFLINE_DIR_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
LFG_ID INTEGER,\
PATH VARCHAR(512) FIXED )"
        ,

        (SChar*) "CREATE TABLE SYS_REPL_ITEM_REPLACE_HISTORY_ ( \
REPLICATION_NAME VARCHAR("QCM_META_NAME_LEN") FIXED, \
USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
OLD_OID BIGINT, \
NEW_OID BIGINT )"
        ,

#endif
        (SChar*) "CREATE TABLE SYS_PRIVILEGES_ ( \
PRIV_ID INTEGER, \
PRIV_TYPE INTEGER, \
PRIV_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED )"
        ,
        (SChar*) "CREATE TABLE SYS_GRANT_SYSTEM_ ( \
GRANTOR_ID INTEGER, \
GRANTEE_ID INTEGER, \
PRIV_ID INTEGER )"
        ,
        (SChar*) "CREATE TABLE SYS_GRANT_OBJECT_ ( \
GRANTOR_ID INTEGER, \
GRANTEE_ID INTEGER, \
PRIV_ID INTEGER, \
USER_ID INTEGER, \
OBJ_ID BIGINT, \
OBJ_TYPE VARCHAR(1), \
WITH_GRANT_OPTION INTEGER )"
        ,
/* BUG-20687 : VARCHAR VARIABLE->FIXED */
        (SChar*) "CREATE TABLE SYS_XA_HEURISTIC_TRANS_ ( \
FORMAT_ID         BIGINT, \
GLOBAL_TX_ID      VARCHAR("QCM_XID_FIELD_STRING_SIZE_STR") FIXED, \
BRANCH_QUALIFIER  VARCHAR("QCM_XID_FIELD_STRING_SIZE_STR") FIXED, \
STATUS            INTEGER, \
OCCUR_TIME        DATE )"
        ,
        (SChar*) "CREATE TABLE SYS_VIEWS_ ( \
USER_ID  INTEGER, \
VIEW_ID  INTEGER, \
STATUS   INTEGER, \
READ_ONLY CHAR(1) )"
        ,
        (SChar*) "CREATE TABLE SYS_VIEW_PARSE_ ( \
USER_ID  INTEGER, \
VIEW_ID  INTEGER, \
SEQ_NO   INTEGER, \
PARSE    VARCHAR("QCM_MAX_PROC_LEN_STR") FIXED )"
        ,
        (SChar*) "CREATE TABLE SYS_VIEW_RELATED_ ( \
USER_ID             INTEGER, \
VIEW_ID             INTEGER, \
RELATED_USER_ID     INTEGER, \
RELATED_OBJECT_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
RELATED_OBJECT_TYPE INTEGER)"
        ,
/* PROJ-1371 SYS_DIRECTORIES_ */
        (SChar*) "CREATE TABLE SYS_DIRECTORIES_ ( \
DIRECTORY_ID        BIGINT, \
USER_ID             INTEGER, \
DIRECTORY_NAME      VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
DIRECTORY_PATH      VARCHAR("QCM_MAX_DEFAULT_VAL_STR"), \
CREATED             DATE, \
LAST_DDL_TIME       DATE )"
,
/* Project-1076 For Synony' */
        (SChar*) "CREATE TABLE SYS_SYNONYMS_ ( \
SYNONYM_OWNER_ID    INTEGER, \
SYNONYM_NAME        VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
OBJECT_OWNER_NAME   VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
OBJECT_NAME         VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
CREATED             DATE, \
LAST_DDL_TIME       DATE )"
,

#if !defined(SMALL_FOOTPRINT)
/* BUG-15076 DUMMY TABLE */
        (SChar*) "CREATE TABLE SYS_DUMMY_ ( \
DUMMY VARCHAR(1))"
        ,
#endif

/**************************************************/
/* PROJ-1488 for Altibase Spatio-Temporal DBMS */
/**************************************************/
/* PROJ-2422 srid */
        (SChar*) "CREATE TABLE SYS_GEOMETRIES_ ( \
USER_ID             INTEGER, \
TABLE_ID            INTEGER, \
COLUMN_ID           INTEGER, \
COORD_DIMENSION     INTEGER, \
SRID                INTEGER )"
        ,
        (SChar*) "CREATE TABLE USER_SRS_ ( \
SRID                INTEGER NOT NULL,   \
AUTH_NAME           VARCHAR(256) FIXED, \
AUTH_SRID           INTEGER,            \
SRTEXT              VARCHAR(2048),      \
PROJ4TEXT           VARCHAR(2048) )"
        ,

/**************************************************
 * PROJ-1502 PARTITIONED DISK TABLE - TABLE CREATE
 **************************************************/
        (SChar*) "CREATE TABLE SYS_PART_TABLES_ ( \
USER_ID               INTEGER, \
TABLE_ID              INTEGER, \
PARTITION_METHOD      INTEGER, \
PARTITION_KEY_COUNT   INTEGER, \
ROW_MOVEMENT          CHAR(1) )"
        ,
        (SChar*) "CREATE TABLE SYS_PART_INDICES_ ( \
USER_ID               INTEGER, \
TABLE_ID              INTEGER, \
INDEX_ID              INTEGER, \
PARTITION_TYPE        INTEGER, \
IS_LOCAL_UNIQUE       CHAR(1) )"
        ,
        (SChar*) "CREATE TABLE SYS_TABLE_PARTITIONS_ ( \
USER_ID               INTEGER, \
TABLE_ID              INTEGER, \
PARTITION_OID         BIGINT, \
PARTITION_ID          INTEGER, \
PARTITION_NAME        VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARTITION_MIN_VALUE   VARCHAR(4000) VARIABLE, \
PARTITION_MAX_VALUE   VARCHAR(4000) VARIABLE, \
PARTITION_ORDER       INTEGER, \
TBS_ID                INTEGER, \
PARTITION_ACCESS      CHAR(1) NOT NULL, \
PARTITION_USABLE      CHAR(1) NOT NULL, \
REPLICATION_COUNT     INTEGER, \
REPLICATION_RECOVERY_COUNT INTEGER, \
CREATED               DATE, \
LAST_DDL_TIME         DATE )"
        ,
        (SChar*) "CREATE TABLE SYS_INDEX_PARTITIONS_ ( \
USER_ID               INTEGER, \
TABLE_ID              INTEGER, \
INDEX_ID              INTEGER, \
TABLE_PARTITION_ID    INTEGER, \
INDEX_PARTITION_ID    INTEGER, \
INDEX_PARTITION_NAME  VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, \
PARTITION_MIN_VALUE   VARCHAR(4000) VARIABLE, \
PARTITION_MAX_VALUE   VARCHAR(4000) VARIABLE, \
TBS_ID                INTEGER, \
CREATED               DATE, \
LAST_DDL_TIME         DATE )"
        ,
        (SChar*) "CREATE TABLE SYS_PART_KEY_COLUMNS_ ( \
USER_ID               INTEGER, \
PARTITION_OBJ_ID      INTEGER, \
COLUMN_ID             INTEGER, \
OBJECT_TYPE           INTEGER, \
PART_COL_ORDER        INTEGER )"
        ,

        (SChar*) "CREATE TABLE SYS_PART_LOBS_ ( \
USER_ID               INTEGER, \
TABLE_ID              INTEGER, \
PARTITION_ID          INTEGER, \
COLUMN_ID             INTEGER, \
TBS_ID                INTEGER, \
LOGGING               CHAR(1), \
BUFFER                CHAR(1) )"
        ,
        
/**************************************************
 * BUG-21387 COMMENT
 **************************************************/
        (SChar*) "CREATE TABLE SYS_COMMENTS_ ( \
USER_NAME             VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, \
TABLE_NAME            VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, \
COLUMN_NAME           VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED,          \
COMMENTS              VARCHAR("QC_MAX_COMMENT_LITERAL_LEN_STR") )"
        ,
/**************************************************
 * PROJ-2002 COLUMN SECURITY
 *
 * SYS_SECURITY_
 * MODULE_NAME       : 외부 보안모듈의 이름
 * MODULE_VERSION    : 외부 보안모듈의 버전
 * ECC_POLICY_NAME   : ECC 생성에 사용하는 policy name
 * ECC_POLICY_CODE   : ECC policy의 유효성 검증 코드
 *
 * SYS_ENCRYPTED_COLUMNS_
 * USER_ID           : User ID
 * TABLE_ID          : Table ID
 * COLUMN_ID         : Column ID
 * ENCRYPT_PRECISION : 컬럼 암호화의 precision
 * POLICY_NAME       : 컬럼 암호화에 사용한 policy name
 * POLICY_CODE       : 컬럼 암호화에 사용한 policy의 유효성 검증 코드
 **************************************************/
        (SChar*) "CREATE TABLE SYS_SECURITY_ ( \
MODULE_NAME           VARCHAR(24)  FIXED, \
MODULE_VERSION        VARCHAR(40)  FIXED, \
ECC_POLICY_NAME       VARCHAR(16)  FIXED, \
ECC_POLICY_CODE       VARCHAR(64)  FIXED )"
        ,

        (SChar*) "CREATE TABLE SYS_ENCRYPTED_COLUMNS_ ( \
USER_ID               INTEGER,            \
TABLE_ID              INTEGER,            \
COLUMN_ID             INTEGER,            \
ENCRYPT_PRECISION     INTEGER,            \
POLICY_NAME           VARCHAR(16)  FIXED, \
POLICY_CODE           VARCHAR(128) FIXED )"
        ,

/* PROJ-2207 Password policy support */        
        (SChar*) "CREATE TABLE SYS_PASSWORD_HISTORY_ ( \
USER_ID               INTEGER,            \
PASSWORD              VARCHAR(256) FIXED,  \
PASSWORD_DATE         DATE )"
        ,

/* PROJ-2211 Materialized View */
        (SChar*) "CREATE TABLE SYS_MATERIALIZED_VIEWS_ ( "
"USER_ID             INTEGER     NOT NULL, "
"MVIEW_ID            INTEGER     NOT NULL, "
"MVIEW_NAME          VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, "
"TABLE_ID            INTEGER     NOT NULL, "
"VIEW_ID             INTEGER     NOT NULL, "
"REFRESH_TYPE        CHAR(1)     NOT NULL, "
"REFRESH_TIME        CHAR(1)     NOT NULL, "
"CREATED             DATE        NOT NULL, "
"LAST_DDL_TIME       DATE        NOT NULL, "
"LAST_REFRESH_TIME   DATE )"
        ,
// PROJ-1685
        (SChar*) "CREATE TABLE SYS_LIBRARIES_ ( "
"LIBRARY_ID          BIGINT      NOT NULL, "
"USER_ID             INTEGER     NOT NULL, "
"LIBRARY_NAME        VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, "
"FILE_SPEC           VARCHAR("QCM_MAX_DEFAULT_VAL_STR"), "
"DYNAMIC             VARCHAR(1) FIXED, "
"STATUS              VARCHAR(7) FIXED, "
"CREATED             DATE        NOT NULL, "
"LAST_DDL_TIME       DATE        NOT NULL )"
        ,
        /* PROJ-1832 New database link */
        (SChar *)"CREATE TABLE SYS_DATABASE_LINKS_ ("
        "USER_ID            INTEGER,"
        "LINK_ID            INTEGER             NOT NULL,"
        "LINK_OID           BIGINT              NOT NULL,"
        "LINK_NAME          VARCHAR("QCM_META_NAME_LEN") FIXED   NOT NULL,"
        "USER_MODE          INTEGER             NOT NULL,"
        "REMOTE_USER_ID     VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED,"
        "REMOTE_USER_PWD    BYTE(40)    FIXED,"
        "LINK_TYPE          INTEGER             NOT NULL,"
        "TARGET_NAME        VARCHAR("QCM_META_NAME_LEN") FIXED   NOT NULL,"
        "CREATED            DATE                NOT NULL,"
        "LAST_DDL_TIME      DATE                NOT NULL )"
        ,
/* PROJ-2223 audit */
        (SChar*) "CREATE TABLE SYS_AUDIT_ ( "
"IS_STARTED          INTEGER     NOT NULL, "
"START_TIME          DATE,                 "
"STOP_TIME           DATE,                 "
"RELOAD_TIME         DATE )"
        ,

        (SChar*) "CREATE TABLE SYS_AUDIT_ALL_OPTS_ ( "
"USER_ID             INTEGER     NOT NULL, "
"OBJECT_NAME         VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NULL, "
"SELECT_SUCCESS      CHAR(1)     NOT NULL, "
"SELECT_FAILURE      CHAR(1)     NOT NULL, "
"INSERT_SUCCESS      CHAR(1)     NOT NULL, "
"INSERT_FAILURE      CHAR(1)     NOT NULL, "
"UPDATE_SUCCESS      CHAR(1)     NOT NULL, "
"UPDATE_FAILURE      CHAR(1)     NOT NULL, "
"DELETE_SUCCESS      CHAR(1)     NOT NULL, "
"DELETE_FAILURE      CHAR(1)     NOT NULL, "
"MOVE_SUCCESS        CHAR(1)     NOT NULL, "
"MOVE_FAILURE        CHAR(1)     NOT NULL, "
"MERGE_SUCCESS       CHAR(1)     NOT NULL, "
"MERGE_FAILURE       CHAR(1)     NOT NULL, "
"ENQUEUE_SUCCESS     CHAR(1)     NOT NULL, "
"ENQUEUE_FAILURE     CHAR(1)     NOT NULL, "
"DEQUEUE_SUCCESS     CHAR(1)     NOT NULL, "
"DEQUEUE_FAILURE     CHAR(1)     NOT NULL, "
"LOCK_SUCCESS        CHAR(1)     NOT NULL, "
"LOCK_FAILURE        CHAR(1)     NOT NULL, "
"EXECUTE_SUCCESS     CHAR(1)     NOT NULL, "
"EXECUTE_FAILURE     CHAR(1)     NOT NULL, "
"COMMIT_SUCCESS      CHAR(1)     NOT NULL, "
"COMMIT_FAILURE      CHAR(1)     NOT NULL, "
"ROLLBACK_SUCCESS    CHAR(1)     NOT NULL, "
"ROLLBACK_FAILURE    CHAR(1)     NOT NULL, "
"SAVEPOINT_SUCCESS   CHAR(1)     NOT NULL, "
"SAVEPOINT_FAILURE   CHAR(1)     NOT NULL, "
"CONNECT_SUCCESS     CHAR(1)     NOT NULL, "
"CONNECT_FAILURE     CHAR(1)     NOT NULL, "
"DISCONNECT_SUCCESS  CHAR(1)     NOT NULL, "
"DISCONNECT_FAILURE  CHAR(1)     NOT NULL, "
"ALTER_SESSION_SUCCESS  CHAR(1)  NOT NULL, "
"ALTER_SESSION_FAILURE  CHAR(1)  NOT NULL, "
"ALTER_SYSTEM_SUCCESS   CHAR(1)  NOT NULL, "
"ALTER_SYSTEM_FAILURE   CHAR(1)  NOT NULL, "
"DDL_SUCCESS         CHAR(1)     NOT NULL, "
"DDL_FAILURE         CHAR(1)     NOT NULL, "
"CREATED             DATE        NOT NULL, "
"LAST_DDL_TIME       DATE )"
        ,
// PROJ-2264 Dictionary Table
        (SChar*) "CREATE TABLE SYS_COMPRESSION_TABLES_ ( "
"TABLE_ID            INTEGER     NOT NULL, "
"COLUMN_ID           INTEGER     NOT NULL, "
"DIC_TABLE_ID        INTEGER     NOT NULL, "
"MAXROWS             BIGINT      NOT NULL )"
        ,

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        (SChar*) "CREATE TABLE SYS_CONSTRAINT_RELATED_ ( "
"USER_ID                INTEGER           NOT NULL, "
"TABLE_ID               INTEGER           NOT NULL, "
"CONSTRAINT_ID          INTEGER           NOT NULL, "
"RELATED_USER_ID        INTEGER           NOT NULL, "
"RELATED_PROC_NAME      VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL )"
        ,

        (SChar*) "CREATE TABLE SYS_INDEX_RELATED_ ( "
"USER_ID                INTEGER           NOT NULL, "
"TABLE_ID               INTEGER           NOT NULL, "
"INDEX_ID               INTEGER           NOT NULL, "
"RELATED_USER_ID        INTEGER           NOT NULL, "
"RELATED_PROC_NAME      VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL )"
        ,

/* PROJ-1438 Job Scheduler */
// BUG-41317 each job enable disable
        (SChar*) "CREATE TABLE SYS_JOBS_ ( "
"JOB_ID             INTEGER                 NOT NULL, "
"JOB_NAME           VARCHAR("QCM_META_OBJECT_NAME_LEN")   FIXED     NOT NULL, "
"EXEC_QUERY         VARCHAR(1000) VARIABLE  NOT NULL, "
"START_TIME         DATE                    NOT NULL, "
"END_TIME           DATE                            , "
"INTERVAL           INTEGER                 NOT NULL, "
"INTERVAL_TYPE      CHAR(2)                         , "
"STATE              INTEGER                 NOT NULL, "
"LAST_EXEC_TIME     DATE                            , "
"EXEC_COUNT         INTEGER                 NOT NULL, "
"ERROR_CODE         CHAR(7)                         , "
"IS_ENABLE          CHAR(1)                 NOT NULL, "
"COMMENT            VARCHAR(4000) VARIABLE )"
        ,
/* PROJ-1812 ROLE */
        (SChar*) "CREATE TABLE SYS_USER_ROLES_ ( "
"GRANTOR_ID            INTEGER                 NOT NULL, "
"GRANTEE_ID            INTEGER                 NOT NULL, "
"ROLE_ID               INTEGER                 NOT NULL )"
        ,
/* INDEX OF   DBA_USERS_  */
        (SChar*) "CREATE UNIQUE INDEX DBA_USERS_INDEX1 ON \
DBA_USERS_ (USER_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX DBA_USERS_INDEX2 ON \
DBA_USERS_ (USER_NAME)"
        ,
/* INDEX OF   SYS_DN_USERS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_DN_USERS_INDEX1 ON \
SYS_DN_USERS_ (USER_DN, USER_AID, USER_NAME)"
        ,
/*
  (SChar*) "ALTER TABLE SYS_DN_USERS_ ALTER COLUMN \
  ( USER_NAME NOT NULL )"
  ,
  (SChar*) "ALTER TABLE SYS_DN_USERS_ ALTER COLUMN \
  ( REGISTERED NOT NULL )"
  ,
*/
/* INDEX OF   SYS_TBS_USERS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_TBS_USERS_INDEX1 ON \
SYS_TBS_USERS_ (TBS_ID, USER_ID)"
        ,
/* INDEX OF   SYS_TABLES_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_TABLES_INDEX1 ON \
SYS_TABLES_ (TABLE_NAME, USER_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_TABLES_INDEX2 ON \
SYS_TABLES_ (TABLE_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_TABLES_INDEX3 ON \
SYS_TABLES_ (USER_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_TABLES_INDEX4 ON \
SYS_TABLES_ (TBS_ID)"
        ,
/* INDEX OF   SYS_COLUMNS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_COLUMNS_INDEX1 ON \
SYS_COLUMNS_ (COLUMN_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_COLUMNS_INDEX2 ON \
SYS_COLUMNS_ (TABLE_ID, COLUMN_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_COLUMNS_INDEX3 ON \
SYS_COLUMNS_ (TABLE_ID, COLUMN_ORDER)"
        ,
/* INDEX OF   SYS_INDICES_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_INDICES_INDEX1 ON \
SYS_INDICES_ (INDEX_ID, INDEX_TYPE)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_INDICES_INDEX2 ON \
SYS_INDICES_ (USER_ID, INDEX_NAME)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_INDICES_INDEX3 ON \
SYS_INDICES_ (TABLE_ID, INDEX_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_INDICES_INDEX4 ON \
SYS_INDICES_ (TBS_ID)"
        ,
/* INDEX OF   SYS_INDEX_COLUMNS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_INDEX_COLUMNS_INDEX1 ON \
SYS_INDEX_COLUMNS_ (INDEX_ID, COLUMN_ID, SORT_ORDER)"
        ,
/* INDEX OF   SYS_CONSTRAINTS_  */
        (SChar*) "CREATE        INDEX SYS_CONSTRAINTS_INDEX1 ON \
SYS_CONSTRAINTS_ (TABLE_ID, CONSTRAINT_TYPE)"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINTS_INDEX2 ON \
SYS_CONSTRAINTS_ (TABLE_ID, INDEX_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINTS_INDEX3 ON \
SYS_CONSTRAINTS_ (REFERENCED_TABLE_ID, REFERENCED_INDEX_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINTS_INDEX4 ON \
SYS_CONSTRAINTS_ (USER_ID, CONSTRAINT_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINTS_INDEX5 ON \
SYS_CONSTRAINTS_ (USER_ID, REFERENCED_TABLE_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_CONSTRAINTS_INDEX6 ON \
SYS_CONSTRAINTS_ (CONSTRAINT_ID)"
        ,
        (SChar*) "CREATE INDEX SYS_CONSTRAINTS_INDEX7 ON "
                 "SYS_CONSTRAINTS_ (CONSTRAINT_NAME)"
        ,
/* INDEX OF   SYS_CONSTRAINT_COLUMNS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_CONSTRAINT_COLUMNS_INDEX1 ON \
SYS_CONSTRAINT_COLUMNS_ (CONSTRAINT_ID, CONSTRAINT_COL_ORDER)"
        ,
/* BUG-34320 Add index in meta to increase performance of drop table. */
        (SChar*) "CREATE INDEX SYS_CONSTRAINT_COLUMNS_INDEX2 ON \
SYS_CONSTRAINT_COLUMNS_ (TABLE_ID)"
        ,
#if !defined(SMALL_FOOTPRINT)
/* INDEX OF   SYS_REPLICATIONS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLICATIONS_INDEX1 ON \
SYS_REPLICATIONS_ (REPLICATION_NAME)"
        ,
/* INDEX OF   SYS_REPL_RECEIVER_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPL_RECEIVER_INDEX1 ON \
SYS_REPL_RECEIVER_ (REPLICATION_NAME)"
        ,
/* INDEX OF   SYS_REPL_HOSTS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPL_HOSTS_INDEX1 ON \
SYS_REPL_HOSTS_ (HOST_NO)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_REPL_HOSTS_INDEX2 ON \
SYS_REPL_HOSTS_ (REPLICATION_NAME, HOST_IP, PORT_NO)"
        ,
        (SChar*) "CREATE INDEX SYS_REPL_HOSTS_INDEX3 ON \
SYS_REPL_HOSTS_ (HOST_IP, PORT_NO)"
        ,
/* INDEX OF   SYS_REPL_ITEMS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLITEMS_INDEX1 ON \
SYS_REPL_ITEMS_ ( REPLICATION_NAME, TABLE_OID )"
        ,
        (SChar*) "CREATE INDEX SYS_REPLITEMS_INDEX2 ON \
SYS_REPL_ITEMS_ ( TABLE_OID )"
        ,
/* INDEX OF   SYS_REPL_OLD_ITEMS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLOLDITEMS_INDEX1 ON \
SYS_REPL_OLD_ITEMS_ ( REPLICATION_NAME, TABLE_OID )"
        ,
/* INDEX OF   SYS_REPL_OLD_COLUMNS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLOLDCOLS_INDEX1 ON \
SYS_REPL_OLD_COLUMNS_ ( REPLICATION_NAME, TABLE_OID, COLUMN_NAME )"
        ,
/* INDEX OF   SYS_REPL_OLD_INDICES_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLOLDIDXS_INDEX1 ON \
SYS_REPL_OLD_INDICES_ ( REPLICATION_NAME, TABLE_OID, INDEX_ID )"
        ,
/* INDEX OF   SYS_REPL_OLD_INDEX_COLUMNS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLOLDIDXCOLS_INDEX1 ON \
SYS_REPL_OLD_INDEX_COLUMNS_ ( REPLICATION_NAME, TABLE_OID, \
INDEX_ID, KEY_COLUMN_ID )"
        ,
/* INDEX Of    SYS_REPL_OLD_CHECKS_ */ /* PROJ-2642 */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLOLDCHECKS_INDEX1 ON \
SYS_REPL_OLD_CHECKS_ ( REPLICATION_NAME, TABLE_OID, CONSTRAINT_ID )"\
        ,
/* INDEX Of    SYS_REPL_OLD_CHECK_COLUMNS_ */ /* PROJ-2642 */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPLOLDCHECKCOLUMNS_INDEX1 ON \
SYS_REPL_OLD_CHECK_COLUMNS_ ( REPLICATION_NAME, TABLE_OID, CONSTRAINT_ID, COLUMN_ID )"\
        ,
/* PROJ-1915 */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPL_OFFLINE_DIR_INDEX1 ON \
SYS_REPL_OFFLINE_DIR_ ( REPLICATION_NAME, LFG_ID )"
        ,
/* INDEX OF   SYS_REPL_ITEM_REPLACE_HISTORY_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_REPL_ITEM_REPLACE_HISTORY_INDEX1 ON \
SYS_REPL_ITEM_REPLACE_HISTORY_ ( REPLICATION_NAME, OLD_OID )"
, 
#endif
/* INDEX OF   SYS_PROCEDURES_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_PROCEDURES_INDEX1 ON \
SYS_PROCEDURES_ ( PROC_NAME, USER_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_PROCEDURES_INDEX2 ON \
SYS_PROCEDURES_ ( PROC_OID )"
        ,
        (SChar*) "CREATE        INDEX SYS_PROCEDURES_INDEX3 ON \
SYS_PROCEDURES_ ( USER_ID )"
        ,
/* INDEX OF   SYS_PROC_PARAS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_PROC_PARAS_INDEX2 ON \
SYS_PROC_PARAS_ ( PROC_OID, PARA_NAME )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_PROC_PARAS_INDEX3 ON \
SYS_PROC_PARAS_ ( PROC_OID, PARA_ORDER )"
        ,
/* INDEX OF   SYS_PROC_PARSE_  */
        (SChar*) "CREATE        INDEX SYS_PROC_PARSE_INDEX1 ON \
SYS_PROC_PARSE_ ( USER_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_PROC_PARSE_INDEX2 ON \
SYS_PROC_PARSE_ ( PROC_OID, SEQ_NO )"
        ,
/* INDEX OF   SYS_PROC_RELATED_  */
        (SChar*) "CREATE        INDEX SYS_PROC_RELATED_INDEX1 ON \
SYS_PROC_RELATED_ ( USER_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_PROC_RELATED_INDEX2 ON \
SYS_PROC_RELATED_ ( PROC_OID )"
        ,
        (SChar*) "CREATE        INDEX SYS_PROC_RELATED_INDEX3 ON \
SYS_PROC_RELATED_ ( RELATED_USER_ID, RELATED_OBJECT_NAME, RELATED_OBJECT_TYPE )"
        ,
// PROJ-1073 Package
/* INDEX OF   SYS_PACKAGES_        */
(SChar*) "CREATE UNIQUE INDEX SYS_PACKAGES_INDEX1 ON \
SYS_PACKAGES_ ( PACKAGE_NAME, PACKAGE_TYPE, USER_ID )"
        ,
(SChar*) "CREATE UNIQUE INDEX SYS_PACKAGES_INDEX2 ON \
SYS_PACKAGES_ ( PACKAGE_OID )"
        ,
(SChar*) "CREATE        INDEX SYS_PACKAGES_INDEX3 ON \
SYS_PACKAGES_ ( PACKAGE_TYPE )"
        ,
(SChar*) "CREATE        INDEX SYS_PACKAGES_INDEX4 ON \
SYS_PACKAGES_ ( USER_ID )"
        ,
/* INDEX OF   SYS_PACKAGE_PARAS_   */
(SChar*) "CREATE       INDEX SYS_PACKAGE_PARAS_INDEX1 ON \
SYS_PACKAGE_PARAS_ ( OBJECT_NAME, PACKAGE_OID, PARA_ORDER )"
        ,
(SChar*) "CREATE       INDEX SYS_PACKAGE_PARAS_INDEX2 ON \
SYS_PACKAGE_PARAS_ ( OBJECT_NAME, PACKAGE_OID, PARA_NAME )"
        ,
(SChar*) "CREATE       INDEX SYS_PACKAGE_PARAS_INDEX3 ON \
SYS_PACKAGE_PARAS_ ( PACKAGE_OID, OBJECT_NAME, PACKAGE_NAME )"
        ,
/* INDEX OF   SYS_PACKAGE_PARSE_   */
(SChar*) "CREATE UNIQUE INDEX SYS_PACKAGE_PARSE_INDEX1 ON \
SYS_PACKAGE_PARSE_ ( PACKAGE_OID, PACKAGE_TYPE, SEQ_NO )"
        ,
(SChar*) "CREATE        INDEX SYS_PACKAGE_PARSE_INDEX2 ON \
SYS_PACKAGE_PARSE_ ( USER_ID )"
        ,
/* INDEX OF   SYS_PACKAGE_RELATED_ */
(SChar*) "CREATE        INDEX SYS_PACKAGE_RELATED_INDEX1 ON \
SYS_PACKAGE_RELATED_ ( USER_ID )"
        ,
(SChar*) "CREATE        INDEX SYS_PACKAGE_RELATED_INDEX2 ON \
SYS_PACKAGE_RELATED_ ( PACKAGE_OID )"
        ,
(SChar*) "CREATE        INDEX SYS_PACKAGE_RELATED_INDEX3 ON \
SYS_PACKAGE_RELATED_ ( RELATED_USER_ID, RELATED_OBJECT_NAME, RELATED_OBJECT_TYPE )"
        ,
//=========================================================
// [PROJ-1359] SYS_TRIGGERS_를 위한 Index
//=========================================================

        (SChar*) "CREATE UNIQUE INDEX SYS_TRIGGERS_USERID_TRIGGERNAME_IDX ON \
SYS_TRIGGERS_ ( USER_ID, TRIGGER_NAME ) "
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_TRIGGERS_TABLEID_TRIGGEROID_IDX ON \
SYS_TRIGGERS_ ( TABLE_ID, TRIGGER_OID ) "
        ,

//=========================================================
// [PROJ-1359] SYS_TRIGGER_STRINGS_ 를 위한 Index
//=========================================================

        (SChar *) "CREATE INDEX SYS_TRIGGER_STRINGS_OID_SEQNO_IDX ON \
SYS_TRIGGER_STRINGS_ ( TRIGGER_OID, SEQNO )"
        ,
        (SChar *) "CREATE INDEX SYS_TRIGGER_STRINGS_TABLEID_IDX ON \
SYS_TRIGGER_STRINGS_ ( TABLE_ID )"
        ,

//=========================================================
// [PROJ-1359] SYS_TRIGGER_UPDATE_COLUMNS_ 를 위한 Index
//=========================================================

        (SChar *) "CREATE INDEX SYS_TRIGGER_UPDATE_COLUMNS_OID_IDX ON \
SYS_TRIGGER_UPDATE_COLUMNS_ ( TRIGGER_OID )"
        ,
        (SChar *) "CREATE INDEX SYS_TRIGGER_UPDATE_COLUMNS_TID_IDX ON \
SYS_TRIGGER_UPDATE_COLUMNS_ ( TABLE_ID )"
        ,

//=========================================================
// [PROJ-1359] SYS_TRIGGER_DML_TABLES_ 를 위한 Index
//=========================================================

        (SChar *) "CREATE INDEX SYS_TRIGGER_DML_TABLES_TABLE_ID_IDX ON \
SYS_TRIGGER_DML_TABLES_ ( TABLE_ID )"
        ,
        (SChar *) "CREATE INDEX SYS_TRIGGER_DML_TABLES_TRIGGER_OID_IDX ON \
SYS_TRIGGER_DML_TABLES_ ( TRIGGER_OID )"
        ,
//=========================================================
// [PROJ-1362] SYS_LOBS_ 를 위한 Index
//=========================================================
        (SChar*) "CREATE UNIQUE INDEX SYS_LOBS_INDEX1 ON \
SYS_LOBS_ (COLUMN_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_LOBS_INDEX2 ON \
SYS_LOBS_ (TABLE_ID, COLUMN_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_LOBS_INDEX3 ON \
SYS_LOBS_ (TBS_ID)"
        ,

/* INDEX OF   SYS_PRIVILEGES_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_PRIVILEGES_INDEX1 ON \
SYS_PRIVILEGES_ ( PRIV_ID )"
        ,
/* INDEX OF   SYS_GRANT_SYSTEM_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_GRANT_SYSTEM_INDEX1 ON \
SYS_GRANT_SYSTEM_ (GRANTEE_ID, PRIV_ID, GRANTOR_ID)"
        ,
/* INDEX OF   SYS_GRANT_OBJECT_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_GRANT_OBJECT_INDEX1 ON \
SYS_GRANT_OBJECT_ (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTOR_ID, GRANTEE_ID, USER_ID)"
        ,
/* INDEX OF   SYS_GRANT_OBJECT_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_GRANT_OBJECT_INDEX2 ON \
SYS_GRANT_OBJECT_ (OBJ_ID, OBJ_TYPE, PRIV_ID, GRANTEE_ID, WITH_GRANT_OPTION)"
        ,
/* INDEX OF   SYS_GRANT_OBJECT_  */
        (SChar*) "CREATE INDEX SYS_GRANT_OBJECT_INDEX3 ON \
SYS_GRANT_OBJECT_ (OBJ_ID, OBJ_TYPE, GRANTOR_ID, GRANTEE_ID)"
        ,
/* INDEX OF   SYS_XA_HEURISTIC_TRANS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_XA_HEURISTIC_TRANS_INDEX1 ON \
SYS_XA_HEURISTIC_TRANS_ (FORMAT_ID, GLOBAL_TX_ID, BRANCH_QUALIFIER)"
        ,
/* INDEX OF   SYS_VIEWS_  */
        (SChar*) "CREATE UNIQUE INDEX SYS_VIEWS_INDEX1 ON \
SYS_VIEWS_ (VIEW_ID)"
        ,
        (SChar*) "CREATE        INDEX SYS_VIEWS_INDEX2 ON \
SYS_VIEWS_ (USER_ID)"
        ,
/* INDEX OF   SYS_VIEW_PARSE_  */
        (SChar*) "CREATE        INDEX SYS_VIEW_PARSE_INDEX1 ON \
SYS_VIEW_PARSE_ ( USER_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_VIEW_PARSE_INDEX2 ON \
SYS_VIEW_PARSE_ ( VIEW_ID, SEQ_NO )"
        ,
/* INDEX OF   SYS_VIEW_RELATED_  */
        (SChar*) "CREATE        INDEX SYS_VIEW_RELATED_INDEX1 ON \
SYS_VIEW_RELATED_ ( USER_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_VIEW_RELATED_INDEX2 ON \
SYS_VIEW_RELATED_ ( VIEW_ID, RELATED_OBJECT_TYPE )"
        ,
        (SChar*) "CREATE        INDEX SYS_VIEW_RELATED_INDEX3 ON \
SYS_VIEW_RELATED_ ( RELATED_USER_ID, RELATED_OBJECT_NAME, RELATED_OBJECT_TYPE )"
        ,
/* Proj-1076 Synonym INDEX OF SYS_SYNONYMS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_SYNONYMS_INDEX1 ON \
SYS_SYNONYMS_ (SYNONYM_OWNER_ID, SYNONYM_NAME)"
        ,
/* PROJ-1812 ROLE */
        (SChar*) "CREATE UNIQUE INDEX SYS_USER_ROLES_INDEX1 ON \
SYS_USER_ROLES_ ( GRANTOR_ID, GRANTEE_ID, ROLE_ID )"
        ,
        (SChar*) "CREATE INDEX SYS_USER_ROLES_INDEX2 ON \
SYS_USER_ROLES_ ( GRANTEE_ID )"
        ,
        (SChar*) "CREATE INDEX SYS_USER_ROLES_INDEX3 ON \
SYS_USER_ROLES_ ( ROLE_ID )"
        ,
        (SChar*) "CREATE ROLE PUBLIC"
        ,
/* SYSTEM USER */
        (SChar*) "CREATE USER SYSTEM_ IDENTIFIED BY MANAGER"
        ,
        (SChar*) "CREATE USER SYS IDENTIFIED BY MANAGER"
        ,
//=========================================================
// [PROJ-1371] SYS_DIRECTORIES_ 를 위한 Index
//=========================================================
        (SChar*) "CREATE UNIQUE INDEX SYS_DIRECTORIES_INDEX1 ON \
SYS_DIRECTORIES_ ( DIRECTORY_ID )"
        ,
        (SChar*) "CREATE INDEX SYS_DIRECTORIES_INDEX2 ON \
SYS_DIRECTORIES_ ( USER_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_DIRECTORIES_INDEX3 ON \
SYS_DIRECTORIES_ ( DIRECTORY_NAME )"
        ,

/**************************************************/
/* PROJ-1488 for Altibase Spatio-Temporal DBMS */
/**************************************************/
/* INDEX OF SYS_GEOMETRIES_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_GEOMETRIES_INDEX1 ON \
SYS_GEOMETRIES_ (COLUMN_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_GEOMETRIES_INDEX2 ON \
SYS_GEOMETRIES_ (TABLE_ID, COLUMN_ID)"
        ,
/* INDEX OF USER_SRS_ */
        (SChar*) "CREATE UNIQUE INDEX USER_SRS_INDEX1 ON \
USER_SRS_ (SRID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX USER_SRS_INDEX2 ON \
USER_SRS_ (AUTH_SRID)"
        ,

/**************************************************
 * PROJ-1502 PARTITIONED DISK TABLE - INDEX CREATE
 **************************************************/
/* INDEX OF SYS_PART_TABLES_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_PART_TABLES_IDX1 ON \
SYS_PART_TABLES_ (TABLE_ID)"
        ,
/* INDEX OF SYS_PART_INDICES_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_PART_INDICES_IDX1 ON \
SYS_PART_INDICES_ (INDEX_ID)"
        ,
/* INDEX OF SYS_TABLE_PARTITIONS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_TABLE_PARTITIONS_IDX1 ON \
SYS_TABLE_PARTITIONS_ (PARTITION_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_TABLE_PARTITIONS_IDX2 ON \
SYS_TABLE_PARTITIONS_ (TABLE_ID, PARTITION_NAME)"
        ,
        (SChar*) "CREATE INDEX SYS_TABLE_PARTITIONS_IDX3 ON \
SYS_TABLE_PARTITIONS_ (USER_ID)"
        ,
        (SChar*) "CREATE INDEX SYS_TABLE_PARTITIONS_IDX4 ON \
SYS_TABLE_PARTITIONS_ (TBS_ID)"
        ,
        (SChar*) "CREATE INDEX SYS_TABLE_PARTITIONS_IDX5 ON \
SYS_TABLE_PARTITIONS_ (TABLE_ID, PARTITION_ORDER)"
        ,
/* INDEX OF SYS_INDEX_PARTITIONS_ */
        (SChar*) "CREATE INDEX SYS_INDEX_PARTITIONS_IDX1 ON \
SYS_INDEX_PARTITIONS_ (TABLE_PARTITION_ID, INDEX_PARTITION_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_INDEX_PARTITIONS_IDX2 ON \
SYS_INDEX_PARTITIONS_ (INDEX_ID, INDEX_PARTITION_NAME)"
        ,
        (SChar*) "CREATE INDEX SYS_INDEX_PARTITIONS_IDX3 ON \
SYS_INDEX_PARTITIONS_ (TBS_ID)"
        ,
        (SChar*) "CREATE INDEX SYS_INDEX_PARTITIONS_IDX4 ON \
SYS_INDEX_PARTITIONS_ (USER_ID)"
        ,
/* INDEX OF SYS_PART_KEY_COLUMNS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_PART_KEY_COLUMNS_IDX1 ON \
SYS_PART_KEY_COLUMNS_ (OBJECT_TYPE, PARTITION_OBJ_ID, PART_COL_ORDER)"
        ,
/* INDEX OF SYS_PART_LOBS_ */
        (SChar*) "CREATE UNIQUE INDEX SYS_PART_LOBS_IDX1 ON \
SYS_PART_LOBS_ (USER_ID, TABLE_ID, PARTITION_ID, COLUMN_ID)"
        ,
        (SChar*) "CREATE INDEX SYS_PART_LOBS_IDX2 ON \
SYS_PART_LOBS_ (TBS_ID)"
        ,

/**************************************************
 * BUG-21387 COMMENT
 **************************************************/
        (SChar*) "CREATE UNIQUE INDEX SYS_COMMENTS_IDX1 ON \
SYS_COMMENTS_ (USER_NAME, TABLE_NAME, COLUMN_NAME)"
        ,
        
/**************************************************
 * PROJ-2002 Column Security
 **************************************************/
        (SChar*) "CREATE UNIQUE INDEX SYS_ENCRYPTED_COLUMNS_INDEX1 ON \
SYS_ENCRYPTED_COLUMNS_ (COLUMN_ID)"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_ENCRYPTED_COLUMNS_INDEX2 ON \
SYS_ENCRYPTED_COLUMNS_ (TABLE_ID, COLUMN_ID)"
        ,
/* PROJ-2207 Password policy support */        
        (SChar*) "CREATE UNIQUE INDEX SYS_PASSWORD_HISTORY_INDEX1 ON \
SYS_PASSWORD_HISTORY_ (USER_ID, PASSWORD)"
        ,
        (SChar*) "CREATE INDEX SYS_PASSWORD_HISTORY_INDEX2 ON \
SYS_PASSWORD_HISTORY_ (USER_ID, PASSWORD_DATE)"
        ,        
/* PROJ-2211 Materialized View */
        (SChar*) "CREATE UNIQUE INDEX SYS_MATERIALIZED_VIEWS_INDEX1 ON "
"SYS_MATERIALIZED_VIEWS_ ( MVIEW_NAME, USER_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_MATERIALIZED_VIEWS_INDEX2 ON "
"SYS_MATERIALIZED_VIEWS_ ( MVIEW_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_MATERIALIZED_VIEWS_INDEX3 ON "
"SYS_MATERIALIZED_VIEWS_ ( USER_ID )"
        ,
// PROJ-1685
        (SChar*) "CREATE UNIQUE INDEX SYS_LIBRARIES_INDEX1 ON "
"SYS_LIBRARIES_ ( LIBRARY_NAME, USER_ID )"
        ,
        (SChar*) "CREATE INDEX SYS_LIBRARIES_INDEX2 ON "
"SYS_LIBRARIES_ ( USER_ID )"
        ,

/**************************************************
 * PROJ-1832 New database link
 **************************************************/        
        (SChar *) "CREATE UNIQUE INDEX SYS_DATABASE_LINKS_INDEX ON "
        "SYS_DATABASE_LINKS_ ( USER_ID, LINK_ID )"
        ,
// PROJ-2264 Dictionary table
        (SChar*) "CREATE UNIQUE INDEX SYS_COMPRESSION_TABLES_IDX1 ON \
SYS_COMPRESSION_TABLES_ (TABLE_ID, COLUMN_ID, DIC_TABLE_ID)"
        ,
/* PROJ-2223 audit */
        (SChar*) "CREATE UNIQUE INDEX SYS_AUDIT_ALL_OPTS_INDEX1 ON "
"SYS_AUDIT_ALL_OPTS_ ( USER_ID, OBJECT_NAME )"
        ,

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        (SChar*) "CREATE        INDEX SYS_CONSTRAINT_RELATED_INDEX1 ON "
"SYS_CONSTRAINT_RELATED_ ( USER_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINT_RELATED_INDEX2 ON "
"SYS_CONSTRAINT_RELATED_ ( TABLE_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINT_RELATED_INDEX3 ON "
"SYS_CONSTRAINT_RELATED_ ( CONSTRAINT_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_CONSTRAINT_RELATED_INDEX4 ON "
"SYS_CONSTRAINT_RELATED_ ( RELATED_PROC_NAME, RELATED_USER_ID )"
        ,

        (SChar*) "CREATE        INDEX SYS_INDEX_RELATED_INDEX1 ON "
"SYS_INDEX_RELATED_ ( USER_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_INDEX_RELATED_INDEX2 ON "
"SYS_INDEX_RELATED_ ( TABLE_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_INDEX_RELATED_INDEX3 ON "
"SYS_INDEX_RELATED_ ( INDEX_ID )"
        ,
        (SChar*) "CREATE        INDEX SYS_INDEX_RELATED_INDEX4 ON "
"SYS_INDEX_RELATED_ ( RELATED_PROC_NAME, RELATED_USER_ID )"
        ,
/* PROJ-1438 Job Scheduler */
        (SChar*) "CREATE UNIQUE INDEX SYS_JOBS_INDEX1 ON "
"SYS_JOBS_ ( JOB_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_JOBS_INDEX2 ON "
"SYS_JOBS_ ( JOB_NAME )"
        ,
/* The last item should be NULL */
        NULL    };

/* Proj-1076 Synonym creataion */
    SChar * sCrtSynonymSql[] = {
//(SChar *) "CREATE PUBLIC SYNONYM PRINT FOR SYSTEM_.PRINT",
//(SChar *) "CREATE PUBLIC SYNONYM PRINTLN FOR SYSTEM_.PRINTLN",
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'PRINT', VARCHAR'SYSTEM_', VARCHAR'PRINT', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'PRINTLN', VARCHAR'SYSTEM_', VARCHAR'PRINTLN', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'SLEEP', VARCHAR'SYSTEM_', VARCHAR'SLEEP', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FCLOSE', VARCHAR'SYSTEM_', VARCHAR'FCLOSE', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FCOPY', VARCHAR'SYSTEM_', VARCHAR'FCOPY', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FFLUSH', VARCHAR'SYSTEM_', VARCHAR'FFLUSH', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FOPEN', VARCHAR'SYSTEM_', VARCHAR'FOPEN', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FREMOVE', VARCHAR'SYSTEM_', VARCHAR'FREMOVE', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FRENAME', VARCHAR'SYSTEM_', VARCHAR'FRENAME', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GET_LINE', VARCHAR'SYSTEM_', VARCHAR'GET_LINE', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'IS_OPEN', VARCHAR'SYSTEM_', VARCHAR'IS_OPEN', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'NEW_LINE', VARCHAR'SYSTEM_', VARCHAR'NEW_LINE', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'PUT', VARCHAR'SYSTEM_', VARCHAR'PUT', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'PUT_LINE', VARCHAR'SYSTEM_', VARCHAR'PUT_LINE', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'FCLOSE_ALL', VARCHAR'SYSTEM_', VARCHAR'FCLOSE_ALL', SYSDATE, SYSDATE )"
        ,
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'RAISE_APPLICATION_ERROR', VARCHAR'SYSTEM_', VARCHAR'RAISE_APPLICATION_ERROR', SYSDATE, SYSDATE )"
        ,
/* PROJ-2083 DUAL Table */
        (SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'DUAL', VARCHAR'', VARCHAR'X$DUAL', SYSDATE, SYSDATE )"
        ,
/* BUG-25999 */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'REMOVE_XID', VARCHAR'SYSTEM_', VARCHAR'REMOVE_XID', SYSDATE, SYSDATE )"
,
/* TASK-4990 changing the method of collecting index statistics */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GATHER_SYSTEM_STATS', VARCHAR'SYSTEM_', VARCHAR'GATHER_SYSTEM_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GATHER_DATABASE_STATS', VARCHAR'SYSTEM_', VARCHAR'GATHER_DATABASE_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GATHER_TABLE_STATS', VARCHAR'SYSTEM_', VARCHAR'GATHER_TABLE_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GATHER_INDEX_STATS', VARCHAR'SYSTEM_', VARCHAR'GATHER_INDEX_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'SET_SYSTEM_STATS', VARCHAR'SYSTEM_', VARCHAR'SET_SYSTEM_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'SET_COLUMN_STATS', VARCHAR'SYSTEM_', VARCHAR'SET_COLUMN_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'SET_TABLE_STATS', VARCHAR'SYSTEM_', VARCHAR'SET_TABLE_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'SET_INDEX_STATS', VARCHAR'SYSTEM_', VARCHAR'SET_INDEX_STATS', SYSDATE, SYSDATE )"
,
/* BUG-40119 get statistics psm */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GET_SYSTEM_STATS', VARCHAR'SYSTEM_', VARCHAR'GET_SYSTEM_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GET_COLUMN_STATS', VARCHAR'SYSTEM_', VARCHAR'GET_COLUMN_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GET_TABLE_STATS', VARCHAR'SYSTEM_', VARCHAR'GET_TABLE_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'GET_INDEX_STATS', VARCHAR'SYSTEM_', VARCHAR'GET_INDEX_STATS', SYSDATE, SYSDATE )"
,
/* BUG-38236 Interfaces for removing statistics information */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'DELETE_SYSTEM_STATS', VARCHAR'SYSTEM_', VARCHAR'DELETE_SYSTEM_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'DELETE_TABLE_STATS', VARCHAR'SYSTEM_', VARCHAR'DELETE_TABLE_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'DELETE_COLUMN_STATS', VARCHAR'SYSTEM_', VARCHAR'DELETE_COLUMN_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'DELETE_INDEX_STATS', VARCHAR'SYSTEM_', VARCHAR'DELETE_INDEX_STATS', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES (NULL, \
  VARCHAR'DELETE_DATABASE_STATS', VARCHAR'SYSTEM_', VARCHAR'DELETE_DATABASE_STATS', SYSDATE, SYSDATE )"
,
/* PROJ-2211 Materialized View */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'REFRESH_MATERIALIZED_VIEW', VARCHAR'SYSTEM_', VARCHAR'REFRESH_MATERIALIZED_VIEW', SYSDATE, SYSDATE )"
,
/* PROJ-1832 New database link */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'REMOTE_EXECUTE_IMMEDIATE', VARCHAR'SYSTEM_', VARCHAR'REMOTE_EXECUTE_IMMEDIATE', SYSDATE, SYSDATE )"
,
/* PROJ-1832 New database link */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'REMOTE_FREE_STATEMENT', VARCHAR'SYSTEM_', VARCHAR'REMOTE_FREE_STATEMENT', SYSDATE, SYSDATE )"
,
        
/* PROJ-1832 New database link */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'MEMORY_ALLOCATOR_DUMP', VARCHAR'SYSTEM_', VARCHAR'MEMORY_ALLOCATOR_DUMP', SYSDATE, SYSDATE )"
,
/* PROJ-2441 flashback */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'RECYCLEBIN', VARCHAR'SYSTEM_', VARCHAR'SYS_RECYCLEBIN_', SYSDATE, SYSDATE )"
,
/* BUG-41452 Built-in functions for getting array binding info. */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'IS_ARRAY_BOUND', VARCHAR'SYSTEM_', VARCHAR'IS_ARRAY_BOUND', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'IS_FIRST_ARRAY_BOUND', VARCHAR'SYSTEM_', VARCHAR'IS_FIRST_ARRAY_BOUND', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'IS_LAST_ARRAY_BOUND', VARCHAR'SYSTEM_', VARCHAR'IS_LAST_ARRAY_BOUND', SYSDATE, SYSDATE )"
,
(SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, "
"VARCHAR'SET_CLIENT_INFO', VARCHAR'SYSTEM_', VARCHAR'SET_CLIENT_INFO', SYSDATE, SYSDATE )"
,
(SChar *) "INSERT INTO SYS_SYNONYMS_ VALUES (NULL, "
"VARCHAR'SET_MODULE', VARCHAR'SYSTEM_', VARCHAR'SET_MODULE', SYSDATE, SYSDATE )"
,

/* PROJ-2422 srid */
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'GEOMETRY_COLUMNS', VARCHAR'SYSTEM_', VARCHAR'SYS_GEOMETRY_COLUMNS_', SYSDATE, SYSDATE )"
,
(SChar *)"INSERT INTO SYS_SYNONYMS_ VALUES ( NULL, "
"VARCHAR'SPATIAL_REF_SYS', VARCHAR'SYSTEM_', VARCHAR'USER_SRS_', SYSDATE, SYSDATE )"
,

/* The last item should be NULL */
        NULL    };

/* insert (dbname, meta_major_ver, meta_minor_ver) */
/* insert privileges */
    SChar   * sInsDefaultValuesSql[] = {
// (dbname, meta_major_ver, meta_minor_ver)
// VALUES( dbname, owner_dn,
//         QCM_META_MAJOR_VER, QCM_META_MINOR_VER, QCM_META_PATCH_VER )
// PROJ-2689 downgrade meta
        (SChar*) "INSERT INTO SYS_DATABASE_ VALUES ( '%s', \
         '%s', \
         "QCM_META_MAJOR_STR_VER", \
         "QCM_META_MINOR_STR_VER", \
         "QCM_META_PATCH_STR_VER", \
         "QCM_META_MAJOR_STR_VER", \
         "QCM_META_MINOR_STR_VER", \
         "QCM_META_PATCH_STR_VER")"
        ,
/* PROJ-2223 audit */
        (SChar*) "INSERT INTO SYS_AUDIT_ VALUES "
"(0, NULL, NULL, NULL)"
        ,
        
// system privileges
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALL_PRIVILEGES_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALL_PRIVILEGES_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_SYSTEM_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_SYSTEM_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_DATABASE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_DATABASE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_DATABASE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_DATABASE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_INDEX_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_INDEX_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_ANY_INDEX_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_INDEX_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_INDEX_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_INDEX_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_PROCEDURE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_PROCEDURE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_PROCEDURE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_PROCEDURE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_ANY_PROCEDURE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_PROCEDURE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_PROCEDURE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_PROCEDURE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_EXECUTE_ANY_PROCEDURE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_EXECUTE_ANY_PROCEDURE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_SEQUENCE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_SEQUENCE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_SEQUENCE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_SEQUENCE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_ANY_SEQUENCE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_SEQUENCE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_SEQUENCE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_SEQUENCE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_SELECT_ANY_SEQUENCE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_SELECT_ANY_SEQUENCE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_SESSION_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_SESSION_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_SESSION_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_SESSION_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DELETE_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DELETE_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_INSERT_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_INSERT_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_LOCK_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_LOCK_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_SELECT_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_SELECT_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_UPDATE_ANY_TABLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_UPDATE_ANY_TABLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_TABLESPACE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_TABLESPACE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_TABLESPACE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_TABLESPACE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_TABLESPACE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_TABLESPACE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_MANAGE_TABLESPACE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_MANAGE_TABLESPACE_STR"')"
        ,
/*
  (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
  ("QCM_PRIV_ID_SYSTEM_UNLIMITED_TABLESPACE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
  '"QCM_PRIV_NAME_SYSTEM_UNLIMITED_TABLESPACE_STR"')"
  ,
*/
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_USER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_USER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_USER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_USER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_USER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_USER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_VIEW_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_VIEW_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_VIEW_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_GRANT_ANY_PRIVILEGES_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_GRANT_ANY_PRIVILEGES_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_SYSDBA_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_SYSDBA_STR"')"
        ,
// object privileges
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_ALTER_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_ALTER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_DELETE_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_DELETE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_EXECUTE_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_EXECUTE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_INDEX_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_INDEX_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_INSERT_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_INSERT_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_REFERENCES_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_REFERENCES_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_SELECT_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_SELECT_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_OBJECT_UPDATE_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_OBJECT_UPDATE_STR"')"
        ,
        
//---------------------------------------------
// PROJ-1359 Trigger
//---------------------------------------------

        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_TRIGGER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_TRIGGER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_TRIGGER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_TRIGGER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_ALTER_ANY_TRIGGER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_TRIGGER_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_TRIGGER_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_TRIGGER_STR"')"
        ,
// Proj-1076 synonym
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_SYNONYM_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_SYNONYM_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_SYNONYM_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_PUBLIC_SYNONYM_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_SYNONYM_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_SYNONYM_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_SYNONYM_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_SYNONYM_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_SYNONYM_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_PUBLIC_SYNONYM_STR"')"
        ,
// PROJ-1371 directories
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_CREATE_ANY_DIRECTORY_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_DIRECTORY_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_SYSTEM_DROP_ANY_DIRECTORY_STR", "QCM_PRIV_TYPE_SYSTEM_STR", \
'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_DIRECTORY_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_DIRECTORY_READ_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_DIRECTORY_READ_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES \
("QCM_PRIV_ID_DIRECTORY_WRITE_STR", "QCM_PRIV_TYPE_OBJECT_STR", \
'"QCM_PRIV_NAME_DIRECTORY_WRITE_STR"')"
        ,

/* PROJ-2211 Materialized View */
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_MATERIALIZED_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_MATERIALIZED_VIEW_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_MATERIALIZED_VIEW_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_MATERIALIZED_VIEW_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_MATERIALIZED_VIEW_STR"')"
        ,
// PROJ-1685
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_LIBRARY_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_LIBRARY_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_ANY_LIBRARY_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_LIBRARY_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_DROP_ANY_LIBRARY_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_LIBRARY_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_ALTER_ANY_LIBRARY_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_LIBRARY_STR"')"
        ,

/* PROJ-1832 New database link */
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_DATABASE_LINK_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_DATABASE_LINK_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_PUBLIC_DATABASE_LINK_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_DROP_PUBLIC_DATABASE_LINK_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_DROP_PUBLIC_DATABASE_LINK_STR"')"
        ,
        /* PROJ-1812 ROLE */
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_ROLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_ROLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_DROP_ANY_ROLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_ROLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_GRANT_ANY_ROLE_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_GRANT_ANY_ROLE_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_CREATE_ANY_JOB_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_CREATE_ANY_JOB_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_ALTER_ANY_JOB_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_ALTER_ANY_JOB_STR"')"
        ,
        (SChar*) "INSERT INTO SYS_PRIVILEGES_ VALUES "
"("QCM_PRIV_ID_SYSTEM_DROP_ANY_JOB_STR", "QCM_PRIV_TYPE_SYSTEM_STR", "
"'"QCM_PRIV_NAME_SYSTEM_DROP_ANY_JOB_STR"')"
        ,

#if !defined(SMALL_FOOTPRINT)
// BUG-15076 DUMMY TABLE
        (SChar*) "INSERT INTO SYS_DUMMY_ VALUES \
( VARCHAR'X')"
        ,
#endif

/* The last item should be NULL */
        NULL    };
    
/* PROJ-2207 Password policy support */
    SChar   * sCrtMetaViewSql[] = {
        (SChar*) "CREATE VIEW SYS_PASSWORD_LIMITS_ \
(USER_ID, USER_NAME, ACCOUNT_STATUS, REMAIN_GRACE_DAY, \
FAILED_LOGIN_ATTEMPTS, PASSWORD_LOCK_TIME, \
PASSWORD_LIFE_TIME, PASSWORD_GRACE_TIME, \
PASSWORD_REUSE_TIME, PASSWORD_REUSE_MAX, \
PASSWORD_VERIFY_FUNCTION )  \
AS \
SELECT \
USER_ID, USER_NAME, \
CAST( \
    CASE WHEN LENGTH(EXPIRY_STATUS) > 0 AND LENGTH(LOCK_STATUS) > 0 THEN \
        EXPIRY_STATUS||'&'||LOCK_STATUS \
    WHEN LENGTH(EXPIRY_STATUS) > 0 THEN EXPIRY_STATUS \
    WHEN LENGTH(LOCK_STATUS) > 0 THEN LOCK_STATUS \
    ELSE VARCHAR'OPEN' \
    END \
    AS VARCHAR(30) \
) ACCOUNT_STATUS, \
CAST( DECODE( REMAIN_GRACE_DAY, 0, VARCHAR'unlimited', \
        REMAIN_GRACE_DAY||' day' ) AS VARCHAR(10) ) REMAIN_GRACE_DAY, \
CAST( DECODE( FAILED_LOGIN_ATTEMPTS_VALUE, 0, VARCHAR'unlimited', \
        FAILED_LOGIN_ATTEMPTS_VALUE||' count' ) AS VARCHAR(10) ) FAILED_LOGIN_ATTEMPTS, \
CAST( DECODE( PASSWORD_LOCK_TIME, 0, VARCHAR'unlimited', \
        PASSWORD_LOCK_TIME||' day' ) AS VARCHAR(10) ) PASSWORD_LOCK_TIME, \
CAST( DECODE( PASSWORD_LIFE_TIME, 0, VARCHAR'unlimited', \
        PASSWORD_LIFE_TIME||' day' ) AS VARCHAR(10) ) PASSWORD_LIFE_TIME, \
CAST( DECODE( PASSWORD_GRACE_TIME, 0, VARCHAR'unlimited', \
        PASSWORD_GRACE_TIME||' day' ) AS VARCHAR(10) ) PASSWORD_GRACE_TIME, \
CAST( DECODE( PASSWORD_REUSE_TIME, 0, VARCHAR'unlimited', \
        PASSWORD_REUSE_TIME||' day' ) AS VARCHAR(10) ) PASSWORD_REUSE_TIME, \
CAST( DECODE( PASSWORD_REUSE_MAX, 0, VARCHAR'unlimited', \
        PASSWORD_REUSE_MAX||' count' ) AS VARCHAR(10) ) PASSWORD_REUSE_MAX, \
PASSWORD_VERIFY_FUNCTION \
FROM \
( \
    SELECT \
    USER_ID, USER_NAME, \
    CASE WHEN CUR_DATE >= EXPIRY_GRACE_DATE THEN VARCHAR'EXPIRED' \
    WHEN CUR_DATE >= EXPIRY_DATE THEN VARCHAR'EXPIRED(GRACE)' \
    ELSE VARCHAR'' \
    END EXPIRY_STATUS, \
    CASE WHEN CUR_DATE >= LOCK_DATE AND LOCK_DATE = LOCK_TIMED_DATE  THEN VARCHAR'LOCKED' \
    WHEN CUR_DATE >= LOCK_DATE AND CUR_DATE < LOCK_TIMED_DATE THEN VARCHAR'LOCKED(TIMED)' \
    ELSE VARCHAR'' \
    END LOCK_STATUS, \
    CASE WHEN CUR_DATE >= EXPIRY_DATE AND CUR_DATE <= EXPIRY_GRACE_DATE THEN EXPIRY_GRACE_DATE - CUR_DATE \
    ELSE VARCHAR'0' \
    END REMAIN_GRACE_DAY, \
    FAILED_LOGIN_ATTEMPTS_VALUE,PASSWORD_LOCK_TIME,PASSWORD_LIFE_TIME,PASSWORD_GRACE_TIME, \
    PASSWORD_REUSE_TIME,PASSWORD_REUSE_MAX,PASSWORD_VERIFY_FUNCTION \
    FROM \
    ( \
        SELECT \
        USER_ID,USER_NAME,FAILED_LOGIN_ATTEMPTS,FAILED_LOGIN_ATTEMPTS FAILED_LOGIN_ATTEMPTS_VALUE, \
        FAILED_LOGIN_COUNT,EXPIRY_DATE,EXPIRY_DATE + PASSWORD_GRACE_TIME EXPIRY_GRACE_DATE, \
        LOCK_DATE,LOCK_DATE + PASSWORD_LOCK_TIME LOCK_TIMED_DATE, \
        CUR_DATE, \
        PASSWORD_LOCK_TIME,PASSWORD_LIFE_TIME,PASSWORD_GRACE_TIME, \
        PASSWORD_REUSE_TIME,PASSWORD_REUSE_MAX,PASSWORD_VERIFY_FUNCTION \
        FROM \
        ( \
            SELECT \
            USER_ID,USER_NAME,PASSWORD_GRACE_TIME,PASSWORD_LOCK_TIME, \
            FAILED_LOGIN_ATTEMPTS,FAILED_LOGIN_ATTEMPTS FAILED_LOGIN_ATTEMPTS_VALUE,FAILED_LOGIN_COUNT, \
            PASSWORD_REUSE_TIME,PASSWORD_REUSE_MAX, \
            DATEDIFF(TO_DATE('20010101','YYYYMMDD'),PASSWORD_EXPIRY_DATE,'DAY') \
            EXPIRY_DATE, \
            DATEDIFF(TO_DATE('20010101','YYYYMMDD'),ACCOUNT_LOCK_DATE,'DAY') LOCK_DATE, \
            DATEDIFF(TO_DATE('20010101','YYYYMMDD'),SYSDATE,'DAY') CUR_DATE, \
            PASSWORD_LIFE_TIME,PASSWORD_VERIFY_FUNCTION \
            FROM \
            SYSTEM_.DBA_USERS_ \
        ) \
    ) \
) \
",
        // PROJ-2223 audit
        (SChar*)
        "CREATE VIEW SYS_AUDIT_OPTS_(USER_NAME, OBJECT_NAME, OBJECT_TYPE, "
        "  SELECT_OP, INSERT_OP, UPDATE_OP, DELETE_OP, MOVE_OP, MERGE_OP, ENQUEUE_OP, DEQUEUE_OP, LOCK_TABLE_OP, EXECUTE_OP, "
        "  COMMIT_OP, ROLLBACK_OP, SAVEPOINT_OP, CONNECT_OP, DISCONNECT_OP, ALTER_SESSION_OP, ALTER_SYSTEM_OP, DDL_OP) "
        "AS "
        "SELECT "
        "  U.USER_NAME, A.OBJECT_NAME, "
        "  CAST(DECODE(T.TABLE_TYPE,'T','TABLE','V','VIEW','M','MATERIALIZED VIEW','A','VIEW','Q','QUEUE','S','SEQUENCE','W','SEQUENCE','UNKNOWN') AS VARCHAR(40)), "
        "  A.SELECT_SUCCESS||'/'||A.SELECT_FAILURE, "
        "  A.INSERT_SUCCESS||'/'||A.INSERT_FAILURE, "
        "  A.UPDATE_SUCCESS||'/'||A.UPDATE_FAILURE, "
        "  A.DELETE_SUCCESS||'/'||A.DELETE_FAILURE, "
        "  A.MOVE_SUCCESS||'/'||A.MOVE_FAILURE, "
        "  A.MERGE_SUCCESS||'/'||A.MERGE_FAILURE, "
        "  A.ENQUEUE_SUCCESS||'/'||A.ENQUEUE_FAILURE, "
        "  A.DEQUEUE_SUCCESS||'/'||A.DEQUEUE_FAILURE, "
        "  A.LOCK_SUCCESS||'/'||A.LOCK_FAILURE, "
        "  A.EXECUTE_SUCCESS||'/'||A.EXECUTE_FAILURE, "
        "  A.COMMIT_SUCCESS||'/'||A.COMMIT_FAILURE, "
        "  A.ROLLBACK_SUCCESS||'/'||A.ROLLBACK_FAILURE, "
        "  A.SAVEPOINT_SUCCESS||'/'||A.SAVEPOINT_FAILURE, "
        "  A.CONNECT_SUCCESS||'/'||A.CONNECT_FAILURE, "
        "  A.DISCONNECT_SUCCESS||'/'||A.DISCONNECT_FAILURE, "
        "  A.ALTER_SESSION_SUCCESS||'/'||A.ALTER_SESSION_FAILURE, "
        "  A.ALTER_SYSTEM_SUCCESS||'/'||A.ALTER_SYSTEM_FAILURE, "
        "  A.DDL_SUCCESS||'/'||A.DDL_FAILURE "
        "FROM "
        "  SYSTEM_.SYS_AUDIT_ALL_OPTS_ A, SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T "
        "WHERE "
        "  A.USER_ID = U.USER_ID AND A.OBJECT_NAME = T.TABLE_NAME AND U.USER_ID = T.USER_ID "
        "  AND U.USER_TYPE = 'U'" /* PROJ-1812 ROLE */
        "UNION ALL "
        "SELECT "
        "  U.USER_NAME, A.OBJECT_NAME, "
        "  CAST(DECODE(P.OBJECT_TYPE,0,'PROCEDURE',1,'FUNCTION',2,'TABLE',3,'TYPESET',4,'SYNONYM','UNKNOWN') AS VARCHAR(40)), "
        "  A.SELECT_SUCCESS||'/'||A.SELECT_FAILURE, "
        "  A.INSERT_SUCCESS||'/'||A.INSERT_FAILURE, "
        "  A.UPDATE_SUCCESS||'/'||A.UPDATE_FAILURE, "
        "  A.DELETE_SUCCESS||'/'||A.DELETE_FAILURE, "
        "  A.MOVE_SUCCESS||'/'||A.MOVE_FAILURE, "
        "  A.MERGE_SUCCESS||'/'||A.MERGE_FAILURE, "
        "  A.ENQUEUE_SUCCESS||'/'||A.ENQUEUE_FAILURE, "
        "  A.DEQUEUE_SUCCESS||'/'||A.DEQUEUE_FAILURE, "
        "  A.LOCK_SUCCESS||'/'||A.LOCK_FAILURE, "
        "  A.EXECUTE_SUCCESS||'/'||A.EXECUTE_FAILURE, "
        "  A.COMMIT_SUCCESS||'/'||A.COMMIT_FAILURE, "
        "  A.ROLLBACK_SUCCESS||'/'||A.ROLLBACK_FAILURE, "
        "  A.SAVEPOINT_SUCCESS||'/'||A.SAVEPOINT_FAILURE, "
        "  A.CONNECT_SUCCESS||'/'||A.CONNECT_FAILURE, "
        "  A.DISCONNECT_SUCCESS||'/'||A.DISCONNECT_FAILURE, "
        "  A.ALTER_SESSION_SUCCESS||'/'||A.ALTER_SESSION_FAILURE, "
        "  A.ALTER_SYSTEM_SUCCESS||'/'||A.ALTER_SYSTEM_FAILURE, "
        "  A.DDL_SUCCESS||'/'||A.DDL_FAILURE "
        "FROM "
        "  SYSTEM_.SYS_AUDIT_ALL_OPTS_ A, SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_PROCEDURES_ P "
        "WHERE "
        "  A.USER_ID = U.USER_ID AND A.OBJECT_NAME = P.PROC_NAME AND U.USER_ID = P.USER_ID "
        "  AND U.USER_TYPE = 'U'" /* PROJ-1812 ROLE */
        "UNION ALL "
        "SELECT "
        "  VARCHAR'ALL', VARCHAR'ALL', VARCHAR'', "
        "  A.SELECT_SUCCESS||'/'||A.SELECT_FAILURE, "
        "  A.INSERT_SUCCESS||'/'||A.INSERT_FAILURE, "
        "  A.UPDATE_SUCCESS||'/'||A.UPDATE_FAILURE, "
        "  A.DELETE_SUCCESS||'/'||A.DELETE_FAILURE, "
        "  A.MOVE_SUCCESS||'/'||A.MOVE_FAILURE, "
        "  A.MERGE_SUCCESS||'/'||A.MERGE_FAILURE, "
        "  A.ENQUEUE_SUCCESS||'/'||A.ENQUEUE_FAILURE, "
        "  A.DEQUEUE_SUCCESS||'/'||A.DEQUEUE_FAILURE, "
        "  A.LOCK_SUCCESS||'/'||A.LOCK_FAILURE, "
        "  A.EXECUTE_SUCCESS||'/'||A.EXECUTE_FAILURE, "
        "  A.COMMIT_SUCCESS||'/'||A.COMMIT_FAILURE, "
        "  A.ROLLBACK_SUCCESS||'/'||A.ROLLBACK_FAILURE, "
        "  A.SAVEPOINT_SUCCESS||'/'||A.SAVEPOINT_FAILURE, "
        "  A.CONNECT_SUCCESS||'/'||A.CONNECT_FAILURE, "
        "  A.DISCONNECT_SUCCESS||'/'||A.DISCONNECT_FAILURE, "
        "  A.ALTER_SESSION_SUCCESS||'/'||A.ALTER_SESSION_FAILURE, "
        "  A.ALTER_SYSTEM_SUCCESS||'/'||A.ALTER_SYSTEM_FAILURE, "
        "  A.DDL_SUCCESS||'/'||A.DDL_FAILURE "
        "FROM "
        "  SYSTEM_.SYS_AUDIT_ALL_OPTS_ A "
        "WHERE "
        "  A.USER_ID = INTEGER'0' AND A.OBJECT_NAME IS NULL "
        "UNION ALL "
        "SELECT "
        "  U.USER_NAME, VARCHAR'ALL', VARCHAR'', "
        "  A.SELECT_SUCCESS||'/'||A.SELECT_FAILURE, "
        "  A.INSERT_SUCCESS||'/'||A.INSERT_FAILURE, "
        "  A.UPDATE_SUCCESS||'/'||A.UPDATE_FAILURE, "
        "  A.DELETE_SUCCESS||'/'||A.DELETE_FAILURE, "
        "  A.MOVE_SUCCESS||'/'||A.MOVE_FAILURE, "
        "  A.MERGE_SUCCESS||'/'||A.MERGE_FAILURE, "
        "  A.ENQUEUE_SUCCESS||'/'||A.ENQUEUE_FAILURE, "
        "  A.DEQUEUE_SUCCESS||'/'||A.DEQUEUE_FAILURE, "
        "  A.LOCK_SUCCESS||'/'||A.LOCK_FAILURE, "
        "  A.EXECUTE_SUCCESS||'/'||A.EXECUTE_FAILURE, "
        "  A.COMMIT_SUCCESS||'/'||A.COMMIT_FAILURE, "
        "  A.ROLLBACK_SUCCESS||'/'||A.ROLLBACK_FAILURE, "
        "  A.SAVEPOINT_SUCCESS||'/'||A.SAVEPOINT_FAILURE, "
        "  A.CONNECT_SUCCESS||'/'||A.CONNECT_FAILURE, "
        "  A.DISCONNECT_SUCCESS||'/'||A.DISCONNECT_FAILURE, "
        "  A.ALTER_SESSION_SUCCESS||'/'||A.ALTER_SESSION_FAILURE, "
        "  A.ALTER_SYSTEM_SUCCESS||'/'||A.ALTER_SYSTEM_FAILURE, "
        "  A.DDL_SUCCESS||'/'||A.DDL_FAILURE "
        "FROM "
        "  SYSTEM_.SYS_AUDIT_ALL_OPTS_ A, SYSTEM_.DBA_USERS_ U "
        "WHERE "
        "  A.USER_ID = U.USER_ID AND A.OBJECT_NAME IS NULL "
        "  AND U.USER_TYPE = 'U'" /* PROJ-1812 ROLE */
        "UNION ALL "
        "SELECT "
        "  U.USER_NAME, A.OBJECT_NAME, "
        "  CAST(DECODE(P.PACKAGE_TYPE,6,'PACKAGE','UNKNOWN') AS VARCHAR(40)), "
        "  A.SELECT_SUCCESS||'/'||A.SELECT_FAILURE, "
        "  A.INSERT_SUCCESS||'/'||A.INSERT_FAILURE, "
        "  A.UPDATE_SUCCESS||'/'||A.UPDATE_FAILURE, "
        "  A.DELETE_SUCCESS||'/'||A.DELETE_FAILURE, "
        "  A.MOVE_SUCCESS||'/'||A.MOVE_FAILURE, "
        "  A.MERGE_SUCCESS||'/'||A.MERGE_FAILURE, "
        "  A.ENQUEUE_SUCCESS||'/'||A.ENQUEUE_FAILURE, "
        "  A.DEQUEUE_SUCCESS||'/'||A.DEQUEUE_FAILURE, "
        "  A.LOCK_SUCCESS||'/'||A.LOCK_FAILURE, "
        "  A.EXECUTE_SUCCESS||'/'||A.EXECUTE_FAILURE, "
        "  A.COMMIT_SUCCESS||'/'||A.COMMIT_FAILURE, "
        "  A.ROLLBACK_SUCCESS||'/'||A.ROLLBACK_FAILURE, "
        "  A.SAVEPOINT_SUCCESS||'/'||A.SAVEPOINT_FAILURE, "
        "  A.CONNECT_SUCCESS||'/'||A.CONNECT_FAILURE, "
        "  A.DISCONNECT_SUCCESS||'/'||A.DISCONNECT_FAILURE, "
        "  A.ALTER_SESSION_SUCCESS||'/'||A.ALTER_SESSION_FAILURE, "
        "  A.ALTER_SYSTEM_SUCCESS||'/'||A.ALTER_SYSTEM_FAILURE, "
        "  A.DDL_SUCCESS||'/'||A.DDL_FAILURE "
        "FROM "
        "  SYSTEM_.SYS_AUDIT_ALL_OPTS_ A, SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_PACKAGES_ P "
        "WHERE "
        "  A.USER_ID = U.USER_ID AND A.OBJECT_NAME = P.PACKAGE_NAME AND U.USER_ID = P.USER_ID AND P.PACKAGE_TYPE = 6"
        "  AND U.USER_TYPE = 'U'" /* PROJ-1812 ROLE */
        ,
        /* PROJ-2441 flashback */
(SChar*)"CREATE VIEW SYS_RECYCLEBIN_ AS "
" SELECT /*+GROUP_SORT*/ USER_NAME, TABLE_NAME, SUBSTRB(TABLE_NAME, 1, LENGTHB(TABLE_NAME) - 32) ORIGINAL_TABLE_NAME, "
" TBS_NAME, SUM(MEMORY_SIZE) MEMORY_SIZE, SUM(DISK_SIZE) DISK_SIZE, LAST_DDL_TIME DROPPED "
" FROM "
" ( "
"  SELECT /*+USE_HASH(ALL_TABLES ALL_SEGMENTS)*/ ALL_TABLES.USER_NAME, ALL_TABLES.TABLE_NAME, ALL_TABLES.TBS_NAME, "
"  ALL_TABLES.LAST_DDL_TIME, BIGINT'0' MEMORY_SIZE, ALL_SEGMENTS.TOTAL_USED_SIZE DISK_SIZE "
"  FROM "
"  ( "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, T.LAST_DDL_TIME, T.TABLE_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T "
"   WHERE U.USER_ID = T.USER_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'R' "
"   UNION ALL "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, T.LAST_DDL_TIME, P.PARTITION_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T, SYSTEM_.SYS_TABLE_PARTITIONS_ P "
"   WHERE U.USER_ID = T.USER_ID AND T.TABLE_ID = P.TABLE_ID AND U.USER_ID > 1 "
"   AND T.TABLE_TYPE = 'R' AND T.IS_PARTITIONED = 'T' "
"   ) ALL_TABLES, "
"  X$SEGMENT ALL_SEGMENTS "
"  WHERE ALL_TABLES.TABLE_OID = ALL_SEGMENTS.TABLE_OID "
"  UNION ALL "
"  SELECT /*+USE_HASH(ALL_TABLES ALL_PAGES)*/ ALL_TABLES.USER_NAME, ALL_TABLES.TABLE_NAME, ALL_TABLES.TBS_NAME, "
"  ALL_TABLES.LAST_DDL_TIME, "
"  CAST((ALL_PAGES.MEM_PAGE_CNT + ALL_PAGES.MEM_VAR_PAGE_CNT) * 1024 * 32 AS BIGINT) MEMORY_SIZE, BIGINT'0' DISK_SIZE "
"  FROM "
"  ( "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, T.LAST_DDL_TIME, T.TABLE_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T "
"   WHERE U.USER_ID = T.USER_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'R' "
"   UNION ALL "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, T.LAST_DDL_TIME, P.PARTITION_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T, SYSTEM_.SYS_TABLE_PARTITIONS_ P "
"   WHERE U.USER_ID = T.USER_ID AND T.TABLE_ID = P.TABLE_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'R' "
"   AND T.IS_PARTITIONED = 'T' "
"   ) ALL_TABLES, "
"  X$TABLE_INFO ALL_PAGES "
"  WHERE ALL_TABLES.TABLE_OID = ALL_PAGES.TABLE_OID "
"  ) "
" GROUP BY USER_NAME, TABLE_NAME, TBS_NAME, LAST_DDL_TIME; "
,
(SChar*)"CREATE VIEW SYS_TABLE_SIZE_ AS "
" SELECT /*+GROUP_SORT*/ USER_NAME, TABLE_NAME, TBS_NAME, SUM(MEMORY_SIZE) MEMORY_SIZE, SUM(DISK_SIZE) DISK_SIZE "
" FROM "
" ( "
"  SELECT /*+USE_HASH(ALL_TABLES ALL_SEGMENTS)*/ ALL_TABLES.USER_NAME, ALL_TABLES.TABLE_NAME, ALL_TABLES.TBS_NAME, "
"  BIGINT'0' MEMORY_SIZE, ALL_SEGMENTS.TOTAL_USED_SIZE DISK_SIZE "
"  FROM "
"  ( "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, T.TABLE_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T "
"   WHERE U.USER_ID = T.USER_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'T' "
"   UNION ALL "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, P.PARTITION_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T, SYSTEM_.SYS_TABLE_PARTITIONS_ P "
"   WHERE U.USER_ID = T.USER_ID AND T.TABLE_ID = P.TABLE_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'T' "
"   AND T.IS_PARTITIONED = 'T' "
"   ) ALL_TABLES, "
"  X$SEGMENT ALL_SEGMENTS "
"  WHERE ALL_TABLES.TABLE_OID = ALL_SEGMENTS.TABLE_OID "
"  UNION ALL "
"  SELECT /*+USE_HASH(ALL_TABLES ALL_PAGES)*/ ALL_TABLES.USER_NAME, ALL_TABLES.TABLE_NAME, ALL_TABLES.TBS_NAME, "
"  CAST((ALL_PAGES.MEM_PAGE_CNT + ALL_PAGES.MEM_VAR_PAGE_CNT) * 1024 * 32 AS BIGINT) MEMORY_SIZE, BIGINT'0' DISK_SIZE "
"  FROM "
"  ( "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, T.TABLE_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T "
"   WHERE U.USER_ID = T.USER_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'T' "
"   UNION ALL "
"   SELECT U.USER_NAME, T.TABLE_NAME, T.TBS_NAME, P.PARTITION_OID "
"   FROM SYSTEM_.DBA_USERS_ U, SYSTEM_.SYS_TABLES_ T, SYSTEM_.SYS_TABLE_PARTITIONS_ P "
"   WHERE U.USER_ID = T.USER_ID AND T.TABLE_ID = P.TABLE_ID AND U.USER_ID > 1 AND T.TABLE_TYPE = 'T' "
"   AND T.IS_PARTITIONED = 'T' "
"   ) ALL_TABLES, "
"  X$TABLE_INFO ALL_PAGES "
"  WHERE ALL_TABLES.TABLE_OID = ALL_PAGES.TABLE_OID "
"  ) "
" GROUP BY USER_NAME, TABLE_NAME, TBS_NAME; "
,
// BUG-41230 SYS_USERS_ => DBA_USERS_
(SChar*) " CREATE VIEW SYS_USERS_ ( \
USER_ID, USER_NAME, PASSWORD, DEFAULT_TBS_ID, TEMP_TBS_ID, ACCOUNT_LOCK, ACCOUNT_LOCK_DATE, \
PASSWORD_LIMIT_FLAG, FAILED_LOGIN_ATTEMPTS, FAILED_LOGIN_COUNT, PASSWORD_LOCK_TIME,  \
PASSWORD_EXPIRY_DATE, PASSWORD_LIFE_TIME, PASSWORD_GRACE_TIME, PASSWORD_REUSE_DATE, \
PASSWORD_REUSE_TIME, PASSWORD_REUSE_MAX, PASSWORD_REUSE_COUNT, PASSWORD_VERIFY_FUNCTION, \
USER_TYPE, DISABLE_TCP, CREATED, LAST_DDL_TIME ) \
AS \
SELECT \
USER_ID, USER_NAME, CAST('' AS VARCHAR(256)) PASSWORD, DEFAULT_TBS_ID, TEMP_TBS_ID, ACCOUNT_LOCK, ACCOUNT_LOCK_DATE, \
PASSWORD_LIMIT_FLAG, FAILED_LOGIN_ATTEMPTS, FAILED_LOGIN_COUNT, PASSWORD_LOCK_TIME,  \
PASSWORD_EXPIRY_DATE, PASSWORD_LIFE_TIME, PASSWORD_GRACE_TIME, PASSWORD_REUSE_DATE, \
PASSWORD_REUSE_TIME, PASSWORD_REUSE_MAX, PASSWORD_REUSE_COUNT, PASSWORD_VERIFY_FUNCTION, \
USER_TYPE, DISABLE_TCP, CREATED, LAST_DDL_TIME \
FROM DBA_USERS_; "
        ,
        /* PROJ-2422 srid */
        (SChar*)"CREATE VIEW SYS_GEOMETRY_COLUMNS_(F_TABLE_SCHEMA, F_TABLE_NAME, F_GEOMETRY_COLUMN, "
        "  COORD_DIMENSION, SRID) "
        "AS "
        "SELECT U.USER_NAME, T.TABLE_NAME, C.COLUMN_NAME, G.COORD_DIMENSION, G.SRID "
        "FROM SYSTEM_.SYS_USERS_ U, SYSTEM_.SYS_TABLES_ T, SYSTEM_.SYS_COLUMNS_ C, "
        "  SYSTEM_.SYS_GEOMETRIES_ G "
        "WHERE G.USER_ID = U.USER_ID AND G.TABLE_ID = T.TABLE_ID AND G.COLUMN_ID = C.COLUMN_ID"
        ,
        NULL };

    SInt            i;
    qcStatement     sStatement;
    smiStatement  * sDummySmiStmt;
    smiStatement    sSmiStmt;
    UInt            sStage = 0;

    UInt            sSmiStmtFlag  = 0;

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   aStatistics )
              != IDE_SUCCESS );

    sStage = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    /****************************************************/
    /* PHASE 1 : CREATE TABLES, INDICES                 */
    /****************************************************/
    for (i = 0; sCrtMetaSql[i] != NULL; i++)
    {
        IDE_TEST(aTrans->begin( &sDummySmiStmt,
                                aStatistics,
                                (SMI_ISOLATION_NO_PHANTOM     |
                                 SMI_TRANSACTION_NORMAL       |
                                 SMI_TRANSACTION_REPL_DEFAULT |
                                 SMI_COMMIT_WRITE_NOWAIT))
                 != IDE_SUCCESS);
        sStage = 2;

        IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag)
                 != IDE_SUCCESS);
        sStage = 3;

        IDE_TEST( executeDDL( &sStatement,
                              sCrtMetaSql[i] ) != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST(aTrans->commit() != IDE_SUCCESS);
    }

    /****************************************************/
    /* PHASE 2 : initialize meta table flag             */
    /****************************************************/
    IDE_TEST(aTrans->begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_ISOLATION_NO_PHANTOM     |
                            SMI_TRANSACTION_NORMAL       |
                            SMI_TRANSACTION_REPL_DEFAULT |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sStage = 2;

    IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
             != IDE_SUCCESS);
    sStage = 3;

    // initialize meta table flags
    IDE_TEST(qcmCreate::initMetaTableFlag(&sSmiStmt) != IDE_SUCCESS );

    sStage = 2;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(aTrans->commit() != IDE_SUCCESS);
    
    /****************************************************/
    /* PHASE 3 : initialize meta module                 */
    /****************************************************/
    IDE_TEST(aTrans->begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_ISOLATION_NO_PHANTOM     |
                            SMI_TRANSACTION_NORMAL       |
                            SMI_TRANSACTION_REPL_DEFAULT |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sStage = 2;

    IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
             != IDE_SUCCESS);
    sStage = 3;

    // initialize global meta handles ( table, index )
    IDE_TEST(qcm::initMetaHandles(&sSmiStmt) != IDE_SUCCESS );

    // initialize meta cache
    IDE_TEST(qcm::initMetaCaches(&sSmiStmt) != IDE_SUCCESS);

    sStage = 2;
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(aTrans->commit() != IDE_SUCCESS);

    /****************************************************/
    /* PHASE 4 : create procedures                      */
    /****************************************************/
    IDE_TEST(aTrans->begin(&sDummySmiStmt,
                           aStatistics,
                           (SMI_ISOLATION_NO_PHANTOM     |
                            SMI_TRANSACTION_NORMAL       |
                            SMI_TRANSACTION_REPL_DEFAULT |
                            SMI_COMMIT_WRITE_NOWAIT))
             != IDE_SUCCESS);
    sStage = 2;
 
    IDE_TEST( qcmCreate::createBuiltInPSM( aStatistics, &sStatement, &sSmiStmt, sDummySmiStmt )
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST(aTrans->commit() != IDE_SUCCESS);

    /**************************************************************/
    /* PHASE 5 : INSERT DEFAULT VALUES                            */
    /*         - insert into SYS_DATABASE_ default values         */
    /*         - insert into SYS_PRIVILEGES values PRIVILEGES     */
    /*         - insert into SYS_DUMMY_ values 'X'                */
    /**************************************************************/
    for (i = 0; sInsDefaultValuesSql[i] != NULL; i++)
    {
        IDE_TEST(aTrans->begin(&sDummySmiStmt,
                               aStatistics,
                               (SMI_ISOLATION_NO_PHANTOM     |
                                SMI_TRANSACTION_NORMAL       |
                                SMI_TRANSACTION_REPL_DEFAULT |
                                SMI_COMMIT_WRITE_NOWAIT))
                 != IDE_SUCCESS);
        sStage = 2;

        IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
                 != IDE_SUCCESS);
        sStage = 3;

        /*
         * especially treat when i equals zero,
         * for formatted string of SYS_DATABASE_
         */
        if( i == 0 )
        {
            SChar sInsSql[4096];
            SChar sOwnerDN[4096];
            IDE_TEST_RAISE( aOwnerDNLen > (ID_SIZEOF(sOwnerDN) -1), QCM_ERR_DNBufferOverflow );
            idlOS::memcpy(sOwnerDN,aOwnerDN,aOwnerDNLen);
            sOwnerDN[aOwnerDNLen] = '\0';
            idlOS::snprintf(sInsSql, ID_SIZEOF(sInsSql), sInsDefaultValuesSql[0], aDBName, sOwnerDN);
            IDE_TEST( executeDDL( &sStatement,
                                  sInsSql )
                != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( executeDDL( &sStatement,
                                  sInsDefaultValuesSql[i] )
                != IDE_SUCCESS );
        }
        sStage = 2;

        if (sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS)
        {

            sStage = 1;
            idlOS::abort();

        }

        IDE_TEST(aTrans->commit() != IDE_SUCCESS);

    }

#if !defined(SMALL_FOOTPRINT)
    // Project-1076 create synonym
    for (i = 0;  sCrtSynonymSql[i] != NULL; i++)
    {
        IDE_TEST(aTrans->begin(&sDummySmiStmt,
                               aStatistics,
                               (SMI_ISOLATION_NO_PHANTOM     |
                                SMI_TRANSACTION_NORMAL       |
                                SMI_TRANSACTION_REPL_DEFAULT |
                                SMI_COMMIT_WRITE_NOWAIT))
                 != IDE_SUCCESS);
        sStage = 2;

        IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
                 != IDE_SUCCESS);
        sStage = 3;

        IDE_TEST( executeDDL( &sStatement,
                              sCrtSynonymSql[i] )
                  != IDE_SUCCESS );
        sStage = 2;

        if (sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS)
        {
            sStage = 1;
            idlOS::abort();
        }

        IDE_TEST(aTrans->commit() != IDE_SUCCESS);
    }
#endif
    
    /* PROJ-2207 Password policy support */
    for (i = 0;  sCrtMetaViewSql[i] != NULL; i++)
    {
        IDE_TEST(aTrans->begin(&sDummySmiStmt,
                               aStatistics,
                               (SMI_ISOLATION_NO_PHANTOM     |
                                SMI_TRANSACTION_NORMAL       |
                                SMI_TRANSACTION_REPL_DEFAULT |
                                SMI_COMMIT_WRITE_NOWAIT))
                 != IDE_SUCCESS);
        sStage = 2;

        IDE_TEST(sSmiStmt.begin( aStatistics, sDummySmiStmt, sSmiStmtFlag )
                 != IDE_SUCCESS);
        sStage = 3;

        IDE_TEST( executeDDL( &sStatement,
                              sCrtMetaViewSql[i] )
                  != IDE_SUCCESS );
        sStage = 2;

        if (sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS)
        {
            sStage = 1;
            idlOS::abort();
        }

        IDE_TEST(aTrans->commit() != IDE_SUCCESS);
    }
    
    // free the members of qcStatement
    sStage = 0;

    IDE_TEST( qcg::freeStatement( &sStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( QCM_ERR_DNBufferOverflow );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_DNBufferOverflow));
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 3:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);

        case 2:
            (void)aTrans->rollback();

        case 1:
            // free the members of qcStatement
            (void) qcg::freeStatement( &sStatement );
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::executeDDL(
    qcStatement * aStatement,
    SChar       * aText )
{
    SChar *         sText = NULL;

    aStatement->myPlan->stmtTextLen = idlOS::strlen(aText);

    IDU_LIMITPOINT("qcmCreate::executeDDL::malloc");
    // BUG-39521 Supporting stored package
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc(
                  idlOS::align4(aStatement->myPlan->stmtTextLen + 1),
                  (void**)&sText)
              != IDE_SUCCESS );

    idlOS::strncpy(sText, aText, aStatement->myPlan->stmtTextLen );
    sText[aStatement->myPlan->stmtTextLen] = '\0';
    aStatement->myPlan->stmtText = sText;

    aStatement->myPlan->parseTree   = NULL;
    aStatement->myPlan->plan        = NULL;

    // parsing
    IDE_TEST(qcpManager::parseIt(aStatement) != IDE_SUCCESS);

    IDE_TEST(aStatement->myPlan->parseTree->parse(aStatement)  != IDE_SUCCESS);
    IDE_TEST(qtc::fixAfterParsing( QC_SHARED_TMPLATE(aStatement))
             != IDE_SUCCESS);

    // validation
    IDE_TEST(aStatement->myPlan->parseTree->validate(aStatement)  != IDE_SUCCESS);

    IDE_TEST(qtc::fixAfterValidation( QC_QMP_MEM(aStatement),
                                      QC_SHARED_TMPLATE(aStatement))
             != IDE_SUCCESS);

    // optimizaing
    IDE_TEST(aStatement->myPlan->parseTree->optimize(aStatement)  != IDE_SUCCESS);

    //IDE_TEST(qtc::fixAfterOptimization( aStatement )
    //         != IDE_SUCCESS);

    IDE_TEST(qcg::setPrivateArea(aStatement) != IDE_SUCCESS);

    IDE_TEST(qcg::stepAfterPVO(aStatement) != IDE_SUCCESS);
    
    // execution
    IDE_TEST(aStatement->myPlan->parseTree->execute(aStatement) != IDE_SUCCESS);

    // set success
    QC_PRIVATE_TMPLATE(aStatement)->flag &= ~QC_TMP_EXECUTION_MASK;
    QC_PRIVATE_TMPLATE(aStatement)->flag |= QC_TMP_EXECUTION_SUCCESS;

    qcg::clearStatement(aStatement,
                        ID_FALSE ); /* aRebuild = ID_FALSE */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmCreate::upgradeMeta( idvSQL * aStatistics,
                               SInt     aMinorVer )
{
    UInt            i;
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement  * sDummySmiStmt = NULL;
    qcStatement     sStatement;
    UInt            sStage = 0;

    iduMemory       sIduMem;
    UInt            sSmiStmtFlag  = 0;

    SChar * sUpgradeVer1ToVer2[] = {
        (SChar*) "DROP TABLE SYS_DATABASE_"
            ,
        (SChar*) "CREATE TABLE SYS_DATABASE_ ( "
            "DB_NAME VARCHAR(40) FIXED, "
            "OWNER_DN VARCHAR(2048) FIXED, "
            "META_MAJOR_VER INTEGER, "
            "META_MINOR_VER INTEGER, "
            "META_PATCH_VER INTEGER,"
            "PREV_META_MAJOR_VER INTEGER, "
            "PREV_META_MINOR_VER INTEGER, "
            "PREV_META_PATCH_VER INTEGER )"
            ,
        (SChar*) "INSERT INTO SYS_DATABASE_ VALUES ( '%s','%s', "
            QCM_META_MAJOR_STR_VER", "
            QCM_META_MINOR_STR_VER", "
            QCM_META_PATCH_STR_VER", "
            QCM_META_MAJOR_STR_VER", "
            "%"ID_INT32_FMT", "
            QCM_META_PATCH_STR_VER" )"
            ,
        (SChar*) "ALTER TABLE SYS_REPL_OLD_ITEMS_ ADD COLUMN ( "
            "REMOTE_USER_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED DEFAULT NULL, "
            "REMOTE_TABLE_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED DEFAULT NULL, "
            "REMOTE_PARTITION_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED DEFAULT NULL, "
            "PARTITION_ORDER       INTEGER DEFAULT 0, "
            "PARTITION_MIN_VALUE   VARCHAR(4000) VARIABLE DEFAULT NULL, "
            "PARTITION_MAX_VALUE   VARCHAR(4000) VARIABLE DEFAULT NULL, "
            "INVALID_MAX_SN BIGINT DEFAULT -1 )"
            ,
        (SChar*) "ALTER TABLE SYS_REPLICATIONS_ "
            "ADD COLUMN ( REMOTE_LAST_DDL_XSN BIGINT DEFAULT -1 )"
            ,
        (SChar*) "UPDATE SYSTEM_.SYS_REPL_OLD_ITEMS_ SET "
            "( REMOTE_USER_NAME, REMOTE_TABLE_NAME, REMOTE_PARTITION_NAME, INVALID_MAX_SN ) = "
            "( SELECT REMOTE_USER_NAME, REMOTE_TABLE_NAME, REMOTE_PARTITION_NAME, INVALID_MAX_SN "
            "FROM SYSTEM_.SYS_REPL_ITEMS_ WHERE "
            "SYSTEM_.SYS_REPL_ITEMS_.REPLICATION_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.REPLICATION_NAME AND "
            "SYSTEM_.SYS_REPL_ITEMS_.LOCAL_USER_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.USER_NAME AND "
            "SYSTEM_.SYS_REPL_ITEMS_.LOCAL_TABLE_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.TABLE_NAME AND "
            "( SYSTEM_.SYS_REPL_ITEMS_.LOCAL_PARTITION_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.PARTITION_NAME OR "
            "( SYSTEM_.SYS_REPL_ITEMS_.LOCAL_PARTITION_NAME is NULL AND "
            "SYSTEM_.SYS_REPL_OLD_ITEMS_.PARTITION_NAME is NULL ) ) )"
            ,
        (SChar*) "UPDATE SYSTEM_.SYS_REPL_OLD_ITEMS_ SET "
            "( PARTITION_ORDER, PARTITION_MIN_VALUE, PARTITION_MAX_VALUE ) = "
            "( SELECT PARTITION_ORDER, PARTITION_MIN_VALUE, PARTITION_MAX_VALUE "
            "FROM SYSTEM_.SYS_TABLE_PARTITIONS_, SYSTEM_.SYS_TABLES_, SYSTEM_.SYS_USERS_ WHERE "
            "( SYSTEM_.SYS_REPL_OLD_ITEMS_.PARTITION_NAME = SYSTEM_.SYS_TABLE_PARTITIONS_.PARTITION_NAME ) AND "
            "( SYSTEM_.SYS_TABLES_.TABLE_ID = SYSTEM_.SYS_TABLE_PARTITIONS_.TABLE_ID ) AND "
            "( SYSTEM_.SYS_TABLES_.TABLE_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.TABLE_NAME ) AND"
            "( SYSTEM_.SYS_TABLE_PARTITIONS_.USER_ID = SYSTEM_.SYS_USERS_.USER_ID ) AND "
            "( SYSTEM_.SYS_REPL_OLD_ITEMS_.USER_NAME = SYSTEM_.SYS_USERS_.USER_NAME ) )"
            ,
        (SChar*) "ALTER TABLE SYS_TABLES_ "
            "ADD COLUMN ( USABLE CHAR(1) NOT NULL, SHARD_FLAG INTEGER NOT NULL )"
            ,
        (SChar*) "ALTER TABLE SYS_TABLE_PARTITIONS_ "
            "ADD COLUMN ( PARTITION_USABLE CHAR(1) NOT NULL )"
            ,
        NULL    };

    //-----------------------------------------------------------------
    // 업그레이드시,
    //
    // (1) 아래와 같이 필요한 sql문들의 배열을 생성
    //
    //    예)
    //    SChar * sUpgradeVer5ToVer6[] = {
    //     (SChar*) "DROP TABLE SYS_DATABASE_"
    //     ,
    //     (SChar*) "CREATE TABLE SYS_DATABASE_ (
    //               DB_NAME VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED,
    //               OWNER_DN VARCHAR(2048) FIXED,
    //               META_MAJOR_VER INTEGER,
    //               META_MINOR_VER INTEGER,
    //               META_PATCH_VER INTEGER,
    //               PREV_META_MAJOR_VER INTEGER,
    //               PREV_META_MINOR_VER INTEGER,
    //               PREV_META_PATCH_VER INTEGER )"
    //     ,
    //     (SChar*) "INSERT INTO SYS_DATABASE_ VALUES ( NULL,
    //               '%s',
    //               "QCM_META_MAJOR_STR_VER",
    //               "QCM_META_MINOR_STR_VER",
    //               "QCM_META_PATCH_STR_VER",
    //               "QCM_META_MAJOR_STR_VER", 
    //               "%"ID_INT32_FMT", "
    //               "QCM_META_PATCH_STR_VER" )"
    //     ,
    //     /* The last item should be NULL */
    //     NULL    };
    //
    // (2) 밑 부분 switch case 에서,
    //     upgrade하려는 version에서 (1)의 sql문 실행.
    //
    //    switch ( aMinorVer )
    //    {
    //       // have NO break statement
    //     case 0: // upgrate from minor version 0 to minor version 1
    //     case 1:
    //     case 2:
    //     case 3:
    //     case 4:
    //         // 버전 4까지 지원불가.
    //         IDE_RAISE( err_upgrade_meta_fail );
    //     case 5:
    //         for (i = 0; sUpgradeVer5ToVer6[i] != NULL; i++)
    //         {
    //             IDE_TEST( sSmiStmt.begin(sDummySmiStmt, sSmiStmtFlag)
    //                 != IDE_SUCCESS );
    //             sStage = 3;
    //
    //             /* runDDLForMETA 의 sInsDefaultValuesSql 참조
    //              * especially treat when i equals 2,
    //              * for formatted string of SYS_DATABASE_
    //              */
    //             if( i == 2 )
    //             {
    //                 SChar sInsSql[4096];
    //                 SChar sOwnerDN[4096];
    //                 
    //                 /* 바로 2번 SQL이
    //                  *INSERT INTO SYS_DATABASE_ VALUES ( '%s','%s',
    //                  *  QCM_META_MAJOR_STR_VER", " 
    //                  *  QCM_META_MINOR_STR_VER", "
    //                  *  QCM_META_PATCH_STR_VER", "
    //                  *  QCM_META_MAJOR_STR_VER", " 
    //                  *  "%"ID_INT32_FMT", "
    //                  *  QCM_META_PATCH_STR_VER" )"
    //                  * 이다. 즉 DB의 META 버전을 업그레이드 시켜야 하는데
    //                  * SYS_DATABASE_의 OwnerDN과 DBName을 알 수 없다.
    //                  * (ParseTree가 구축되지 않는 metaUpgrade의 특성상
    //                  *  Update로 위 데이터만 바꾸는 것이 불가능하다 )
    //                  *  따라서 따로 데이터를 가져와서 Sql을 수행한다.) */
    //                  * //PROJ-2689 
    //                  *  메타 업그레이드 될 경우, 이전 메타 버전을 PREV_로
    //                  *  저장해야한다.  */
    //
    //                 // OwnerDN 획득
    //                 IDE_TEST( qcmDatabase::checkDatabase( &sSmiStmt,
    //                                                       (SChar*)sOwnerDN,
    //                                                       ID_SIZEOF(sOwnerDN) )
    //                           != IDE_SUCCESS );
    //                 
    //                 // DBName획득, 업데이트 이전의 메타버전 저장
    //                 idlOS::snprintf( sInsSql, 
    //                                  ID_SIZEOF(sInsSql), 
    //                                  sUpgradeVer5ToVer6[i], 
    //                                  smiGetDBName(), sOwnerDN, aMinorVer );
    //                 
    //                 IDE_TEST( executeDDL( &sStatement,
    //                                       sInsSql )
    //                           != IDE_SUCCESS );
    //             }
    //             else
    //             {
    //                IDE_TEST( executeDDL( &sStatement,
    //                                      sUpgradeVer5ToVer6[i] )
    //                    != IDE_SUCCESS );
    //             }
    //             sStage = 2;
    //
    //             IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS)
    //                 != IDE_SUCCESS );
    //             sStage = 1;
    //             ideLog::log(IDE_QP_0, "[QCM_META_UPGRADE] UpgradeMeta %s\n",
    //                                    sUpgradeVer5ToVer6[i]);
    //         }
    //     case 6:
    //         // nothing to do.
    //     default:
    //         break;
    //    }
    //-----------------------------------------------------------------

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    sIduMem.init(IDU_MEM_QCM);
    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    sStage = 1;

    ideLog::log(IDE_QP_0,"[QCM_META_UPGRADE] ALLOC STATEMENT SUCCESS\n");

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_ALTER_META_MASK;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_ALTER_META_ENABLE;

    ideLog::log(IDE_QP_0,"[QCM_META_UPGRADE] SET STATEMENT SUCCESS\n");

    // transaction begin
    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            aStatistics,
                            (SMI_ISOLATION_NO_PHANTOM     |
                             SMI_TRANSACTION_NORMAL       |
                             SMI_TRANSACTION_REPL_DEFAULT |
                             SMI_COMMIT_WRITE_NOWAIT))
              != IDE_SUCCESS );
    sStage = 2;

    ideLog::log(IDE_QP_0,"[QCM_META_UPGRADE] TRANSACTION BEGIN SUCCESS\n");

    switch ( aMinorVer )
    {
        // have NO break statement
        case 0:
            // upgrade from minor version 0 to minor version 1
            /* fall through */
        case 1:
            // upgrade from minor version 1 to minor version 2
            for (i = 0; sUpgradeVer1ToVer2[i] != NULL; i++)
            {
                IDE_TEST( sSmiStmt.begin(aStatistics, 
                                         sDummySmiStmt, 
                                         sSmiStmtFlag)
                          != IDE_SUCCESS );
                sStage = 3;

                /* runDDLForMETA 의 sInsDefaultValuesSql 참조
                 * especially treat when i equals 2,
                 * for formatted string of SYS_DATABASE_
                 */
                if( i == 2 )
                {
                    SChar sInsSql[4096];
                    SChar sOwnerDN[4096];

                    // OwnerDN 획득
                    IDE_TEST( qcmDatabase::checkDatabase( &sSmiStmt,
                                                          (SChar*)sOwnerDN,
                                                          ID_SIZEOF(sOwnerDN) )
                              != IDE_SUCCESS );

                    // DBName획득, 업데이트 이전의 메타버전 저장
                    idlOS::snprintf( sInsSql, 
                                     ID_SIZEOF(sInsSql), 
                                     sUpgradeVer1ToVer2[i], 
                                     smiGetDBName(), sOwnerDN, aMinorVer );

                    IDE_TEST( executeDDL( &sStatement,
                                          sInsSql )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( executeDDL( &sStatement,
                                          sUpgradeVer1ToVer2[i] )
                              != IDE_SUCCESS );
                }
                sStage = 2;

                IDE_TEST( sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS)
                          != IDE_SUCCESS );
                sStage = 1;
                ideLog::log(IDE_QP_0, "[QCM_META_UPGRADE] UpgradeMeta %s\n",
                            sUpgradeVer1ToVer2[i]);
            }
            /* fall through */
        default:
            break;
    }

    // transaction commit
    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    ideLog::log(IDE_QP_0,"[QCM_META_UPGRADE] TRANSACTION COMMIT SUCCESS\n");

    // free the members of qcStatement
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sTrans.destroy( aStatistics ) != IDE_SUCCESS );

    sIduMem.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 3:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 2:
            (void)sTrans.rollback();

            ideLog::log(IDE_QP_0,"[QCM_META_UPGRADE] TRANSACTION ROLLBACK\n");
        case 1:
            (void) qcg::freeStatement(&sStatement);
            (void) sTrans.destroy( aStatistics );
    }

    sIduMem.destroy();
    return IDE_FAILURE;
}

/*
    Meta Table의 SM Flag를 초기화 한다.
 */
IDE_RC qcmCreate::initMetaTableFlag( smiStatement     * aSmiStmt )
{

    IDE_TEST( turnOffMetaTableLogFlushFlag( aSmiStmt ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    

/*
   [ Meta Table의 Log Flush Flag를 끈다. ]

   => Create Database도중에만 불리운다.

   ** TASK-2401 Disk/Memory 테이블의 Log분리 **
   Meta Table에 대해 Log Flush여부 결정

   LFG=2로 설정된 경우 Hybrid Transaction이 Commit할 경우
   Dependent한 LFG에 대해 Flush를 하여야 한다.

   이때 Disk Table의 Validation과정에서 Meta Table을 접근하게 되어
   항상 Hybrid Transaction으로 분류되는 문제를 해결하기 위한 Flag이다.
   
   => 문제점 :
      Disk Table에 대한 DML시 Validation과정에서
      Table의 Meta Table을 참조하게 된다
      그런데 Meta Table은 Memory Table이기 때문에,
      Memory Table(Meta)을 읽고 Disk Table에 DML을 하게 되면
      Hybrid Transaction으로 분류가 되어
      Memory 로그가 항상 Flush되는 문제가 발생한다.
      
      => 해결책 :
        Meta Table을 읽는 경우 Hybrid Transaction으로 분류하지 않는 대신
        Meta Table의 변경시에 Memory Log를 Flush하도록 한다.
      
   => 또다른 문제점 :
      Meta Table중 Replication의 XSN과 같이 DML과 관계없이 빈번하게
      수정되는 Meta Table의 경우 매번 Log를 Flush할 경우 성능저하발생
      
      => 해결책 :
         Meta Table마다 변경이 되었을 때 Log를 Flush할지 여부를
         Flag로 둔다.
         이 Flag가 켜져 있는 경우에만 Meta Table변경한 Transaction의
         Commit시에 로그를 Flush하도록 한다.
 */
IDE_RC qcmCreate::turnOffMetaTableLogFlushFlag( smiStatement * aSmiStmt )
{
    // Replication
    SChar * sTableName4NoFlush[] = {
#if !defined(SMALL_FOOTPRINT)
        (SChar*)"SYS_REPLICATIONS_",
#endif
        NULL
    };
    

    UInt    i;
    SChar * sTableName;
    smSCN   sTableSCN;
    void  * sTableHandle;

    for (i=0; sTableName4NoFlush[i] != NULL; i++ )
    {
        sTableName = sTableName4NoFlush[i];

        // Table의 이름으로부터 Table Handle을 알아낸다
        IDE_TEST( qcm::getTableHandleByName( aSmiStmt,
                                             QC_SYSTEM_USER_ID,
                                             (UChar*) sTableName,
                                             idlOS::strlen(sTableName),
                                             & sTableHandle,
                                             & sTableSCN )
                  != IDE_SUCCESS );

        // Meta Table의 Log Flush Flag를 끈다.
        IDE_TEST(smiTable::alterTableFlag(
                     aSmiStmt,
                     sTableHandle,
                     SMI_TABLE_META_LOG_FLUSH_MASK,
                     SMI_TABLE_META_LOG_FLUSH_FALSE )
                 != IDE_SUCCESS);
        
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-45571 Upgrade meta for authid clause of PSM
IDE_RC qcmCreate::createBuiltInPSM( idvSQL       * aStatistics,
                                    qcStatement  * aStatement,
                                    smiStatement * aSmiStmt,
                                    smiStatement * aDummySmiStmt )
{
/* procedure creation */
    SChar   * sCrtProcSql[] = {
        (SChar*) "CREATE OR REPLACE PROCEDURE print( message IN VARCHAR(65534) ) AUTHID CURRENT_USER \
AS                                                                      \
   v1 VARCHAR(65534);                                                   \
BEGIN                                                                   \
   v1 := print_out( message );                                          \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE println( message IN VARCHAR(65533) ) AUTHID CURRENT_USER \
AS                                                                        \
   v1 VARCHAR(65534);                                                     \
BEGIN                                                                     \
   v1 := print_out( message || CHR(10) );                                 \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE sleep( seconds IN INTEGER ) AUTHID CURRENT_USER \
AS                                                                        \
   v1 INTEGER;                                                            \
BEGIN                                                                     \
   v1 := sp_sleep( seconds );                                             \
END"
        ,
// PROJ-1371 PSM file handling
        (SChar*) "CREATE OR REPLACE PROCEDURE FCLOSE( file IN OUT FILE_TYPE ) AUTHID CURRENT_USER \
AS                                                                    \
BEGIN                                                                 \
file := FILE_CLOSE( file );                                           \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE FCOPY( location IN VARCHAR(40),                \
                                             filename IN VARCHAR(256),               \
                                             dest_dir IN VARCHAR(40),                \
                                             dest_file IN VARCHAR(256),              \
                                             start_line IN INTEGER DEFAULT 1,        \
                                             end_line IN INTEGER DEFAULT NULL ) AUTHID CURRENT_USER \
AS                                                                                   \
dummy BOOLEAN;                                                                       \
BEGIN                                                                                \
dummy := FILE_COPY( location, filename, dest_dir, dest_file, start_line, end_line ); \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE FFLUSH( file IN FILE_TYPE ) AUTHID CURRENT_USER \
AS                                                                \
dummy BOOLEAN;                                                    \
BEGIN                                                             \
dummy := FILE_FLUSH( file );                                      \
END"
        ,
        (SChar*) "CREATE OR REPLACE FUNCTION FOPEN( location IN VARCHAR(40),  \
                                            filename IN VARCHAR(256), \
                                            open_mode IN VARCHAR(4) ) \
RETURN FILE_TYPE AUTHID CURRENT_USER                                  \
AS                                                                    \
file FILE_TYPE;                                                       \
BEGIN                                                                 \
file := FILE_OPEN( location, filename, open_mode );                   \
RETURN file;                                                          \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE FREMOVE( location IN VARCHAR(40),   \
                                               filename IN VARCHAR(256) ) AUTHID CURRENT_USER \
AS                                                                        \
dummy BOOLEAN;                                                            \
BEGIN                                                                     \
dummy := FILE_REMOVE( location, filename );                               \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE FRENAME( location IN VARCHAR(40),             \
                                               filename IN VARCHAR(256),            \
                                               dest_dir IN VARCHAR(40),             \
                                               dest_file IN VARCHAR(256),           \
                                               overwrite IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER \
AS                                                                                  \
dummy BOOLEAN;                                                                      \
BEGIN                                                                               \
dummy := FILE_RENAME( location, filename, dest_dir, dest_file, overwrite );         \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE GET_LINE( file IN FILE_TYPE,            \
                                                buffer OUT VARCHAR(32768),    \
                                                len IN INTEGER DEFAULT NULL ) AUTHID CURRENT_USER \
AS                                                                            \
BEGIN                                                                         \
buffer := FILE_GETLINE( file, len );                                          \
END"
        ,
        (SChar*) "CREATE OR REPLACE FUNCTION IS_OPEN( file IN FILE_TYPE ) \
RETURN BOOLEAN AUTHID CURRENT_USER                                \
AS                                                                \
BEGIN                                                             \
IF file IS NULL THEN                                              \
  RETURN FALSE;                                                   \
ELSE                                                              \
  RETURN TRUE;                                                    \
END IF;                                                           \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE NEW_LINE( file IN FILE_TYPE,           \
                                                lines IN INTEGER DEFAULT 1 ) AUTHID CURRENT_USER  \
AS                                                                           \
v1 INTEGER;                                                                  \
dummy BOOLEAN;                                                               \
BEGIN                                                                        \
FOR v1 IN 1 .. lines LOOP                                                    \
  dummy := FILE_PUT( file, CHR(10), FALSE );                                 \
END LOOP;                                                                    \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE PUT( file IN FILE_TYPE,         \
                                           buffer IN VARCHAR(32768) ) AUTHID CURRENT_USER  \
AS                                                                    \
dummy BOOLEAN;                                                        \
BEGIN                                                                 \
dummy := FILE_PUT( file, buffer, FALSE );                             \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE PUT_LINE( file IN FILE_TYPE,                   \
                                                buffer IN VARCHAR(32767),            \
                                                autoflush IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER  \
AS                                                                                   \
dummy BOOLEAN;                                                                       \
BEGIN                                                                                \
dummy := FILE_PUT( file, buffer||CHR(10), autoflush );                               \
END"
        ,
        (SChar*) "CREATE OR REPLACE PROCEDURE FCLOSE_ALL AUTHID CURRENT_USER  \
AS                                               \
dummy BOOLEAN;                                   \
BEGIN                                            \
dummy := FILE_CLOSEALL(TRUE);                    \
END"
        ,
/* PROJ-1335 RAISE_APPLICATION_ERROR */
        (SChar*) "CREATE OR REPLACE PROCEDURE RAISE_APPLICATION_ERROR(       \
                                            errcode IN INTEGER,      \
                                            errmsg  IN VARCHAR(2047) ) AUTHID CURRENT_USER  \
AS                                                                   \
dummy BOOLEAN;                                                       \
BEGIN                                                                \
dummy := RAISE_APP_ERR(errcode, errmsg);                             \
END"
        ,
/* BUG-25999 */
(SChar *)"CREATE OR REPLACE PROCEDURE REMOVE_XID( NAME IN VARCHAR(256) ) AUTHID CURRENT_USER \
AS                                                                        \
    V1 INTEGER;                                                           \
BEGIN                                                                     \
    V1 := SP_REMOVE_XID( NAME );                                          \
END"
,
/* TASK-4990 */
/* BUG-34664 Object name must be case-insensitive */
(SChar *)"CREATE OR REPLACE PROCEDURE GATHER_SYSTEM_STATS() AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"BEGIN"
"    V1 := SP_GATHER_SYSTEM_STATS();"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE GATHER_DATABASE_STATS("
"                              ESTIMATE_PERCENT    IN FLOAT   DEFAULT 0,"
"                              DEGREE              IN INTEGER DEFAULT 0,"
"                              GATHER_SYSTEM_STATS IN BOOLEAN DEFAULT FALSE,"
"                              NO_INVALIDATE       IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    V2 VARCHAR(65534);"
"BEGIN"
"    DECLARE"
"        CURSOR C1 IS SELECT U.USER_NAME AS OWNER,T.TABLE_NAME"
"            FROM SYSTEM_.DBA_USERS_ AS U, SYSTEM_.SYS_TABLES_ AS T"
"                WHERE  U.USER_ID = T.USER_ID"
"                    AND TABLE_TYPE = 'T';"
"        TAB_REC C1%ROWTYPE;"
"    BEGIN"
"        IF GATHER_SYSTEM_STATS = TRUE THEN"
"            V1 := SP_GATHER_SYSTEM_STATS();"
"        END IF;"
"        OPEN C1;"
"            LOOP"
"                FETCH C1 INTO TAB_REC;"
"                EXIT WHEN C1%NOTFOUND;"
"                V2 := PRINT_OUT( TAB_REC.OWNER ||'.'||"
"                                 TAB_REC.TABLE_NAME || CHR(10) );"
"                V1 := SP_GATHER_TABLE_STATS("
"                                TAB_REC.OWNER, TAB_REC.TABLE_NAME, NULL,"
"                                ESTIMATE_PERCENT, DEGREE, NO_INVALIDATE );"
"            END LOOP;"
"        CLOSE C1;"
"    END;"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE GATHER_TABLE_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              ESTIMATE_PERCENT IN FLOAT   DEFAULT 0,"
"                              DEGREE           IN INTEGER DEFAULT 0,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_GATHER_TABLE_STATS( UP_OWNER, UP_TABLENAME, UP_PARTITIONNAME,"
"                                 ESTIMATE_PERCENT, DEGREE, NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE GATHER_INDEX_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              IDXNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              ESTIMATE_PERCENT IN FLOAT   DEFAULT 0,"
"                              DEGREE           IN INTEGER DEFAULT 0,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_IDXNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( IDXNAME, CHR(34) ) = 0 THEN"
"        UP_IDXNAME := UPPER( IDXNAME );"
"    ELSE"
"        UP_IDXNAME := REPLACE2( IDXNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_GATHER_INDEX_STATS( UP_OWNER, UP_IDXNAME,"
"                                 ESTIMATE_PERCENT, DEGREE, NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE SET_SYSTEM_STATS("
"                              STATNAME            IN VARCHAR(100),"
"                              STATVALUE           IN DOUBLE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"BEGIN"
"    V1 := SP_SET_SYSTEM_STATS( STATNAME, STATVALUE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE SET_TABLE_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              NUMROW           IN BIGINT  DEFAULT NULL,"
"                              NUMPAGE          IN BIGINT  DEFAULT NULL,"
"                              AVGRLEN          IN BIGINT  DEFAULT NULL,"
"                              ONEROWREADTIME   IN DOUBLE  DEFAULT NULL,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_SET_TABLE_STATS( UP_OWNER, UP_TABLENAME, UP_PARTITIONNAME,"
"                              NUMROW,NUMPAGE,AVGRLEN, ONEROWREADTIME,"
"                              NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE SET_COLUMN_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              COLNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              NUMDIST          IN BIGINT  DEFAULT NULL,"
"                              NUMNULL          IN BIGINT  DEFAULT NULL,"
"                              AVGCLEN          IN BIGINT  DEFAULT NULL,"
"                              MINVALUE         IN VARCHAR("QCM_MAX_MINMAX_VALUE_LEN") DEFAULT NULL,"
"                              MAXVALUE         IN VARCHAR("QCM_MAX_MINMAX_VALUE_LEN") DEFAULT NULL,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_COLNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( COLNAME, CHR(34) ) = 0 THEN"
"        UP_COLNAME := UPPER( COLNAME );"
"    ELSE"
"        UP_COLNAME := REPLACE2( COLNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_SET_COLUMN_STATS( UP_OWNER, UP_TABLENAME, UP_COLNAME,"
"                               UP_PARTITIONNAME, NUMDIST,NUMNULL,AVGCLEN,"
"                               MINVALUE, MAXVALUE,"
"                               NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE SET_INDEX_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              IDXNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              KEYCOUNT         IN BIGINT  DEFAULT NULL,"
"                              NUMPAGE          IN BIGINT  DEFAULT NULL,"
"                              NUMDIST          IN BIGINT  DEFAULT NULL,"
"                              CLUSTERINGFACTOR IN BIGINT  DEFAULT NULL,"
"                              INDEXHEIGHT      IN BIGINT  DEFAULT NULL,"
"                              AVGSLOTCNT       IN BIGINT  DEFAULT NULL,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_IDXNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( IDXNAME, CHR(34) ) = 0 THEN"
"        UP_IDXNAME := UPPER( IDXNAME );"
"    ELSE"
"        UP_IDXNAME := REPLACE2( IDXNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_SET_INDEX_STATS( UP_OWNER, UP_IDXNAME,"
"                              KEYCOUNT, NUMPAGE, NUMDIST, CLUSTERINGFACTOR,"
"                              INDEXHEIGHT, AVGSLOTCNT, NO_INVALIDATE );"
"END"
,
/* BUG-38236 Interfaces for removing statistics information */
(SChar *)"CREATE OR REPLACE PROCEDURE DELETE_SYSTEM_STATS() AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"BEGIN"
"    V1 := SP_DELETE_SYSTEM_STATS();"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE DELETE_TABLE_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              CASCADE_PART     IN BOOLEAN DEFAULT TRUE,"
"                              CASCADE_COLUMNS  IN BOOLEAN DEFAULT TRUE,"
"                              CASCADE_INDEXES  IN BOOLEAN DEFAULT TRUE,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_DELETE_TABLE_STATS( UP_OWNER, UP_TABLENAME, UP_PARTITIONNAME,"
"                                 CASCADE_PART, CASCADE_COLUMNS,"
"                                 CASCADE_INDEXES, NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE DELETE_COLUMN_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              COLNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              CASCADE_PART     IN BOOLEAN DEFAULT TRUE,"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_COLNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( COLNAME, CHR(34) ) = 0 THEN"
"        UP_COLNAME := UPPER( COLNAME );"
"    ELSE"
"        UP_COLNAME := REPLACE2( COLNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_DELETE_COLUMN_STATS( UP_OWNER, UP_TABLENAME, UP_COLNAME,"
"                                  UP_PARTITIONNAME, CASCADE_PART, NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE DELETE_INDEX_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              IDXNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              NO_INVALIDATE    IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_IDXNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( IDXNAME, CHR(34) ) = 0 THEN"
"        UP_IDXNAME := UPPER( IDXNAME );"
"    ELSE"
"        UP_IDXNAME := REPLACE2( IDXNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_DELETE_INDEX_STATS( UP_OWNER, UP_IDXNAME, NO_INVALIDATE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE DELETE_DATABASE_STATS("
"                              NO_INVALIDATE IN BOOLEAN DEFAULT FALSE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    V2 VARCHAR(65534);"
"BEGIN"
"    DECLARE"
"        CURSOR C1 IS SELECT U.USER_NAME AS OWNER,T.TABLE_NAME"
"            FROM SYSTEM_.DBA_USERS_ AS U, SYSTEM_.SYS_TABLES_ AS T"
"                WHERE  U.USER_ID = T.USER_ID"
"                    AND TABLE_TYPE = 'T';"
"        TAB_REC C1%ROWTYPE;"
"    BEGIN"
"        V1 := SP_DELETE_SYSTEM_STATS();"
"        OPEN C1;"
"            LOOP"
"                FETCH C1 INTO TAB_REC;"
"                EXIT WHEN C1%NOTFOUND;"
"                V2 := PRINT_OUT( TAB_REC.OWNER ||'.'||"
"                                 TAB_REC.TABLE_NAME || CHR(10) );"
"                V1 := SP_DELETE_TABLE_STATS("
"                                TAB_REC.OWNER, TAB_REC.TABLE_NAME, NULL,"
"                                TRUE, TRUE, TRUE, NO_INVALIDATE );"
"            END LOOP;"
"        CLOSE C1;"
"    END;"
"END"
,
/* BUG-40119 get statistics psm */
(SChar *)"CREATE OR REPLACE PROCEDURE GET_SYSTEM_STATS("
"                              STATNAME            IN VARCHAR(100),"
"                              STATVALUE           OUT DOUBLE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"BEGIN"
"    V1 := SP_GET_SYSTEM_STATS( STATNAME, STATVALUE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE GET_TABLE_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              NUMROW           OUT BIGINT,"
"                              NUMPAGE          OUT BIGINT,"
"                              AVGRLEN          OUT BIGINT,"
"                              CACHEDPAGE       OUT BIGINT,"
"                              ONEROWREADTIME   OUT DOUBLE ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_GET_TABLE_STATS( UP_OWNER, UP_TABLENAME, UP_PARTITIONNAME,"
"                              NUMROW,NUMPAGE,AVGRLEN,CACHEDPAGE,ONEROWREADTIME );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE GET_COLUMN_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              TABNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              COLNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              NUMDIST          OUT BIGINT,"
"                              NUMNULL          OUT BIGINT,"
"                              AVGCLEN          OUT BIGINT,"
"                              MINVALUE         OUT VARCHAR("QCM_MAX_MINMAX_VALUE_LEN"),"
"                              MAXVALUE         OUT VARCHAR("QCM_MAX_MINMAX_VALUE_LEN") ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_TABLENAME     VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_COLNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( TABNAME, CHR(34) ) = 0 THEN"
"        UP_TABLENAME := UPPER( TABNAME );"
"    ELSE"
"        UP_TABLENAME := REPLACE2( TABNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( COLNAME, CHR(34) ) = 0 THEN"
"        UP_COLNAME := UPPER( COLNAME );"
"    ELSE"
"        UP_COLNAME := REPLACE2( COLNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_GET_COLUMN_STATS( UP_OWNER, UP_TABLENAME, UP_COLNAME,"
"                               UP_PARTITIONNAME, NUMDIST, NUMNULL, AVGCLEN,"
"                               MINVALUE, MAXVALUE );"
"END"
,
(SChar *)"CREATE OR REPLACE PROCEDURE GET_INDEX_STATS("
"                              OWNNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              IDXNAME          IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                              PARTNAME         IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL,"
"                              KEYCOUNT         OUT BIGINT,"
"                              NUMPAGE          OUT BIGINT,"
"                              NUMDIST          OUT BIGINT,"
"                              CLUSTERINGFACTOR OUT BIGINT,"
"                              INDEXHEIGHT      OUT BIGINT,"
"                              CACHEDPAGE       OUT BIGINT,"
"                              AVGSLOTCNT       OUT BIGINT ) AUTHID CURRENT_USER "
"AS"
"    V1 INTEGER;"
"    UP_OWNER         VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_IDXNAME       VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    UP_PARTITIONNAME VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"BEGIN"
"    IF INSTR( OWNNAME, CHR(34) ) = 0 THEN"
"        UP_OWNER := UPPER( OWNNAME );"
"    ELSE"
"        UP_OWNER := REPLACE2( OWNNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( IDXNAME, CHR(34) ) = 0 THEN"
"        UP_IDXNAME := UPPER( IDXNAME );"
"    ELSE"
"        UP_IDXNAME := REPLACE2( IDXNAME, CHR(34) );"
"    END IF;"
"    IF INSTR( PARTNAME, CHR(34) ) = 0 THEN"
"        UP_PARTITIONNAME := UPPER( PARTNAME );"
"    ELSE"
"        UP_PARTITIONNAME := REPLACE2( PARTNAME, CHR(34) );"
"    END IF;"
"    V1 := SP_GET_INDEX_STATS( UP_OWNER, UP_IDXNAME, UP_PARTITIONNAME,"
"                              KEYCOUNT, NUMPAGE, NUMDIST, CLUSTERINGFACTOR,"
"                              INDEXHEIGHT, CACHEDPAGE, AVGSLOTCNT );"
"END"
,
/* PROJ-2211 Materialized View */
(SChar *)"CREATE OR REPLACE PROCEDURE REFRESH_MATERIALIZED_VIEW("
"                            owner_name IN VARCHAR("QCM_META_OBJECT_NAME_LEN"),"
"                            materialized_view_name IN VARCHAR("QCM_META_OBJECT_NAME_LEN") ) "
" AS"
"    mview_owner_name            VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    mview_table_name            VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    mview_view_name             VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"    mview_owner_id              INTEGER;"
"    mview_table_id              INTEGER;"
"    mview_view_id               INTEGER;"
"    existUser                   CHAR(1);"
"    existMView                  CHAR(1);"
"    isRoot                      CHAR(1);"
"    isOwner                     CHAR(1);"
"    hasAllPrivilege             CHAR(1);"
"    hasRefreshPrivilege         CHAR(1);"
"    hasSelectSystemPrivilege    CHAR(1);"
"    hasDeleteSystemPrivilege    CHAR(1);"
"    hasInsertSystemPrivilege    CHAR(1);"
"    hasSelectObjectPrivilege    CHAR(1);"
"    hasDeleteObjectPrivilege    CHAR(1);"
"    hasInsertObjectPrivilege    CHAR(1);"
"    dummy                       BOOLEAN;"
" BEGIN"
"    IF owner_name IS NULL THEN"
"        SELECT USER_NAME()"
"          INTO mview_owner_name"
"          FROM X$DUAL;"
"    ELSE"
"        IF INSTR( owner_name, CHR(34) ) = 0 THEN"
"            mview_owner_name := UPPER( owner_name );"
"        ELSE"
"            mview_owner_name := REPLACE2( owner_name, CHR(34) );"
"        END IF;"
"    END IF;"
""
"    IF materialized_view_name IS NULL THEN"
"        dummy := RAISE_APP_ERR( 990000, 'Refresh needs the materialized view name.' );"
"    ELSE"
"        IF INSTR( materialized_view_name, CHR(34) ) = 0 THEN"
"            mview_table_name := UPPER( materialized_view_name );"
"        ELSE"
"            mview_table_name := REPLACE2( materialized_view_name, CHR(34) );"
"        END IF;"
""
"        mview_view_name := mview_table_name || '"QDM_MVIEW_VIEW_SUFFIX_PROC"';"
"    END IF;"
""
"    SELECT CASE2( COUNT(*) = 1, 'Y', 'N' )"
"      INTO existUser"
"      FROM SYSTEM_.DBA_USERS_"
"     WHERE USER_NAME = mview_owner_name;"
""
"    IF existUser = 'N' THEN"
"        dummy := RAISE_APP_ERR( 990001, 'The owner does not exist.' );"
"    END IF;"
""
"    SELECT USER_ID"
"      INTO mview_owner_id"
"      FROM SYSTEM_.DBA_USERS_"
"     WHERE USER_NAME = mview_owner_name;"
""
"    SELECT CASE2( COUNT(*) = 1, 'Y', 'N' )"
"      INTO existMView"
"      FROM SYSTEM_.SYS_MATERIALIZED_VIEWS_"
"     WHERE MVIEW_NAME = mview_table_name AND USER_ID = mview_owner_id;"
""
"    IF existMView = 'N' THEN"
"        dummy := RAISE_APP_ERR( 990002, 'The materialized view does not exist.' );"
"    END IF;"
""
"    SELECT CASE2( COUNT(*) = 1, 'Y', 'N' )"
"      INTO existMView"
"      FROM SYSTEM_.SYS_TABLES_"
"     WHERE TABLE_NAME = mview_table_name AND USER_ID = mview_owner_id;"
""
"    IF existMView = 'N' THEN"
"        dummy := RAISE_APP_ERR( 990002, 'The materialized view does not exist.' );"
"    END IF;"
""
"    SELECT TABLE_ID"
"      INTO mview_table_id"
"      FROM SYSTEM_.SYS_TABLES_"
"     WHERE TABLE_NAME = mview_table_name AND USER_ID = mview_owner_id;"
""
"    SELECT CASE2( COUNT(*) = 1, 'Y', 'N' )"
"      INTO existMView"
"      FROM SYSTEM_.SYS_TABLES_"
"     WHERE TABLE_NAME = mview_view_name AND USER_ID = mview_owner_id;"
""
"    IF existMView = 'N' THEN"
"        dummy := RAISE_APP_ERR( 990002, 'The materialized view does not exist.' );"
"    END IF;"
""
"    SELECT TABLE_ID"
"      INTO mview_view_id"
"      FROM SYSTEM_.SYS_TABLES_"
"     WHERE TABLE_NAME = mview_view_name AND USER_ID = mview_owner_id;"
""
"    SELECT CASE2( USER_NAME() IN ( 'SYS', 'SYSTEM_' ),  'Y', 'N' ), CASE2( USER_NAME() = mview_owner_name, 'Y', 'N' )"
"      INTO isRoot, isOwner"
"      FROM X$DUAL;"
""
"    SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"      INTO hasAllPrivilege"
"      FROM SYSTEM_.SYS_GRANT_SYSTEM_"
"     WHERE GRANTEE_ID = USER_ID() AND"
"           PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 2 AND PRIV_NAME = 'ALL' );"
""
"    IF isRoot = 'N' AND isOwner = 'N' AND hasAllPrivilege = 'N' THEN"
"        SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"          INTO hasRefreshPrivilege"
"          FROM SYSTEM_.SYS_GRANT_SYSTEM_"
"         WHERE GRANTEE_ID = USER_ID() AND"
"               PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 2 AND PRIV_NAME = 'ALTER_ANY_MATERIALIZED_VIEW' );"
""
"        IF hasRefreshPrivilege = 'N' THEN"
"            dummy := RAISE_APP_ERR( 990003, 'The refresher needs the ALTER_ANY_MATERIALIZED_VIEW privilege.' );"
"        END IF;"
""
"        SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"          INTO hasSelectSystemPrivilege"
"          FROM SYSTEM_.SYS_GRANT_SYSTEM_"
"         WHERE GRANTEE_ID = USER_ID() AND"
"               PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 2 AND PRIV_NAME = 'SELECT_ANY_TABLE' );"
""
"        IF hasSelectSystemPrivilege = 'N' THEN"
"            SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"              INTO hasSelectObjectPrivilege"
"              FROM SYSTEM_.SYS_GRANT_OBJECT_"
"             WHERE OBJ_ID = mview_view_id AND"
"                   PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 1 AND PRIV_NAME = 'SELECT' ) AND"
"                   GRANTEE_ID = USER_ID() AND"
"                   USER_ID = mview_owner_id;"
""
"            IF hasSelectObjectPrivilege = 'N' THEN"
"                dummy := RAISE_APP_ERR( 990004, 'The refresher needs the SELECT_ANY_TABLE privilege.' );"
"            END IF;"
"        END IF;"
""
"        SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"          INTO hasDeleteSystemPrivilege"
"          FROM SYSTEM_.SYS_GRANT_SYSTEM_"
"         WHERE GRANTEE_ID = USER_ID() AND"
"               PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 2 AND PRIV_NAME = 'DELETE_ANY_TABLE' );"
""
"        IF hasDeleteSystemPrivilege = 'N' THEN"
"            SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"              INTO hasDeleteObjectPrivilege"
"              FROM SYSTEM_.SYS_GRANT_OBJECT_"
"             WHERE OBJ_ID = mview_table_id AND"
"                   PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 1 AND PRIV_NAME = 'DELETE' ) AND"
"                   GRANTEE_ID = USER_ID() AND"
"                   USER_ID = mview_owner_id;"
""
"            IF hasDeleteObjectPrivilege = 'N' THEN"
"                dummy := RAISE_APP_ERR( 990005, 'The refresher needs the DELETE_ANY_TABLE privileges.' );"
"            END IF;"
"        END IF;"
""
"        SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"          INTO hasInsertSystemPrivilege"
"          FROM SYSTEM_.SYS_GRANT_SYSTEM_"
"         WHERE GRANTEE_ID = USER_ID() AND"
"               PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 2 AND PRIV_NAME = 'INSERT_ANY_TABLE' );"
""
"        IF hasInsertSystemPrivilege = 'N' THEN"
"            SELECT CASE2( COUNT(*) > 0, 'Y', 'N' )"
"              INTO hasInsertObjectPrivilege"
"              FROM SYSTEM_.SYS_GRANT_OBJECT_"
"             WHERE OBJ_ID = mview_table_id AND"
"                   PRIV_ID = ( SELECT PRIV_ID FROM SYSTEM_.SYS_PRIVILEGES_ WHERE PRIV_TYPE = 1 AND PRIV_NAME = 'INSERT' ) AND"
"                   GRANTEE_ID = USER_ID() AND"
"                   USER_ID = mview_owner_id;"
""
"            IF hasInsertObjectPrivilege = 'N' THEN"
"                dummy := RAISE_APP_ERR( 990006, 'The refresher needs the INSERT_ANY_TABLE privileges.' );"
"            END IF;"
"        END IF;"
"    END IF;"
""
"    UPDATE SYSTEM_.SYS_MATERIALIZED_VIEWS_"
"       SET LAST_REFRESH_TIME = SYSDATE"
"     WHERE MVIEW_NAME = mview_table_name AND"
"           USER_ID = mview_owner_id;"
""
"    EXECUTE IMMEDIATE 'DELETE /*+ REFRESH_MVIEW NO_PLAN_CACHE */ FROM \"' || mview_owner_name || '\".\"' || mview_table_name || '\"';"
""
"    EXECUTE IMMEDIATE 'INSERT /*+ REFRESH_MVIEW NO_PLAN_CACHE */ INTO \"' || mview_owner_name || '\".\"' || mview_table_name || '\" SELECT * FROM \"' || mview_owner_name || '\".\"' || mview_view_name || '\"';"
" END"
,
        /* PROJ-1832 New database link */
        (SChar *)"CREATE OR REPLACE PROCEDURE REMOTE_EXECUTE_IMMEDIATE(      \
    DBLINK_NAME IN VARCHAR("QCM_META_NAME_LEN"), QUERY IN VARCHAR(32000) ) AUTHID CURRENT_USER \
AS                                                                           \
    V1 INTEGER;                                                              \
BEGIN                                                                        \
    V1 := REMOTE_EXECUTE_IMMEDIATE_INTERNAL( DBLINK_NAME, QUERY );           \
END"
,
        /* PROJ=2408 Memory dump function */
        (SChar*)"CREATE OR REPLACE PROCEDURE MEMORY_ALLOCATOR_DUMP("
"                    DUMPTARGET IN VARCHAR("QCM_META_OBJECT_NAME_LEN") DEFAULT NULL, "
"                    DUMPLEVEL  IN INTEGER     DEFAULT NULL) AUTHID CURRENT_USER "
"                AS"
"                    V1 VARCHAR("QCM_META_OBJECT_NAME_LEN");"
"                BEGIN"
"                    V1 := __MEMORY_ALLOCATOR_DUMP_INTERNAL(DUMPTARGET, DUMPLEVEL);"
"                END"
,
/* BUG-41452 Built-in functions for getting array binding info. */
(SChar*)"CREATE OR REPLACE FUNCTION IS_ARRAY_BOUND() "
" RETURN BOOLEAN AUTHID CURRENT_USER "
" AS "
" BEGIN "
" IF SP_IS_ARRAY_BOUND() = 1 THEN "
"   RETURN TRUE; "
" ELSE "
"   RETURN FALSE; "
" END IF; "
" END; "
,
(SChar*)"CREATE OR REPLACE FUNCTION IS_FIRST_ARRAY_BOUND() "
" RETURN BOOLEAN AUTHID CURRENT_USER "
" AS "
" BEGIN "
" IF SP_IS_FIRST_ARRAY_BOUND() = 1 THEN "
"   RETURN TRUE; "
" ELSE "
"   RETURN FALSE; "
" END IF; "
" END; "
,
(SChar*)"CREATE OR REPLACE FUNCTION IS_LAST_ARRAY_BOUND() "
" RETURN BOOLEAN AUTHID CURRENT_USER"
" AS "
" BEGIN "
" IF SP_IS_LAST_ARRAY_BOUND() = 1 THEN "
"   RETURN TRUE; "
" ELSE "
"   RETURN FALSE; "
" END IF; "
" END; "
,
(SChar *)" CREATE OR REPLACE PROCEDURE SET_CLIENT_INFO ( CLIENT_INFO IN VARCHAR(128) ) AUTHID CURRENT_USER "
"AS "
"  v1 INTEGER; "
"BEGIN "
"  v1 := SP_SET_CLIENT_INFO( CLIENT_INFO ); "
"END; "
,
(SChar *)" CREATE OR REPLACE PROCEDURE SET_MODULE ( MODULE IN VARCHAR(128), ACTION IN VARCHAR(128) ) AUTHID CURRENT_USER "
"AS "
"  v1 INTEGER; "
"BEGIN "
"  v1 := SP_SET_MODULE( MODULE, ACTION ); "
"END; "
,
(SChar *)" CREATE OR REPLACE PROCEDURE SET_PREVMETAVER ( MAJOR IN INTEGER, MINOR IN INTEGER, PATCH IN INTEGER ) "
"AS "
"  v1 BOOLEAN;"
"BEGIN "
"  v1 := SP_SET_PREVMETAVER( MAJOR, MINOR, PATCH );"
"END; "
,
/* The last item should be NULL */
        NULL    };

    SInt            i;
    UInt            sStage = 0;
    UInt            sSmiStmtFlag  = 0;

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;


#if !defined(SMALL_FOOTPRINT)
    for (i = 0; sCrtProcSql[i] != NULL; i++)
    {
        IDE_TEST(aSmiStmt->begin( aStatistics, aDummySmiStmt, sSmiStmtFlag )
                 != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST( executeDDL( aStatement,
                              sCrtProcSql[i] ) != IDE_SUCCESS );

        sStage = 0;
        IDE_TEST(aSmiStmt->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            (void)aSmiStmt->end(SMI_STATEMENT_RESULT_FAILURE);
        default:
            /* Nothing to do */
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmCreate::downgradeMeta( idvSQL * aStatistics,
                                 SInt     aPrevMinorVer )
{
    UInt            i;
    smiTrans        sTrans;
    smiStatement    sSmiStmt;
    smiStatement  * sDummySmiStmt = NULL;
    qcStatement     sStatement;
    UInt            sStage = 0;

    iduMemory       sIduMem;

    UInt            sSmiStmtFlag  = 0;

    SChar * sDowngradeVer2ToVer1[] = {
        (SChar*) "DROP TABLE SYS_DATABASE_"
            ,
        (SChar*) "CREATE TABLE SYS_DATABASE_ ( "
            "DB_NAME VARCHAR(40) FIXED, "
            "OWNER_DN VARCHAR(2048) FIXED, "
            "META_MAJOR_VER INTEGER, "
            "META_MINOR_VER INTEGER, "
            "META_PATCH_VER INTEGER,"
            "PREV_META_MAJOR_VER INTEGER, "
            "PREV_META_MINOR_VER INTEGER, "
            "PREV_META_PATCH_VER INTEGER )"
            ,
        (SChar*) "INSERT INTO SYS_DATABASE_ VALUES ( '%s', "
            "'%s',"
            QCM_META_MAJOR_STR_VER", "
            "%"ID_INT32_FMT", "
            QCM_META_PATCH_STR_VER","
            QCM_META_MAJOR_STR_VER", "
            "%"ID_INT32_FMT", "
            QCM_META_PATCH_STR_VER" )"
            ,
        (SChar*) "UPDATE SYSTEM_.SYS_REPL_ITEMS_ SET "
            "( INVALID_MAX_SN ) = "
            "( SELECT INVALID_MAX_SN "
            "FROM SYSTEM_.SYS_REPL_OLD_ITEMS_ WHERE "
            "SYSTEM_.SYS_REPL_ITEMS_.REPLICATION_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.REPLICATION_NAME AND "
            "SYSTEM_.SYS_REPL_ITEMS_.LOCAL_USER_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.USER_NAME AND "
            "SYSTEM_.SYS_REPL_ITEMS_.LOCAL_TABLE_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.TABLE_NAME AND "
            "( SYSTEM_.SYS_REPL_ITEMS_.LOCAL_PARTITION_NAME = SYSTEM_.SYS_REPL_OLD_ITEMS_.PARTITION_NAME OR "
            "( SYSTEM_.SYS_REPL_ITEMS_.LOCAL_PARTITION_NAME is NULL AND "
            "SYSTEM_.SYS_REPL_OLD_ITEMS_.PARTITION_NAME is NULL ) ) )"
            ,
        (SChar*) "ALTER TABLE SYS_REPL_OLD_ITEMS_ DROP COLUMN ( "
            "REMOTE_USER_NAME, "
            "REMOTE_TABLE_NAME, "
            "REMOTE_PARTITION_NAME, "
            "PARTITION_ORDER, "
            "PARTITION_MIN_VALUE, "
            "PARTITION_MAX_VALUE, "
            "INVALID_MAX_SN )"
            ,
        (SChar*) "ALTER TABLE SYS_REPLICATIONS_ DROP COLUMN ( "
            "REMOTE_LAST_DDL_XSN )"
            ,
        (SChar*) "ALTER TABLE SYS_TABLES_ DROP COLUMN ( "
            "USABLE, "
            "SHARD_FLAG )"
            ,
        (SChar*) "ALTER TABLE SYS_TABLE_PARTITIONS_ DROP COLUMN ( "
            "PARTITION_USABLE )"
            ,
        NULL    };

    //---------------------------------------------------
    // PROJ-2689 downgrade meta
    //
    // 다운그레이드 시,
    // (1) 아래와 같이 필요한 sql문들의 배열을 생성
    //  
    //     SChar         * sDowngradeVer6ToVer5[] = {
    //         (SChar*) "DROP TABLE SYS_DATABASE_"
    //         ,
    //         (SChar*) "CREATE TABLE SYS_DATABASE_ ( "
    //                  "DB_NAME VARCHAR(40) FIXED, "
    //                  "OWNER_DN VARCHAR(2048) FIXED, "
    //                  "META_MAJOR_VER INTEGER, "
    //                  "META_MINOR_VER INTEGER, "
    //                  "META_PATCH_VER INTEGER,"
    //                  "PREV_META_MAJOR_VER INTEGER, "
    //                  "PREV_META_MINOR_VER INTEGER, "
    //                  "PREV_META_PATCH_VER INTEGER )"
    //         ,
    //         (SChar*) "INSERT INTO SYS_DATABASE_ VALUES ( '%s', "
    //                  "'%s',"
    //                  QCM_META_MAJOR_STR_VER", "
    //                  "%"ID_INT32_FMT", "
    //                  QCM_META_PATCH_STR_VER","
    //                  QCM_META_MAJOR_STR_VER", "
    //                  "%"ID_INT32_FMT", "
    //                  QCM_META_PATCH_STR_VER" )"
    //         ,
    //         NULL };
    //  
    //     qcmCreate::upgradeMeta와 상반되는 SQL구문 작성
    //     EX) 
    //     upgrade meta SQL                    | downgrade meta SQL
    //     ----------------------------------------------------------------------
    //      CREATE TABLE TMP(...);             | DROP TABLE TMP;
    //      CRETAE VIEW    ...;                | DROP VIEW ...;
    //      CREATE INDEX ...;                  | DROP INDEX ...;
    //      ALTER TABLE TMP ADD COLUMN(...);   | ALTER TABLE TMP DROP (...);
    //      ALTER TABLE TMP MODIFY COLUMN ...; | ALTER TABLE TMP MODIFY COLUMN ...;
    //      INSERT TMP VALUES (...);           | DELETE FROM TMP WHERE ...;
    //      CREATE PROCEDURE ...;              | DROP PROCEDURE ...;   // built-in procedure
    //  
    //     CAUTION!!!!!!!)
    //     1. upgrade meta SQL에 Drop TABLE / ALTER TABLE .. DROP COLUMN .. 과 같은 구문은 사용할 수 없다.
    //         사용하지않는 메타(컬럼 또는 테이블)인 경우에 한번에 제거한 후, major버전으로 올려야한다.
    //     2. upgrade meta SQL에 DELETE / TRUNCATE 와 같이 메타의 데이터를 제거하는 구문의 사용을 자제해야한다.
    //         만약 다운그레이드가 가능하다고 판단되는 경우에만 사용 할 수 있다.
    //  
    // (2) 밑 부분 switch case 에서,
    //     downgrade하려는 version에서 (1)의 sql문 실행
    //  
    //     switch ( QCM_META_MINOR_VER )
    //     {
    //         case 5:
    //             // downgrade from minor version 5 to minor version 4
    //             for ( i = 0; sDowngradeVer5ToVer4[i] != NULL; i++ )
    //             {
    //                 IDE_TEST( sSmiStmt.begin( aStatistics,
    //                                           sDummySmiStmt,
    //                                           sSmiStmtFlag )
    //                           != IDE_SUCCESS );
    //                 sStage = 3;
    //                 if ( i == 2 )
    //                 {
    //                     SChar sInsSql[4096] = { 0, };
    //                     SChar sOwnerDN[4096] = { 0, };
    //                     IDE_TEST( qcmDatabase::checkDatabase( &sSmiStmt,
    //                                                           (SChar*)sOwnerDN,
    //                                                           ID_SIZEOF(sOwnerDN) )
    //                               != IDE_SUCCESS );
    //                     /* 다운그레이드시, SYS_DATABASE_.PREV_META_MINOR_VER값은
    //                      * aPrevMinorVer 이여야한다. 
    //                      * 이후 메타 다운그레이드 될 수 없도록 해야하기
    //                      * 때문이다. */
    //                     idlOS::snprintf( sInsSql,
    //                                      ID_SIZEOF(sInsSql),
    //                                      sDowngradeVer5ToVer4[i],
    //                                      smiGetDBName(), sOwnerDN, aPrevMinorVer, aPrevMinorVer );
    //                     IDE_TEST( executeDDL( &sStatement,
    //                                           sInsSql )
    //                               != IDE_SUCCESS );
    //                 }
    //                 else
    //                 {
    //                     IDE_TEST( executeDDL( &sStatement,
    //                                           sDowngradeVer5ToVer4[i] )
    //                               != IDE_SUCCESS );
    //                 }
    //                 sStage = 2;
    //                 IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    //                 ideLog::log( IDE_QP_0, "[QCM_META_DOWNGRADE] DowngradeMeta %s\n",
    //                              sDowngradeVer5ToVer4[i] );
    //  
    //                 IDE_TEST_CONT( aPrevMinorVer == 4, STOP_DOWNGRADE ) // stop
    //                 /* fall through */
    //         case 4 :
    //         case 3 :
    //         case 2 :
    //             // 버전 2까지 지원불가.
    //             IDE_RAISE( err_downgrade_meta_fail );
    //         case 1 :
    //             // downgrade from minor version 1 to minor version 0
    //             /* fall through */
    //         defaule :
    //             break;
    //     }
    //  
    //     IDE_EXCEPTION_CONT( STOP_DOWNGRADE );
    //---------------------------------------------------

    // initialize smiStatement flag
    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST(sIduMem.init(IDU_MEM_QCM) != IDE_SUCCESS);
    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );

    sStage = 1;

    ideLog::log(IDE_QP_0, "[QCM_META_DOWNGRADE] ALLOC STATEMENT SUCCESS\n");

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    sStatement.session->mQPSpecific.mFlag &= ~QC_SESSION_ALTER_META_MASK;
    sStatement.session->mQPSpecific.mFlag |= QC_SESSION_ALTER_META_ENABLE;

    ideLog::log(IDE_QP_0, "[QCM_META_DOWNGRADE] SET STATEMENT SUCCESS\n");

    // transaction begin
    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            aStatistics,
                            (SMI_ISOLATION_NO_PHANTOM     |
                             SMI_TRANSACTION_NORMAL       |
                             SMI_TRANSACTION_REPL_DEFAULT |
                             SMI_COMMIT_WRITE_NOWAIT))
              != IDE_SUCCESS );
    sStage = 2;

    ideLog::log(IDE_QP_0, "[QCM_META_DOWNGRADE] TRANSACTION BEGIN SUCCESS\n");

    switch ( QCM_META_MINOR_VER )
    {
        case 2:
            // downgrade from minor version 2 to minor version 1
            for ( i = 0; sDowngradeVer2ToVer1[i] != NULL; i++ )
            {
                IDE_TEST( sSmiStmt.begin( aStatistics,
                                          sDummySmiStmt,
                                          sSmiStmtFlag )
                          != IDE_SUCCESS );
                sStage = 3;
                if ( i == 2 )
                {
                    SChar sInsSql[4096] = { 0, };
                    SChar sOwnerDN[4096] = { 0, };
                    IDE_TEST( qcmDatabase::checkDatabase( &sSmiStmt,
                                                          (SChar*)sOwnerDN,
                                                          ID_SIZEOF(sOwnerDN) )
                              != IDE_SUCCESS );
                    /* 다운그레이드시, SYS_DATABASE_.PREV_META_MINOR_VER값은
                     * aPrevMinorVer 이여야한다. 
                     * 이후 메타 다운그레이드 될 수 없도록 해야하기
                     * 때문이다. */
                    idlOS::snprintf( sInsSql,
                                     ID_SIZEOF(sInsSql),
                                     sDowngradeVer2ToVer1[i],
                                     smiGetDBName(), sOwnerDN, aPrevMinorVer, aPrevMinorVer );
                    IDE_TEST( executeDDL( &sStatement,
                                          sInsSql )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( executeDDL( &sStatement,
                                          sDowngradeVer2ToVer1[i] )
                              != IDE_SUCCESS );
                }
                sStage = 2;
                IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
                ideLog::log( IDE_QP_0, "[QCM_META_DOWNGRADE] DowngradeMeta %s\n",
                             sDowngradeVer2ToVer1[i] );
            }
            IDE_TEST_CONT( aPrevMinorVer == 1, STOP_DOWNGRADE ) // stop
            /* fall through */
        case 1:
            // downgrade from minor version 1 to minor version 0
            /* fall through */
        default:
            break;
    }

    // downgrade 추가 시 주석 제거 
    IDE_EXCEPTION_CONT( STOP_DOWNGRADE );

    // transaction commit
    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );

    ideLog::log(IDE_QP_0, "[QCM_META_DOWNGRADE] TRANSACTION COMMIT SUCCESS\n");

    // free the members of qcStatement
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );
    IDE_TEST( sTrans.destroy( aStatistics ) != IDE_SUCCESS );

    sIduMem.destroy();

    return IDE_SUCCESS;
    
   // downgrade 추가 시 주석 제거 
/*
    IDE_EXCEPTION(err_downgrade_meta_fail);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INITDB));
        ideLog::log(IDE_QP_0, "[QCM_META_DOWNGRADE] Not supported.\n");
    }
*/
    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 3:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 2:
            (void)sTrans.rollback();

            ideLog::log(IDE_QP_0, "[QCM_META_DOWNGRADE] TRANSACTION ROLLBACK\n");
        case 1:
            (void) qcg::freeStatement(&sStatement);
            (void) sTrans.destroy( aStatistics );
    }

    sIduMem.destroy();
    return IDE_FAILURE;
}
