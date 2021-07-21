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
 * $Id: qdbCreate.cpp 90824 2021-05-13 05:35:21Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdbCommon.h>
#include <qdbCreate.h>
#include <qdn.h>
#include <qmv.h>
#include <qtc.h>
#include <qcm.h>
#include <qcg.h>
#include <qcmCache.h>
#include <qcmUser.h>
#include <qcmTableSpace.h>
#include <qmvQuerySet.h>
#include <qmo.h>
#include <qmn.h>
#include <qcmTableInfo.h>
#include <qcuSqlSourceInfo.h>
#include <qdx.h>
#include <qdd.h>
#include <qmx.h>
#include <qdpPrivilege.h>
#include <qcmView.h>
#include <smErrorCode.h>
#include <smiTableSpace.h>
#include <qmcInsertCursor.h>
#include <qcsModule.h>
#include <qcmDictionary.h>
#include <qcuProperty.h>
#include <qdpRole.h>
#include <qmvShardTransform.h> /* TASK-7219 Shard Transformer Refactoring */

IDE_RC qdbCreate::validateCreateTable(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 문의 validation 수행 함수
 *
 * Implementation :
 *      1. 테이블 이름의 중복 검사
 *      2. 테이블 생성 권한 검사
 *      3. validation of TABLESPACE
 *      4. validation of columns specification
 *      5. validation of lob column attributes
 *      6. validation of constraints
 *      7. validation of partitioned table
 *      8. validation of compress column specification
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::validateCreateTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree     * sParseTree;
    idBool                 sExist = ID_FALSE;
    qsOID                  sProcID;
    qcuSqlSourceInfo       sqlInfo;

#if defined(DEBUG)
    qcmCompressionColumn * sCompColumn = NULL;
    qcmCompressionColumn * sCompPrev = NULL;
    qcmColumn            * sColumn = NULL;
#endif
    
#ifdef ALTI_CFG_EDITION_DISK
    SInt                   sCountMemType  = 0;
    SInt                   sCountVolType  = 0;
#endif

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->tableName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    // check table exist.
    if (gQcmSynonyms == NULL)
    {
        // in createdb phase -> no synonym meta.
        // so skip check duplicated name from synonym, psm
        IDE_TEST( qdbCommon::checkDuplicatedObject(
                      aStatement,
                      sParseTree->userName,
                      sParseTree->tableName,
                      &(sParseTree->userID) )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST(
            qcm::existObject(
                aStatement,
                ID_FALSE,
                sParseTree->userName,
                sParseTree->tableName,
                QS_OBJECT_MAX,
                &(sParseTree->userID),
                &sProcID,
                &sExist)
            != IDE_SUCCESS);

        IDE_TEST_RAISE(sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME);
    }

    // check grant
    IDE_TEST( qdpRole::checkDDLCreateTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );
    
    // validation of TABLESPACE
    IDE_TEST( validateTableSpace(aStatement) != IDE_SUCCESS );

#if defined(DEBUG)
    // BUG-38501
    // PROJ-2264 Dictionary table
    if( QCU_FORCE_COMPRESSION_COLUMN == 1 )
    {
        // 다음을 모두 만족하는 경우에 강제로 compression column 으로 만든다.
        // 1. Debug 모드로 빌드 되었을 때
        // 2. Patition table 이 아닐 때
        // 3. Dictionary table 이 아닐 때
        // 4. Compress 절이 없을 때
        // 5. Data memory/disk table 일 때
        // 6. Compression column 이 지원되는 데이터 타입일 때
        // 7. (BUG-38610) Queue 객체를 만드는 경우가 아닐 때

        if (smiTableSpace::isDataTableSpaceType( sParseTree->TBSAttr.mType ) == ID_TRUE )
        {
            if ((smiTableSpace::isMemTableSpaceType(sParseTree->TBSAttr.mType) != ID_TRUE) &&
                (smiTableSpace::isDiskTableSpaceType(sParseTree->TBSAttr.mType) != ID_TRUE))
            {
                /* do not compress */
                IDE_RAISE(LABEL_PASS);
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* do not compress */
            IDE_RAISE(LABEL_PASS);
        }

        if ( ( sParseTree->partTable == NULL ) &&
             ( ( sParseTree->tableAttrValue & SMI_TABLE_DICTIONARY_MASK )
               == SMI_TABLE_DICTIONARY_FALSE ) &&
             ( sParseTree->compressionColumn == NULL ) &&
             ( ( sParseTree->flag & QDQ_QUEUE_MASK ) != QDQ_QUEUE_TRUE ) )
        {
            for( sColumn = sParseTree->columns; sColumn != NULL; sColumn = sColumn->next )
            {
                switch( sColumn->basicInfo->type.dataTypeId )
                {
                    case MTD_CHAR_ID :
                    case MTD_VARCHAR_ID :
                    case MTD_NCHAR_ID :
                    case MTD_NVARCHAR_ID :
                    case MTD_BYTE_ID :
                    case MTD_VARBYTE_ID :
                    case MTD_NIBBLE_ID :
                    case MTD_BIT_ID :
                    case MTD_VARBIT_ID :
                    case MTD_DATE_ID :
                        // Column 의 저장크기(size)가 smOID 보다 작으면 skip
                        // 우선 compression 하면 오히려 저장 공간을 더 사용하게되고
                        // in-place update 시에 logging 문제도 발생하게 된다.
                        if( sColumn->basicInfo->column.size < ID_SIZEOF(smOID) )
                        {
                            sColumn->basicInfo->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                            sColumn->basicInfo->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
                        }
                        else
                        {
                            // BUG-38670
                            // Hidden column 은 compress 대상에서 제외한다.
                            // (Function based index)
                            if ( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
                                 == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
                            {
                                sColumn->basicInfo->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                                sColumn->basicInfo->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
                            }
                            else
                            {
                                // Compression column 임을 나타내도록 flag 세팅
                                sColumn->basicInfo->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                                sColumn->basicInfo->column.flag |= SMI_COLUMN_COMPRESSION_TRUE;

                                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qcmCompressionColumn),
                                                                         (void**)&sCompColumn )
                                          != IDE_SUCCESS );
                                QCM_COMPRESSION_COLUMN_INIT( sCompColumn );
                                SET_POSITION(sCompColumn->namePos, sColumn->namePos);

                                if( sCompPrev == NULL )
                                {
                                    sParseTree->compressionColumn = sCompColumn;
                                    sCompPrev                     = sCompColumn;
                                }
                                else
                                {
                                    sCompPrev->next               = sCompColumn;
                                    sCompPrev                     = sCompPrev->next;
                                }
                            }
                        }
                        break;
                    default :
                        sColumn->basicInfo->column.flag &= ~SMI_COLUMN_COMPRESSION_MASK;
                        sColumn->basicInfo->column.flag |= SMI_COLUMN_COMPRESSION_FALSE;
                        break;
                }
            }
        }
    }

    IDE_EXCEPTION_CONT(LABEL_PASS);
#endif

    /* BUG-45641 disk partitioned table에 압축 컬럼을 추가하다가 실패하는데, memory partitioned table 오류가 나옵니다. */
    // PROJ-2264 Dictionary table
    IDE_TEST( qcmDictionary::validateCompressColumn( aStatement,
                                                     sParseTree,
                                                     sParseTree->TBSAttr.mType )
              != IDE_SUCCESS );

    // validation of columns specification
    IDE_TEST( qdbCommon::validateColumnListForCreate(
                  aStatement,
                  sParseTree->columns,
                  ID_TRUE )
              != IDE_SUCCESS );

    // PROJ-1362
    // validation of lob column attributes
    IDE_TEST(qdbCommon::validateLobAttributeList(
                 aStatement,
                 NULL,
                 sParseTree->columns,
                 &(sParseTree->TBSAttr),
                 sParseTree->lobAttr )
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sParseTree->partTable != NULL )
    {
        IDE_TEST( qdbCommon::validatePartitionedTable( aStatement,
                                                       ID_TRUE ) /* aIsCreateTable */
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do
    }

    /* PROJ-2639 Altibase Disk Edition */
#ifdef ALTI_CFG_EDITION_DISK
    if ( ( ( sParseTree->flag & QDQ_QUEUE_MASK ) == QDQ_QUEUE_TRUE ) ||
         ( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK ) == QDT_CREATE_TEMPORARY_TRUE ) ||
         ( QCG_GET_SESSION_USER_ID(aStatement) == QC_SYSTEM_USER_ID ) )
    {
        /* Nothing to do */
    }
    else
    {
        IDE_TEST_RAISE( smiTableSpace::isDiskTableSpaceType( sParseTree->TBSAttr.mType ) == ID_FALSE,
                        ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );

        if ( sParseTree->partTable != NULL )
        {
            qdbCommon::getTableTypeCountInPartAttrList( NULL,
                                                        sParseTree->partTable->partAttr,
                                                        NULL,
                                                        & sCountMemType,
                                                        & sCountVolType );

            IDE_TEST_RAISE( ( sCountMemType + sCountVolType ) > 0,
                            ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );
        }
        else
        {
            /* Nothing to do */
        }
    }
#endif

    // validation of constraints
    IDE_TEST(qdn::validateConstraints( aStatement,
                                       NULL,
                                       sParseTree->userID,
                                       sParseTree->TBSAttr.mID,
                                       sParseTree->TBSAttr.mType,
                                       sParseTree->constraints,
                                       QDN_ON_CREATE_TABLE,
                                       NULL )
             != IDE_SUCCESS);

    /* PROJ-2464 [기능성] hybrid partaitioned table 지원
     *  - qdn::validateConstraints, qdbCommon::validatePartitionedTable 이후에 호출
     */
    IDE_TEST( qdbCommon::validateConstraintRestriction( aStatement,
                                                        sParseTree )
              != IDE_SUCCESS );

    IDE_TEST( calculateTableAttrFlag( aStatement,
                                      sParseTree )
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - 관련내용 : PRJ-1671 Bitmap TableSpace And Segment Space Management
     *               Segment Storage 속성 validation
     */
    IDE_TEST( qdbCommon::validatePhysicalAttr( sParseTree )
              != IDE_SUCCESS );

    // PROJ-1407 Temporary Table
    IDE_TEST( validateTemporaryTable( aStatement,
                                      sParseTree )
              != IDE_SUCCESS );

    if (  QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE ) 
    {
        qrc::setDDLSrcInfo( aStatement,
                            ID_TRUE,
                            0,
                            NULL,
                            0,
                            NULL );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
#ifdef ALTI_CFG_EDITION_DISK
    IDE_EXCEPTION( ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USE_ONLY_DISK_TABLE_PARTITION_IN_DISK_EDITION ) );
    }
#endif
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCreate::parseCreateTableAsSelect(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    CREATE TABLE ... AS SELECT ... 의 parse 수행
 *
 * Implementation :
 *    1. SELECT 의 parse 수행 => qmv::parseSelect
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::parseCreateTableAsSelect"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree    * sParseTree;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* TASK-7219 Shard Transformer Refactoring */
    IDE_TEST( qmv::parseSelectInternal( sParseTree->select )
              != IDE_SUCCESS );

    IDE_TEST( qmvShardTransform::doTransform( sParseTree->select )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCreate::validateCreateTableAsSelect(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    CREATE TABLE ... AS SELECT ... 의 validation 수행
 *
 * Implementation :
 *    1. 테이블 이름이 이미 존재하는지 체크
 *    2. CREATE TABLE 권한이 있는지 체크
 *    3. validation of TABLESPACE
 *    4. validation of SELECT statement
 *    5. validation of column and target
 *    6. validation of constraints
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::validateCreateTableAsSelect"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree     * sParseTree;
    qsOID                  sProcID;
    idBool                 sExist = ID_FALSE;
    qdPartitionAttribute * sPartAttr;
    UInt                   sPartCount = 0;
    qmsPartCondValList     sOldPartCondVal;
    qcmColumn            * sColumns;
    qcuSqlSourceInfo       sqlInfo;
    qdConstraintSpec     * sConstraint;

#ifdef ALTI_CFG_EDITION_DISK
    SInt                   sCountMemType  = 0;
    SInt                   sCountVolType  = 0;
#endif

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    
    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->tableName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }
    
    IDE_TEST(
        qcm::existObject(
            aStatement,
            ID_FALSE,
            sParseTree->userName,
            sParseTree->tableName,
            QS_OBJECT_MAX,
            &(sParseTree->userID),
            &sProcID,
            &sExist)
        != IDE_SUCCESS);

    IDE_TEST_RAISE(sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME);

    // check grant
    IDE_TEST( qdpRole::checkDDLCreateTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );
        
    // validation of TABLESPACE
    IDE_TEST( validateTableSpace(aStatement) != IDE_SUCCESS );

    // validation of SELECT statement
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        &= ~(QMV_PERFORMANCE_VIEW_CREATION_MASK);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        |= (QMV_PERFORMANCE_VIEW_CREATION_FALSE);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        &= ~(QMV_VIEW_CREATION_MASK);
    ((qmsParseTree*)(sParseTree->select->myPlan->parseTree))->querySet->lflag
        |= (QMV_VIEW_CREATION_FALSE);
    IDE_TEST(qmv::validateSelect(sParseTree->select) != IDE_SUCCESS);

    // fix BUG-18752
    aStatement->myPlan->parseTree->currValSeqs =
        sParseTree->select->myPlan->parseTree->currValSeqs;

    // validation of column and target
    IDE_TEST(validateTargetAndMakeColumnList(aStatement)
             != IDE_SUCCESS);

    IDE_TEST(qdbCommon::validateColumnListForCreate(
                 aStatement,
                 sParseTree->columns,
                 ID_TRUE )
             != IDE_SUCCESS);

    // fix BUG-17237
    // column의 not null 정보 제거.
    for( sColumns = sParseTree->columns;
         sColumns != NULL;
         sColumns = sColumns->next )
    {
        sColumns->basicInfo->flag &= ~MTC_COLUMN_NOTNULL_MASK;
        sColumns->basicInfo->flag |= MTC_COLUMN_NOTNULL_FALSE;
    }

    // PROJ-1362
    // validation of lob column attributes
    IDE_TEST(qdbCommon::validateLobAttributeList(
                 aStatement,
                 NULL,
                 sParseTree->columns,
                 &(sParseTree->TBSAttr),
                 sParseTree->lobAttr )
             != IDE_SUCCESS);

    // validation of constraints
    IDE_TEST( qdn::validateConstraints( aStatement,
                                        NULL,
                                        sParseTree->userID,
                                        sParseTree->TBSAttr.mID,
                                        sParseTree->TBSAttr.mType,
                                        sParseTree->constraints,
                                        QDN_ON_CREATE_TABLE,
                                        NULL )
              != IDE_SUCCESS);

    /* PROJ-1107 Check Constraint 지원 */
    for ( sConstraint = sParseTree->constraints;
          sConstraint != NULL;
          sConstraint = sConstraint->next )
    {
        // BUG-32627
        // Create as select with foreign key constraints not allowed.
        IDE_TEST_RAISE( sConstraint->constrType == QD_FOREIGN,
                        ERR_FOREIGN_KEY_CONSTRAINT );

        if ( sConstraint->constrType == QD_CHECK )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(sConstraint->checkCondition->position) );
            IDE_RAISE( ERR_SET_CHECK_CONSTRAINT );
        }
        else
        {
            /* Nothing to do */
        }
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sParseTree->partTable != NULL )
    {
        IDE_TEST( qdbCommon::validatePartitionedTable( aStatement,
                                                       ID_TRUE ) /* aIsCreateTable */
                  != IDE_SUCCESS );

        if ( (sParseTree->partTable->partMethod == QCM_PARTITION_METHOD_RANGE) ||
             (sParseTree->partTable->partMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH) ||
             (sParseTree->partTable->partMethod == QCM_PARTITION_METHOD_LIST) )
        {
            for( sPartAttr = sParseTree->partTable->partAttr, sPartCount = 0;
                 sPartAttr != NULL;
                 sPartAttr = sPartAttr->next, sPartCount++ )
            {
                IDU_LIMITPOINT("qdbCreate::validateCreateTableAsSelect::malloc1");
                IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                                      qdAlterPartition,
                                      (void**)& sPartAttr->alterPart)
                         != IDE_SUCCESS);
                QD_SET_INIT_ALTER_PART( sPartAttr->alterPart );

                IDU_LIMITPOINT("qdbCreate::validateCreateTableAsSelect::malloc2");
                IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                                      qmsPartCondValList,
                                      (void**)& sPartAttr->alterPart->partCondMinVal)
                         != IDE_SUCCESS);

                IDU_LIMITPOINT("qdbCreate::validateCreateTableAsSelect::malloc3");
                IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                                      qmsPartCondValList,
                                      (void**)& sPartAttr->alterPart->partCondMaxVal)
                         != IDE_SUCCESS);

                if( sPartCount == 0 )
                {
                    sPartAttr->alterPart->partCondMinVal->partCondValCount = 0;
                    sPartAttr->alterPart->partCondMinVal->partCondValType =
                        QMS_PARTCONDVAL_NORMAL;
                    sPartAttr->alterPart->partCondMinVal->partCondValues[0] = 0;

                    sPartAttr->alterPart->partCondMaxVal
                        = & sPartAttr->partCondVal;
                }
                else
                {
                    idlOS::memcpy( sPartAttr->alterPart->partCondMinVal,
                                   & sOldPartCondVal,
                                   ID_SIZEOF(qmsPartCondValList) );

                    sPartAttr->alterPart->partCondMaxVal
                        = & sPartAttr->partCondVal;
                }

                sOldPartCondVal = *(sPartAttr->alterPart->partCondMaxVal );
            }
        }
    }

    /* PROJ-2639 Altibase Disk Edition */
