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
 * $Id: qcmDatabase.cpp 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <qcm.h>
#include <qcg.h>
#include <qcmDatabase.h>

const void * gQcmDatabase;

IDE_RC qcmDatabase::getMetaVersion( smiStatement * aSmiStmt,
                                    UInt         * aMajorVersion,
                                    UInt         * aMinorVersion,
                                    UInt         * aPatchVersion )
{
/***********************************************************************
 *
 * Description :
 *    데이터베이스 메타의 버전을 구한다.
 *
 * Implementation :
 *    gQcmDatabase 로부터 메타의 MajorVersion, MinorVersion 을 구한다.
 *
 ***********************************************************************/

    UInt              sStage = 0;
    smiTableCursor    sCursor;
    const void      * sRow;
    mtcColumn       * sMtcColumn;

    scGRID            sRid; // Disk Table을 위한 Record IDentifier

    // initialize
    sCursor.initialize();

    // read a tuple in SYS_DATABASE_
    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmDatabase,
                          NULL,
                          smiGetRowSCN(gQcmDatabase),
                          NULL,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &gMetaDefaultCursorProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if ( sRow == NULL )
    {
        // If SYS_DATABASE_ have no record,
        //  then current major version is 0,
        //       current minor version is 0.
        *aMajorVersion = 0;
        *aMinorVersion = 0;
    }
    else
    {
        // get META_MAJOR_VERSION
        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_META_MAJOR_VER_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        *aMajorVersion = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);

        // get META_MINOR_VERSION
        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_META_MINOR_VER_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        *aMinorVersion = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);

        // get META_PATCH_VERSION

        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_META_PATCH_VER_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        *aPatchVersion = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);
    }

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

