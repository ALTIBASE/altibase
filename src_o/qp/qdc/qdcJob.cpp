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
 * $Id: qdcJob.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * PROJ-1438 Job Scheduler
 *
 **********************************************************************/
#include <qcg.h>
#include <qdcJob.h>
#include <qdbCommon.h>
#include <qcmJob.h>
#include <qcuSqlSourceInfo.h>
#include <qdpRole.h>
/**
 * validate Create Job
 */
IDE_RC qdcJob::validateCreateJob( qcStatement * aStatement )
{
    qdJobParseTree * sParseTree;
    idBool           sIsExist;
    qcuSqlSourceInfo sqlInfo;

    IDE_DASSERT( aStatement != NULL );

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    // BUG-41408 normal user create, alter, drop job
    IDE_TEST( qdpRole::checkDDLCreateAnyJobPriv( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qcmJob::isExistJob( QC_SMI_STMT( aStatement ),
                                  &sParseTree->jobName,
                                  &sIsExist )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsExist == ID_TRUE, ERR_JOB_NAME );

    if ( sParseTree->exec.size > QDC_JOB_EXEC_MAX_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->exec );
        IDE_RAISE( ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->start.size > QDC_JOB_DATE_MAX_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->start );
        IDE_RAISE( ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->end.size > QDC_JOB_DATE_MAX_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->end );
        IDE_RAISE( ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_JOB_NAME)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_LENGTH,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::validateAlterJob( qcStatement * aStatement )
{
    qdJobParseTree   * sParseTree;
    idBool             sIsExist;
    qcuSqlSourceInfo   sqlInfo;

    IDE_DASSERT( aStatement != NULL );

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    // BUG-41408 normal user create, alter, drop job
    IDE_TEST( qdpRole::checkDDLAlterAnyJobPriv( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qcmJob::isExistJob( QC_SMI_STMT( aStatement ),
                                  &sParseTree->jobName,
                                  &sIsExist )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsExist == ID_FALSE, ERR_NOT_EXIST_OBJECT );

    if ( sParseTree->exec.size > QDC_JOB_EXEC_MAX_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->exec );
        IDE_RAISE( ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->start.size > QDC_JOB_DATE_MAX_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->start );
        IDE_RAISE( ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sParseTree->end.size > QDC_JOB_DATE_MAX_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sParseTree->end );
        IDE_RAISE( ERR_INVALID_LENGTH );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_OBJECT);
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                                  "") );
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_INVALID_LENGTH,
                                sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::validateDropJob( qcStatement * aStatement )
{
    qdJobParseTree   * sParseTree;
    idBool             sIsExist;
    qcuSqlSourceInfo   sqlInfo;

    IDE_DASSERT( aStatement != NULL );

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    // BUG-41408 normal user create, alter, drop job
    IDE_TEST( qdpRole::checkDDLDropAnyJobPriv( aStatement )
              != IDE_SUCCESS );

    IDE_TEST( qcmJob::isExistJob( QC_SMI_STMT( aStatement ),
                                  &sParseTree->jobName,
                                  &sIsExist )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsExist == ID_FALSE, ERR_NOT_EXIST_OBJECT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_OBJECT);
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_NOT_EXISTS_OBJECT,
                                  "") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::executeCreateJob( qcStatement * aStatement )
{
    qdJobParseTree  * sParseTree;
    SChar           * sSqlStr    = NULL;
    vSLong            sRowCnt    = 0;
    SChar             sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar             sExec[ QDC_JOB_EXEC_MAX_LEN + 1 ];
    SChar             sStart[ QDC_JOB_DATE_MAX_LEN + 1 ];
    SChar             sEnd[ QDC_JOB_DATE_MAX_LEN + 1];
    UInt              sJobID;
    SChar             sIntervalType[3];
    SChar             sEnable[2];
    SChar             sComment[ QC_MAX_COMMENT_LITERAL_LEN + 1 ];

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::execCreateJob::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );
    QC_STR_COPY( sStart, sParseTree->start );

    IDE_TEST( makeExecTxt( sExec, &sParseTree->exec )
              != IDE_SUCCESS );

    makeIntervalTypeTxt( sIntervalType, sParseTree->intervalType );

    if ( QC_IS_NULL_NAME( sParseTree->end ) == ID_TRUE )
    {
        idlOS::memcpy( sEnd, "NULL", 4 );
        sEnd[4] = '\0';
    }
    else
    {
        QC_STR_COPY( sEnd, sParseTree->end );
    }

    IDE_TEST( qcm::getNextJobID( aStatement, &sJobID )
              != IDE_SUCCESS );

    // BUG-41713 each job enable disable
    if ( sParseTree->enable == ID_TRUE )
    {
        sEnable[0] = 'T';
        sEnable[1] = '\0';
    }
    else
    {
        sEnable[0] = 'F';
        sEnable[1] = '\0';
    }

    // BUG-41713 each job enable disable
    if ( QC_IS_NULL_NAME( sParseTree->comment ) == ID_TRUE )
    {
        sComment[0] = '\0';
    }
    else
    {
        QC_STR_COPY( sComment, sParseTree->comment );
    }

    if ( sParseTree->intervalType == QDC_JOB_INTERVAL_TYPE_NONE )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_JOBS_ VALUES( "
                         QCM_SQL_INT32_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         "%s, "
                         "%s, "
                         "0, "
                         "NULL, "
                         QCM_SQL_INT32_FMT", "
                         "NULL, "
                         QCM_SQL_INT32_FMT", "
                         "NULL, "
                         QCM_SQL_CHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT" )",
                         sJobID,
                         sJobName,
                         sExec,
                         sStart,
                         sEnd,
                         0,
                         0,
                         sEnable,
                         sComment );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_JOBS_ VALUES( "
                         QCM_SQL_INT32_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         "%s, "
                         "%s, "
                         QCM_SQL_INT32_FMT", "
                         QCM_SQL_CHAR_FMT", "
                         QCM_SQL_INT32_FMT", "
                         "NULL, "
                         QCM_SQL_INT32_FMT", "
                         "NULL, "
                         QCM_SQL_CHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT" )",
                         sJobID,
                         sJobName,
                         sExec,
                         sStart,
                         sEnd,
                         sParseTree->interval,
                         sIntervalType,
                         0,
                         0,
                         sEnable,
                         sComment );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::executeAlterJobExec( qcStatement * aStatement )
{
    qdJobParseTree  * sParseTree;
    SChar           * sSqlStr    = NULL;
    vSLong            sRowCnt    = 0;
    SChar             sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar             sExec[ QDC_JOB_EXEC_MAX_LEN + 1 ];

    IDE_DASSERT( aStatement != NULL );

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::executeAlterJobExec::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );

    IDE_TEST( makeExecTxt( sExec, &sParseTree->exec )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_JOBS_ SET EXEC_QUERY = "
                     QCM_SQL_VARCHAR_FMT
                     " WHERE JOB_NAME = "
                     QCM_SQL_VARCHAR_FMT,
                     sExec,
                     sJobName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::executeAlterJobStartEnd( qcStatement * aStatement )
{
    qdJobParseTree * sParseTree;
    SChar          * sSqlStr    = NULL;
    vSLong           sRowCnt    = 0;
    SChar            sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar            sTime[ QDC_JOB_DATE_MAX_LEN + 1 ];

    IDE_DASSERT( aStatement != NULL );

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::executeAlterJobStartEnd::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );

    if ( QC_IS_NULL_NAME( sParseTree->end ) == ID_TRUE )
    {
        QC_STR_COPY( sTime, sParseTree->start );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_JOBS_ SET START_TIME = %s "
                         "WHERE JOB_NAME = "
                         QCM_SQL_VARCHAR_FMT,
                         sTime,
                         sJobName );
    }
    else
    {
        QC_STR_COPY( sTime, sParseTree->end );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_JOBS_ SET END_TIME = %s "
                         "WHERE JOB_NAME = "
                         QCM_SQL_VARCHAR_FMT,
                         sTime,
                         sJobName );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::executeAlterJobInterval( qcStatement * aStatement )
{
    qdJobParseTree * sParseTree;
    SChar          * sSqlStr    = NULL;
    vSLong           sRowCnt    = 0;
    SChar            sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar            sIntervalType[3];

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::executeAlterJobInterval::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );

    makeIntervalTypeTxt( sIntervalType, sParseTree->intervalType );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_JOBS_ SET INTERVAL = "
                     QCM_SQL_INT32_FMT", "
                     "INTERVAL_TYPE = "
                     QCM_SQL_CHAR_FMT" "
                     "WHERE JOB_NAME = "
                     QCM_SQL_VARCHAR_FMT,
                     sParseTree->interval,
                     sIntervalType,
                     sJobName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-41713 each job enable disable
IDE_RC qdcJob::executeAlterJobEnable( qcStatement * aStatement )
{
    qdJobParseTree * sParseTree;
    SChar          * sSqlStr    = NULL;
    vSLong           sRowCnt    = 0;
    SChar            sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar            sEnable[2];

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::executeAlterJobEnable::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );

    if ( sParseTree->enable == ID_TRUE )
    {
        sEnable[0] = 'T';
        sEnable[1] = '\0';
    }
    else
    {
        sEnable[0] = 'F';
        sEnable[1] = '\0';
    }

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_JOBS_ SET IS_ENABLE = "
                     QCM_SQL_CHAR_FMT" "
                     "WHERE JOB_NAME = "
                     QCM_SQL_VARCHAR_FMT,
                     sEnable,
                     sJobName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-41713 each job enable disable
IDE_RC qdcJob::executeAlterJobComment( qcStatement * aStatement )
{
    qdJobParseTree * sParseTree;
    SChar          * sSqlStr    = NULL;
    vSLong           sRowCnt    = 0;
    SChar            sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar            sComment[ QC_MAX_COMMENT_LITERAL_LEN + 1 ];

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::executeAlterJobComment::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );

    // BUG-41713 each job enable disable
    if ( QC_IS_NULL_NAME( sParseTree->comment ) == ID_TRUE )
    {
        sComment[0] = '\0';
    }
    else
    {
        QC_STR_COPY( sComment, sParseTree->comment );
    }

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_JOBS_ SET COMMENT = "
                     QCM_SQL_VARCHAR_FMT" "
                     "WHERE JOB_NAME = "
                     QCM_SQL_VARCHAR_FMT,
                     sComment,
                     sJobName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::executeDropJob( qcStatement * aStatement )
{
    qdJobParseTree * sParseTree;
    SChar          * sSqlStr    = NULL;
    vSLong           sRowCnt    = 0;
    SChar            sJobName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    sParseTree = ( qdJobParseTree * )aStatement->myPlan->parseTree;

    /* Sql Query 공간 할당 */
    IDU_FIT_POINT("qdcJob::executeDropJob::malloc1");
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    QC_STR_COPY( sJobName, sParseTree->jobName );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_JOBS_ WHERE JOB_NAME = "
                     QCM_SQL_VARCHAR_FMT,
                     sJobName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdcJob::makeExecTxt( SChar          * aExecTxt,
                            qcNamePosition * aExec )
{
    SChar           * sStart;
    SChar           * sChar;
    SInt              sSize;
    SInt              i;

    sSize = aExec->size;

    //---------------------------------------
    // ltrim
    //---------------------------------------

    for ( sChar = aExec->stmtText + aExec->offset, i = 0;
          ( idlOS::pdl_isspace(*sChar) != 0 ) && ( i < sSize );
          sChar++ )
    {
        i++;
    }

    sSize = sSize - i;
    sStart = sChar;

    //---------------------------------------
    // rtrim
    //---------------------------------------

    IDE_TEST_RAISE( sSize <= 0, ERR_INVALID_SIZE );

    for ( sChar = sStart + sSize - 1, i = 0;
          ( idlOS::pdl_isspace(*sChar) != 0 ) && ( i < sSize );
          sChar-- )
    {
        i++;
    }

    sSize = sSize - i;

    IDE_TEST_RAISE( sSize <= 0, ERR_INVALID_SIZE );

    //---------------------------------------
    // etc
    //---------------------------------------

    IDE_TEST( qdbCommon::addSingleQuote4PartValue( sStart,
                                                   sSize,
                                                   &aExecTxt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdcJob::makeExecTxt",
                                  "invalid size" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdcJob::makeIntervalTypeTxt( SChar * aBufTxt,
                                  SInt    aIntervalType )
{
    switch ( aIntervalType )
    {
        case QDC_JOB_INTERVAL_TYPE_YEAR:
            *aBufTxt = 'Y';
            *( aBufTxt + 1 ) = 'Y';
            *( aBufTxt + 2 ) = '\0';
            break;
        case QDC_JOB_INTERVAL_TYPE_MONTH:
            *aBufTxt = 'M';
            *( aBufTxt + 1 ) = 'M';
            *( aBufTxt + 2 ) = '\0';
            break;
        case QDC_JOB_INTERVAL_TYPE_DAY:
            *aBufTxt = 'D';
            *( aBufTxt + 1 ) = 'D';
            *( aBufTxt + 2 ) = '\0';
            break;
        case QDC_JOB_INTERVAL_TYPE_HOUR:
            *aBufTxt = 'H';
            *( aBufTxt + 1 ) = 'H';
            *( aBufTxt + 2 ) = '\0';
            break;
        case QDC_JOB_INTERVAL_TYPE_MINUTE:
            *aBufTxt = 'M';
            *( aBufTxt + 1 ) = 'I';
            *( aBufTxt + 2 ) = '\0';
            break;
        default:
            *aBufTxt = '\0';
            break;
    };
}