#ifdef ALTI_CFG_EDITION_DISK
    if ( ( ( sParseTree->flag & QDQ_QUEUE_MASK ) == QDQ_QUEUE_TRUE ) ||
         ( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK ) == QDT_CREATE_TEMPORARY_TRUE ) ||
         ( QCG_GET_SESSION_USER_ID(aStatement) == QC_SYSTEM_USER_ID ) )
    {
        /* Nothing to do */
    }
    else
    {
        IDE_TEST_RAISE( smiTableSpace::isDiskTableSpaceType( sParseTree->TBSAttr.mType ) == ID_FALSE,
                        ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );

        if ( sParseTree->partTable != NULL )
        {
            qdbCommon::getTableTypeCountInPartAttrList( NULL,
                                                        sParseTree->partTable->partAttr,
                                                        NULL,
                                                        & sCountMemType,
                                                        & sCountVolType );

            IDE_TEST_RAISE( ( sCountMemType + sCountVolType ) > 0,
                            ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );
        }
        else
        {
            /* Nothing to do */
        }
    }
#endif

    /* PROJ-2464 hybrid partitioned table 지원
     *  - qdn::validateConstraint, qdbCommon::validatePartitionedTable에 호출한다.
     */
    IDE_TEST( qdbCommon::validateConstraintRestriction( aStatement,
                                                        sParseTree )
              != IDE_SUCCESS );

    // Attribute List로부터 Table의 Flag 계산
    IDE_TEST( calculateTableAttrFlag( aStatement,
                                      sParseTree )
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - 관련내용 : PRJ-1671 Bitmap TableSpace And Segment Space Management
     *               Segment Storage 속성 validation
     */
    IDE_TEST( qdbCommon::validatePhysicalAttr( sParseTree )
              != IDE_SUCCESS );

    // PROJ-1407 Temporary Table
    IDE_TEST( validateTemporaryTable( aStatement,
                                      sParseTree )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
   
    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_SET_CHECK_CONSTRAINT );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NOT_ALLOWED_CHECK_CONSTRAINT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_FOREIGN_KEY_CONSTRAINT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NOT_ALLOWED_FOREIGN_KEY_CONSTRAINT) );
    }
#ifdef ALTI_CFG_EDITION_DISK
    IDE_EXCEPTION( ERR_NOT_SUPPORT_MEMORY_VOLATILE_TABLESPACE_IN_DISK_EDITION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USE_ONLY_DISK_TABLE_PARTITION_IN_DISK_EDITION ) );
    }