IDE_RC qcmDatabase::checkDatabase(
    smiStatement      * aSmiStmt,
    SChar             * aOwnerDN,
    UInt                aOwnerDNLen )
{
/***********************************************************************
 *
 * Description :
 *     데이터베이스가 존재하는지 검사함
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt              sStage = 0;
    smiTableCursor    sCursor;
    const void      * sRow;
    mtcColumn       * sMtcColumn;

    scGRID            sRid; // Disk Table을 위한 Record IDentifier
    mtdCharType     * sOwnerDN;

    // initialize
    sCursor.initialize();

    // read a tuple in SYS_DATABASE_
    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmDatabase,
                          NULL,
                          smiGetRowSCN(gQcmDatabase),
                          NULL,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &gMetaDefaultCursorProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if ( sRow == NULL )
    {
        // If SYS_DATABASE_ have no record,
        //  then current major version is 0,
        //       current minor version is 0.
        *aOwnerDN = 0;
    }
    else
    {
        // get OWNER_DN
        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_OWNER_DN_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        sOwnerDN   = (mtdCharType *)((UChar*)sRow + sMtcColumn->column.offset);
        IDE_TEST_RAISE( aOwnerDNLen < (UInt)(sOwnerDN->length + 1), QCM_ERR_DNBufferOverflow );
        idlOS::memcpy(aOwnerDN, sOwnerDN->value, sOwnerDN->length);
        aOwnerDN[sOwnerDN->length] = '\0';
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( QCM_ERR_DNBufferOverflow );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_DNBufferOverflow));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmDatabase::getMetaPrevVersion( smiStatement * aSmiStmt,
                                        UInt         * aPrevMajorVersion,
                                        UInt         * aPrevMinorVersion,
                                        UInt         * aPrevPatchVersion )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2689 downgrade meta
 *    데이터베이스 이전 메타의 버전을 구한다.
 *
 * Implementation :
 *    gQcmDatabase 로부터 메타의 PrvMajorVersion, PrevMinorVersion 을 구한다.
 *
 ***********************************************************************/

    UInt              sStage = 0;
    smiTableCursor    sCursor;
    const void      * sRow;
    mtcColumn       * sMtcColumn;

    scGRID            sRid; // Disk Table을 위한 Record IDentifier

    // initialize
    sCursor.initialize();

    // read a tuple in SYS_DATABASE_
    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmDatabase,
                          NULL,
                          smiGetRowSCN(gQcmDatabase),
                          NULL,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &gMetaDefaultCursorProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    if ( sRow == NULL )
    {
        // If SYS_DATABASE_ have no record,
        //  then current major version is CURR,
        //       current minor version is CURR.
        *aPrevMajorVersion = QCM_META_MAJOR_VER;
        *aPrevMinorVersion = QCM_META_MINOR_VER;
    }
    else
    {
        // get PREV_META_MAJOR_VERSION
        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_PREV_META_MAJOR_VER_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        *aPrevMajorVersion = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);

        // get PREV_META_MINOR_VERSION
        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_PREV_META_MINOR_VER_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        *aPrevMinorVersion = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);

        // get PREV_META_PATCH_VERSION
        IDE_TEST( smiGetTableColumns( gQcmDatabase,
                                      QCM_DATABASE_PREV_META_PATCH_VER_COL_ORDER,
                                      (const smiColumn**)&sMtcColumn )
                  != IDE_SUCCESS );
        *aPrevPatchVersion = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qcmDatabase::setMetaPrevVersionInternal( smiStatement    * aSmiStmt,
                                                UInt              aPrevMajorVersion,
                                                UInt              aPrevMinorVersion,
                                                UInt              aPrevPatchVersion )
{
    SChar    sBuffer[QD_MAX_SQL_LENGTH];
    vSLong   sRowCnt;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_DATABASE_ "
                     "SET    PREV_META_MAJOR_VER = "QCM_SQL_INT32_FMT", "
                     "       PREV_META_MINOR_VER = "QCM_SQL_INT32_FMT", "
                     "       PREV_META_PATCH_VER = "QCM_SQL_INT32_FMT,
                     (SInt)aPrevMajorVersion,
                     (SInt)aPrevMinorVersion,
                     (SInt)aPrevPatchVersion);

    IDE_TEST( qcg::runDMLforDDL( aSmiStmt,
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_updated_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_updated_count_is_not_1 );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_INTERNAL_ARG,
                                "[qcmDatabase::setMetaPrevVersionInternal]"
                                "err_updated_count_is_not_1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmDatabase::setMetaPrevVersion( qcStatement     * aQcStmt,
                                        UInt              aPrevMajorVersion,
                                        UInt              aPrevMinorVersion,
                                        UInt              aPrevPatchVersion )
{
    smiTrans       sSmiTrans;
    smiStatement * sDummySmiStmt = NULL;
    smiStatement   sSmiStmt;
    smiStatement * sSmiStmtOrg   = NULL;
    SInt           sState        = 0;
    UInt           sSmiStmtFlag  = 0;

    // transaction initialze
    IDE_TEST( sSmiTrans.initialize() != IDE_SUCCESS );
    sState = 1;

    // transaction begin
    IDE_TEST( sSmiTrans.begin( &sDummySmiStmt,
                               aQcStmt->mStatistics)
              != IDE_SUCCESS );
    sState = 2;

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_NORMAL;

    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    // statement begin
    IDE_TEST( sSmiStmt.begin( aQcStmt->mStatistics, sDummySmiStmt, sSmiStmtFlag )
              != IDE_SUCCESS );

    // swap
    qcg::getSmiStmt( aQcStmt, &sSmiStmtOrg );
    qcg::setSmiStmt( aQcStmt, &sSmiStmt );
    sState = 3;

    IDE_TEST ( qcmDatabase::setMetaPrevVersionInternal( &sSmiStmt,
                                                        aPrevMajorVersion,
                                                        aPrevMinorVersion,
                                                        aPrevPatchVersion )
               != IDE_SUCCESS );

    // restore
    qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );

    // statement close
    sState = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )!= IDE_SUCCESS );

    // transaction commit
    sState = 1;
    IDE_TEST( sSmiTrans.commit() != IDE_SUCCESS );

    // transaction destroy
    sState = 0;
    IDE_TEST( sSmiTrans.destroy( aQcStmt->mStatistics ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 3:
            qcg::setSmiStmt( aQcStmt, sSmiStmtOrg );
            sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            sSmiTrans.rollback();
        case 1:
            sSmiTrans.destroy( aQcStmt->mStatistics );
            break;
    }

    return IDE_FAILURE;
}
