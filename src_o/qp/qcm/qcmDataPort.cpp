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
 * $Id: qcmDataPort.cpp 26440 2008-06-10 04:02:48Z jdlee $
 *
 * Description :
 *
 *     [PROJ-2059] DB Upgrade 기능 DataPort
 *
 *     DataPort를 위한 메타 관리
 *
 **********************************************************************/

#include <idl.h>
#include <smiDef.h>
#include <smiDataPort.h>
#include <qdParseTree.h>
#include <qcmDataPort.h>
#include <qcg.h>

const void * gQcmDataPorts;
const void * gQcmDataPortsIndex[QCM_MAX_META_INDICES];

IDE_RC qcmDataPort::getDataPortByName(
            smiStatement          * aStatement,
            SChar                 * aJobName,
            qcmDataPortInfo       * aInfo,
            idBool                * aExist )
{
/***********************************************************************
 *
 * Description :
 *      meta에서 이름으로 DataPort를 얻어옴
 *
 * Implementation :
 *      1. name으로 meta range구성
 *      2. qcmDataPortInfo구조체에 결과 저장
 *
 ***********************************************************************/
    vSLong             sRowCount;
    smiRange           sRange;
    qtcMetaRangeColumn sFirstRangeColumn;

    qcNameCharBuffer   sNameBuffer;
    mtdCharType      * sName = (mtdCharType *)&sNameBuffer;
    mtcColumn        * sFstMtcColumn;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJobName   != NULL );
    IDE_DASSERT( aInfo      != NULL );

    qtc::setVarcharValue( sName, NULL, aJobName, idlOS::strlen( aJobName ) );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_NAME_COL_ORDER,
                                  (const smiColumn**)&sFstMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sFirstRangeColumn,
        sFstMtcColumn,
            (const void*)sName,
            &sRange);

    IDE_TEST(
        qcm::selectRow(
            aStatement,
            gQcmDataPorts,
            smiGetDefaultFilter(),
            &sRange,
            gQcmDataPortsIndex[QCM_DATA_PORTS_IDX1_ORDER],
            (qcmSetStructMemberFunc)qcmSetDataPort,
            (void*)aInfo,
            0,
            1,
            &sRowCount )
        != IDE_SUCCESS );

    if( sRowCount == 0 )
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmDataPort::addMetaInfo( smiStatement * aStatement,
                                 SChar        * aJobName,
                                 SChar        * aUserName,
                                 SChar        * aOperation,
                                 SChar        * aState,
                                 SChar        * aOwnerName,
                                 SChar        * aTableName,
                                 SChar        * aPartitionName,
                                 SChar        * aObjectName,
                                 SChar        * aDirectoryName,
                                 SLong          aProcessedRowCnt,
                                 SLong          aFirstRowSeq,
                                 SLong          aLastRowSeq,
                                 SLong          aSplit )
{

/***********************************************************************
 *
 * Description :
 *      meta DataPort에 새 Record 삽입
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar          sBuffer[QD_MAX_SQL_LENGTH];
    vSLong         sRowCnt;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aJobName       != NULL );
    IDE_DASSERT( aUserName      != NULL );
    IDE_DASSERT( aOperation     != NULL );
    IDE_DASSERT( aState         != NULL );
    IDE_DASSERT( aOwnerName     != NULL );
    IDE_DASSERT( aTableName     != NULL );
    IDE_DASSERT( aObjectName    != NULL );
    IDE_DASSERT( aDirectoryName != NULL );

    if( aPartitionName != NULL )
    {
        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_DATA_PORTS_ "
                         "(NAME, USER_NAME, OPERATION, STATE, OWNER_NAME, "
                         "TABLE_NAME, PARTITION_NAME, OBJECT_NAME, DIRECTORY_NAME,"
                         "PROCESSED_ROW_CNT, FIRST_ROW_SEQ, LAST_ROW_SEQ, "
                         "SPLIT )"
                         "VALUES( VARCHAR'%s', VARCHAR'%s', VARCHAR'%s', "
                         "VARCHAR'%s', VARCHAR'%s', VARCHAR'%s', "
                         "VARCHAR'%s', VARCHAR'%s', VARCHAR'%s',"
                         "BIGINT'%"ID_UINT64_FMT"', BIGINT'%"ID_UINT64_FMT"', "
                         "BIGINT'%"ID_UINT64_FMT"', "
                         "BIGINT'%"ID_UINT64_FMT"' )",
                         aJobName,
                         aUserName,
                         aOperation,
                         aState,
                         aOwnerName,
                         aTableName,
                         aPartitionName,
                         aObjectName,
                         aDirectoryName,
                         aProcessedRowCnt,
                         aFirstRowSeq,
                         aLastRowSeq,
                         aSplit );
    }
    else
    {
        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_DATA_PORTS_ "
                         "(NAME, USER_NAME, OPERATION, STATE, OWNER_NAME, "
                         "TABLE_NAME, OBJECT_NAME, DIRECTORY_NAME,"
                         "PROCESSED_ROW_CNT, FIRST_ROW_SEQ, LAST_ROW_SEQ, "
                         "SPLIT )"
                         "VALUES( VARCHAR'%s', VARCHAR'%s', VARCHAR'%s', "
                         "VARCHAR'%s', VARCHAR'%s', "
                         "VARCHAR'%s', VARCHAR'%s', VARCHAR'%s',"
                         "BIGINT'%"ID_UINT64_FMT"', BIGINT'%"ID_UINT64_FMT"', "
                         "BIGINT'%"ID_UINT64_FMT"', "
                         "BIGINT'%"ID_UINT64_FMT"' )",
                         aJobName,
                         aUserName,
                         aOperation,
                         aState,
                         aOwnerName,
                         aTableName,
                         aObjectName,
                         aDirectoryName,
                         aProcessedRowCnt,
                         aFirstRowSeq,
                         aLastRowSeq,
                         aSplit );
    }

    IDE_TEST(qcg::runDMLforDDL( aStatement,
                                sBuffer,
                                &sRowCnt )
             != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDataPort::updateState( smiStatement * aStatement,
                                 SChar        * aJobName,
                                 SChar        * aDstState,
                                 SLong        * aRowCnt )
{

/***********************************************************************
 *
 * Description :
 *      meta DataPort에 상태를 갱신
 *
 * Implementation :
 *      JobName과 srcState가 존재할 경우 Where절 로써 Filtering
 *
 ***********************************************************************/

    SChar          sBuffer[QD_MAX_SQL_LENGTH];
    vSLong         sRowCnt;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aJobName       != NULL );
    IDE_DASSERT( aDstState      != NULL );

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_DATA_PORTS_ "
                     "SET STATE = VARCHAR'%s' "
                     "WHERE NAME = VARCHAR'%s' ",
                     aDstState,
                     aJobName );
    IDE_TEST(qcg::runDMLforDDL( aStatement,
                                sBuffer,
                                &sRowCnt )
              != IDE_SUCCESS );

    if( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDataPort::updateProcessedRowCnt( smiStatement * aStatement,
                                           SChar        * aJobName,
                                           SLong          aProcessedRowCnt,
                                           SLong        * aRowCnt )
{

/***********************************************************************
 *
 * Description :
 *      meta DataPort의 처리된 Row개수를 갱신
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar       sBuffer[ QD_MAX_SQL_LENGTH ];
    vSLong      sRowCnt;

    // Parameter 검증
    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aJobName       != NULL );

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_DATA_PORTS_ "
                     "SET PROCESSED_ROW_CNT = BIGINT'%"ID_UINT64_FMT"' "
                     "WHERE NAME = VARCHAR'%s' ",
                     aProcessedRowCnt,
                     aJobName );

    IDE_TEST(qcg::runDMLforDDL( aStatement,
                                sBuffer,
                                &sRowCnt )
              != IDE_SUCCESS );

    if( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




IDE_RC qcmDataPort::removeMetaInfo( smiStatement * aStatement,
                                    SChar        * aJobName,
                                    UInt         * aRowCnt )
{
/***********************************************************************
 *
 * Description :
 *      meta에서 DataPort를 삭제
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar          sBuffer[QD_MAX_SQL_LENGTH];
    vSLong         sRowCnt;
    void         * sHandle;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJobName   != NULL );

    // SM으로부터 객체를 가져온다.
    IDE_TEST( smiDataPort::findHandle( aJobName,
                                       &sHandle )
              != IDE_SUCCESS );

    // 객체가 없어야, 즉 동작중이지 않아야 지울 수 있다.
    IDE_TEST_RAISE( sHandle != NULL,
                    ERR_RUNNING_JOB );

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_DATA_PORTS_ "
                     "WHERE NAME = VARCHAR'%s'",
                     aJobName );

    IDE_TEST(qcg::runDMLforDDL( aStatement,
                                (SChar*)sBuffer,
                                &sRowCnt) != IDE_SUCCESS);

    if( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_RUNNING_JOB);
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_RUNNING_JOB,
                                  aJobName) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDataPort::refineMetaInfo( smiStatement * aStatement,
                                    SChar        * aJobName )
{
/***********************************************************************
 *
 * Description :
 *       해당 Job을 SM에서 찾아 보고, 실제 상태에 따라 맞춤
 *
 * Implementation :
 *
 ***********************************************************************/

    void                   * sHandle;
    qcmDataPortInfo          sInfo;
    idBool                   sExist;

    IDE_TEST( getDataPortByName( aStatement,
                                       aJobName,
                                       &sInfo,
                                       &sExist )
              != IDE_SUCCESS );

    // Meta에 등록되지 않은 경우는, 다른 처리할 것이 없다
    IDE_TEST_CONT( sExist == ID_FALSE, NORMAL_EXIT );

    // SM으로부터 객체를 가져온다.
    IDE_TEST( smiDataPort::findHandle( aJobName,
                                       &sHandle )
              != IDE_SUCCESS );

    // Meta에 이상한 값이 있는 경우
    IDE_TEST_RAISE( ( ( idlOS::strMatch( sInfo.mOperation,
                                         idlOS::strlen( sInfo.mOperation ),
                                         QCM_DATA_PORT_OPERATION_EXPORT,
                                         idlOS::strlen( QCM_DATA_PORT_OPERATION_EXPORT ) ) != 0 ) &&
                      ( idlOS::strMatch( sInfo.mOperation,
                                         idlOS::strlen( sInfo.mOperation ),
                                         QCM_DATA_PORT_OPERATION_IMPORT,
                                         idlOS::strlen( QCM_DATA_PORT_OPERATION_IMPORT ) ) != 0 ) ) ||
                    ( ( idlOS::strMatch( sInfo.mState,
                                         idlOS::strlen( sInfo.mState ),
                                         QCM_DATA_PORT_STATE_START,
                                         idlOS::strlen( QCM_DATA_PORT_STATE_START ) ) != 0 ) &&
                      ( idlOS::strMatch( sInfo.mState,
                                         idlOS::strlen( sInfo.mState ),
                                         QCM_DATA_PORT_STATE_FINISH,
                                         idlOS::strlen(QCM_DATA_PORT_STATE_FINISH ) ) != 0 ) ),
                    ERR_META_CRASH );

    IDE_TEST_RAISE( ( sHandle == NULL ) &&
                    ( idlOS::strMatch( sInfo.mOperation,
                                       idlOS::strlen( sInfo.mOperation ),
                                       QCM_DATA_PORT_OPERATION_IMPORT,
                                       idlOS::strlen( QCM_DATA_PORT_OPERATION_IMPORT ) ) == 0 ) &&
                    ( idlOS::strMatch( sInfo.mState,
                                       idlOS::strlen( sInfo.mState ),
                                       QCM_DATA_PORT_STATE_START,
                                       idlOS::strlen( QCM_DATA_PORT_STATE_START ) ) == 0 ), 
                    ERR_SUSPENDED_JOB );

    // Export중이라 기록되었는데, 동작중이지 않을 경우
    if ( ( sHandle == NULL ) &&
         ( idlOS::strMatch( sInfo.mOperation,
                            idlOS::strlen( sInfo.mOperation ),
                            QCM_DATA_PORT_OPERATION_EXPORT,
                            idlOS::strlen( QCM_DATA_PORT_OPERATION_EXPORT ) ) == 0 ) )
    {
        IDE_TEST( removeMetaInfo( aStatement, aJobName, NULL )
                  != IDE_SUCCESS );
    }

    // 동작중이지 않은데, 완료한 Import 작업일 경우
    if ( ( sHandle == NULL ) &&
         ( idlOS::strMatch( sInfo.mOperation,
                            idlOS::strlen( sInfo.mOperation ),
                            QCM_DATA_PORT_OPERATION_IMPORT,
                            idlOS::strlen( QCM_DATA_PORT_OPERATION_IMPORT ) ) == 0 ) &&
         ( idlOS::strMatch( sInfo.mState,
                            idlOS::strlen( sInfo.mState ),
                            QCM_DATA_PORT_STATE_FINISH,
                            idlOS::strlen( QCM_DATA_PORT_STATE_FINISH ) ) == 0 ) )
    {
        IDE_TEST( removeMetaInfo( aStatement, aJobName, NULL )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SUSPENDED_JOB );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_SUSPENDED_JOB));
    }
    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qcmSetDataPort(
    idvSQL                * aStatistics,
    const void            * aRow,
    qcmDataPortInfo       * aInfo )
{
    mtcColumn   * sPortNameMtcColumn;
    mtcColumn   * sUserNameMtcColumn;
    mtcColumn   * sPortOperMtcColumn;
    mtcColumn   * sStateMtcColumn;
    mtcColumn   * sOwnerNameMtcColumn;
    mtcColumn   * sTableNameMtcColumn;
    mtcColumn   * sPartitionNameMtcColumn;
    mtcColumn   * sObjectNameMtcColumn;
    mtcColumn   * sDirectorytNameMtcColumn;
    mtcColumn   * sProcessedRowMtcColumn;
    mtcColumn   * sFirstRowSeqMtcColumn;
    mtcColumn   * sLastRowSeqMtcColumn;
    mtcColumn   * sSplitMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_NAME_COL_ORDER,
                                  (const smiColumn**)&sPortNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sPortNameMtcColumn,
                            aInfo->mName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sUserNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sUserNameMtcColumn,
                            aInfo->mUserName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_OPERATION_COL_ORDER,
                                  (const smiColumn**)&sPortOperMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sPortOperMtcColumn,
                            aInfo->mOperation );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_STATE_COL_ORDER,
                                  (const smiColumn**)&sStateMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sStateMtcColumn,
                            aInfo->mState );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_OWNER_NAME_COL_ORDER,
                                  (const smiColumn**)&sOwnerNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sOwnerNameMtcColumn,
                            aInfo->mOwnerName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_TABLE_NAME_COL_ORDER,
                                  (const smiColumn**)&sTableNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sTableNameMtcColumn,
                            aInfo->mTableName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sPartitionNameMtcColumn,
                            aInfo->mPartitionName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sObjectNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sObjectNameMtcColumn,
                            aInfo->mObjectName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_DIRECTORY_NAME_COL_ORDER,
                                  (const smiColumn**)&sDirectorytNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue( aRow,
                            sDirectorytNameMtcColumn,
                            aInfo->mDirectoryName );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_PROCESSED_ROW_CNT_COL_ORDER,
                                  (const smiColumn**)&sProcessedRowMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue( aRow,
                              sProcessedRowMtcColumn,
                              &aInfo->mProcessedRowCnt );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_FIRST_ROW_SEQ_COL_ORDER,
                                  (const smiColumn**)&sFirstRowSeqMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue( aRow,
                              sFirstRowSeqMtcColumn,
                              &aInfo->mFirstRowSeq );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_LAST_ROW_SEQ_COL_ORDER,
                                  (const smiColumn**)&sLastRowSeqMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue( aRow,
                              sLastRowSeqMtcColumn,
                              &aInfo->mLastRowSeq );

    IDE_TEST( smiGetTableColumns( gQcmDataPorts,
                                  QCM_DATA_PORTS_SPLIT_COL_ORDER,
                                  (const smiColumn**)&sSplitMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue( aRow,
                              sSplitMtcColumn,
                              &aInfo->mSplit );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