#endif
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCreate::validateTableSpace(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *    1. TABLESPACE 에 대한 validation
 *    2. INSERT HIGH LIMIT validation
 *    3. INSERT LOW LIMIT validation
 *
 * Implementation :
 *    1. TABLESPACE 에 대한 validation
 *    if ( TABLESPACENAME 명시한 경우 )
 *    {
 *      1.1.1 SM 에서 존재하는 테이블스페이스명인지 검색
 *      1.1.2 존재하지 않으면 오류
 *      1.1.3 테이블스페이스의 종류가 UNDO tablespace 또는 temporary
 *            tablespace 이면 오류
 *      1.1.4 USER_ID 와 TBS_ID 로 SYS_TBS_USERS_ 검색해서 레코드가 존재하고
 *            QUOTA 값이 0 이면 오류
 *    }
 *    else
 *    {
 *      1.2.1 USER_ID 로 SYS_USERS_ 검색해서 DEFAULT_TBS_ID 값을 읽어 테이블을
 *            위한 테이블스페이스로 지정
 *    }
 *    2. PCTFREE validation
 *    if ( PCTFREE 값을 명시한 경우 )
 *    {
 *      0 <= PCTFREE <= 99
 *    }
 *    else
 *    {
 *      PCTFREE = QD_TABLE_DEFAULT_PCTFREE
 *    }
 *    3. PCTUSED validation
 *    if ( PCTUSED 값을 명시한 경우 )
 *    {
 *      0 <= PCTUSED <= 99
 *    }
 *    else
 *    {
 *      PCTUSED = QD_TABLE_DEFAULT_PCTUSED
 *    }
 *
 ***********************************************************************/

    qdTableParseTree* sParseTree;
    scSpaceID         sTBSID;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    if ( QC_IS_NULL_NAME(sParseTree->TBSName) == ID_TRUE )
    {
        // BUG-37377 queue 의 경우 메모리 테이블 스페이스에 생성되어야 한다.
        if( (sParseTree->flag & QDQ_QUEUE_MASK) == QDQ_QUEUE_TRUE )
        {
            sTBSID = SMI_ID_TABLESPACE_SYSTEM_MEMORY_DATA;
        }
        else
        {
            IDE_TEST( qcmUser::getDefaultTBS( aStatement,
                                              sParseTree->userID,
                                              &sTBSID) != IDE_SUCCESS );
        }

        IDE_TEST(
            qcmTablespace::getTBSAttrByID( sTBSID,
                                           &(sParseTree->TBSAttr) )
            != IDE_SUCCESS );

    }
    else
    {
        IDE_TEST( qcmTablespace::getTBSAttrByName(
                      aStatement,
                      sParseTree->TBSName.stmtText + sParseTree->TBSName.offset,
                      sParseTree->TBSName.size,
                      &(sParseTree->TBSAttr))
                  != IDE_SUCCESS );

        if( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK ) ==
            QDT_CREATE_TEMPORARY_FALSE )
        {
            IDE_TEST_RAISE( smiTableSpace::isDataTableSpaceType(
                                sParseTree->TBSAttr.mType ) == ID_FALSE,
                            ERR_NO_CREATE_IN_SYSTEM_TBS );
        }
        else
        {
            IDE_TEST_RAISE( smiTableSpace::isVolatileTableSpaceType(
                                sParseTree->TBSAttr.mType ) == ID_FALSE,
                            ERR_NO_CREATE_NON_VOLATILE_TBS );
        }

        IDE_TEST( qdpRole::checkAccessTBS(
                      aStatement,
                      sParseTree->userID,
                      sParseTree->TBSAttr.mID) != IDE_SUCCESS );
    }


    //----------------------------------------------
    // [Cursor Flag 의 설정]
    // 접근하는 테이블 스페이스의 종류에 따라, Ager의 동작이 다르게 된다.
    // 이를 위해 Memory, Disk, Memory와 Disk에 접근하는 지를
    // 정확하게 판단하여야 한다.
    // 질의문에 여러개의 다른 테이블이 존재할 수 있으므로,
    // 해당 Cursor Flag을 ORing하여 그 정보가 누적되게 한다.
    //----------------------------------------------
    /* PROJ-2464 hybrid partitioned table 지원
     *  - HPT 인 경우에, Memory, Disk Partition를 모두 지닐 수 있다.
     *  - 따라서 Memory Partition이 포함되어도 SegAttr, SegStoAttr 옵션을 지닐 수 있다.
     *  - Validate과 기본값 설정을 Partition의 TBS Validate 이후에 수행한다.
     *  - qdbCommon::validatePhysicalAttr
     */
    if ((smiTableSpace::isMemTableSpaceType(sParseTree->TBSAttr.mType) == ID_TRUE) ||
        (smiTableSpace::isVolatileTableSpaceType(sParseTree->TBSAttr.mType) == ID_TRUE))
    {
        // Memory Table Space를 사용하는 DDL인 경우
        // Cursor Flag의 누적
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
    }
    else
    {
        // Disk Table Space를 사용하는 DDL인 경우
        // Cursor Flag의 누적
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_CREATE_IN_SYSTEM_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_CREATE_IN_SYSTEM_TBS));
    }
    IDE_EXCEPTION(ERR_NO_CREATE_NON_VOLATILE_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CANNOT_CREATE_TEMPORARY_TABLE_IN_NONVOLATILE_TBS));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCreate::validateTargetAndMakeColumnList(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE ... AS SELECT 문의 validation 수행 함수로부터 호출
 *      : validation of target and make column
 *
 * Implementation :
 *      1. if... create 문장의 컬럼이 명시 X
 *         : select 컬럼의 앨리어스 명 중복 검사
 *      2. else... create 문장의 컬럼이 명시 O
 *         : create 문장의 컬럼개수와 select 컬럼의 개수 일치 검사
 *
 *      fix BUG-24917
 *         : create as select 또는 view생성시에는 이 함수를 호출하여
 *           target으로부터 컬럼을 생성하고
 *           컬럼에 대한 validation은 qdbCommon::validateColumnList(..)를 호출
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree;
    qcmColumn           * sPrevColumn = NULL;
    qcmColumn           * sCurrColumn = NULL;
    qmsTarget           * sTarget;
    SInt                  sColumnCount = 0;
    SChar               * sColumnName;
    qcuSqlSourceInfo      sqlInfo;
    SInt                  i = 0;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    if (sParseTree->columns == NULL)
    {
        for (sTarget =
                 ((qmsParseTree *)(sParseTree->select->myPlan->parseTree))->querySet->target;
             sTarget != NULL;
             sTarget = sTarget->next)
        {
            IDE_TEST_RAISE(sTarget->aliasColumnName.name == NULL,
                           ERR_NOT_EXIST_ALIAS_OF_TARGET);

            // make current qcmColumn
            IDU_LIMITPOINT("qdbCreate::validateTargetAndMakeColumnList::malloc");
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                                  qcmColumn,
                                  (void**)&sCurrColumn)
                     != IDE_SUCCESS);

            sCurrColumn->namePos.stmtText = NULL;
            sCurrColumn->namePos.offset   = (SInt)0;
            sCurrColumn->namePos.size     = (SInt)0;

            //fix BUG-17635
            sCurrColumn->flag = 0;

            // fix BUG-21021
            sCurrColumn->flag &= ~QCM_COLUMN_TYPE_MASK;
            sCurrColumn->flag |= QCM_COLUMN_TYPE_DEFAULT;
            sCurrColumn->inRowLength = ID_UINT_MAX;
            sCurrColumn->defaultValue = NULL;

            if (sTarget->aliasColumnName.size > QC_MAX_OBJECT_NAME_LEN)
            {
                if (sTarget->targetColumn != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sTarget->targetColumn->position);
                }
                else
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sCurrColumn->namePos);
                }
                IDE_RAISE(ERR_MAX_COLUMN_NAME_LENGTH);
            }
            else
            {
                /* BUG-33681 */
                for ( i = 0; i < sTarget->aliasColumnName.size; i ++ )
                {
                    IDE_TEST_RAISE(*(sTarget->aliasColumnName.name + i) == '\'',
                                   ERR_NOT_EXIST_ALIAS_OF_TARGET);
                }
            }

            idlOS::strncpy(sCurrColumn->name,
                           sTarget->aliasColumnName.name,
                           sTarget->aliasColumnName.size);
            sCurrColumn->name[sTarget->aliasColumnName.size] = '\0';

            IDE_TEST(qdbCommon::getMtcColumnFromTarget(
                         aStatement,
                         sTarget->targetColumn,
                         sCurrColumn,
                         sParseTree->TBSAttr.mID) != IDE_SUCCESS);

            sCurrColumn->next = NULL;

            // link column list
            if (sParseTree->columns == NULL)
            {
                sParseTree->columns = sCurrColumn;
                sPrevColumn         = sCurrColumn;
            }
            else
            {
                sPrevColumn->next   = sCurrColumn;
                sPrevColumn         = sCurrColumn;
            }

            sColumnCount++;
        }

        for (sCurrColumn = sParseTree->columns; sCurrColumn != NULL;
             sCurrColumn = sCurrColumn->next)
        {
            for (sPrevColumn = sCurrColumn->next; sPrevColumn != NULL;
                 sPrevColumn = sPrevColumn->next)
            {
                if ( idlOS::strMatch( sCurrColumn->name,
                                      idlOS::strlen( sCurrColumn->name ),
                                      sPrevColumn->name,
                                      idlOS::strlen( sPrevColumn->name ) ) == 0 )
                {
                    sColumnName = sCurrColumn->name;
                    IDE_RAISE( ERR_DUP_ALIAS_NAME );
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }
    else
    {
        for (sCurrColumn = sParseTree->columns,
                 sTarget = ((qmsParseTree *)(sParseTree->select->myPlan->parseTree))
                 ->querySet->target;
             sCurrColumn != NULL;
             sCurrColumn = sCurrColumn->next,
                 sTarget = sTarget->next)
        {
            IDE_TEST_RAISE(sTarget == NULL, ERR_MISMATCH_NUMBER_OF_COLUMN);

            IDE_TEST(qdbCommon::getMtcColumnFromTarget(
                         aStatement,
                         sTarget->targetColumn,
                         sCurrColumn,
                         sParseTree->TBSAttr.mID) != IDE_SUCCESS);

            sColumnCount++;
        }

        IDE_TEST_RAISE(sTarget != NULL, ERR_MISMATCH_NUMBER_OF_COLUMN);
    }

    IDE_TEST_RAISE(sColumnCount > QC_MAX_COLUMN_COUNT,
                   ERR_INVALID_COLUMN_COUNT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DUP_ALIAS_NAME);
    {
        char sBuffer[256];
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "\"%s\"", sColumnName );
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN,
                                sBuffer));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_ALIAS_OF_TARGET);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NOT_EXISTS_ALIAS));
    }
    IDE_EXCEPTION(ERR_MISMATCH_NUMBER_OF_COLUMN);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_MISMATCH_COL_COUNT));
    }
    IDE_EXCEPTION(ERR_INVALID_COLUMN_COUNT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_COLUMN_COUNT));
    }
    IDE_EXCEPTION(ERR_MAX_COLUMN_NAME_LENGTH);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC qdbCreate::optimize(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE ... AS SELECT 문의 optimization 수행 함수
 *
 * Implementation :
 *      1. select 쿼리에 대한 optimization 수행
 *      2. make internal PROJ
 *      3. set execution plan tree
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::optimze"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree    * sParseTree;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    if (sParseTree->select != NULL)
    {
        if ( (sParseTree->flag & QDV_OPT_STATUS_MASK) == QDV_OPT_STATUS_VALID )
        {
            IDE_TEST(qmo::optimizeCreateSelect( sParseTree->select )
                     != IDE_SUCCESS);
        }
        else
        {
            sParseTree->select->myPlan->plan = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qdbCreate::executeCreateTable(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE 문의 execution 수행 함수
 *
 * Implementation :
 *      1. validateDefaultValueType
 *      2. 새로운 테이블을 위한 TableID 구하기
 *      3. createTableOnSM
 *      4. SYS_TABLES_ 메타 테이블에 테이블 정보 입력
 *      5. SYS_COLUMNS_ 메타 테이블에 테이블 컬럼 정보 입력
 *      6. primary key, not null, check constraint 생성
 *      7. 메타 캐쉬 수행
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::executeCreateTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree     * sParseTree;
    UInt                   sTableID;
    smOID                  sTableOID;
    UInt                   sColumnCount;
    UInt                   sPartKeyCount = 0;
    qcmColumn            * sColumn;
    qcmColumn            * sPartKeyColumns;
    qdConstraintSpec     * sConstraint;
    qcmTableInfo         * sTempTableInfo = NULL;
    smSCN                  sSCN;
    UInt                   i;
    void                 * sTableHandle;
    idBool                 sIsQueue;
    idBool                 sIsPartitioned;
    qcmPartitionInfoList * sOldPartInfoList  = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;
    qcmTemporaryType       sTemporaryType;
    qdIndexTableList     * sIndexTable;
    
    SChar                  sTableOwnerName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    SChar                  sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    SChar                  sColumnName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };

    // PROJ-2264 Dictionary table
    qcmCompressionColumn * sCompColumn;
    qcmDictionaryTable   * sDicTable;
    qcmDictionaryTable   * sDictionaryTable;

    qcmIndex             * sTempIndex = NULL; 
    UInt                   sShardFlag = 0;

    //---------------------------------
    // 기본 정보 초기화
    //---------------------------------
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sColumnCount = 0;
    sColumn = sParseTree->columns;
    sDictionaryTable = NULL;

    // PROJ-1502 PARTITIONED DISK TABLE
    // 논파티션드 테이블 생성
    
    if( sParseTree->partTable == NULL )
    {
        sIsPartitioned = ID_FALSE;
    }
    // 파티션드 테이블 생성
    else
    {
        sIsPartitioned = ID_TRUE;

        sPartKeyCount = 0;

        for( sPartKeyColumns = sParseTree->partTable->partKeyColumns;
             sPartKeyColumns != NULL;
             sPartKeyColumns = sPartKeyColumns->next )
        {
            sPartKeyCount++;
        }
    }

    // PROJ-1407 Temporary Table
    if ( sParseTree->temporaryOption == NULL )
    {
        if ( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK ) ==
             QDT_CREATE_TEMPORARY_TRUE )
        {
            sTemporaryType = QCM_TEMPORARY_ON_COMMIT_DELETE_ROWS;
        }
        else
        {
            sTemporaryType = QCM_TEMPORARY_ON_COMMIT_NONE;
        }
    }
    else
    {
        sTemporaryType = sParseTree->temporaryOption->temporaryType;
    }

    while(sColumn != NULL)
    {
        sColumnCount++;
        if (sColumn->defaultValue != NULL)
        {
            IDE_TEST(qdbCommon::convertDefaultValueType(
                         aStatement,
                         &sColumn->basicInfo->type,
                         sColumn->defaultValue,
                         NULL)
                     != IDE_SUCCESS);
        }
        sColumn = sColumn->next;
    }

    if ( sParseTree->parallelDegree == 0 )
    {
        // PROJ-1071 Parallel query
        // Table 생성 시 parallel degree 가 지정되지 않았다면
        // 최소값인 1 로 변경한다.
        sParseTree->parallelDegree = 1;
    }
    else
    {
        /* nothing to do */
    }

    if ( QCU_FORCE_PARALLEL_DEGREE > 1 )
    {
        // 1. Dictionary table 이 아닐 때
        // 2. Parallel degree 이 1 일 때
        // 강제로 parallel degree 를 property 크기로 지정한다.
        if ( ( (sParseTree->tableAttrValue & SMI_TABLE_DICTIONARY_MASK)
               == SMI_TABLE_DICTIONARY_FALSE ) &&
             ( sParseTree->parallelDegree == 1 ) )
        {
            sParseTree->parallelDegree = QCU_FORCE_PARALLEL_DEGREE;
        }
    }

    // PROJ-2264 Dictionary Table
    for ( sCompColumn = sParseTree->compressionColumn;
          sCompColumn != NULL;
          sCompColumn = sCompColumn->next )
    {
        IDE_TEST( qcmDictionary::makeDictionaryTable( aStatement,
                                                      sParseTree->TBSAttr.mID,
                                                      sParseTree->columns,
                                                      sCompColumn,
                                                      &sDictionaryTable )
                  != IDE_SUCCESS );
    }

    IDE_TEST(qcm::getNextTableID(aStatement, &sTableID) != IDE_SUCCESS);

    IDE_TEST(qdbCommon::createTableOnSM(aStatement,
                                        sParseTree->columns,
                                        sParseTree->userID,
                                        sTableID,
                                        sParseTree->maxrows,
                                        sParseTree->TBSAttr.mID,
                                        sParseTree->segAttr,
                                        sParseTree->segStoAttr,
                                        sParseTree->tableAttrMask,
                                        sParseTree->tableAttrValue,
                                        sParseTree->parallelDegree,
                                        &sTableOID)
             != IDE_SUCCESS);

    /* TASK-7307 DML Data Consistency in Shard */
    if ( sParseTree->mShardFlag == QCM_SHARD_FLAG_TABLE_META )
    {
        sShardFlag &= ~QCM_TABLE_SHARD_FLAG_TABLE_MASK;
        sShardFlag |= QCM_TABLE_SHARD_FLAG_TABLE_META;
    }
    else if ( sParseTree->mShardFlag == QCM_SHARD_FLAG_TABLE_BACKUP )
    {
        sShardFlag &= ~QCM_TABLE_SHARD_FLAG_TABLE_MASK;
        sShardFlag |= QCM_TABLE_SHARD_FLAG_TABLE_BACKUP;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST(qdbCommon::insertTableSpecIntoMeta(aStatement,
                                                sIsPartitioned,
                                                sParseTree->flag,
                                                sParseTree->tableName,
                                                sParseTree->userID,
                                                sTableOID,
                                                sTableID,
                                                sColumnCount,
                                                sParseTree->maxrows,
                                                sParseTree->accessOption, /* PROJ-2359 Table/Partition Access Option */
                                                sParseTree->TBSAttr.mID,
                                                sParseTree->segAttr,
                                                sParseTree->segStoAttr,
                                                sTemporaryType,
                                                sParseTree->parallelDegree, // PROJ_1071
                                                sShardFlag)     // TASK-7307
             != IDE_SUCCESS);

    if ((sParseTree->flag & QDQ_QUEUE_MASK) == QDQ_QUEUE_TRUE)
    {
        sIsQueue = ID_TRUE;
    }
    else
    {
        sIsQueue = ID_FALSE;
    }

    IDE_TEST(qdbCommon::insertColumnSpecIntoMeta(aStatement,
                                                 sParseTree->userID,
                                                 sTableID,
                                                 sParseTree->columns,
                                                 sIsQueue)
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if ( sIsPartitioned == ID_TRUE )
    {
        IDE_TEST(qdbCommon::insertPartTableSpecIntoMeta(aStatement,
                                                        sParseTree->userID,
                                                        sTableID,
                                                        sParseTree->partTable->partMethod,
                                                        sPartKeyCount,
                                                        sParseTree->isRowmovement )
                 != IDE_SUCCESS);

        IDE_TEST(qdbCommon::insertPartKeyColumnSpecIntoMeta(aStatement,
                                                            sParseTree->userID,
                                                            sTableID,
                                                            sParseTree->columns,
                                                            sParseTree->partTable->partKeyColumns,
                                                            QCM_TABLE_OBJECT_TYPE)
                 != IDE_SUCCESS);

        IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                             sTableID,
                                             sTableOID) != IDE_SUCCESS);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sTableID,
                                       &sTempTableInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);

        IDE_TEST( executeCreateTablePartition( aStatement,
                                               sTableID,
                                               sTempTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      aStatement->qmxMem,
                                                      sTableID,
                                                      & sTempPartInfoList )
                  != IDE_SUCCESS );

        // 모든 파티션에 LOCK(X)
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sTempPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sParseTree->partTable->partInfoList = sTempPartInfoList;
    }

    for (sConstraint = sParseTree->constraints;
         sConstraint != NULL;
         sConstraint = sConstraint->next)
    {
        fillColumnID(sParseTree->columns,
                     sConstraint);

        IDE_TEST(qdbCommon::createConstrPrimaryUnique(aStatement,
                                                      sTableOID,
                                                      sConstraint,
                                                      sParseTree->userID,
                                                      sTableID,
                                                      sTempPartInfoList,
                                                      sParseTree->maxrows)
                 != IDE_SUCCESS);

        IDE_TEST(qdbCommon::createConstrNotNull(aStatement,
                                                sConstraint,
                                                sParseTree->userID,
                                                sTableID)
                 != IDE_SUCCESS);

        /* PROJ-1107 Check Constraint 지원 */
        IDE_TEST( qdbCommon::createConstrCheck( aStatement,
                                                sConstraint,
                                                sParseTree->userID,
                                                sTableID,
                                                sParseTree->relatedFunctionNames )
                  != IDE_SUCCESS );

        IDE_TEST(qdbCommon::createConstrTimeStamp(aStatement,
                                                  sConstraint,
                                                  sParseTree->userID,
                                                  sTableID)
                 != IDE_SUCCESS);
    }

    if( sTempTableInfo != NULL )
    {
        (void)qcm::destroyQcmTableInfo(sTempTableInfo);
    }
    sTempTableInfo = NULL;

    IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                         sTableID,
                                         sTableOID) != IDE_SUCCESS);

    IDE_TEST(qcm::getTableInfoByID(aStatement,
                                   sTableID,
                                   &sTempTableInfo,
                                   &sSCN,
                                   &sTableHandle)
             != IDE_SUCCESS);

    IDE_TEST(qcm::validateAndLockTable(aStatement,
                                       sTableHandle,
                                       sSCN,
                                       SMI_TABLE_LOCK_X)
             != IDE_SUCCESS);

    for (sConstraint = sParseTree->constraints;
         sConstraint != NULL;
         sConstraint = sConstraint->next)
    {
        if (sConstraint->constrType == QD_FOREIGN)
        {
            (void)qcm::destroyQcmTableInfo(sTempTableInfo);
            sTempTableInfo = NULL;

            IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT(aStatement),
                                                 sTableID,
                                                 sTableOID) != IDE_SUCCESS);

            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                           sTableID,
                                           &sTempTableInfo,
                                           &sSCN,
                                           &sTableHandle)
                     != IDE_SUCCESS);

            IDE_TEST(qdbCommon::createConstrForeign(aStatement,
                                                    sConstraint,
                                                    sParseTree->userID,
                                                    sTableID) != IDE_SUCCESS);

            (void)qcm::destroyQcmTableInfo(sTempTableInfo);
            sTempTableInfo = NULL;

            IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                                 sTableID,
                                                 sTableOID) != IDE_SUCCESS);

            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                           sTableID,
                                           &sTempTableInfo,
                                           &sSCN,
                                           &sTableHandle)
                     != IDE_SUCCESS);
        }
    }

    if (sParseTree->constraints != NULL)
    {
        if (sTempTableInfo->primaryKey != NULL)
        {
            for ( i = 0; i < sTempTableInfo->primaryKey->keyColCount; i++)
            {
                IDE_TEST(qdbCommon::makeColumnNotNull(
                        aStatement,
                        sTempTableInfo->tableHandle,
                        sParseTree->maxrows,
                        sTempPartInfoList,
                        sIsPartitioned,
                        sTempTableInfo->primaryKey->keyColumns[i].column.id)
                    != IDE_SUCCESS);
            }
        }

        (void)qcm::destroyQcmTableInfo(sTempTableInfo);
        sTempTableInfo = NULL;

        IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                             sTableID,
                                             sTableOID) != IDE_SUCCESS);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sTableID,
                                       &sTempTableInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);
    }

    // BUG-11266
    IDE_TEST(qcmView::recompileAndSetValidViewOfRelated(
            aStatement,
            sParseTree->userID,
            (SChar *) (sParseTree->tableName.stmtText +
                       sParseTree->tableName.offset),
            sParseTree->tableName.size,
            QS_TABLE)
        != IDE_SUCCESS);

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
     */
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sTempTableInfo->name )
                  != IDE_SUCCESS );

        IDE_TEST(qci::mManageReplicationCallback.mWriteTableMetaLog(
                aStatement,
                0,
                sTableOID)
            != IDE_SUCCESS);
    }


    // PROJ-2264 Dictionary Table
    for( sDicTable = sDictionaryTable;
         sDicTable != NULL;
         sDicTable = sDicTable->next )
    {
        IDE_TEST( qdbCommon::insertCompressionTableSpecIntoMeta(
                aStatement,
                sTableID,                                    // data table id
                sDicTable->dataColumn->basicInfo->column.id, // data table's column id
                sDicTable->dictionaryTableInfo->tableID,     // dictionary table id
                sDicTable->dictionaryTableInfo->maxrows )    // dictionary table's max rows
            != IDE_SUCCESS);
    }

    (void)qcm::destroyQcmTableInfo(sTempTableInfo);
    sTempTableInfo = NULL;

    IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                         sTableID,
                                         sTableOID) != IDE_SUCCESS);

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     & sTempTableInfo,
                                     & sSCN,
                                     & sTableHandle )
              != IDE_SUCCESS );

    if ( sIsPartitioned == ID_TRUE )
    {
        sOldPartInfoList = sTempPartInfoList;
        IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                      sTempTableInfo,
                                                                      sOldPartInfoList,
                                                                      & sTempPartInfoList )
                  != IDE_SUCCESS );

        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-2002 Column Security
    // 보안 컬럼 생성시 보안 모듈에 알린다.
    sColumn = sParseTree->columns;

    while(sColumn != NULL)
    {
        if ( (sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK)
             == MTD_ENCRYPT_TYPE_TRUE )
        {
            if ( sTableOwnerName[0] == '\0' )
            {
                // 최초 한번만 얻어온다.
                IDE_TEST( qcmUser::getUserName( aStatement,
                                                sParseTree->userID,
                                                sTableOwnerName )
                          != IDE_SUCCESS );

                QC_STR_COPY( sTableName, sParseTree->tableName );
            }
            else
            {
                // Nothing to do.
            }

            QC_STR_COPY( sColumnName, sColumn->namePos );

            (void) qcsModule::setColumnPolicy(
                sTableOwnerName,
                sTableName,
                sColumnName,
                sColumn->basicInfo->mColumnAttr.mEncAttr.mPolicy );
        }
        else
        {
            // Nothing to do.
        }

        sColumn = sColumn->next;
    }

    if ( QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE ) 
    {
        /* BUG-47811 DDL that requires NEED_DDL_INFO (transactional DDL, DDL Sync, DDL ASync )
         * should not have global non parititioned index. */
        IDE_TEST_RAISE( qcm::existGlobalNonPartitionedIndex( sTempTableInfo, &sTempIndex ) == ID_TRUE,
                        ERR_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX );

        qrc::setDDLDestInfo( aStatement,
                             1,
                             &(sTempTableInfo->tableOID),
                             0,
                             NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_NON_PART_INDEX,
                                  sTempIndex->name ) );

    } 
    IDE_EXCEPTION_END;

    // PROJ-2264 Dictionary Table
    // DDL 실패시 dictionary table 을 삭제한다.
    for( sDicTable = sDictionaryTable;
         sDicTable != NULL;
         sDicTable = sDicTable->next )
    {
        // BUG-36719
        // Dictionary table 과 메타테이블은 롤백되어 사라질 것이다.
        // 여기서는 table info 만 해제한다.
        (void)qcm::destroyQcmTableInfo( sDicTable->dictionaryTableInfo );
    }

    (void)qcm::destroyQcmTableInfo( sTempTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sTempPartInfoList );

    for ( sIndexTable = sParseTree->newIndexTables;
          sIndexTable != NULL;
          sIndexTable = sIndexTable->next )
    {
        (void)qcm::destroyQcmTableInfo(sIndexTable->tableInfo);
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdbCreate::executeCreateTablePartition(qcStatement   * aStatement,
                                              UInt            aTableID,
                                              qcmTableInfo  * aTableInfo)
{
/***********************************************************************
 *
 * Description :
 *      PROJ-1502 PARTITIONED DISK TABLE
 *
 *      CREATE TABLE 문의 execution 수행 함수
 *      각각의 파티션을 모두 생성한다.
 *
 * Implementation :
 *      1. 파티션의 개수만큼 반복
 *          1-1. 파티션 키 조건 값(PARTITION_MAX_VALUE, PARTITION_MIN_VALUE )
 *               을 구한다.(해시 파티션드 테이블 제외)
 *          1-2. 파티션 순서(PARTITION_ORDER)를 구한다.
 *               (해시 파티션드 테이블만 해당함)
 *          1-3. 파티션 생성
 *          1-4. SYS_TABLE_PARTITIONS_ 메타 테이블에 파티션 정보 입력
 *          1-5. SYS_PART_LOBS_ 메타 테이블에 파티션 정보 입력
 *          1-6. 테이블 파티션 메타 캐시 생성
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree;
    UInt                      sPartitionID;
    smOID                     sPartitionOID;
    qdPartitionAttribute    * sPartAttr;
    UInt                      sPartOrder;
    SChar                   * sPartMinVal = NULL;
    SChar                   * sPartMaxVal = NULL;
    SChar                   * sOldPartMaxVal = NULL;
    qcmPartitionMethod        sPartMethod;
    UInt                      sPartCount;
    qcmTableInfo            * sPartitionInfo = NULL;
    void                    * sPartitionHandle;
    smSCN                     sSCN;
    UInt                      sColumnCount = 0;
    qcmColumn               * sColumn;
    smiSegAttr                sSegAttr;
    smiSegStorageAttr         sSegStoAttr;
    UInt                      sPartType = 0;

    IDU_LIMITPOINT("qdbCreate::executeCreateTablePartition::malloc1");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN+1,
                                    & sPartMaxVal)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCreate::executeCreateTablePartition::malloc2");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN+1,
                                    & sPartMinVal)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCreate::executeCreateTablePartition::malloc3");
    IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                    SChar,
                                    QC_MAX_PARTKEY_COND_VALUE_LEN+1,
                                    & sOldPartMaxVal)
             != IDE_SUCCESS);

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    sPartMethod = sParseTree->partTable->partMethod;

    //-----------------------------------------------------
    // 1. 파티션 개수만큼 반복하면서 파티션을 생성한다.
    //-----------------------------------------------------
    // 현재 파티션들의 리스트는 키 조건 값을 기준으로 오름차순 정렬되어
    // 있는 상태이다.
    // qdbCommon::validateCondValues()함수에서 정렬했음.
    for( sPartAttr = sParseTree->partTable->partAttr, sPartCount = 0;
         sPartAttr != NULL;
         sPartAttr = sPartAttr->next, sPartCount++ )
    {
        //-----------------------------------------------------
        // 1-1. 파티션 키 조건 값을 구한다.(해시 파티션드 테이블 제외)
        //-----------------------------------------------------
        if( sPartMethod != QCM_PARTITION_METHOD_HASH )
        {
            IDE_TEST( qdbCommon::getPartitionMinMaxValue( aStatement,
                                                          sPartAttr,
                                                          sPartCount,
                                                          sPartMethod,
                                                          sPartMaxVal,
                                                          sPartMinVal,
                                                          sOldPartMaxVal )
                      != IDE_SUCCESS );
        }
        else
        {
            sPartMinVal[0] = '\0';
            sPartMaxVal[0] = '\0';
        }

        //-----------------------------------------------------
        // 1-2. 파티션 순서를 구한다.(해시 파티션드 테이블만 해당)
        //-----------------------------------------------------
        if( sPartMethod == QCM_PARTITION_METHOD_HASH )
        {
            sPartOrder = sPartAttr->partOrder;
        }
        else
        {
            sPartOrder = QDB_NO_PARTITION_ORDER;
        }

        //-----------------------------------------------------
        // 1-3. 파티션 생성
        //-----------------------------------------------------
        IDE_TEST(qcmPartition::getNextTablePartitionID(
                     aStatement,
                     & sPartitionID) != IDE_SUCCESS);

        // 디폴트 값 validation
        sColumn = sPartAttr->columns;
        while(sColumn != NULL)
        {
            sColumnCount++;
            if (sColumn->defaultValue != NULL)
            {
                IDE_TEST(qdbCommon::convertDefaultValueType(
                             aStatement,
                             &sColumn->basicInfo->type,
                             sColumn->defaultValue,
                             NULL)
                         != IDE_SUCCESS);
            }
            sColumn = sColumn->next;
        }

        sPartType = qdbCommon::getTableTypeFromTBSID( sPartAttr->TBSAttr.mID );

        /* PROJ-2464 hybrid partitioned table 지원 */
        qdbCommon::adjustPhysicalAttr( sPartType,
                                       sParseTree->segAttr,
                                       sParseTree->segStoAttr,
                                       & sSegAttr,
                                       & sSegStoAttr,
                                       ID_TRUE /* aIsTable */ );

        IDE_TEST(qdbCommon::createTableOnSM(aStatement,
                                            sPartAttr->columns,
                                            sParseTree->userID,
                                            aTableID,
                                            sParseTree->maxrows,
                                            sPartAttr->TBSAttr.mID,
                                            sSegAttr,
                                            sSegStoAttr,
                                            sParseTree->tableAttrMask,
                                            sParseTree->tableAttrValue,
                                            sParseTree->parallelDegree,
                                            &sPartitionOID)
                 != IDE_SUCCESS);

        //-----------------------------------------------------
        // 1-4. SYS_TABLE_PARTITIONS_ 메타 테이블에 파티션 정보 입력
        //-----------------------------------------------------
        IDE_TEST(qdbCommon::insertTablePartitionSpecIntoMeta(aStatement,
                                     sParseTree->userID,
                                     aTableID,
                                     sPartitionOID,
                                     sPartitionID,
                                     sPartAttr->tablePartName,
                                     sPartMinVal,
                                     sPartMaxVal,
                                     sPartOrder,
                                     sPartAttr->TBSAttr.mID,
                                     sPartAttr->accessOption)   /* PROJ-2359 Table/Partition Access Option */
                 != IDE_SUCCESS);

        //-----------------------------------------------------
        // 1-5. SYS_PART_LOBS_ 메타 테이블에 파티션 정보 입력
        //-----------------------------------------------------
        IDE_TEST(qdbCommon::insertPartLobSpecIntoMeta(aStatement,
                                                      sParseTree->userID,
                                                      aTableID,
                                                      sPartitionID,
                                                      sPartAttr->columns )
                 != IDE_SUCCESS);

        //-----------------------------------------------------
        // 1-6. 테이블 파티션 메타 캐시 생성
        //-----------------------------------------------------
        IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo(
                      QC_SMI_STMT( aStatement ),
                      sPartitionID,
                      sPartitionOID,
                      aTableInfo,
                      NULL )
                  != IDE_SUCCESS );

        IDE_TEST(qcmPartition::getPartitionInfoByID(
                     aStatement,
                     sPartitionID,
                     & sPartitionInfo,
                     & sSCN,
                     & sPartitionHandle)
                 != IDE_SUCCESS);

        IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                             sPartitionHandle,
                                                             sSCN,
                                                             SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                             SMI_TABLE_LOCK_X,
                                                             smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sPartitionInfo != NULL)
    {
        (void)qcmPartition::destroyQcmPartitionInfo(sPartitionInfo);
    }

    return IDE_FAILURE;
}

