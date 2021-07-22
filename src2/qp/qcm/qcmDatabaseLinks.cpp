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
 

#include <idl.h>
#include <ide.h>

#include <idsRC4.h>

#include <qc.h>

#include <qcg.h>
#include <qcm.h>

#include <qcmDatabaseLinks.h>

static const void * gQcmDatabaseLinksTable;
static const void * gQcmDatabaseLinksIndex;
static void       * gQcmDatabaseLinksSequence;

/*
 *
 */
IDE_RC qcmDatabaseLinksInitializeMetaHandle( smiStatement * aStatement )
{
    const void * sTableHandle;
    void       * sSequenceHandle;

    if ( ( gQcmDatabaseLinksTable == NULL ) &&
         ( gQcmDatabaseLinksSequence == NULL ) &&
         ( gQcmDatabaseLinksIndex == NULL ) )
    {
        IDE_TEST( qcm::getMetaTable( QCM_DATABASE_LINKS_,
                                     &sTableHandle,
                                     aStatement )
                  != IDE_SUCCESS );

        IDE_TEST( qcm::getSequenceHandleByName(
                      aStatement,
                      QC_SYSTEM_USER_ID,
                      (UChar *)"NEXT_DATABASE_LINK_ID",
                      idlOS::strlen( "NEXT_DATABASE_LINK_ID" ),
                      &sSequenceHandle )
                  != IDE_SUCCESS );

        gQcmDatabaseLinksTable         = sTableHandle;
        gQcmDatabaseLinksIndex         = smiGetTableIndexes( sTableHandle, 0 );
        gQcmDatabaseLinksSequence      = sSequenceHandle;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC searchDatabaseLinkId( smiStatement * aSmiStatement,
                                    SInt           aDatabaseLinkId,
                                    idBool       * aExist )
{
    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void        * sRow;
    mtcColumn         * sIndexColumn;
    qtcMetaRangeColumn  sRangeColumn;
    smiStatement        sDummyStatement;
    scGRID              sDummyRid;
    smiCursorProperties sCursorProperty;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_LINK_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        &sRangeColumn,
        (const mtcColumn *) sIndexColumn,
        (const void *) &aDatabaseLinkId,
        &sRange );

    IDE_TEST( sDummyStatement.begin(
                  aSmiStatement->mStatistics,
                  aSmiStatement,
                  SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS);
    sStage = 1;

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open( &sDummyStatement,
                            gQcmDatabaseLinksTable,
                            gQcmDatabaseLinksIndex,
                            smiGetRowSCN( gQcmDatabaseLinksTable ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sDummyRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    if( sRow == NULL )
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    sStage = 1;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sDummyStatement.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)sCursor.close();
        case 1:
            (void)sDummyStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qcmGetNewDatabaseLinkId( qcStatement * aStatement,
                                UInt        * aDatabaseLinkId )
{
    SLong   sDatabaseLinkId;
    SLong   sFirstDatabaseLinkId;
    idBool  sExist = ID_TRUE;
    idBool  sFirst = ID_TRUE;

    IDE_TEST( smiTable::readSequence( QC_SMI_STMT( aStatement ),
                                      gQcmDatabaseLinksSequence,
                                      SMI_SEQUENCE_NEXT,
                                      &sDatabaseLinkId,
                                      NULL )
              != IDE_SUCCESS );
    sFirstDatabaseLinkId = sDatabaseLinkId;

    while ( sExist == ID_TRUE )
    {

        IDE_TEST( searchDatabaseLinkId( QC_SMI_STMT( aStatement ),
                                        (SInt)sDatabaseLinkId,
                                        &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_FALSE )
        {
            break;
        }
        else
        {
            /* do nothing */
        }

        if( ( sFirst == ID_FALSE ) &&
            ( sDatabaseLinkId == sFirstDatabaseLinkId ) )
        {
            IDE_RAISE( ERR_OBJECTS_OVERFLOW );
        }
        else
        {
            sFirst = ID_FALSE;
        }

        IDE_TEST( smiTable::readSequence( QC_SMI_STMT( aStatement ),
                                          gQcmDatabaseLinksSequence,
                                          SMI_SEQUENCE_NEXT,
                                          &sDatabaseLinkId,
                                          NULL )
                  != IDE_SUCCESS );
    }

    *aDatabaseLinkId = (UInt)sDatabaseLinkId;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_OBJECTS_OVERFLOW );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                  "Database links",
                                  QCM_TABLES_SEQ_MAXVALUE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static void makePasswordHex( UChar * aPassword,
                             SChar * aPasswordHex,
                             SInt    aPasswordHexLength )
{
    SInt        sPasswordLength = 0;
    SInt        i = 0;
    SChar       sTargetChar;

    IDE_DASSERT( aPasswordHexLength == (QC_MAX_NAME_LEN * 2) + 1 );

    sPasswordLength = idlOS::strlen( (SChar *)aPassword );

    idlOS::memset( aPasswordHex, 0x00, aPasswordHexLength );

    for ( i = 0; i < sPasswordLength; i++ )
    {
        sTargetChar = (SChar)( ( aPassword[i] & 0xF0 ) >> 4 );
        aPasswordHex[i * 2] = (sTargetChar < 10)
            ? (sTargetChar + '0')
            : (sTargetChar + 'A' - 10);

        sTargetChar = (SChar)( aPassword[i] & 0x0F );
        aPasswordHex[i * 2 + 1] = (sTargetChar < 10)
            ? (sTargetChar + '0')
            : (sTargetChar + 'A' - 10);
    }
}

/*
 * public database link의 경우 user_id는 NULL이다.
 */
IDE_RC qcmDatabaseLinksInsertItem( qcStatement          * aStatement,
                                   qcmDatabaseLinksItem * aItem,
                                   idBool                 aPublicFlag )
{
    SChar       * sSqlString = NULL;
    vSLong        sRowCount = 0;
    SChar         sPasswordHex[(QC_MAX_NAME_LEN * 2) + 1];
    smiStatement  sDummyStatement;
    SInt          sStage = 0;

    makePasswordHex( aItem->remoteUserPassword,
                     sPasswordHex,
                     ID_SIZEOF( sPasswordHex ) );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlString )
              != IDE_SUCCESS);

    if ( aPublicFlag == ID_TRUE )
    {
        idlOS::snprintf( sSqlString, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_DATABASE_LINKS_ VALUES( "
                         "NULL, "                        /* USER_ID */
                         QCM_SQL_UINT32_FMT","           /* LINK_ID */
                         QCM_SQL_BIGINT_FMT","           /* LINK_OID */
                         QCM_SQL_VARCHAR_FMT","          /* LINK_NAME */
                         QCM_SQL_INT32_FMT","            /* USER_MODE */
                         QCM_SQL_VARCHAR_FMT","          /* REMOTE_USER_ID */
                         QCM_SQL_BYTE_FMT","             /* REMOTE_USER_PWD */
                         QCM_SQL_INT32_FMT","            /* LINK_TYPE */
                         QCM_SQL_VARCHAR_FMT","          /* TARGET_NAME */
                         "SYSDATE,"                      /* CREATED */
                         "SYSDATE )",                    /* LAST_DDL_TIME */
                         aItem->linkID,
                         QCM_OID_TO_BIGINT( aItem->linkOID ),
                         aItem->linkName,
                         aItem->userMode,
                         aItem->remoteUserID,
                         sPasswordHex,
                         aItem->linkType,
                         aItem->targetName );
    }
    else
    {
        idlOS::snprintf( sSqlString, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_DATABASE_LINKS_ VALUES( "
                         QCM_SQL_UINT32_FMT","           /* USER_ID */
                         QCM_SQL_UINT32_FMT","           /* LINK_ID */
                         QCM_SQL_BIGINT_FMT","           /* LINK_OID */
                         QCM_SQL_VARCHAR_FMT","          /* LINK_NAME */
                         QCM_SQL_INT32_FMT","            /* USER_MODE */
                         QCM_SQL_VARCHAR_FMT","          /* REMOTE_USER_ID */
                         QCM_SQL_BYTE_FMT","             /* REMOTE_USER_PWD */
                         QCM_SQL_INT32_FMT","            /* LINK_TYPE */
                         QCM_SQL_VARCHAR_FMT","          /* TARGET_NAME */
                         "SYSDATE,"                      /* CREATED */
                         "SYSDATE )",                    /* LAST_DDL_TIME */
                         aItem->userID,
                         aItem->linkID,
                         QCM_OID_TO_BIGINT( aItem->linkOID ),
                         aItem->linkName,
                         aItem->userMode,
                         aItem->remoteUserID,
                         sPasswordHex,
                         aItem->linkType,
                         aItem->targetName );
    }

    IDE_TEST( sDummyStatement.begin(
                  aStatement->mStatistics,
                  QC_SMI_STMT( aStatement ),
                  SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST( qcg::runDMLforDDL( &sDummyStatement,
                                 sSqlString,
                                 &sRowCount )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sDummyStatement.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)sDummyStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qcmDatabaseLinksDeleteItem( qcStatement * aStatement,
                                   smOID         aLinkOID )
{
    SChar       * sSqlString = NULL;
    vSLong        sRowCount = 0;
    smiStatement  sDummyStatement;
    SInt          sStage = 0;

    IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                         (void **)&sSqlString )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlString, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_DATABASE_LINKS_ WHERE LINK_OID = "
                     QCM_SQL_BIGINT_FMT,
                     QCM_OID_TO_BIGINT( aLinkOID ) );

    IDE_TEST( sDummyStatement.begin(
                  aStatement->mStatistics,
                  QC_SMI_STMT( aStatement ),
                  SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST( qcg::runDMLforDDL( &sDummyStatement, sSqlString, &sRowCount )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sDummyStatement.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)sDummyStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qcmDatabaseLinksDeleteItemByUserId( qcStatement * aStatement,
                                           UInt          aUserId )
{
    SChar       * sSqlString = NULL;
    vSLong        sRowCount = 0;
    smiStatement  sDummyStatement;
    SInt          sStage = 0;

    IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                         (void **)&sSqlString )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlString, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_DATABASE_LINKS_ WHERE USER_ID = "
                     QCM_SQL_UINT32_FMT,
                     aUserId );

    IDE_TEST( sDummyStatement.begin(
                  aStatement->mStatistics,
                  QC_SMI_STMT( aStatement ),
                  SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST( qcg::runDMLforDDL( &sDummyStatement, sSqlString, &sRowCount )
              != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sDummyStatement.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );
#if 0
    /* PROJ-2446 BUGBUG */
    IDE_TEST_RAISE( sRowCount < 1, ERR_META_CRASH );
#endif

    return IDE_SUCCESS;

#if 0
    /* BUGBUG */
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
#endif
    
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)sDummyStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;

}

/*
 * Get column value from record( SYS_DATABASE_LINKS_ meta table ) and
 * set column value to allocated qcmDatabaseLinksItem structure.
 */
static IDE_RC setDatabaseLinkItem( const void            * aRow,
                                   qcmDatabaseLinksItem ** aItem )
{
    qcmDatabaseLinksItem    * sItem     = NULL;
    mtdCharType             * sCharData;
    mtdByteType             * sByteData;
    mtdIntegerType            sIntegerData;
    mtdBigintType             sBigIntData;
    mtcColumn               * sColumn   = NULL;
    UInt                      sState    = 0;

    /* BUG-37967 */
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_QCM,
                                 1,
                                 ID_SIZEOF( *sItem ),
                                 (void **)&sItem )
              != IDE_SUCCESS);
    sState = 1;

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sIntegerData = *(mtdIntegerType *)((UChar *)aRow + sColumn->column.offset);
    if ( sIntegerData == MTD_INTEGER_NULL )
    {
        sItem->userID = QC_PUBLIC_USER_ID;
    }
    else
    {
        sItem->userID = (UInt)sIntegerData;
    }

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_LINK_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sIntegerData = *(mtdIntegerType *)((UChar *)aRow + sColumn->column.offset);
    sItem->linkID = (UInt)sIntegerData;

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_LINK_OID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sBigIntData = *(mtdBigintType *)((UChar *)aRow + sColumn->column.offset);
    sItem->linkOID = (smOID)sBigIntData;

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_LINK_NAME_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType *)((UChar *)aRow + sColumn->column.offset);
    idlOS::memcpy( sItem->linkName, sCharData->value, sCharData->length );
    sItem->linkName[ sCharData->length ] = '\0';

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_USER_MODE_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sIntegerData = *(mtdIntegerType *)((UChar *)aRow + sColumn->column.offset);
    sItem->userMode = (UInt)sIntegerData;

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_REMOTE_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType *)((UChar *)aRow + sColumn->column.offset);
    idlOS::memcpy( sItem->remoteUserID, sCharData->value, sCharData->length );
    sItem->remoteUserID[ sCharData->length ] = '\0';

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_REMOTE_USER_PASSWORD_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sByteData = (mtdByteType *)((UChar *)aRow + sColumn->column.offset);
    idlOS::memcpy( sItem->remoteUserPassword, sByteData->value, sByteData->length );
    sItem->remoteUserPassword[ sByteData->length ] = '\0';

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_LINK_TYPE_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    sIntegerData = *(mtdIntegerType *)((UChar *)aRow + sColumn->column.offset);
    sItem->linkType = (UInt)sIntegerData;

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_TARGET_NAME_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );

    sCharData = (mtdCharType *)((UChar *)aRow + sColumn->column.offset);
    idlOS::memcpy( sItem->targetName, sCharData->value, sCharData->length );
    sItem->targetName[ sCharData->length ] = '\0';

    sItem->next = NULL;

    *aItem = sItem;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)iduMemMgr::free(sItem);
            /* fall through */
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qcmDatabaseLinksGetFirstItem( idvSQL * aStatistics, qcmDatabaseLinksItem ** aFirstItem )
{
    UInt                   sStage = 0;
    smiTableCursor         sCursor;
    const void           * sRow;
    smiTrans               sDummyTransaction;
    smiStatement         * sRootStatement = NULL;
    smiStatement           sDummyStatement;
    scGRID                 sDummyRid;
    smiCursorProperties    sCursorProperty;
    qcmDatabaseLinksItem * sFirstItem = NULL;
    qcmDatabaseLinksItem * sCurrentItem = NULL;
    qcmDatabaseLinksItem * sPreviousItem = NULL;

    sCursor.initialize();

    IDE_TEST( sDummyTransaction.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sDummyTransaction.begin( &sRootStatement,
                                       aStatistics,
                                       ( SMI_ISOLATION_NO_PHANTOM |
                                         SMI_TRANSACTION_NORMAL |
                                         SMI_TRANSACTION_REPL_NONE |
                                         SMI_COMMIT_WRITE_NOWAIT ),
                                       SMX_NOT_REPL_TX_ID )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sDummyStatement.begin(
                  aStatistics,
                  sRootStatement,
                  SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS);
    sStage = 3;

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open( &sDummyStatement,
                            gQcmDatabaseLinksTable,
                            NULL,
                            smiGetRowSCN( gQcmDatabaseLinksTable ),
                            NULL,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty )
              != IDE_SUCCESS );
    sStage = 4;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sDummyRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );
    while ( sRow != NULL )
    {
        IDE_TEST( setDatabaseLinkItem( sRow, &sCurrentItem )
                  != IDE_SUCCESS );

        if ( sFirstItem == NULL )
        {
            sFirstItem = sCurrentItem;
        }
        else
        {
            sPreviousItem->next = sCurrentItem;
        }
        sPreviousItem = sCurrentItem;

        IDE_TEST( sCursor.readRow( &sRow, &sDummyRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sStage = 3;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    sStage = 2;
    IDE_TEST( sDummyStatement.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( sDummyTransaction.commit() != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( sDummyTransaction.destroy( aStatistics ) != IDE_SUCCESS );

    *aFirstItem = sFirstItem;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 4:
            (void)sCursor.close();
        case 3:
            (void)sDummyStatement.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            (void)sDummyTransaction.rollback();
        case 1:
            (void)sDummyTransaction.destroy( aStatistics );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC qcmDatabaseLinksNextItem( qcmDatabaseLinksItem  * aCurrentItem,
                                 qcmDatabaseLinksItem ** aNextItem )
{

    *aNextItem = aCurrentItem->next;

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC qcmDatabaseLinksFreeItems( qcmDatabaseLinksItem * aFirstItem )
{
    qcmDatabaseLinksItem * sCurrentItem = NULL;
    qcmDatabaseLinksItem * sNextItem = NULL;

    sCurrentItem = aFirstItem;
    while ( sCurrentItem != NULL )
    {
        sNextItem = sCurrentItem->next;

        (void)iduMemMgr::free( sCurrentItem );

        sCurrentItem = sNextItem;
    }

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC qcmDatabaseLinksIsUserExisted( qcStatement * aStatement,
                                      UInt          aUserId,
                                      idBool      * aExistedFlag )
{
    mtcColumn            * sColumn = NULL;
    qtcMetaRangeColumn     sRangeColumn;
    smiRange               sRange;
    smiCursorProperties    sCursorProperty;
    const void           * sRow = NULL;
    scGRID                 sRid; // Disk Table을 위한 Record IDentifier
    SInt                   sStage = 0;
    smiTableCursor         sCursor;

    sCursor.initialize();

    IDE_TEST( smiGetTableColumns( gQcmDatabaseLinksTable,
                                  QCM_DATABASE_LINKS_USER_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn *)sColumn,
                                    (const void *) &(aUserId),
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gQcmDatabaseLinksTable,
                            gQcmDatabaseLinksIndex,
                            smiGetRowSCN( gQcmDatabaseLinksTable ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        *aExistedFlag = ID_TRUE;
    }
    else
    {
        *aExistedFlag = ID_FALSE;

    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)sCursor.close();
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}
