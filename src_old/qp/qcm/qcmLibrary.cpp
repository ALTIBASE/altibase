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
 * $Id: qcmLibrary.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 **********************************************************************/

#include <idl.h>
#include <smiDef.h>
#include <qdParseTree.h>
#include <qcmLibrary.h>
#include <qcg.h>

const void * gQcmLibraries;
const void * gQcmLibrariesIndex[QCM_MAX_META_INDICES];

IDE_RC qcmLibrary::getLibrary( qcStatement    * aStatement,
                               UInt             aUserID,
                               SChar          * aLibraryName,
                               SInt             aLibraryNameLen,
                               qcmLibraryInfo * aLibraryInfo,
                               idBool         * aExist )
{
    vSLong             sRowCount;
    smiRange           sRange;
    qtcMetaRangeColumn sFirstMetaRange;
    qtcMetaRangeColumn sSecondMetaRange;
    qcNameCharBuffer   sLibraryNameBuffer;
    mtdIntegerType     sIntData;
    mtdCharType      * sLibraryName = (mtdCharType *)&sLibraryNameBuffer;
    mtcColumn        * sFirstMtcColumn;
    mtcColumn        * sSceondMtcColumn;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLibraryName != NULL );
    IDE_DASSERT( aLibraryInfo != NULL );

    if ( aLibraryNameLen > QC_MAX_OBJECT_NAME_LEN )
    {
        *aExist = ID_FALSE;
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        // Nothing to do.
    }

    qtc::setVarcharValue( sLibraryName,
                          NULL,
                          aLibraryName,
                          aLibraryNameLen );

    sIntData = (mtdIntegerType)aUserID;

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_LIBRARY_NAME_ORDER,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_USER_ID_ORDER,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                    &sSecondMetaRange,
                                    sFirstMtcColumn,
                                    (const void*)sLibraryName,
                                    sSceondMtcColumn,
                                    &sIntData,
                                    &sRange );

    IDE_TEST(
        qcm::selectRow(
            QC_SMI_STMT(aStatement),
            gQcmLibraries,
            smiGetDefaultFilter(),
            &sRange,
            gQcmLibrariesIndex[QCM_LIBRARIES_IDX_NAME_USERID],
            (qcmSetStructMemberFunc)qcmSetLibrary,
            (void*)aLibraryInfo,
            0,
            1,
            &sRowCount )
        != IDE_SUCCESS );

    if( sRowCount == 0 )
    {
        *aExist = ID_FALSE;
    }
    else if( sRowCount == 1 )
    {
        *aExist = ID_TRUE;
    }
    else
    {
        IDE_RAISE(ERR_META_CRASH);
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmLibrary::addMetaInfo( qcStatement * aStatement,
                                UInt          aUserID,
                                SChar       * aLibraryName,
                                SChar       * aFileSpec,
                                idBool        aReplace )
{
    SChar     * sSqlStr;
    vSLong      sRowCnt;
    SLong       sLibraryID;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if( aReplace == ID_TRUE )
    {
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_LIBRARIES_ SET "
                         "FILE_SPEC = '%s', "
                         "LAST_DDL_TIME = SYSDATE "
                         "WHERE LIBRARY_NAME = '%s' AND USER_ID = "QCM_SQL_INT32_FMT,
                         aFileSpec,
                         aLibraryName,
                         aUserID );
    }
    else
    {
        IDE_TEST( qcm::getNextLibraryID( aStatement,
                                         &sLibraryID )
                  != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_LIBRARIES_ "
                         "VALUES ( "
                         "%"ID_INT64_FMT","
                         "%"ID_INT32_FMT", "
                         "'%s', "
                         "'%s', "
                         "'%s', "
                         "'%s', "
                         "SYSDATE, SYSDATE )",
                         sLibraryID,
                         aUserID,
                         aLibraryName,
                         aFileSpec,
                         "Y",
                         "VALID" );
    }

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmLibrary::delMetaInfo( qcStatement * aStatement,
                                UInt          aUserID,
                                SChar       * aLibraryName )
{
    SChar     * sSqlStr;
    vSLong      sRowCnt;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( (SChar*)sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_LIBRARIES_ WHERE "
                     "LIBRARY_NAME = '%s' AND USER_ID = "QCM_SQL_INT32_FMT,
                     aLibraryName,
                     aUserID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 (SChar*)sSqlStr, &sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmSetLibrary( idvSQL         * /* aStatistics */,
                      const void     * aRow,
                      qcmLibraryInfo * aLibraryInfo )
{
    mtcColumn   * sFileSpecColInfo;
    mtdCharType * sFileSpecStr;
    SLong         sSLongID;
    mtcColumn   * sLibararyIDMtcColumn;
    mtcColumn   * sUserIDMtcColumn;
    mtcColumn   * sLibararyNameMtcColumn;
    mtcColumn   * sDynamicOrderMtcColumn;
    mtcColumn   * sStatusOrderMtcColumn;

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_LIBRARY_ID_ORDER,
                                  (const smiColumn**)&sLibararyIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getBigintFieldValue(
        aRow,
        sLibararyIDMtcColumn,
        &sSLongID );

    aLibraryInfo->libraryID = (vULong)sSLongID;

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_USER_ID_ORDER,
                                  (const smiColumn**)&sUserIDMtcColumn )
              != IDE_SUCCESS );

    qcm::getIntegerFieldValue(
        aRow,
        sUserIDMtcColumn,
        &aLibraryInfo->userID );

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_LIBRARY_NAME_ORDER,
                                  (const smiColumn**)&sLibararyNameMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue(
        aRow,
        sLibararyNameMtcColumn,
        aLibraryInfo->libraryName );

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_FILE_SPEC_ORDER,
                                  (const smiColumn**)&sFileSpecColInfo )
              != IDE_SUCCESS );

    IDE_TEST( mtd::moduleById( &(sFileSpecColInfo->module),
                               sFileSpecColInfo->type.dataTypeId )
             != IDE_SUCCESS);

    IDE_TEST( mtl::moduleById( &sFileSpecColInfo->language,
                               sFileSpecColInfo->type.languageId )
             != IDE_SUCCESS);

    sFileSpecStr = (mtdCharType*)
        mtc::value( sFileSpecColInfo, aRow, MTD_OFFSET_USE );

    if( sFileSpecStr->length > 0 )
    {
        idlOS::memcpy( aLibraryInfo->fileSpec,
                       sFileSpecStr->value,
                       sFileSpecStr->length );

        aLibraryInfo->fileSpec[sFileSpecStr->length] = '\0';
    }
    else
    {
        aLibraryInfo->fileSpec[0] = '\0';
    }

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_DYNAMIC_ORDER,
                                  (const smiColumn**)&sDynamicOrderMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue(
        aRow,
        sDynamicOrderMtcColumn,
        aLibraryInfo->dynamic );

    IDE_TEST( smiGetTableColumns( gQcmLibraries,
                                  QCM_LIBRARIES_STATUS_ORDER,
                                  (const smiColumn**)&sStatusOrderMtcColumn )
              != IDE_SUCCESS );

    qcm::getCharFieldValue(
        aRow,
        sStatusOrderMtcColumn,
        aLibraryInfo->status );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