IDE_RC qdbCreate::executeCreateTableAsSelect(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *      CREATE TABLE ... AS SELECT 문의 execution 수행 함수
 *
 * Implementation :
 *      1. convertDefaultValueType
 *      2. 새로운 테이블을 위한 TableID 구하기
 *      3. createTableOnSM
 *      4. SYS_TABLES_ 메타 테이블에 테이블 정보 입력
 *      5. SYS_COLUMNS_ 메타 테이블에 테이블 컬럼 정보 입력
 *      6. primary key, not null constraint 생성
 *      7. 메타 캐쉬 수행
 *      8. insertRow
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::executeCreateTableAsSelect"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qdTableParseTree    * sParseTree;
    smiValue            * sInsertedRow;
    smiValue            * sSmiValues = NULL;
    smiValue              sInsertedRowForPartition[QC_MAX_COLUMN_COUNT];
    qdConstraintSpec    * sConstraint;
    smOID                 sTableOID;
    UInt                  sTableID;
    qmcRowFlag            sRowFlag;
    smSCN                 sSCN;
    qcmTableInfo        * sTempTableInfo = NULL;
    smiTableCursor      * sCursor;
    smiTableCursor      * sPartitionCursor;
    qcmColumn           * sColumn;
    SInt                  sStage = 0;
    UInt                  sColumnCount = 0;
    UInt                  sLobColumnCount = 0;
    UInt                  i;
    iduMemoryStatus       sQmxMemStatus;
    smiCursorProperties   sCursorProperty;
    void                * sTableHandle;
    qmxLobInfo          * sLobInfo = NULL;
    void                * sRow = NULL;
    scGRID                sRowGRID;
    idBool                sIsPartitioned = ID_FALSE;
    qcmColumn           * sPartKeyColumns;
    UInt                  sPartKeyCount;
    qcmPartitionInfoList* sPartInfoList     = NULL;
    qcmPartitionInfoList* sOldPartInfoList  = NULL;
    qcmPartitionInfoList* sTempPartInfoList = NULL;
    qcmTableInfo        * sPartInfo;
    UInt                  sPartCount = 0;
    qcmTemporaryType      sTemporaryType;
    qdIndexTableList    * sIndexTable;

    qmsTableRef         * sTableRef = NULL;
    qmsPartitionRef     * sPartitionRef = NULL;
    qmsPartitionRef     * sFirstPartitionRef = NULL;
    qmcInsertCursor       sInsertCursorMgr;
    qdPartitionAttribute* sPartAttr;
    
    qdIndexTableCursors   sIndexTableCursorInfo;
    idBool                sInitedCursorInfo = ID_FALSE;
    smOID                 sPartOID;

    qcmTableInfo        * sSelectedPartitionInfo = NULL;

    sRowFlag = QMC_ROW_INITIALIZE;

    SMI_CURSOR_PROP_INIT_FOR_FULL_SCAN( &sCursorProperty, aStatement->mStatistics );

    sCursorProperty.mIsUndoLogging = ID_FALSE;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    for ( sColumn = sParseTree->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        sColumnCount++;

        if (sColumn->defaultValue != NULL)
        {
            IDE_TEST(qdbCommon::convertDefaultValueType(
                         aStatement,
                         &sColumn->basicInfo->type,
                         sColumn->defaultValue,
                         NULL)
                     != IDE_SUCCESS);
        }

        // PROJ-1362
        if ( (sColumn->basicInfo->module->flag & MTD_COLUMN_TYPE_MASK)
             == MTD_COLUMN_TYPE_LOB )
        {
            sLobColumnCount++;
        }
        else
        {
            // Nothing to do.
        }

        // TODO : Source의 LOB Column 수를 정확하게 지정하면, 메모리 사용량을 줄일 수 있다.
        sLobColumnCount++;
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    // 논파티션드 테이블 생성
    if( sParseTree->partTable == NULL )
    {
        sIsPartitioned = ID_FALSE;
    }
    // 파티션드 테이블 생성
    else
    {
        sIsPartitioned = ID_TRUE;

        sPartKeyCount = 0;

        for( sPartKeyColumns = sParseTree->partTable->partKeyColumns;
             sPartKeyColumns != NULL;
             sPartKeyColumns = sPartKeyColumns->next )
        {
            sPartKeyCount++;
        }
    }

    // PROJ-1407 Temporary Table
    if ( sParseTree->temporaryOption == NULL )
    {
        if ( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK ) ==
             QDT_CREATE_TEMPORARY_TRUE )
        {
            sTemporaryType = QCM_TEMPORARY_ON_COMMIT_DELETE_ROWS;
        }
        else
        {
            sTemporaryType = QCM_TEMPORARY_ON_COMMIT_NONE;
        }
    }
    else
    {
        sTemporaryType = sParseTree->temporaryOption->temporaryType;
    }

    // PROJ-1362
    IDE_TEST( qmx::initializeLobInfo(aStatement,
                                     & sLobInfo,
                                     (UShort)sLobColumnCount)
              != IDE_SUCCESS );

    /* BUG-36211 각 Row Insert 후, 해당 Lob Cursor를 바로 해제합니다. */
    qmx::setImmediateCloseLobInfo( sLobInfo, ID_TRUE );

    IDE_TEST(qcm::getNextTableID(aStatement, &sTableID) != IDE_SUCCESS);

    IDE_TEST(qdbCommon::createTableOnSM(aStatement,
                                        sParseTree->columns,
                                        sParseTree->userID,
                                        sTableID,
                                        sParseTree->maxrows,
                                        sParseTree->TBSAttr.mID,
                                        sParseTree->segAttr,
                                        sParseTree->segStoAttr,
                                        sParseTree->tableAttrMask,
                                        sParseTree->tableAttrValue,
                                        sParseTree->parallelDegree,
                                        &sTableOID)
             != IDE_SUCCESS);

    IDE_TEST(qdbCommon::insertTableSpecIntoMeta(aStatement,
                                                sIsPartitioned,
                                                sParseTree->flag,
                                                sParseTree->tableName,
                                                sParseTree->userID,
                                                sTableOID,
                                                sTableID,
                                                sColumnCount,
                                                sParseTree->maxrows,
                                                sParseTree->accessOption, /* PROJ-2359 Table/Partition Access Option */
                                                sParseTree->TBSAttr.mID,
                                                sParseTree->segAttr,
                                                sParseTree->segStoAttr,
                                                sTemporaryType,
                                                sParseTree->parallelDegree,      // PROJ-1071
                                                sParseTree->mShardFlag)          // TASK-7307
             != IDE_SUCCESS);

    IDE_TEST(qdbCommon::insertColumnSpecIntoMeta(aStatement,
                                                 sParseTree->userID,
                                                 sTableID,
                                                 sParseTree->columns,
                                                 ID_FALSE /* is queue */)
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sIsPartitioned == ID_TRUE )
    {
        IDE_TEST(qdbCommon::insertPartTableSpecIntoMeta(aStatement,
                                                        sParseTree->userID,
                                                        sTableID,
                                                        sParseTree->partTable->partMethod,
                                                        sPartKeyCount,
                                                        sParseTree->isRowmovement )
                 != IDE_SUCCESS);

        IDE_TEST(qdbCommon::insertPartKeyColumnSpecIntoMeta(aStatement,
                                                            sParseTree->userID,
                                                            sTableID,
                                                            sParseTree->columns,
                                                            sParseTree->partTable->partKeyColumns,
                                                            QCM_TABLE_OBJECT_TYPE)
                 != IDE_SUCCESS);

        IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                             sTableID,
                                             sTableOID) != IDE_SUCCESS);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sTableID,
                                       &sTempTableInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);

        IDE_TEST( executeCreateTablePartition( aStatement,
                                               sTableID,
                                               sTempTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      aStatement->qmxMem,
                                                      sTableID,
                                                      & sTempPartInfoList )
                  != IDE_SUCCESS );

        // 모든 파티션에 LOCK(X)
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sTempPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sParseTree->partTable->partInfoList = sTempPartInfoList;
    }

    for (sConstraint = sParseTree->constraints;
         sConstraint != NULL;
         sConstraint = sConstraint->next)
    {
        fillColumnID( sParseTree->columns, sConstraint );

        IDE_TEST(qdbCommon::createConstrPrimaryUnique(aStatement,
                                                      sTableOID,
                                                      sConstraint,
                                                      sParseTree->userID,
                                                      sTableID,
                                                      sTempPartInfoList,
                                                      sParseTree->maxrows )
                 != IDE_SUCCESS);

        IDE_TEST(qdbCommon::createConstrNotNull(aStatement,
                                                sConstraint,
                                                sParseTree->userID,
                                                sTableID)
                 != IDE_SUCCESS);

        IDE_TEST(qdbCommon::createConstrTimeStamp(aStatement,
                                                  sConstraint,
                                                  sParseTree->userID,
                                                  sTableID)
                 != IDE_SUCCESS);
    }

    if( sTempTableInfo != NULL )
    {
        (void)qcm::destroyQcmTableInfo(sTempTableInfo);
    }
    sTempTableInfo = NULL;

    IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                         sTableID,
                                         sTableOID) != IDE_SUCCESS);

    IDE_TEST(qcm::getTableInfoByID(aStatement,
                                   sTableID,
                                   &sTempTableInfo,
                                   &sSCN,
                                   &sTableHandle)
             != IDE_SUCCESS);

    IDE_TEST(qcm::validateAndLockTable(aStatement,
                                       sTableHandle,
                                       sSCN,
                                       SMI_TABLE_LOCK_X)
             != IDE_SUCCESS);

    for (sConstraint = sParseTree->constraints;
         sConstraint != NULL;
         sConstraint = sConstraint->next)
    {
        if (sConstraint->constrType == QD_FOREIGN)
        {
            (void)qcm::destroyQcmTableInfo(sTempTableInfo);
            sTempTableInfo = NULL;

            IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT(aStatement),
                                                 sTableID,
                                                 sTableOID) != IDE_SUCCESS);

            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                           sTableID,
                                           &sTempTableInfo,
                                           &sSCN,
                                           &sTableHandle)
                     != IDE_SUCCESS);

            IDE_TEST(qdbCommon::createConstrForeign(aStatement,
                                                    sConstraint,
                                                    sParseTree->userID,
                                                    sTableID) != IDE_SUCCESS);
        }

        if (sConstraint->constrType == QD_NOT_NULL)
        {
            for (i = 0; i < sConstraint->constrColumnCount; i++)
            {
                sColumn = sConstraint->constraintColumns;

                IDE_TEST(qdbCommon::makeColumnNotNull(
                                aStatement,
                                sTempTableInfo->tableHandle,
                                sParseTree->maxrows,
                                sTempPartInfoList,
                                sIsPartitioned,
                                sColumn->basicInfo->column.id)
                         != IDE_SUCCESS);

                sColumn = sColumn->next;
            }
        }
    }

    if (sTempTableInfo->primaryKey != NULL)
    {
        for ( i = 0; i < sTempTableInfo->primaryKey->keyColCount; i++)
        {
            IDE_TEST(qdbCommon::makeColumnNotNull(
                         aStatement,
                         sTempTableInfo->tableHandle,
                         sParseTree->maxrows,
                         sTempPartInfoList,
                         sIsPartitioned,
                         sTempTableInfo->primaryKey->keyColumns[i].column.id)
                     != IDE_SUCCESS);
        }
    }

    (void)qcm::destroyQcmTableInfo( sTempTableInfo );
    sTempTableInfo = NULL;

    IDE_TEST(qcm::makeAndSetQcmTableInfo(QC_SMI_STMT( aStatement ),
                                         sTableID,
                                         sTableOID) != IDE_SUCCESS);

    IDE_TEST(qcm::getTableInfoByID(aStatement,
                                   sTableID,
                                   &sTempTableInfo,
                                   &sSCN,
                                   &sTableHandle)
             != IDE_SUCCESS);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sIsPartitioned == ID_TRUE )
    {
        sOldPartInfoList = sTempPartInfoList;
        IDE_TEST( qcmPartition::makeAndSetAndGetQcmPartitionInfoList( aStatement,
                                                                      sTempTableInfo,
                                                                      sOldPartInfoList,
                                                                      & sTempPartInfoList )
                  != IDE_SUCCESS );

        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }

    IDU_FIT_POINT( "qdbCreate::executeCreateTableAsSelect::alloc::sInsertedRow",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(aStatement->qmxMem->alloc(ID_SIZEOF(smiValue) * sColumnCount,
                                       (void**)&sInsertedRow)
             != IDE_SUCCESS);

    // BUG-16535
    // set SYSDATE
    IDE_TEST( qtc::setDatePseudoColumn( QC_PRIVATE_TMPLATE( aStatement ) ) != IDE_SUCCESS );

    // check session cache SEQUENCE.CURRVAL
    if (sParseTree->select->myPlan->parseTree->currValSeqs != NULL)
    {
        IDE_TEST(qmx::findSessionSeqCaches(aStatement,
                                           sParseTree->select->myPlan->parseTree)
                 != IDE_SUCCESS);
    }

    // get SEQUENCE.NEXTVAL
    if (sParseTree->select->myPlan->parseTree->nextValSeqs != NULL)
    {
        IDE_TEST(qmx::addSessionSeqCaches(aStatement,
                                          sParseTree->select->myPlan->parseTree)
                 != IDE_SUCCESS);

        ((qmncPROJ *)(sParseTree->select->myPlan->plan))->nextValSeqs =
            sParseTree->select->myPlan->parseTree->nextValSeqs;
    }

    IDE_TEST(qmnPROJ::init( QC_PRIVATE_TMPLATE(aStatement), sParseTree->select->myPlan->plan)
             != IDE_SUCCESS);

    IDU_LIMITPOINT("qdbCreate::executeCreateTableAsSelect::malloc2");
    IDE_TEST(STRUCT_ALLOC(aStatement->qmxMem,
                          qmsTableRef,
                          (void**)& sTableRef)
             != IDE_SUCCESS);
    QCP_SET_INIT_QMS_TABLE_REF( sTableRef );
    
    sTableRef->userName = sParseTree->userName;
    sTableRef->tableName = sParseTree->tableName;
    sTableRef->userID = sParseTree->userID;
    sTableRef->tableInfo = sTempTableInfo;
    sTableRef->tableHandle = sTempTableInfo->tableHandle;
    sTableRef->tableSCN = smiGetRowSCN( sTempTableInfo->tableHandle );

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sIsPartitioned == ID_FALSE )
    {
        IDE_TEST( sInsertCursorMgr.initialize( aStatement->qmxMem,
                                               sTableRef,
                                               ID_TRUE,
                                               ID_TRUE ) /* init all partitions */
                  != IDE_SUCCESS );
        
        IDE_TEST( sInsertCursorMgr.openCursor( aStatement,
                                               SMI_LOCK_WRITE|
                                               SMI_TRAVERSE_FORWARD|
                                               SMI_PREVIOUS_DISABLE,
                                               & sCursorProperty )
                  != IDE_SUCCESS );

        sStage = 1;

        IDE_TEST(qmnPROJ::doIt( QC_PRIVATE_TMPLATE(aStatement),
                                sParseTree->select->myPlan->plan,
                                &sRowFlag)
                 != IDE_SUCCESS);

        /* PROJ-2359 Table/Partition Access Option */
        if ( (sRowFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( qmx::checkAccessOption( sTempTableInfo,
                                              ID_TRUE, /* aIsInsertion */
                                              QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        // To fix BUG-13978
        // bitwise operation(&) for flag check
        while ((sRowFlag & QMC_ROW_DATA_MASK) ==  QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( QC_QMX_MEM(aStatement)-> getStatus( &sQmxMemStatus )
                      != IDE_SUCCESS);

            (void)qmx::clearLobInfo( sLobInfo );

            IDE_TEST(qmx::makeSmiValueWithResult(sParseTree->columns,
                                                 QC_PRIVATE_TMPLATE(aStatement),
                                                 sTempTableInfo,
                                                 sInsertedRow,
                                                 sLobInfo )
                     != IDE_SUCCESS);

            // To fix BUG-12622
            // not null체크 함수를 makeSmiValueWithResult에서 따로 분리하였음
            IDE_TEST( qmx::checkNotNullColumnForInsert(
                          sTempTableInfo->columns,
                          sInsertedRow,
                          sLobInfo,
                          ID_FALSE )
                      != IDE_SUCCESS );

            //------------------------------------------
            // INSERT를 수행
            //------------------------------------------
            
            IDE_TEST( sInsertCursorMgr.getCursor( &sCursor )
                      != IDE_SUCCESS );

            IDE_TEST( sCursor->insertRow( sInsertedRow,
                                          &sRow,
                                          &sRowGRID )
                      != IDE_SUCCESS);
            
            IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                  sLobInfo,
                                                  sCursor,
                                                  sRow,
                                                  sRowGRID )
                      != IDE_SUCCESS );

            IDE_TEST( QC_QMX_MEM(aStatement)-> setStatus( &sQmxMemStatus )
                      != IDE_SUCCESS );

            IDE_TEST(qmnPROJ::doIt( QC_PRIVATE_TMPLATE(aStatement),
                                    sParseTree->select->myPlan->plan,
                                    &sRowFlag)
                     != IDE_SUCCESS);
        }

        sStage = 0;

        IDE_TEST( sInsertCursorMgr.closeCursor() != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------------
        // open partition insert cursors
        //------------------------------------------
        
        // partitionRef 구성
        for( sPartInfoList = sTempPartInfoList, sPartCount = 0;
             sPartInfoList != NULL;
             sPartInfoList = sPartInfoList->next,
                 sPartCount++ )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            IDU_LIMITPOINT("qdbCreate::executeCreateTableAsSelect::malloc3");
            IDE_TEST(STRUCT_ALLOC(aStatement->qmxMem,
                                  qmsPartitionRef,
                                  (void**)& sPartitionRef)
                     != IDE_SUCCESS);
            QCP_SET_INIT_QMS_PARTITION_REF( sPartitionRef );

            sPartitionRef->partitionID = sPartInfo->partitionID;
            sPartitionRef->partitionInfo = sPartInfo;
            sPartitionRef->partitionSCN = sPartInfoList->partSCN;
            sPartitionRef->partitionHandle = sPartInfoList->partHandle;

            // BUG-48782
            for ( sPartAttr = sParseTree->partTable->partAttr;
                  sPartAttr != NULL;
                  sPartAttr = sPartAttr->next )
            {
                // BUG-37032
                if ( idlOS::strMatch( sPartAttr->tablePartName.stmtText + sPartAttr->tablePartName.offset,
                                      sPartAttr->tablePartName.size,
                                      sPartInfo->name,
                                      idlOS::strlen(sPartInfo->name) ) == 0 )
                {
                    break;
                }

            }

            if(sParseTree->partTable->partMethod != QCM_PARTITION_METHOD_HASH)
            {
                sPartitionRef->minPartCondVal =
                    *(sPartAttr->alterPart->partCondMinVal);
                sPartitionRef->maxPartCondVal =
                    *(sPartAttr->alterPart->partCondMaxVal);

                // minValue의 partCondValType 조정
                if( sPartitionRef->minPartCondVal.partCondValCount == 0 )
                {
                    sPartitionRef->minPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_MIN;
                }
                else
                {
                    sPartitionRef->minPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_NORMAL;
                }

                // maxValue의 partCondValType 조정
                if( sPartitionRef->maxPartCondVal.partCondValCount == 0 )
                {
                    sPartitionRef->maxPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_DEFAULT;
                }
                else
                {
                    sPartitionRef->maxPartCondVal.partCondValType
                        = QMS_PARTCONDVAL_NORMAL;
                }
            }
            else
            {
                sPartitionRef->partOrder = sPartAttr->partOrder;
            }

            if( sFirstPartitionRef == NULL )
            {
                sPartitionRef->next = NULL;
                sFirstPartitionRef = sPartitionRef;
            }
            else
            {
                sPartitionRef->next = sFirstPartitionRef;
                sFirstPartitionRef = sPartitionRef;
            }
        }

        sTableRef->partitionRef = sFirstPartitionRef;
        sTableRef->partitionCount = sPartCount;

        /* PROJ-2464 hybrid partitioned table 지원 */
        IDE_TEST( qcmPartition::makePartitionSummary( aStatement, sTableRef )
                  != IDE_SUCCESS );

        IDE_TEST( sInsertCursorMgr.initialize( aStatement->qmxMem,
                                               sTableRef,
                                               ID_TRUE,
                                               ID_TRUE ) /* init all partitions */
                  != IDE_SUCCESS );

        IDE_TEST( sInsertCursorMgr.openCursor( aStatement,
                                               SMI_LOCK_WRITE|
                                               SMI_TRAVERSE_FORWARD|
                                               SMI_PREVIOUS_DISABLE,
                                               & sCursorProperty )
                  != IDE_SUCCESS );

        sStage = 1;
        
        //------------------------------------------
        // open index table insert cursors
        //------------------------------------------
        
        if ( sParseTree->newIndexTables != NULL )
        {
            IDE_TEST( qdx::initializeInsertIndexTableCursors(
                          aStatement,
                          sParseTree->newIndexTables,
                          &sIndexTableCursorInfo,
                          sTempTableInfo->indices,
                          sTempTableInfo->indexCount,
                          sColumnCount,
                          &sCursorProperty )
                      != IDE_SUCCESS );
            
            sInitedCursorInfo = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
        
        //------------------------------------------
        // SELECT를 수행
        //------------------------------------------
        
        IDE_TEST(qmnPROJ::doIt( QC_PRIVATE_TMPLATE(aStatement),
                                sParseTree->select->myPlan->plan,
                                &sRowFlag)
                 != IDE_SUCCESS)

        /* PROJ-2359 Table/Partition Access Option */
        if ( (sRowFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( qmx::checkAccessOption( sTempTableInfo,
                                              ID_TRUE, /* aIsInsertion */
                                              QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        // To fix BUG-13978
        // bitwise operation(&) for flag check
        while ((sRowFlag & QMC_ROW_DATA_MASK) ==  QMC_ROW_DATA_EXIST )
        {
            IDE_TEST( QC_QMX_MEM(aStatement)-> getStatus( &sQmxMemStatus )
                      != IDE_SUCCESS);

            (void)qmx::clearLobInfo( sLobInfo );

            IDE_TEST(qmx::makeSmiValueWithResult(sParseTree->columns,
                                                 QC_PRIVATE_TMPLATE(aStatement),
                                                 sTempTableInfo,
                                                 sInsertedRow,
                                                 sLobInfo )
                     != IDE_SUCCESS);

            // To fix BUG-12622
            // not null체크 함수를 makeSmiValueWithResult에서 따로 분리하였음
            IDE_TEST( qmx::checkNotNullColumnForInsert(
                          sTempTableInfo->columns,
                          sInsertedRow,
                          sLobInfo,
                          ID_FALSE )
                      != IDE_SUCCESS );

            //------------------------------------------
            // INSERT를 수행
            //------------------------------------------

            IDE_TEST( sInsertCursorMgr.partitionFilteringWithRow(
                          sInsertedRow,
                          sLobInfo,
                          &sSelectedPartitionInfo )
                      != IDE_SUCCESS );

            /* PROJ-2359 Table/Partition Access Option */
            IDE_TEST( qmx::checkAccessOption( sSelectedPartitionInfo,
                                              ID_TRUE, /* aIsInsertion */
                                              QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) )
                      != IDE_SUCCESS );

            IDE_TEST( sInsertCursorMgr.getCursor( &sPartitionCursor )
                      != IDE_SUCCESS );

            if ( QCM_TABLE_TYPE_IS_DISK( sTempTableInfo->tableFlag ) !=
                 QCM_TABLE_TYPE_IS_DISK( sSelectedPartitionInfo->tableFlag ) )
            {
                /* PROJ-2464 hybrid partitioned table 지원
                 * Partitioned Table을 기준으로 만든 smiValue Array를 Table Partition에 맞게 변환한다.
                 */
                IDE_TEST( qmx::makeSmiValueWithSmiValue( sTempTableInfo,
                                                         sSelectedPartitionInfo,
                                                         sTempTableInfo->columns,
                                                         sColumnCount,
                                                         sInsertedRow,
                                                         sInsertedRowForPartition )
                          != IDE_SUCCESS );

                sSmiValues = sInsertedRowForPartition;
            }
            else
            {
                sSmiValues = sInsertedRow;
            }

            IDE_TEST(sPartitionCursor->insertRow( sSmiValues,
                                                  & sRow,
                                                  & sRowGRID )
                     != IDE_SUCCESS);

            //------------------------------------------
            // INSERT를 수행후 Lob 컬럼을 처리
            //------------------------------------------
            
            IDE_TEST( qmx::copyAndOutBindLobInfo( aStatement,
                                                  sLobInfo,
                                                  sPartitionCursor,
                                                  sRow,
                                                  sRowGRID )
                      != IDE_SUCCESS );
            
            //------------------------------------------
            // INSERT를 수행후 non-partitioned index를 처리
            //------------------------------------------

            if ( sParseTree->newIndexTables != NULL )
            {
                IDE_TEST( sInsertCursorMgr.getSelectedPartitionOID( & sPartOID )
                          != IDE_SUCCESS );
                
                IDE_TEST( qdx::insertIndexTableCursors( &sIndexTableCursorInfo,
                                                        sInsertedRow,
                                                        sPartOID,
                                                        sRowGRID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
            
            IDE_TEST( QC_QMX_MEM(aStatement)-> setStatus( &sQmxMemStatus )
                      != IDE_SUCCESS );

            IDE_TEST(qmnPROJ::doIt( QC_PRIVATE_TMPLATE(aStatement),
                                    sParseTree->select->myPlan->plan,
                                    &sRowFlag)
                     != IDE_SUCCESS);
        }

        sStage = 0;

        IDE_TEST(sInsertCursorMgr.closeCursor() != IDE_SUCCESS);

        // close index table cursor
        if ( sParseTree->newIndexTables != NULL )
        {
            IDE_TEST( qdx::closeInsertIndexTableCursors(
                          &sIndexTableCursorInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
       DDL Statement Text의 로깅
    */
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sTempTableInfo->name )
                  != IDE_SUCCESS );

        IDE_TEST(qci::mManageReplicationCallback.mWriteTableMetaLog(
                                                               aStatement,
                                                               0,
                                                               sTableOID)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sInsertCursorMgr.closeCursor();
    }

    if ( sInitedCursorInfo == ID_TRUE )
    {
        qdx::finalizeInsertIndexTableCursors(
            &sIndexTableCursorInfo );
    }
    else
    {
        // Nothing to do.
    }
            
    (void)qcm::destroyQcmTableInfo( sTempTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sTempPartInfoList );

    for ( sIndexTable = sParseTree->newIndexTables;
          sIndexTable != NULL;
          sIndexTable = sIndexTable->next )
    {
        (void)qcm::destroyQcmTableInfo(sIndexTable->tableInfo);
    }
    
    (void)qmx::finalizeLobInfo( aStatement, sLobInfo );

    return IDE_FAILURE;

#undef IDE_FN
}

void qdbCreate::fillColumnID(
    qcmColumn        *columns,
    qdConstraintSpec *constraints)
{
/***********************************************************************
 *
 * Description :
 *    전체 컬럼의 리스트에서 constraint 를 구성하는 컬럼을 찾아서 id 부여
 *
 * Implementation :
 *    1. constraint 를 구성하는 컬럼의 이름과 같은 것을 전체 컬럼중에서
 *       찾은 후에 그 ID 를 qdConstraintSpec 의 컬럼 ID 에 부여
 *    2. constraint 타입이 foreign key 이고 참조하는 테이블이 self 테이블이면
 *       1 의 과정처럼 거쳐서 ID 를 부여
 *
 ***********************************************************************/

#define IDE_FN "qdbCreate::fillColumnID"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcmColumn        *sTableColumn;
    qcmColumn        *sConstraintColumn;
    qcmColumn        *sReferencedColumn;
    qdConstraintSpec *sConstraint;

    sConstraint = constraints;
    while (sConstraint != NULL)
    {
        for (sConstraintColumn = sConstraint->constraintColumns;
             sConstraintColumn != NULL;
             sConstraintColumn = sConstraintColumn->next)
        {
            for (sTableColumn = columns;
                 sTableColumn != NULL;
                 sTableColumn = sTableColumn->next)
            {
                if (sConstraintColumn->namePos.size > 0)
                {
                    if ( idlOS::strMatch( sTableColumn->namePos.stmtText + sTableColumn->namePos.offset,
                                          sTableColumn->namePos.size,
                                          sConstraintColumn->namePos.stmtText + sConstraintColumn->namePos.offset,
                                          sConstraintColumn->namePos.size) == 0 )
                    {
                        // found
                        sConstraintColumn->basicInfo->column.id = sTableColumn->basicInfo->column.id;
                        break;
                    }
                }
                else
                {
                    if ( idlOS::strMatch( sTableColumn->name,
                                          idlOS::strlen(sTableColumn->name),
                                          sConstraintColumn->name,
                                          idlOS::strlen(sConstraintColumn->name) ) == 0 )
                    {
                        // found
                        sConstraintColumn->basicInfo->column.id = sTableColumn->basicInfo->column.id;
                        break;
                    }
                }
            }
        }

        if (sConstraint->constrType == QD_FOREIGN)
        {
            if ( sConstraint->referentialConstraintSpec->referencedTableID
                 == ID_UINT_MAX ) // self
            {
                sReferencedColumn = sConstraint->referentialConstraintSpec->referencedColList;
                while (sReferencedColumn != NULL)
                {
                    sTableColumn = columns;
                    while (sTableColumn != NULL)
                    {
                        if ( idlOS::strMatch( sTableColumn->namePos.stmtText + sTableColumn->namePos.offset,
                                              sTableColumn->namePos.size,
                                              sReferencedColumn->namePos.stmtText + sReferencedColumn->namePos.offset,
                                              sReferencedColumn->namePos.size) == 0 )
                        {
                            sReferencedColumn->basicInfo->column.id = sTableColumn->basicInfo->column.id;
                        }

                        sTableColumn = sTableColumn->next;
                    }
                    sReferencedColumn = sReferencedColumn->next;
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        sConstraint = sConstraint->next;
    }

#undef IDE_FN
}


/*  Table의 Attribute Flag List로부터
    32bit Flag값을 계산

    [IN] qcStatement  - 수행중인 Statement
    [IN] aCreateTable - Create Table의 Parse Tree
 */
IDE_RC qdbCreate::calculateTableAttrFlag( qcStatement      * aStatement,
                                          qdTableParseTree * aCreateTable )
{
    qdPartitionAttribute * sPartAttr    = NULL;
    UInt                   sLoggingMode = 0;

    if ( aCreateTable->tableAttrFlagList != NULL )
    {
        IDE_TEST( qdbCommon::validateTableAttrFlagList(
                      aStatement,
                      aCreateTable->tableAttrFlagList )
                  != IDE_SUCCESS );

        IDE_TEST( qdbCommon::getTableAttrFlagFromList(
                      aCreateTable->tableAttrFlagList,
                      & aCreateTable->tableAttrMask,
                      & aCreateTable->tableAttrValue )
                  != IDE_SUCCESS );
    }
    else
    {

        // Tablespace Attribute List가 지정되지 않은 경우
        // 기본값으로 설정
        //
        // Tablespace Attribute값은 기본값으로 사용될 값에 대해
        // Bit 0를 사용하도록 구성된다.
        //
        // Mask를 0으로 세팅해두면 Create Table시
        // Table의 flag를 건드리지 않고 0으로 남겨둔다.
        aCreateTable->tableAttrMask   = 0;
        aCreateTable->tableAttrValue  = 0;
    }

    // PROJ-1665 : logging / no logging mode가 주어진 경우,
    // 이를 설정함
    if ( aCreateTable->loggingMode != NULL )
    {
        sLoggingMode = *(aCreateTable->loggingMode);
        aCreateTable->tableAttrMask = aCreateTable->tableAttrMask |
                                      SMI_TABLE_LOGGING_MASK;
        aCreateTable->tableAttrValue = aCreateTable->tableAttrValue |
                                       sLoggingMode;
    }
    else
    {
        // nothing to do
    }

    // Volatile Tablespace의 경우 별도 체크 실시
    if ( smiTableSpace::isVolatileTableSpace( aCreateTable->TBSAttr.mID )
         == ID_TRUE )
    {
        // COMPRESSED LOGGING 구문 사용시 에러
        IDE_TEST( checkError4CreateVolatileTable(aCreateTable)
                  != IDE_SUCCESS );

        // 로그 압축 하지 않도록 기본값 설정
        aCreateTable->tableAttrValue &= ~SMI_TABLE_LOG_COMPRESS_MASK;
        aCreateTable->tableAttrValue |=  SMI_TABLE_LOG_COMPRESS_FALSE;
    }

    /* PROJ-2464 hybrid partitioned table 지원 */
    if ( aCreateTable->partTable != NULL )
    {
        for ( sPartAttr = aCreateTable->partTable->partAttr;
              sPartAttr != NULL;
              sPartAttr = sPartAttr->next )
        {
            if ( smiTableSpace::isVolatileTableSpace( sPartAttr->TBSAttr.mID )
                 == ID_TRUE )
            {
                // COMPRESSED LOGGING 구문 사용시 에러
                IDE_TEST( checkError4CreateVolatileTable( aCreateTable )
                          != IDE_SUCCESS );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Volatile Tablespace에 Table생성중 수행하는 에러처리

   => Volatile Tablespace의 경우 Log Compression지원하지 않는다.
      Create Table구문에 COMPRESSED LOGGING
      절을 사용한 경우 에러

   [IN] aAttrFlagList - Tablespace Attribute Flag의 List
*/
IDE_RC qdbCreate::checkError4CreateVolatileTable(
                      qdTableParseTree * aCreateTable )
{
    IDE_DASSERT( aCreateTable != NULL );

    qdTableAttrFlagList * sAttrFlagList = aCreateTable->tableAttrFlagList;
    
    for ( ; sAttrFlagList != NULL ; sAttrFlagList = sAttrFlagList->next )
    {
        if ( (sAttrFlagList->attrMask & SMI_TABLE_LOG_COMPRESS_MASK) != 0 )
        {
            if ( (sAttrFlagList->attrValue & SMI_TABLE_LOG_COMPRESS_MASK )
                 == SMI_TABLE_LOG_COMPRESS_TRUE )
            {
                IDE_RAISE( err_volatile_log_compress );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_volatile_log_compress );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCreate::validateTemporaryTable( qcStatement      * aStatement,
                                          qdTableParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description :
 *    create Temporary table 검증 함수
 *
 * Implementation :
 *     1. temporary table은 volatile tbs에서만 생성할 수 있다.
 *     2. temporary table에만 on commit option을 사용할 수 있다.
 *
 ***********************************************************************/
    qcuSqlSourceInfo  sqlInfo;

    IDE_DASSERT( aParseTree != NULL );

    // PROJ-1407 Temporary Table
    if ( ( aParseTree->flag & QDT_CREATE_TEMPORARY_MASK ) == QDT_CREATE_TEMPORARY_TRUE )
    {
        // temporary table은 volatile tbs에서만 생성할 수 있다.
        IDE_TEST_RAISE( smiTableSpace::isVolatileTableSpaceType(aParseTree->TBSAttr.mType)
                        != ID_TRUE,
                        ERR_CANNOT_CREATE_TEMPORARY_TABLE_IN_NONVOLATILE_TBS );
    }
    else
    {
        // temporary table에만 on commit option을 사용할 수 있다.
        if ( aParseTree->temporaryOption != NULL )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aParseTree->temporaryOption->temporaryPos );
            IDE_RAISE( ERR_INVALID_TABLE_OPTION );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CANNOT_CREATE_TEMPORARY_TABLE_IN_NONVOLATILE_TBS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_CANNOT_CREATE_TEMPORARY_TABLE_IN_NONVOLATILE_TBS));
    }
    IDE_EXCEPTION(ERR_INVALID_TABLE_OPTION)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_TABLE_OPTION,
                                sqlInfo.getErrMessage()) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
