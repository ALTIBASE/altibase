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
 * $Id: rpdCatalog.cpp 53067 2012-05-04 04:50:23Z jakejang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smErrorCode.h>
#include <smi.h>
#include <qci.h>
#include <rpi.h>
#include <rpdCatalog.h>
#include <qcg.h>

typedef struct rpdUpdateTableOID
{
    smOID       mBeforeTableOID;
    smOID       mAfterTableOID;
} rpdUpdateTableOID;

extern "C" int rpdCompareUpdateTableOID( const void * aElem1, const void * aElem2 )
{
    const rpdUpdateTableOID * sUpdateTableOID1 = (const rpdUpdateTableOID*)aElem1;
    const rpdUpdateTableOID * sUpdateTableOID2 = (const rpdUpdateTableOID*)aElem2;
    int                       sJudge = 0;

    if ( sUpdateTableOID1->mBeforeTableOID > sUpdateTableOID2->mBeforeTableOID )
    {
        sJudge = 1;
    }
    else if (  sUpdateTableOID1->mBeforeTableOID < sUpdateTableOID2->mBeforeTableOID )
    {
        sJudge = -1;
    }
    else
    {
        sJudge = 0;
    }

    return sJudge;
}

void * rpdCatalog::rpdGetTableTempInfo( const void * aTable )
{
    void * sTempInfo = NULL;

    IDE_TEST( smiGetTableTempInfo( aTable, (void **)&sTempInfo ) != IDE_SUCCESS );

    return sTempInfo;

    IDE_EXCEPTION_END;

    IDE_ASSERT( 0 );

    return NULL;
}

smiColumn * rpdCatalog::rpdGetTableColumns( const void * aTable,
                                const UInt   aIndex )
{
    smiColumn * sColumn = NULL;

    IDE_TEST( smiGetTableColumns( aTable, aIndex, (const smiColumn **)&sColumn ) != IDE_SUCCESS );

    return sColumn;

    IDE_EXCEPTION_END;

    IDE_ASSERT( 0 );

    return NULL;
}

qciCatalogReplicationCallback rpdCatalog::mCallback =
{
    rpdCatalog::updateReplItemsTableOIDArray,
    rpdCatalog::checkReplicationExistByName,
    rpdCatalog::isConsistentReplication
};

IDE_RC
rpdCatalog::checkReplicationExistByName( void          * aQcStatement,
                                         qciNamePosition aReplName,
                                         idBool        * aIsTrue )
{
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    qciMisc::setVarcharValue( (mtdCharType*) sReplName,
                              NULL,
                              aReplName.stmtText + aReplName.offset,
                              aReplName.size );

    qciMisc::makeMetaRangeSingleColumn(&sMetaRange,
                                   (mtcColumn*)rpdGetTableColumns(gQcmReplications,QCM_REPLICATION_REPL_NAME),
                                   (const void*) sReplName,
                                   & sRange);

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplications,
                            gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                            smiGetRowSCN( gQcmReplications ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        *aIsTrue = ID_TRUE;
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::checkReplicationExistByAddr( void           * aQcStatement,
                                         qciNamePosition  aIpAddr,
                                         SInt             aPortNo,
                                         idBool         * aIsTrue )
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;
    qriMetaRangeColumn  sSecondMetaRange;

    qriNameCharBuffer   sReplIPBuffer;
    mtdCharType       * sReplIP = ( mtdCharType * ) & sReplIPBuffer;

    mtdIntegerType      sIntData;
    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    /* get host information from current statement */
    qciMisc::setVarcharValue( sReplIP,
                              NULL,
                              aIpAddr.stmtText + aIpAddr.offset,
                              aIpAddr.size );

    sIntData = (mtdIntegerType) aPortNo;

    qciMisc::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                        & sSecondMetaRange,
                                        (mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_HOST_IP),
                                        (const void*) sReplIP,
                                        (mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_PORT_NO),
                                        & sIntData,
                                        & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplHostsIndex[QCM_REPLHOST_INDEX_IP_N_PORT] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplHosts,
                            gQcmReplHostsIndex[QCM_REPLHOST_INDEX_IP_N_PORT],
                            smiGetRowSCN( gQcmReplHosts ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        *aIsTrue = ID_TRUE;
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::isOtherReplUseTransWait( smiStatement  *aSmiStmt, 
                                            SChar         *aReplName,
                                            mtdBigintType *aTableOID,
                                            idBool        *aResult )
{
    smiRange               sRange;
    qriMetaRangeColumn     sMetaRange;
    smiTableCursor         sCursor;
    const void           * sRow = NULL;
    smiCursorProperties    sProperty;
    scGRID                 sRid;
    
    rpdReplications        sReplications;
    mtdCharType          * sReplNameSelected = NULL;
    SChar                  sReplNameConverted[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool                 sIsCursorOpen = ID_FALSE;
    idBool                 sIsServiceWait = ID_FALSE;
    
    idlOS::memset( &sReplications, 0x0, ID_SIZEOF( rpdReplications ) );

    sCursor.initialize();
    
    SMI_CURSOR_PROP_INIT( &sProperty, NULL, NULL );

    qciMisc::makeMetaRangeSingleColumn( &sMetaRange,
                                        (mtcColumn*)rpdGetTableColumns(
                                                gQcmReplItems,
                                                QCM_REPLITEM_TABLE_OID
                                        ),
                                        aTableOID,
                                        &sRange );

    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplItems,
                            NULL,
                            smiGetRowSCN( gQcmReplItems ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        sReplNameSelected = (mtdCharType*)
                    ((UChar*) sRow + ((mtcColumn*)rpdGetTableColumns( gQcmReplItems,
                        QCM_REPLITEM_REPL_NAME))->column.offset);
        idlOS::memcpy( sReplNameConverted,
                       sReplNameSelected->value,
                       sReplNameSelected->length );
        sReplNameConverted[sReplNameSelected->length] = '\0';
    }
    else
    {
        /* Nothing to do */
    }

    while ( ( sRow != NULL ) && 
            ( sIsServiceWait == ID_FALSE ) )
    {
        if ( idlOS::strncmp( (const SChar *)aReplName,
                             (const SChar *)sReplNameConverted,
                             sReplNameSelected->length ) != 0 )
        {
            IDE_TEST( selectRepl( aSmiStmt,
                                  sReplNameConverted,
                                  &sReplications,
                                  ID_FALSE ) != IDE_SUCCESS );

            if ( ( sReplications.mReplMode == RP_EAGER_MODE ) ||
                 ( ( sReplications.mOptions & RP_OPTION_GAPLESS_MASK )
                   == RP_OPTION_GAPLESS_SET ) )
            {
                sIsServiceWait = ID_TRUE;
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

        IDE_TEST( sCursor.readRow( &sRow,
                                   &sRid,
                                   SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        if ( sRow != NULL )
        {
            sReplNameSelected = (mtdCharType*)
                        ((UChar*) sRow + ((mtcColumn*)rpdGetTableColumns( gQcmReplItems,
                            QCM_REPLITEM_REPL_NAME))->column.offset);

            idlOS::memcpy( sReplNameConverted,
                           sReplNameSelected->value,
                           sReplNameSelected->length );
            sReplNameConverted[sReplNameSelected->length] = '\0';
        }
        else
        {
            /* Nothing to do */
        }
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );
    
    *aResult = sIsServiceWait;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );
    
    IDE_PUSH();
    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* do nothing */
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::checkReplicationExistByNameAndAddr( void           * aQcStatement,
                                                qciNamePosition  aIpAddr,
                                                SInt             aPortNo,
                                                idBool         * aIsTrue )
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;
    qriMetaRangeColumn  sSecondMetaRange;
    qriMetaRangeColumn  sThirdMetaRange;

    qriParseTree       *sParseTree = (qriParseTree *)QCI_PARSETREE( aQcStatement );

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    qriNameCharBuffer   sReplIPBuffer;
    mtdCharType       * sReplIP = ( mtdCharType * ) & sReplIPBuffer;
    qciNamePosition     sReplNameQciPos;

    mtdIntegerType      sIntData;
    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    sReplNameQciPos = sParseTree->replName;

    /* get host information from current statement */
    qciMisc::setVarcharValue( sReplIP,
                              NULL,
                              aIpAddr.stmtText + aIpAddr.offset,
                              aIpAddr.size );

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              sReplNameQciPos.stmtText + sReplNameQciPos.offset,
                              sReplNameQciPos.size );

    sIntData = (mtdIntegerType) aPortNo;

    qciMisc::makeMetaRangeTripleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        &sThirdMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplHosts, QCM_REPLHOST_REPL_NAME ),
                                        (const void*) sReplName,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplHosts, QCM_REPLHOST_HOST_IP ),
                                        (const void*) sReplIP,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplHosts, QCM_REPLHOST_PORT_NO ),
                                        & sIntData,
                                        & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplHostsIndex[QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplHosts,
                            gQcmReplHostsIndex[QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT],
                            smiGetRowSCN( gQcmReplHosts ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        *aIsTrue = ID_TRUE;
    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::getReplicationCountWithSmiStatement( smiStatement * aSmiStmt,
                                                 UInt         * aReplicationCount )
{
    UInt                sStage = 0;
    smiTableCursor      sCursor;
    const void          *sRow;
    UInt                sCount = 0;
    scGRID              sRid;

    sCursor.initialize();

    // perform a full scan.
    IDE_TEST(sCursor.open( aSmiStmt,
                           gQcmReplications,
                           NULL,
                           smiGetRowSCN(gQcmReplications),
                           NULL,
                           smiGetDefaultKeyRange(),
                           smiGetDefaultKeyRange(),
                           smiGetDefaultFilter(),
                           QCM_META_CURSOR_FLAG,
                           SMI_SELECT_CURSOR,
                           &gMetaDefaultCursorProperty )
            != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sCount++;
        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aReplicationCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }
    *aReplicationCount = sCount;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::checkReplicationStarted( void          * aQcStatement,
                                     qciNamePosition aReplName,
                                     idBool        * aIsStarted )
{
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    mtcColumn         * sMtcColumn;
    UInt                sIsStarted;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName.stmtText + aReplName.offset,
                              aReplName.size );

    qciMisc::makeMetaRangeSingleColumn( &sMetaRange,
                                        (mtcColumn*)rpdGetTableColumns(gQcmReplications,QCM_REPLICATION_REPL_NAME),
                                        (const void*) sReplName,
                                        & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplications,
                            gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                            smiGetRowSCN( gQcmReplications ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_REPLICATION );

    sMtcColumn = (mtcColumn*) rpdGetTableColumns( gQcmReplications ,
                                                  QCM_REPLICATION_IS_STARTED);

    sIsStarted = *(UInt*)((UChar*)sRow + sMtcColumn->column.offset);

    if (sIsStarted != RP_REPL_OFF)
    {
        *aIsStarted = ID_TRUE;
    }
    else
    {
        *aIsStarted = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_REPLICATION)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_NOT_EXIST_REPLICATION));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::insertRepl( smiStatement     * aSmiStmt,
                        rpdReplications  * aReplications )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPLICATIONS_ VALUES ( "
                     "VARCHAR'%s', INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "TO_DATE('%s', '%s'), "
                     "NULL, "   /* GIVE_UP_TIME */
                     "NULL, "   /* GIVE_UP_XSN */
                     "INTEGER'%"ID_INT32_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "VARCHAR'%s', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aReplications->mRepName,
                     aReplications->mLastUsedHostNo,
                     aReplications->mHostCount,
                     aReplications->mIsStarted,
                     (SLong)aReplications->mXSN,
                     aReplications->mItemCount,
                     aReplications->mConflictResolution,
                     aReplications->mReplMode,  //PROJ-1541 Append Mode
                     aReplications->mRole,
                     aReplications->mOptions,           //PROJ-1608
                     aReplications->mInvalidRecovery,   //PROJ-1608
                     aReplications->mRemoteFaultDetectTime, RP_DEFAULT_DATE_FORMAT,
                     aReplications->mParallelApplierCount,
                     aReplications->mApplierInitBufferSize,
                     aReplications->mRemoteXSN,
                     aReplications->mPeerRepName, /* BUG-45236 Local Replication 지원 */
                     aReplications->mRemoteLastDDLXSN,
                     aReplications->mCurrentReadLSNFromXLogfile.mFileNo,
                     aReplications->mCurrentReadLSNFromXLogfile.mOffset );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_RECEIVER_ VALUES ( "
                     "VARCHAR'%s', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"')",
                     aReplications->mRepName,
                     aReplications->mRemoteXSN,
                     aReplications->mCurrentReadLSNFromXLogfile.mFileNo,
                     aReplications->mCurrentReadLSNFromXLogfile.mOffset );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE_RECEIVER );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertRepl] "
                              "[INSERT SYS_REPLICATIONS_ = %"ID_vSLONG_FMT"]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION( ERR_EXECUTE_RECEIVER );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertRepl] "
                              "[INSERT SYS_REPL_RECEIVER = %"ID_vSLONG_FMT"]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectRepl( smiStatement      * aSmiStmt,
                        SChar             * aReplName,
                        rpdReplications   * aReplications,
                        idBool              aForUpdateFlag)
{
    smiTableCursor     sCursor;

    IDE_TEST( rpdCatalog::selectReplWithCursor( aSmiStmt, 
                                                aReplName, 
                                                aReplications, 
                                                aForUpdateFlag, 
                                               &sCursor )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplWithCursor( smiStatement      * aSmiStmt,
                                  SChar             * aReplName,
                                  rpdReplications   * aReplications,
                                  idBool              aForUpdateFlag,
                                  smiTableCursor    * aCursor )
{
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;

    aCursor->initialize();

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName,
                          idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn( & sMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplications,QCM_REPLICATION_REPL_NAME),
                                    (const void*) sReplName,
                                    & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME] );

    if(aForUpdateFlag == ID_TRUE)
    {
        //BUG-16851
        IDE_TEST( aCursor->open( aSmiStmt,
                                 gQcmReplications,
                                 gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                                 smiGetRowSCN( gQcmReplications ),
                                 NULL,
                                 & sRange,
                                 smiGetDefaultKeyRange(),
                                 smiGetDefaultFilter(),
                                 SMI_LOCK_REPEATABLE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                                 SMI_SELECT_CURSOR,
                                 &sProperty ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( aCursor->open( aSmiStmt,
                                 gQcmReplications,
                                 gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                                 smiGetRowSCN( gQcmReplications ),
                                 NULL,
                                 & sRange,
                                 smiGetDefaultKeyRange(),
                                 smiGetDefaultFilter(),
                                 QCM_META_CURSOR_FLAG,
                                 SMI_SELECT_CURSOR,
                                 &sProperty ) != IDE_SUCCESS );
    }
    sStage = 1;

    IDE_TEST( aCursor->beforeFirst() != IDE_SUCCESS );
    IDE_TEST( aCursor->readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, err_no_rows_found );

    IDE_TEST(setReplMember(aReplications, sRow) != IDE_SUCCESS);

    IDE_TEST( aCursor->readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    IDU_FIT_POINT_RAISE( "rpdCatalog::selectRepl::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         err_too_many_rows );
    IDE_TEST_RAISE( sRow != NULL, err_too_many_rows );

    sStage = 0;
    IDE_TEST( aCursor->close() != IDE_SUCCESS );

    IDE_TEST( selectReplReceiver( aSmiStmt,
                                  aReplications ) != IDE_SUCCESS );
    return IDE_SUCCESS;

        IDE_EXCEPTION(err_no_rows_found);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_REPL_NOT_FOUND));
    }
    IDE_EXCEPTION(err_too_many_rows);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::selectRepl] err_too_many_rows"));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) aCursor->close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::removeRepl( smiStatement      * aSmiStmt,
                        SChar             * aReplName )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPLICATIONS_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::removeRepl::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         err_deleted_count_is_not_1 );
    IDE_TEST_RAISE( sRowCnt != 1, err_deleted_count_is_not_1 );

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_RECEIVER_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, err_repl_receiver_deleted_count_is_not_1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_deleted_count_is_not_1);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::removeRepl] "
                              "[DELETE SYS_REPLICATIONS_ = %"ID_vSLONG_FMT"]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::removeRepl] err_deleted_count_is_not_1"));
    }
    IDE_EXCEPTION(err_repl_receiver_deleted_count_is_not_1);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::removeRepl] "
                              "[DELETE SYS_REPL_RECEIVER_ = %"ID_vSLONG_FMT"]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::removeRepl] err_repl_receiver_deleted_count_is_not_1"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateInvalidMaxSN( smiStatement * aSmiStmt,
                             rpdReplItems * aReplItems,
                             smSN           aSN )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    if ( aReplItems->mLocalPartname[0] == '\0' )
    {
        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                        "UPDATE SYS_REPL_ITEMS_ SET INVALID_MAX_SN='%"ID_INT64_FMT"' "
                        "WHERE REPLICATION_NAME = '%s' "
                        "AND LOCAL_USER_NAME = '%s' "
                        "AND LOCAL_TABLE_NAME = '%s' ",
                        aSN,
                        aReplItems->mRepName,
                        aReplItems->mLocalUsername,
                        aReplItems->mLocalTablename);
    }
    else
    {
            idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "UPDATE SYS_REPL_ITEMS_ SET INVALID_MAX_SN='%"ID_INT64_FMT"' "
                    "WHERE REPLICATION_NAME = '%s' "
                    "AND LOCAL_USER_NAME = '%s' "
                    "AND LOCAL_TABLE_NAME = '%s' "
                    "AND LOCAL_PARTITION_NAME = '%s' ",
                    aSN,
                    aReplItems->mRepName,
                    aReplItems->mLocalUsername,
                    aReplItems->mLocalTablename,
                    aReplItems->mLocalPartname);
    }
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::updateInvalidMaxSN::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_EXECUTE );
    IDE_TEST_RAISE( sRowCnt <= 0, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateInvalidMaxSN] "
                              "[UPDATE SYS_REPL_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateInvalidMaxSN] err_updated_count_is_0(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateOldInvalidMaxSN( smiStatement * aSmiStmt,
                                   rpdReplItems * aReplItems,
                                   smSN           aSN )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    if ( aReplItems->mLocalPartname[0] == '\0' )
    {
        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                        "UPDATE SYS_REPL_OLD_ITEMS_ SET INVALID_MAX_SN='%"ID_INT64_FMT"' "
                        "WHERE REPLICATION_NAME = '%s' "
                        "AND USER_NAME = '%s' "
                        "AND TABLE_NAME = '%s' ",
                        aSN,
                        aReplItems->mRepName,
                        aReplItems->mLocalUsername,
                        aReplItems->mLocalTablename);
    }
    else
    {
            idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "UPDATE SYS_REPL_OLD_ITEMS_ SET INVALID_MAX_SN='%"ID_INT64_FMT"' "
                    "WHERE REPLICATION_NAME = '%s' "
                    "AND USER_NAME = '%s' "
                    "AND TABLE_NAME = '%s' "
                    "AND PARTITION_NAME = '%s' ",
                    aSN,
                    aReplItems->mRepName,
                    aReplItems->mLocalUsername,
                    aReplItems->mLocalTablename,
                    aReplItems->mLocalPartname);
    }
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt <= 0, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateOldInvalidMaxSN] "
                              "[UPDATE SYS_REPL_OLD_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateOldInvalidMaxSN] err_updated_count_is_0(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateCurrentXLogfileLSN( smiStatement * aSmiStmt,
                                      SChar        * aReplicationName,
                                      smLSN          aCurrentXLogfileLSN )
{
    smiRange            sRange;
    qriMetaRangeColumn  sRangeColumn;
    mtcColumn          *sReplNameCol;
    mtcColumn          *sCurrentLSNFileNoCol;
    mtcColumn          *sCurrentLSNFileOffsestCol;
    smiTableCursor      sCursor;
    smiValue            sUpdateRow[2];
    SInt                sStage = 0;
    UChar               sBuffer[QC_MAX_OBJECT_NAME_LEN + 2] = {0,};
    void               *sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;
    vSLong              sRowCnt = 0;
    smiColumnList  sUpdateColumn[2];

    sCursor.initialize();

    sReplNameCol = (mtcColumn*) rpdGetTableColumns(gQcmReplReceiver, QCM_REPL_RECEIVER_REPL_NAME);
    sCurrentLSNFileNoCol = (mtcColumn*) rpdGetTableColumns(gQcmReplReceiver, QCM_REPL_RECEIVER_XLOGFILE_CURRENT_LSN_FILENO );
    sCurrentLSNFileOffsestCol = (mtcColumn*) rpdGetTableColumns(gQcmReplReceiver, QCM_REPL_RECEIVER_XLOGFILE_CURRENT_LSN_OFFSET );

    sUpdateColumn[0].column = (smiColumn*)sCurrentLSNFileNoCol;
    sUpdateColumn[0].next = &sUpdateColumn[1];
    sUpdateColumn[1].column = (smiColumn*)sCurrentLSNFileOffsestCol;
    sUpdateColumn[1].next = NULL;

    sUpdateRow[0].value = &(aCurrentXLogfileLSN.mFileNo);
    sUpdateRow[0].length = (ID_SIZEOF(mtdIntegerType));
    sUpdateRow[1].value = &(aCurrentXLogfileLSN.mOffset);
    sUpdateRow[1].length = (ID_SIZEOF(mtdIntegerType));

    qciMisc::setVarcharValue((mtdCharType*) sBuffer,
                              NULL,
                              aReplicationName,
                              idlOS::strlen(aReplicationName) );

    qciMisc::makeMetaRangeSingleColumn(&sRangeColumn,
                                       (const mtcColumn*) sReplNameCol,
                                       (const void *) sBuffer,
                                       &sRange);

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplReceiverIndex[QCM_REPL_RECEIVER_INDEX_REPL_NAME] );

    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplReceiver,
                            gQcmReplReceiverIndex[QCM_REPL_RECEIVER_INDEX_REPL_NAME],
                            smiGetRowSCN(gQcmReplReceiver),
                            sUpdateColumn,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                            SMI_UPDATE_CURSOR,
                            &sProperty )
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid, SMI_FIND_NEXT)
             != IDE_SUCCESS);
    sRowCnt ++;
    IDU_FIT_POINT_RAISE( "rpdCatalog::updateCurrentXLogLSN::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         err_invalid_update );
    IDE_TEST_RAISE(sRow == NULL, err_invalid_update);

    IDE_TEST(sCursor.updateRow(sUpdateRow) != IDE_SUCCESS);
#if defined(DEBUG)
    IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid,
                             SMI_FIND_NEXT) != IDE_SUCCESS);
    sRowCnt ++;
    IDE_TEST_RAISE(sRow != NULL, err_invalid_update);
#endif

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_update );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateCurrentLSNFromXLogfile] "
                              "[UPDATE SYS_REPL_RECEIVER_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateCurrentLSNFromXLogfile] err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::insertReplItem(smiStatement  * aSmiStmt,
                           rpdReplItems  * aReplItems)
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt    = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_ITEMS_ VALUES ( "
                     "CHAR'%s', BIGINT'%"ID_INT64_FMT"', "
                     "CHAR'%s', CHAR'%s', CHAR'%s', "
                     "CHAR'%s', CHAR'%s', CHAR'%s', "
                     "CHAR'%s', BIGINT'%"ID_INT64_FMT"', "
                     "CHAR'%s', "
                     "CHAR'%s', INTEGER'%"ID_INT32_FMT"' )",
                     aReplItems->mRepName,
                     (SLong) aReplItems->mTableOID,
                     aReplItems->mLocalUsername,
                     aReplItems->mLocalTablename,
                     aReplItems->mLocalPartname,
                     aReplItems->mRemoteUsername,
                     aReplItems->mRemoteTablename,
                     aReplItems->mRemotePartname,
                     aReplItems->mIsPartition,
                     (SLong) aReplItems->mInvalidMaxSN,
                     "",
                     aReplItems->mReplicationUnit,
                     aReplItems->mIsConditionSynced );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplItem] "
                              "[INSERT SYS_REPL_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::deleteReplItem( smiStatement  * aSmiStmt,
                            rpdReplItems  * aReplItems,
                            idBool          aIsPartition )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    if ( aIsPartition == ID_TRUE )
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "DELETE FROM SYS_REPL_ITEMS_ "
                         "WHERE REPLICATION_NAME = '%s' "
                         "AND LOCAL_USER_NAME = '%s' "
                         "AND LOCAL_TABLE_NAME = '%s' "
                         "AND LOCAL_PARTITION_NAME = '%s' "
                         "AND REMOTE_USER_NAME = '%s' "
                         "AND REMOTE_TABLE_NAME = '%s' "
                         "AND REMOTE_PARTITION_NAME = '%s' ",
                         aReplItems->mRepName,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename,
                         aReplItems->mLocalPartname,
                         aReplItems->mRemoteUsername,
                         aReplItems->mRemoteTablename,
                         aReplItems->mRemotePartname);
    }
    else
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "DELETE FROM SYS_REPL_ITEMS_ "
                         "WHERE REPLICATION_NAME = '%s' "
                         "AND LOCAL_USER_NAME = '%s' "
                         "AND LOCAL_TABLE_NAME = '%s' "
                         "AND REMOTE_USER_NAME = '%s' "
                         "AND REMOTE_TABLE_NAME = '%s' ",
                         aReplItems->mRepName,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename,
                         aReplItems->mRemoteUsername,
                         aReplItems->mRemoteTablename );
    }
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt,
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt <= 0, ERR_NOT_EXIST_REPL_ITEM );

    if ( aIsPartition == ID_TRUE )
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "DELETE FROM SYS_REPL_ITEMS_HISTORY_ "
                         "WHERE REPLICATION_NAME = '%s' "
                         "AND USER_NAME = '%s' "
                         "AND TABLE_NAME = '%s' "
                         "AND PARTITION_NAME = '%s' ",
                         aReplItems->mRepName,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename,
                         aReplItems->mLocalPartname);
    }
    else
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "DELETE FROM SYS_REPL_ITEMS_HISTORY_ "
                         "WHERE REPLICATION_NAME = '%s' "
                         "AND USER_NAME = '%s' "
                         "AND TABLE_NAME = '%s' ",
                         aReplItems->mRepName,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename);
    }
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt,
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_REPL_ITEM)
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::deleteReplItem] "
                              "[DELETE SYS_REPL_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_NOT_EXIST_REPL_ITEM));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::removeReplItems( smiStatement      * aSmiStmt,
                             SChar             * aReplName )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_ITEMS_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt <= 0, ERR_EXECUTE );

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_ITEMS_HISTORY_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::removeReplItems] "
                              "[DELETE SYS_REPL_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
const SChar * rpdCatalog::ConnTypeEnumToString( RP_SOCKET_TYPE aConnType )
{
    const SChar * sConnStr = NULL;
    switch ( aConnType )
    {
        case RP_SOCKET_TYPE_TCP:
            sConnStr = RP_SOCKET_TCP_STR;
            break;

        case RP_SOCKET_TYPE_UNIX:
            sConnStr = RP_SOCKET_UNIX_DOMAIN_STR;
            break;

        case RP_SOCKET_TYPE_IB:
            sConnStr = RP_SOCKET_IB_STR;
            break;

        default:
            IDE_DASSERT( 0 );
    }
    return sConnStr;
}

RP_SOCKET_TYPE rpdCatalog::ConnTypeStrToEnum( mtdCharType  * aConnStr )
{
    RP_SOCKET_TYPE sConnType = RP_SOCKET_TYPE_TCP;

         if( idlOS::strncmp( RP_SOCKET_TCP_STR,
                             (const SChar *)aConnStr->value,
                             aConnStr->length) == 0)
         {
            sConnType = RP_SOCKET_TYPE_TCP;
         }
         else if ( idlOS::strncmp( RP_SOCKET_UNIX_DOMAIN_STR,
                                   (const SChar *)aConnStr->value,
                                   aConnStr->length) == 0)

         {
            sConnType = RP_SOCKET_TYPE_UNIX;
         }
         else if ( idlOS::strncmp( RP_SOCKET_IB_STR,
                                   (const SChar *)aConnStr->value,
                                   aConnStr->length) == 0)
         {
            sConnType = RP_SOCKET_TYPE_IB;
         }
         else
         {
             IDE_DASSERT( 0 );
         }

    return sConnType;
}

rpIBLatency rpdCatalog::IBLatencyStrToEnum( mtdCharType * aIBLatencyStr )
{
    rpIBLatency sIBLatency = RP_IB_LATENCY_NONE;

    if( idlOS::strncmp( (const SChar *)"N/A",
                        (const SChar *)aIBLatencyStr->value,
                        aIBLatencyStr->length) == 0)
    {
        sIBLatency = RP_IB_LATENCY_NONE;
    }
    else if ( idlOS::strncmp( (const SChar *)"0",
                              (const SChar *)aIBLatencyStr->value,
                              aIBLatencyStr->length) == 0)
    {
        sIBLatency = RP_IB_LATENCY_0;
    }
    else if ( idlOS::strncmp( (const SChar *)"1",
                              (const SChar *)aIBLatencyStr->value,
                              aIBLatencyStr->length) == 0)
    {
        sIBLatency = RP_IB_LATENCY_1;
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    return sIBLatency;
}

IDE_RC
rpdCatalog::insertReplHost( smiStatement  * aSmiStmt,
                            rpdReplHosts  * aReplHosts )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;
    const SChar  *sConnTypeStr;
    const SChar  *sIBLatencyStr;

    sConnTypeStr = ConnTypeEnumToString( aReplHosts->mConnType );

    switch ( aReplHosts->mIBLatency )
    {
        case RP_IB_LATENCY_0:
            sIBLatencyStr = "0";
            break;
        case RP_IB_LATENCY_1:
            sIBLatencyStr = "1";
            break;
        case RP_IB_LATENCY_NONE:
            sIBLatencyStr = "N/A";
            break;
        default:
            IDE_DASSERT( 0 );
    }

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_HOSTS_ VALUES ( "
                     "INTEGER'%"ID_INT32_FMT"',  CHAR'%s',"
                     "TRIM('%s'), INTEGER'%"ID_INT32_FMT"', CHAR'%s', CHAR'%s' )",
                     aReplHosts->mHostNo,
                     aReplHosts->mRepName,
                     aReplHosts->mHostIp,
                     aReplHosts->mPortNo, 
                     sConnTypeStr, 
                     sIBLatencyStr );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplHost] "
                              "[INSERT SYS_REPL_HOSTS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::deleteReplHost( smiStatement  * aSmiStmt,
                            rpdReplHosts  * aReplHosts )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_HOSTS_ "
                     "WHERE REPLICATION_NAME = '%s' "
                     "AND HOST_IP = '%s' "
                     "AND PORT_NO = %"ID_UINT32_FMT,
                     aReplHosts->mRepName,
                     aReplHosts->mHostIp,
                     aReplHosts->mPortNo);

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::deleteReplHost] "
                              "[DELETE SYS_REPL_HOSTS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_NOT_EXIST_REPL_HOST));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::removeReplHosts( smiStatement      * aSmiStmt,
                             SChar             * aReplName )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_HOSTS_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt <= 0, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::removeReplHosts] "
                              "[DELETE SYS_REPL_HOSTS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1915 SYS_REPL_OFFLINE_DIR_ */
IDE_RC
rpdCatalog::insertReplOfflineDirs( smiStatement        * aSmiStmt,
                                   rpdReplOfflineDirs  * aReplOfflineDirs )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_OFFLINE_DIR_ VALUES ( "
                     " VARCHAR'%s'"
                     ", INTEGER'%"ID_INT32_FMT"'"
                     ", VARCHAR'%s' )",
                     aReplOfflineDirs->mRepName,
                     aReplOfflineDirs->mLFG_ID, 
                     aReplOfflineDirs->mLogDirPath);

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplOfflineDirs] "
                              "[INSERT SYS_REPL_OFFLINE_DIR_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::removeReplOfflineDirs( smiStatement * aSmiStmt,
                                   SChar        * aReplName  )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_OFFLINE_DIR_ "
                     "WHERE REPLICATION_NAME = '%s' ",
                     aReplName);

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt <= 0, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::removeReplOfflineDirs] "
                              "[DELETE SYS_REPL_OFFLINE_DIR_ = %ld]\n",
                              sRowCnt);

        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_EXIST_REPL_OFFLINE_DIR_PATH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplOfflineDirs( smiStatement         * aSmiStmt,
                                   SChar                * aReplName,
                                   rpdReplOfflineDirs   * aReplOfflineDirs,
                                   SInt                   aReplOfflineDirCount )
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    SInt                sCount = 0;
    const void        * sRow;
    rpdReplOfflineDirs* sReplOfflineDirs;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName,
                          idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplOfflineDirs,QCM_REPL_OFFLINE_DIR_REPL_NAME),
                                    (const void *) sReplName,
                                    & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplOfflineDirsIndex[QCM_REPL_OFFLINE_DIR_INDEX_REPL_NAME] );

    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplOfflineDirs,
                            gQcmReplOfflineDirsIndex[QCM_REPL_OFFLINE_DIR_INDEX_REPL_NAME],
                            smiGetRowSCN( gQcmReplOfflineDirs ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST_RAISE( sCount >= aReplOfflineDirCount, err_too_many_repl_offline_log_dir_path );

        sReplOfflineDirs = & aReplOfflineDirs[sCount];

        setReplOfflineDirMember( sReplOfflineDirs, sRow );

        sCount++;

        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;

    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDE_TEST_RAISE( sCount != aReplOfflineDirCount, err_not_enough_repl_offline_log_dir_path );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_many_repl_offline_log_dir_path);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OFFLINE_DIR_PATH));
    }
    IDE_EXCEPTION(err_not_enough_repl_offline_log_dir_path);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OFFLINE_DIR_PATH));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::getReplOfflineDirCount( smiStatement * aSmiStmt,
                                    SChar        * aReplName,
                                    UInt         * aReplOfflineDirCount )
{
    
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    vSLong              sRowCount = 0;

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName,
                          idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn( & sMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplOfflineDirs,
                                    QCM_REPL_OFFLINE_DIR_REPL_NAME),
                                    (const void*) sReplName,
                                    & sRange );
    IDE_TEST( qciMisc::selectCount(
                  aSmiStmt,
                  gQcmReplOfflineDirs,
                  &sRowCount,
                  smiGetDefaultFilter(),
                  & sRange,
                  gQcmReplOfflineDirsIndex[QCM_REPL_OFFLINE_DIR_INDEX_REPL_NAME])
              != IDE_SUCCESS );

    *aReplOfflineDirCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC
rpdCatalog::updateLastUsedHostNo( smiStatement * aSmiStmt,
                                  SChar        * aRepName,
                                  SChar        * aHostIP,
                                  UInt           aPortNo )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    if( aHostIP != NULL )
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_REPLICATIONS_ SET "
                         "LAST_USED_HOST_NO = "
                         "( "
                         "SELECT HOST_NO FROM SYS_REPL_HOSTS_ "
                         "WHERE HOST_IP = CHAR'%s' "
                         "AND PORT_NO = INTEGER'%"ID_INT32_FMT"' "
                         "AND REPLICATION_NAME = CHAR'%s' "
                         ") "
                         "WHERE REPLICATION_NAME = CHAR'%s'",
                         aHostIP,
                         aPortNo,
                         aRepName,
                         aRepName );
    }
    else
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_REPLICATIONS_ SET "
                         "LAST_USED_HOST_NO = "
                         "( "
                         "SELECT HOST_NO FROM SYS_REPL_HOSTS_ "
                         "WHERE REPLICATION_NAME = CHAR'%s' "
                         "LIMIT 1 "
                         ") "
                         "WHERE REPLICATION_NAME = CHAR'%s'",
                         aRepName,
                         aRepName );
    }

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::updateLastUsedHostNo::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         err_invalid_update );
    IDE_TEST_RAISE( sRowCnt != 1, err_invalid_update );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_update );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateLastUsedHostNo] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateLastUsedHostNo] err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateIsStarted(smiStatement    * aSmiStmt,
                            SChar           * aReplName,
                            SInt              aIsActive)
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "UPDATE SYS_REPLICATIONS_ SET "
                    "IS_STARTED = INTEGER'%"ID_INT32_FMT"' "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                    aIsActive,
                    aReplName);

    IDE_TEST_RAISE(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt) != IDE_SUCCESS,
                   ERR_RUN_DML_FOR_DDL);

    IDU_FIT_POINT_RAISE( "rpdCatalog::updateIsStarted::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE(sRowCnt != 1, ERR_INVALID_UPDATE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_UPDATE);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateIsStarted] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateIsStarted] "
                                "err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION(ERR_RUN_DML_FOR_DDL);
    {
        ideLog::log(IDE_RP_0,"[rpdCatalog::updateIsStarted] qciMisc::runDMLforDDL() error\n");
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateReplMode( smiStatement    * aSmiStmt,
                            SChar           * aReplName,
                            SInt              aReplMode )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ SET "
                     "REPL_MODE = INTEGER'%"ID_INT32_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplMode,
                     aReplName );

    IDE_TEST(qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateOptions( smiStatement    * aSmiStmt,
                           SChar           * aReplName,
                           SInt              aOptions )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;
    
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ SET "
                     "OPTIONS = INTEGER'%"ID_INT32_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aOptions,
                     aReplName );
    
    IDE_TEST(qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt) != IDE_SUCCESS);
    
    IDU_FIT_POINT_RAISE( "rpdCatalog::updateOptions::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_EXECUTE );
    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateOptions] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateOptions] "
                                "err_updated_count_is_not_1(status)"));
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateInvalidRecovery( smiStatement    * aSmiStmt,
                                   SChar           * aReplName,
                                   SInt              aValue )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;
    
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ SET "
                     "INVALID_RECOVERY = INTEGER'%"ID_INT32_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aValue,
                     aReplName );
    
    IDE_TEST(qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt) != IDE_SUCCESS);
   
    IDU_FIT_POINT_RAISE( "rpdCatalog::updateInvalidRecovery::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_EXECUTE );
    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateInvalidRecovery] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateInvalidRecovery] "
                                "err_updated_count_is_not_1(status)"));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateAllInvalidRecovery( smiStatement    * aSmiStmt,
                                      SInt              aValue,
                                      vSLong          * aAffectedRowCnt)
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;
    
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ SET "
                     "INVALID_RECOVERY = INTEGER'%"ID_INT32_FMT"' ",
                     aValue );
    
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt ) != IDE_SUCCESS );
    *aAffectedRowCnt = sRowCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateXSN( smiStatement    * aSmiStmt,
                              SChar           * aReplName,
                              smSN              aSN)
{
    smiRange            sRange;
    qriMetaRangeColumn  sRangeColumn;
    mtcColumn          *sReplNameCol;
    mtcColumn          *sXSNCol;
    smiTableCursor      sCursor;
    smiValue            sUpdateRow[2];
    SInt                sStage = 0;
    UChar               sBuffer[QC_MAX_OBJECT_NAME_LEN + 2] = {0,};
    void               *sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;
    vSLong              sRowCnt = 0;

    // To fix BUG-14818
    sCursor.initialize();

    smiColumnList  sUpdateColumn[2];

    sReplNameCol = (mtcColumn*) rpdGetTableColumns(gQcmReplications, QCM_REPLICATION_REPL_NAME);
    sXSNCol = (mtcColumn*) rpdGetTableColumns(gQcmReplications, QCM_REPLICATION_XSN );

    sUpdateColumn[0].column = (smiColumn*)sXSNCol;
    sUpdateColumn[0].next = NULL;

    sUpdateRow[0].value = &aSN;
    sUpdateRow[0].length = (ID_SIZEOF(mtdBigintType));

    qciMisc::setVarcharValue((mtdCharType*) sBuffer,
                         NULL,
                         aReplName,
                         idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn(&sRangeColumn,
                                   (const mtcColumn*) sReplNameCol,
                                   (const void *) sBuffer,
                                   &sRange);

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME] );

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 gQcmReplications,
                 gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                 smiGetRowSCN(gQcmReplications),
                 sUpdateColumn,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 SMI_LOCK_WRITE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                 SMI_UPDATE_CURSOR,
                 &sProperty
                 ) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid, SMI_FIND_NEXT)
             != IDE_SUCCESS);
    sRowCnt ++;
    IDU_FIT_POINT_RAISE( "rpdCatalog::updateXSN::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         err_invalid_update );
    IDE_TEST_RAISE(sRow == NULL, err_invalid_update);

    IDE_TEST(sCursor.updateRow(sUpdateRow) != IDE_SUCCESS);
#if defined(DEBUG)
    IDE_TEST(sCursor.readRow((const void **)&sRow, &sRid,
                             SMI_FIND_NEXT) != IDE_SUCCESS);
    sRowCnt ++;
    IDE_TEST_RAISE(sRow != NULL, err_invalid_update);
#endif

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_update );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateXSN] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateXSN] err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    if (sStage == 1)
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateRemoteFaultDetectTime(smiStatement * aSmiStmt,
                                        SChar        * aRepName,
                                        SChar        * aOutTime)
{
    mtdDateType sDateData;
    UInt        sDateStringLen;
    SChar       sBuffer[QD_MAX_SQL_LENGTH];
    vSLong      sRowCnt = 0;

    IDE_TEST( qciMisc::sysdate(&sDateData) != IDE_SUCCESS );

    IDE_TEST(mtdDateInterface::toChar(&sDateData,
                                      (UChar*)aOutTime,
                                      &sDateStringLen,
                                      RP_DEFAULT_DATE_FORMAT_LEN + 1,
                                      (UChar*)RP_DEFAULT_DATE_FORMAT,
                                      RP_DEFAULT_DATE_FORMAT_LEN)
             != IDE_SUCCESS);

    idlOS::snprintf(sBuffer, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_REPLICATIONS_ "
                             "SET REMOTE_FAULT_DETECT_TIME = TO_DATE('%s', '%s') "
                             "WHERE REPLICATION_NAME = CHAR'%s'",
                             aOutTime,
                             RP_DEFAULT_DATE_FORMAT,
                             aRepName);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    IDU_FIT_POINT_RAISE( "rpdCatalog::updateRemoteFaultDetectTime::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE(sRowCnt != 1, ERR_INVALID_UPDATE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_UPDATE);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateRemoteFaultDetectTime] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateRemoteFaultDetectTime] "
                                "err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::resetRemoteFaultDetectTime(smiStatement * aSmiStmt,
                                       SChar        * aRepName)
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf(sBuffer, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_REPLICATIONS_ "
                             "SET REMOTE_FAULT_DETECT_TIME = NULL "
                             "WHERE REPLICATION_NAME = CHAR'%s'",
                             aRepName);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    IDU_FIT_POINT_RAISE( "rpdCatalog::resetRemoteFaultDetectTime::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE(sRowCnt != 1, ERR_INVALID_UPDATE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_UPDATE);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::resetRemoteFaultDetectTime] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::resetRemoteFaultDetectTime] "
                                "err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateGiveupTime( smiStatement * aSmiStmt,
                              SChar        * aRepName )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                              "UPDATE SYS_REPLICATIONS_ "
                              "SET GIVE_UP_TIME = SYSDATE "
                              "WHERE REPLICATION_NAME = CHAR'%s'",
                              aRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::updateGiveupTime::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE( sRowCnt != 1, ERR_INVALID_UPDATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_UPDATE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateGiveupTime] "
                               "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                               sRowCnt );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_INTERNAL_ARG,
                                  "[rpdCatalog::updateGiveupTime] "
                                  "err_updated_count_is_not_1(status)" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::resetGiveupTime( smiStatement * aSmiStmt,
                             SChar        * aRepName )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                              "UPDATE SYS_REPLICATIONS_ "
                              "SET GIVE_UP_TIME = NULL "
                              "WHERE REPLICATION_NAME = CHAR'%s'",
                              aRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::resetGiveupTime::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE( sRowCnt != 1, ERR_INVALID_UPDATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_UPDATE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::resetGiveupTime] "
                               "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                               sRowCnt );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_INTERNAL_ARG,
                                  "[rpdCatalog::resetGiveupTime] "
                                  "err_updated_count_is_not_1(status)" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateGiveupXSN( smiStatement * aSmiStmt,
                             SChar        * aRepName )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                              "UPDATE SYS_REPLICATIONS_ "
                              "SET GIVE_UP_XSN = XSN "
                              "WHERE REPLICATION_NAME = CHAR'%s'",
                              aRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::updateGiveupXSN::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE( sRowCnt != 1, ERR_INVALID_UPDATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_UPDATE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateGiveupXSN] "
                               "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                               sRowCnt );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_INTERNAL_ARG,
                                  "[rpdCatalog::updateGiveupXSN] "
                                  "err_updated_count_is_not_1(status)" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::resetGiveupXSN( smiStatement * aSmiStmt,
                            SChar        * aRepName )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                              "UPDATE SYS_REPLICATIONS_ "
                              "SET GIVE_UP_XSN = NULL "
                              "WHERE REPLICATION_NAME = CHAR'%s'",
                              aRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::resetGiveupXSN::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",    
                         ERR_INVALID_UPDATE );
    IDE_TEST_RAISE( sRowCnt != 1, ERR_INVALID_UPDATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_UPDATE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::resetGiveupXSN] "
                               "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                               sRowCnt );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPD_INTERNAL_ARG,
                                  "[rpdCatalog::resetGiveupXSN] "
                                  "err_updated_count_is_not_1(status)" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplicationsWithSmiStatement( smiStatement    * aSmiStmt,
                                                UInt            * aNumReplications,
                                                rpdReplications * aReplications,
                                                UInt              aMaxReplications )
{
    UInt                sRowCount = 0;
    smiTableCursor      sCursor;
    const void        * sRow;
    SInt                sStage = 0;
    scGRID              sRid;
    SInt                i = 0;
    sCursor.initialize();

    IDE_TEST(sCursor.open( aSmiStmt,
                           gQcmReplications,
                           NULL,
                           smiGetRowSCN(gQcmReplications),
                           NULL,
                           smiGetDefaultKeyRange(),
                           smiGetDefaultKeyRange(),
                           smiGetDefaultFilter(),
                           QCM_META_CURSOR_FLAG,
                           SMI_SELECT_CURSOR,
                           &gMetaDefaultCursorProperty ) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST_RAISE( sRowCount >= aMaxReplications, err_max_replication );

        IDE_TEST(setReplMember(&aReplications[sRowCount], sRow) != IDE_SUCCESS);
        sRowCount++;
        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    for(i = 0; i < (SInt)sRowCount ; i++ )
    {
        IDE_TEST( selectReplReceiver( aSmiStmt,
                                      &aReplications[i] ) != IDE_SUCCESS );
    }

    *aNumReplications = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_max_replication );
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_MAX_REPLICATION));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplHostsWithSmiStatement( smiStatement   * aSmiStmt,
                                             SChar          * aReplName,
                                             rpdReplHosts   * aReplHosts,
                                             SInt             aHostCount )
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    SInt                sCount = 0;
    const void        * sRow;
    rpdReplHosts      * sReplHost;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName,
                          idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_REPL_NAME),
                                    (const void *) sReplName,
                                    & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplHostsIndex[QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT] );

    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplHosts,
                            gQcmReplHostsIndex[QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT],
                            smiGetRowSCN( gQcmReplHosts ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST_RAISE( sCount >= aHostCount, err_too_many_repl_hosts );

        sReplHost = & aReplHosts[sCount];

        setReplHostMember( sReplHost, sRow );

        sCount++;

        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;

    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::selectReplHostsWithSmiStatement::err_not_enough_repl_hosts",
                         err_not_enough_repl_hosts ); 
    IDE_TEST_RAISE( sCount != aHostCount, err_not_enough_repl_hosts );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_many_repl_hosts);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_HOSTS));
    }
    IDE_EXCEPTION(err_not_enough_repl_hosts);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_HOSTS));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::selectAllReplications( smiStatement    * aSmiStmt,
                                          rpdReplications * aReplicationsList,
                                          SInt            * aItemCount )
{
    smiTableCursor     sCursor;

    IDE_TEST( rpdCatalog::selectAllReplicationsWithCursor( aSmiStmt, 
                                                           aReplicationsList, 
                                                           aItemCount, 
                                                          &sCursor )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC rpdCatalog::selectAllReplicationsWithCursor( smiStatement    * aSmiStmt,
                                                    rpdReplications * aReplicationsList,
                                                    SInt            * aItemCount,
                                                    smiTableCursor  * aCursor )
{
    SInt                sStage = 0;
    SInt                sCount = 0;
    const void        * sRow = NULL;
    rpdReplications   * sReplications = NULL;
    scGRID              sRid;
    smiCursorProperties sProperty;
    SInt                i = 0;

    aCursor->initialize();

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME] );

    IDE_TEST( aCursor->open( aSmiStmt,
                             gQcmReplications,
                             gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                             smiGetRowSCN( gQcmReplications ),
                             NULL,
                             smiGetDefaultKeyRange(),
                             smiGetDefaultKeyRange(),
                             smiGetDefaultFilter(),
                             QCM_META_CURSOR_FLAG,
                             SMI_SELECT_CURSOR,
                             &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT( "rpdCatalog::selectAllReplications::sCursor::beforeFirst",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "sCursor.beforeFirst()" );
    IDE_TEST( aCursor->beforeFirst() != IDE_SUCCESS );
    IDE_TEST( aCursor->readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sReplications = &aReplicationsList[sCount];

        IDE_TEST( setReplMember( sReplications, sRow ) != IDE_SUCCESS );

        sCount++;

        IDE_TEST( aCursor->readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;

    IDE_TEST( aCursor->close() != IDE_SUCCESS );

    for(i = 0; i < sCount ; i++ )
    {
        IDE_TEST( selectReplReceiver( aSmiStmt,
                                      &aReplicationsList[i] ) != IDE_SUCCESS );
    }

    *aItemCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void) aCursor->close();
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplItems( smiStatement   * aSmiStmt,
                             SChar          * aReplName,
                             rpdMetaItem    * aMetaItems,
                             SInt             aItemCount,
                             idBool           aForUpdateFlag )
{
    smiTableCursor    sCursor;

    IDE_TEST( rpdCatalog::selectReplItemsWithCursor( aSmiStmt, 
                                                     aReplName, 
                                                     aMetaItems, 
                                                     aItemCount, 
                                                    &sCursor,
                                                     aForUpdateFlag )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplItemsWithCursor( smiStatement   * aSmiStmt,
                                       SChar          * aReplName,
                                       rpdMetaItem    * aMetaItems,
                                       SInt             aItemCount,
                                       smiTableCursor * aCursor,
                                       idBool           aForUpdateFlag )
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    SInt                sStage = 0;
    SInt                sCount = 0;
    const void        * sRow;
    rpdReplItems      * sReplItem;
    scGRID               sRid;
    smiCursorProperties sProperty;

    aCursor->initialize();

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName,
                          idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn(&sFirstMetaRange,
                                   (mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REPL_NAME),
                                   (const void *) sReplName,
                                   & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID] );

    if ( aForUpdateFlag != ID_TRUE )
    {
        IDE_TEST( aCursor->open( aSmiStmt,
                                 gQcmReplItems,
                                 gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID],
                                 smiGetRowSCN( gQcmReplItems ),
                                 NULL,
                                 & sRange,
                                 smiGetDefaultKeyRange(),
                                 smiGetDefaultFilter(),
                                 QCM_META_CURSOR_FLAG,
                                 SMI_SELECT_CURSOR,
                                 &sProperty ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( aCursor->open( aSmiStmt,
                                 gQcmReplItems,
                                 gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID],
                                 smiGetRowSCN( gQcmReplItems ),
                                 NULL,
                                 & sRange,
                                 smiGetDefaultKeyRange(),
                                 smiGetDefaultFilter(),
                                 SMI_LOCK_REPEATABLE|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                                 SMI_SELECT_CURSOR,
                                 &sProperty ) != IDE_SUCCESS );
    }
    sStage = 1;

    IDE_TEST( aCursor->beforeFirst() != IDE_SUCCESS );
    IDE_TEST( aCursor->readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST_RAISE( sCount >= aItemCount, err_too_many_repl_items );

        sReplItem = & aMetaItems[sCount].mItem;

        setReplItemMember(sReplItem, sRow);

        sCount++;

        IDE_TEST( aCursor->readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;

    IDE_TEST( aCursor->close() != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::selectReplItems::err_not_enough_repl_items", err_not_enough_repl_items );
    IDE_TEST_RAISE( sCount != aItemCount, err_not_enough_repl_items );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_many_repl_items);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_ITEMS));
    }
    IDE_EXCEPTION(err_not_enough_repl_items);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_ITEMS));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) aCursor->close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateReplTransWaitFlag( void         * aQcStatement,
                                            SInt           aTableID,
                                            idBool         aIsTransWait,
                                            smiTBSLockValidType aTBSLvType,
                                            SChar        * aReplName )
{
    smiStatement         * sSmiStmt = QCI_SMI_STMT( aQcStatement );
    void                 * sTableHandle = NULL;
    qcmTableInfo         * sTableInfo = NULL;
    smSCN                  sSCN = SM_SCN_INIT;
    UInt                   sReplFlag = 0;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;
    idBool                 sFound = ID_FALSE;

    mtdBigintType          sTableOID = 0;

    IDE_TEST( qciMisc::getTableHandleByID( sSmiStmt,
                                           aTableID,
                                           &sTableHandle,
                                           &sSCN )
              != IDE_SUCCESS );

    // Caller에서 이미 Lock을 잡았다. (Partitioned Table)

    sTableInfo = (qcmTableInfo *)rpdGetTableTempInfo( sTableHandle );

    /* PROJ-1502 PARTITIONED DISK TABLE */
    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qciMisc::getPartitionInfoList( aQcStatement,
                                                 sSmiStmt,
                                                 ( iduMemory * )QCI_QMX_MEM( aQcStatement ),
                                                 sTableInfo->tableID,
                                                 &sPartInfoList )
                  != IDE_SUCCESS );

        // Caller에서 이미 Lock을 잡았다. (Table Partition)
    }
    else
    {
        /* do nothing */
    }

    sReplFlag = smiGetTableFlag( (const void *)sTableHandle );

    if ( aIsTransWait == ID_TRUE )
    {
        /* BUG-39143 */
        sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sReplFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_ENABLE;
    }
    else
    {
        if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {

            for ( sTempPartInfoList = sPartInfoList;
                  sTempPartInfoList != NULL;
                  sTempPartInfoList = sTempPartInfoList->next )
            {
                sTableOID = (mtdBigintType)smiGetTableId( sTempPartInfoList->partHandle );

                IDE_TEST( isOtherReplUseTransWait( sSmiStmt,
                                                   aReplName,
                                                   &sTableOID,
                                                   &sFound )
                          != IDE_SUCCESS );
                if ( sFound == ID_TRUE )
                {
                    break;
                }
                else
                {
                    /* continue */
                }
            }
        }
        else
        {
            sTableOID = (mtdBigintType)smiGetTableId( sTableHandle );

            IDE_TEST( isOtherReplUseTransWait( sSmiStmt,
                                               aReplName,
                                               &sTableOID,
                                               &sFound )
                      != IDE_SUCCESS );
        }

        if ( sFound == ID_FALSE )
        {
            sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
            sReplFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_DISABLE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    //---------------------------
    // change Table Info
    //---------------------------

    // update SM2 meta / smiTable::touchTable
    IDE_TEST( smiTable::modifyTableInfo( sSmiStmt,
                                         sTableHandle,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         sReplFlag,
                                         aTBSLvType,
                                         sTableInfo->maxrows,
                                         0, /* Parallel Degree */
                                         ID_TRUE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateReplPartitionTransWaitFlag(  void       *  aQcStatement,
                                                      qcmTableInfo * aPartInfo,
                                                      idBool        aIsTransWait,
                                                      smiTBSLockValidType aTBSLvType,
                                                      SChar        * aReplName )
{
    smiStatement         * sSmiStmt = QCI_SMI_STMT( aQcStatement );
    qcmTableInfo         * sTableInfo = NULL;
    UInt                   sReplFlag = 0;
    idBool                 sFound = ID_FALSE;

    mtdBigintType          sTableOID = 0;

    // Caller에서 이미 Lock을 잡았다. (Partitioned Table, Table Partition)

    sTableInfo = (qcmTableInfo *)rpdGetTableTempInfo( aPartInfo->tableHandle );

    sReplFlag = smiGetTableFlag( (const void *)aPartInfo->tableHandle );

    if ( aIsTransWait == ID_TRUE )
    {
        sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sReplFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_ENABLE;
    }
    else
    {
        sTableOID = (mtdBigintType)smiGetTableId( aPartInfo->tableHandle );

        IDE_TEST( isOtherReplUseTransWait( sSmiStmt,
                                           aReplName,
                                           &sTableOID,
                                           &sFound )
                  != IDE_SUCCESS );

        if ( sFound == ID_FALSE )
        {
            sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
            sReplFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_DISABLE;
        }
        else
        {
            /* Nothing to do */
        }
    }

    //---------------------------
    // change Table Info
    //---------------------------

    /* update SM2 meta / smiTable::touchTable */
    IDE_TEST( smiTable::modifyTableInfo( sSmiStmt,
                                         aPartInfo->tableHandle,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         sReplFlag,
                                         aTBSLvType,
                                         sTableInfo->maxrows,
                                         0, /* Parallel Degree */
                                         ID_TRUE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpdCatalog::updateReceiverApplyCount( void         * /* QcStatement */,
                                             smiStatement * aSmiStmt,
                                             SChar        * aReplicationName,
                                             SInt           aReceiverApplyCount )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET PARALLEL_APPLIER_COUNT = INTEGER'%"ID_INT32_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReceiverApplyCount,
                     aReplicationName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log( IDE_RP_0, RP_TRC_D_ERR_UPDATE_RECEVIER_APPLY_COUNT, 
                     sRowCnt);
        IDE_SET( ideSetErrorCode( rpERR_FATAL_RPD_REPL_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateRemoteXSN( smiStatement * aSmiStmt,
                                    SChar        * aReplName,
                                    smSN           aSN )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPL_RECEIVER_ "
                     "SET REMOTE_XSN = BIGINT'%"ID_INT64_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aSN,
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateRemoteXSN] [%s]",
                     sBuffer );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_RPD_REPL_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateRemoteLastDDLXSN( smiStatement * aSmiStmt,
                                           SChar        * aReplName,
                                           smSN           aSN )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET REMOTE_LAST_DDL_XSN = BIGINT'%"ID_INT64_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aSN,
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateRemoteLastDDLXSN] [%s]",
                     sBuffer );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_RPD_REPL_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateReceiverApplierInitBufferSize( smiStatement * aSmiStmt,
                                                        SChar        * aReplicationName,
                                                        ULong          aInitBufferSize )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET APPLIER_INIT_BUFFER_SIZE = BIGINT'%"ID_INT64_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aInitBufferSize,
                     aReplicationName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateReceiverApplierBufferSize] [%s]", 
                     sBuffer );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_RPD_REPL_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::updateReplicationFlag( void         * aQcStatement,
                                   smiStatement * aSmiStmt,
                                   SInt           aTableID,
                                   SInt           aEventFlag,
                                   smiTBSLockValidType aTBSLvType )
{
    SChar                  sBuffer[QD_MAX_SQL_LENGTH];
    void                 * sTableHandle      = NULL;
    qcmTableInfo         * sTableInfo        = NULL;
    smSCN                  sSCN              = SM_SCN_INIT;
    UInt                   sTableFlag        = 0;
    UInt                   sReplFlag;
    vSLong                 sRowCnt           = 0;
    UInt                   i;
    smOID                  sDictOID;
    smiColumn            * sSmiColumn        = NULL;
    qcmTableInfo         * sDictTableInfo    = NULL;
    const void           * sDictHandle       = NULL;
    smSCN                  sDictSCN;

    IDE_TEST( qciMisc::getTableHandleByID( aSmiStmt,
                                           aTableID,
                                           & sTableHandle,
                                           & sSCN )
            != IDE_SUCCESS );

    // Caller에서 이미 Lock을 잡았다. (Partitioned Table)

    sTableInfo = (qcmTableInfo *)rpdGetTableTempInfo(sTableHandle);

    /*    PROJ-2397 Compressed Column Table Replication     */
    for ( i = 0 ; i < sTableInfo->columnCount ; i++ )
    {
        sSmiColumn = &(sTableInfo->columns->basicInfo->column);
        if ( ( sSmiColumn->flag & SMI_COLUMN_COMPRESSION_MASK ) ==
             SMI_COLUMN_COMPRESSION_TRUE )
        {
            sDictOID       = sSmiColumn->mDictionaryTableOID;
            sDictHandle    = smiGetTable( sDictOID );

            sDictSCN       = smiGetRowSCN( sDictHandle );
            sDictTableInfo = (qcmTableInfo *) rpdGetTableTempInfo( sDictHandle );

            IDE_TEST( smiValidateAndLockObjects( (QCI_SMI_STMT( aQcStatement ))->getTrans(),
                                                 sDictHandle,
                                                 sDictSCN,
                                                 aTBSLvType,
                                                 SMI_TABLE_LOCK_X,
                                                 smiGetDDLLockTimeOut((QCI_SMI_STMT( aQcStatement ))->getTrans()),
                                                 ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    if (aEventFlag == RP_REPL_ON)
    {
        // {USED, NOT_USED} -> USED++
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLES_ SET "
                         "REPLICATION_COUNT = REPLICATION_COUNT + 1 "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                         aTableID );

        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

        sReplFlag = SMI_TABLE_REPLICATION_ENABLE;
    }
    else
    {
        IDE_TEST_RAISE( aEventFlag != RP_REPL_OFF, ERR_INTERNAL );

        // USED(==1) -> USED--
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLES_ SET "
                         "REPLICATION_COUNT = REPLICATION_COUNT - 1 "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND REPLICATION_COUNT = 1",
                         aTableID );

        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );

        if ( sRowCnt == 1 )
        {
            sReplFlag = SMI_TABLE_REPLICATION_DISABLE;
        }
        else
        {
            // USED(>1) -> USED--
            idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                             "UPDATE SYS_TABLES_ SET "
                             "REPLICATION_COUNT = REPLICATION_COUNT - 1 "
                             "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                             "AND REPLICATION_COUNT > 1",
                             aTableID );

            IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );
            sReplFlag = SMI_TABLE_REPLICATION_ENABLE;
        }
    }


    //---------------------------
    // change Table Info
    //---------------------------

    sTableFlag = ( smiGetTableFlag( (const void *)sTableHandle ) & ~SMI_TABLE_REPLICATION_MASK )
        | sReplFlag;

    // update SM2 meta / smiTable::touchTable
    IDE_TEST(smiTable::modifyTableInfo( aSmiStmt,
                                        sTableHandle,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        sTableFlag,
                                        aTBSLvType,
                                        sTableInfo->maxrows,
                                        0, /* Parallel Degree */
                                        ID_TRUE )
             != IDE_SUCCESS );

    /*    PROJ-2397 Compressed Column Table Replication     */
    for ( i = 0 ; i < sTableInfo->columnCount ; i++ )
    {
        sSmiColumn = &(sTableInfo->columns->basicInfo->column);
        if ( ( sSmiColumn->flag & SMI_COLUMN_COMPRESSION_MASK ) ==
             SMI_COLUMN_COMPRESSION_TRUE )
        {
            sDictOID = sSmiColumn->mDictionaryTableOID;

            sDictTableInfo = (qcmTableInfo *) rpdGetTableTempInfo( smiGetTable( sDictOID ) );

            sTableFlag = ( smiGetTableFlag( (const void *)sDictTableInfo->tableHandle ) & ~SMI_TABLE_REPLICATION_MASK )
                | sReplFlag;

            IDE_TEST(smiTable::modifyTableInfo( aSmiStmt,
                                                sDictTableInfo->tableHandle,
                                                NULL,
                                                0,
                                                NULL,
                                                0,
                                                sTableFlag,
                                                aTBSLvType,
                                                sDictTableInfo->maxrows,
                                                0, /* Parallel Degree */
                                                ID_TRUE ) /* aIsInitRowTemplate */ 
                             != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateReplicationFlag] "
                              "[UPDATE SYS_TABLES_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION( ERR_INTERNAL );
    {
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




IDE_RC rpdCatalog::updatePartitionReplicationFlag( void                 * /* aQcStatement */,
                                                   smiStatement         *aSmiStmt,
                                                   qcmTableInfo         *aPartInfo,
                                                   SInt                  aTableID,
                                                   SInt                  aEventFlag,
                                                   smiTBSLockValidType   aTBSLvType )
{
    SChar                  sBuffer[QD_MAX_SQL_LENGTH];
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    smSCN                  sSCN;
    UInt                   sReplFlag;
    vSLong                 sRowCnt = 0;

    IDE_TEST( qciMisc::getTableHandleByID( aSmiStmt,
                                           aTableID,
                                           & sTableHandle,
                                           & sSCN )
            != IDE_SUCCESS );

    // Caller에서 이미 Lock을 잡았다. (Partitioned Table, Table Partition)

    sTableInfo = (qcmTableInfo *)rpdGetTableTempInfo(sTableHandle);

    sReplFlag = smiGetTableFlag( (const void *)aPartInfo->tableHandle );

    if (aEventFlag == RP_REPL_ON)
    {
        // {USED, NOT_USED} -> USED++
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLE_PARTITIONS_ SET "
                         "REPLICATION_COUNT = REPLICATION_COUNT + 1 "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND PARTITION_ID = INTEGER'%"ID_INT32_FMT"'",
                         aTableID,
                         aPartInfo->partitionID);

        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

        sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_MASK );
        sReplFlag |= SMI_TABLE_REPLICATION_ENABLE;
    }
    else
    {
        IDE_TEST_RAISE( aEventFlag != RP_REPL_OFF, ERR_INTERNAL );

        // USED(==1) -> USED--
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLE_PARTITIONS_ SET "
                         "REPLICATION_COUNT = REPLICATION_COUNT - 1 "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND PARTITION_ID = INTEGER'%"ID_INT32_FMT"'"
                         "AND REPLICATION_COUNT = 1",
                         aTableID,
                         aPartInfo->partitionID);

        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );

        if ( sRowCnt == 1 )
        {
            sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_MASK );
            sReplFlag |= SMI_TABLE_REPLICATION_DISABLE;
        }
        else
        {
            // USED(>1) -> USED--
            idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                             "UPDATE SYS_TABLE_PARTITIONS_ SET "
                             "REPLICATION_COUNT = REPLICATION_COUNT - 1 "
                             "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                             "AND PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                             "AND REPLICATION_COUNT > 1",
                             aTableID,
                             aPartInfo->partitionID);

            IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );
            sReplFlag = sReplFlag & ( ~SMI_TABLE_REPLICATION_MASK );
            sReplFlag |= SMI_TABLE_REPLICATION_ENABLE;
        }
    }

    //---------------------------
    // change Table Info
    //---------------------------

    // PROJ-1502 PARTITIONED DISK TABLE

    IDE_TEST( smiTable::modifyTableInfo( aSmiStmt,
                                         aPartInfo->tableHandle,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         sReplFlag,
                                         aTBSLvType,
                                         sTableInfo->maxrows,
                                         0, /* Parallel Degree */
                                         ID_TRUE )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updatePartitionReplicationFlag] "
                              "[UPDATE SYS_TABLE_PARTITIONS_ = %ld, TABLE_ID = '%d']\n",
                              sRowCnt,
                              sTableInfo->tableID);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION( ERR_INTERNAL );
    {
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
rpdCatalog::updateReplRecoveryCnt( void         * aQcStatement,
                                   smiStatement * aSmiStmt,
                                   SInt           aTableID,
                                   idBool         aIsRecoveryOn,
                                   UInt           aReplRecoveryCount,
                                   smiTBSLockValidType aTBSLvType )
{
    SChar                  sBuffer[QD_MAX_SQL_LENGTH];
    void                 * sTableHandle;
    qcmTableInfo         * sTableInfo;
    smSCN                  sSCN;
    vSLong                 sRowCnt = 0;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;

    UInt                   sCount = 0;

    IDE_TEST( qciMisc::getTableHandleByID( aSmiStmt,
                                       aTableID,
                                       & sTableHandle,
                                       & sSCN ) != IDE_SUCCESS );

    // Caller에서 이미 Lock을 잡았다. (Partitioned Table)

    sTableInfo = (qcmTableInfo *)rpdGetTableTempInfo(sTableHandle);

    // PROJ-1502 PARTITIONED DISK TABLE
    if( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qciMisc::getPartitionInfoList( aQcStatement,
                                                 aSmiStmt,
                                                 ( iduMemory * )QCI_QMX_MEM( aQcStatement ),
                                                 sTableInfo->tableID,
                                                 &sPartInfoList )
                  != IDE_SUCCESS );

        // Caller에서 이미 Lock을 잡았다. (Table Partition)

        for( sTempPartInfoList = sPartInfoList;
             sTempPartInfoList != NULL;
             sTempPartInfoList = sTempPartInfoList->next )
        {
            sCount++;
        }
    }
    else
    {
        /*
         * normal table
         */
        sCount = 1;

    }

    if(aIsRecoveryOn == ID_TRUE) //recovery count on(1)
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLES_ SET "
                         "REPLICATION_RECOVERY_COUNT = REPLICATION_RECOVERY_COUNT + '%"ID_INT32_FMT"' "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'"
                         "AND REPLICATION_RECOVERY_COUNT >= 0"
                         "AND REPLICATION_RECOVERY_COUNT <= '%"ID_INT32_FMT"' - '%"ID_INT32_FMT"'",
                         aReplRecoveryCount,
                         aTableID,
                         sCount,
                         aReplRecoveryCount );

        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_REPL_RECOVERY_COUNT);
    }
    else //a Recovery off(0)
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLES_ SET "
                         "REPLICATION_RECOVERY_COUNT = REPLICATION_RECOVERY_COUNT - '%"ID_INT32_FMT"' "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'"
                         "AND REPLICATION_RECOVERY_COUNT >= '%"ID_INT32_FMT"' "
                         "AND REPLICATION_RECOVERY_COUNT <= '%"ID_INT32_FMT"'",
                         aReplRecoveryCount,
                         aTableID,
                         aReplRecoveryCount,
                         sCount );

        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_REPL_RECOVERY_COUNT );
    }
    //---------------------------
    // change Table Info
    //---------------------------

    // update SM2 meta / smiTable::touchTable
    IDE_TEST(smiTable::touchTable( aSmiStmt,
                                   sTableHandle,
                                   aTBSLvType ) != IDE_SUCCESS );

    // PROJ-1502 PARTITIONED DISK TABLE
    for( ; sPartInfoList != NULL; sPartInfoList = sPartInfoList->next )
    {
        IDE_TEST(smiTable::touchTable( aSmiStmt,
                                       sPartInfoList->partHandle,
                                       aTBSLvType ) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPL_RECOVERY_COUNT );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateReplRecoveryCnt] "
                              "[UPDATE SYS_TABLES_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updatePartitionReplRecoveryCnt( void          * /* aQcStatement */,
                                                   smiStatement  *aSmiStmt,
                                                   qcmTableInfo  *aPartInfo,
                                                   SInt           aTableID,
                                                   idBool         aIsRecoveryOn,
                                                   smiTBSLockValidType aTBSLvType )
{
    SChar                  sBuffer[QD_MAX_SQL_LENGTH];
    void                 * sTableHandle;
    smSCN                  sSCN;
    vSLong                 sRowCnt = 0;


    IDE_TEST( qciMisc::getTableHandleByID( aSmiStmt,
                                           aTableID,
                                           & sTableHandle,
                                           & sSCN )
              != IDE_SUCCESS );

    // Caller에서 이미 Lock을 잡았다. (Partitioned Table, Table Partition)

    if ( aIsRecoveryOn == ID_TRUE ) //recovery count on(1)
    {
        // 0 -> 1
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLE_PARTITIONS_ SET "
                         "REPLICATION_RECOVERY_COUNT = REPLICATION_RECOVERY_COUNT + 1 "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND REPLICATION_RECOVERY_COUNT = 0",
                         aTableID,
                         aPartInfo->partitionID );
        
        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );
        //이미 다른 replication에 의해서 recovery가
        //지원되고 있는 상황일 때 row cnt가 0일 수 있으나 validation에서 걸러져야함
        IDE_TEST_RAISE( sRowCnt != 1, ERR_REPL_RECOVERY_COUNT);
    }
    else //a Recovery off(0)
    {
        // 1 --> 0
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "UPDATE SYS_TABLE_PARTITIONS_ SET "
                         "REPLICATION_RECOVERY_COUNT = REPLICATION_RECOVERY_COUNT - 1 "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                         "AND REPLICATION_RECOVERY_COUNT > 0",
                         aTableID,
                         aPartInfo->partitionID );
        
        IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_REPL_RECOVERY_COUNT );
    }
    //---------------------------
    // change Table Info
    //---------------------------

    // update SM2 meta / smiTable::touchTable
    IDE_TEST(smiTable::touchTable( aSmiStmt,
                                   sTableHandle,
                                   aTBSLvType ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPL_RECOVERY_COUNT );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateReplRecoveryCnt] "
                              "[UPDATE SYS_TABLE_PARTITIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::setReplMember( rpdReplications * aRepl,
                           const void      * aRow )
{
    // ------ SYS_REPLICATIONS_ -------
    // REPLICATION_NAME         VARCHAR(40)
    // LAST_USED_HOST_NO        INTEGER
    // HOST_COUNT               INTEGER
    // IS_STARTED               INTEGER
    // XSN                      BIGINT
    // ITEM_COUNT               INTEGER
    // CONFLICT_RESOLUTION      INTEGER
    // REPL_MODE                INTEGER
    // ROLE                     INTEGER
    // OPTIONS                  INTEGER
    // INVALID_RECOVERY         INTEGER
    // REMOTE_FAULT_DETECT_TIME DATE ==> dblink bug로 생긴 프로퍼피임
    // PARALLEL_APPIER_COUNT    INTEGER
    // APPIER_INIT_BUFFER_SIZE  BIGINT
    // REMOTE_XSN               BIGINT
    // PEER_REPLICATION_NAME    VARCHAR(40)
    // REMOTE_LAST_DDL_XSN      BIGINT
    // XLOGFILE_CURRENT_LSN_FILENO  INTEGER
    // XLOGFILE_CURRENT_LSN_OFFSET  INTEGER
    // --------------------------------

    mtdCharType    *sCharData;
    mtdIntegerType  sIntData;
    mtdBigintType   sBigIntData;
    mtdDateType    *sDateData;
    UInt            sDateStringLen;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.REPLICATION_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_REPL_NAME))->column.offset);
    idlOS::memcpy( aRepl->mRepName,
                   sCharData->value,
                   sCharData->length );
    aRepl->mRepName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.LAST_USED_HOST_NO
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_LAST_USED_HOST_NO))->column.offset);
    aRepl->mLastUsedHostNo = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.HOST_COUNT
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_HOST_COUNT))->column.offset);
    aRepl->mHostCount = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.IS_STARTED
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_IS_STARTED))->column.offset);
    aRepl->mIsStarted = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.XSN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_XSN))->column.offset);
    aRepl->mXSN = *( (smSN*) &sBigIntData );

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.ITEM_COUNT
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_ITEM_COUNT))->column.offset);
    aRepl->mItemCount = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.CONFLICT_RESOLUTION
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_CONFLICT_RESOLUTION))->column.offset);
    aRepl->mConflictResolution = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.REPL_MODE
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_REPL_MODE))->column.offset);
    aRepl->mReplMode = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.ROLE
    //-------------------------------------------
    // PROJ-1537
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_ROLE))->column.offset);
    aRepl->mRole = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.OPTIONS
    //-------------------------------------------
    // PROJ-1608
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_OPTIONS))->column.offset);
    aRepl->mOptions = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.INVALID_RECOVERY
    //-------------------------------------------
    // PROJ-1608
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_INVALID_RECOVERY))->column.offset);
    aRepl->mInvalidRecovery = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.REMOTE_FAULT_DETECT_TIME
    //-------------------------------------------
    sDateData = (mtdDateType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_REMOTE_FAULT_DETECT_TIME))->column.offset);
    IDE_TEST(mtdDateInterface::toChar(sDateData,
                                      (UChar*)aRepl->mRemoteFaultDetectTime,
                                      &sDateStringLen,
                                      RP_DEFAULT_DATE_FORMAT_LEN + 1,
                                      (UChar*)RP_DEFAULT_DATE_FORMAT,
                                      RP_DEFAULT_DATE_FORMAT_LEN)
             != IDE_SUCCESS);

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.PARALLEL_APPIER_COUNT
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_PARALLEL_APPLIER_COUNT))->column.offset);
    aRepl->mParallelApplierCount = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.APPLIER_INIT_BUFFER_SIZE
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_APPLIER_INIT_BUFFER_SIZE))->column.offset);
    aRepl->mApplierInitBufferSize = (ULong) sBigIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.REMOTE_XSN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_REMOTE_XSN))->column.offset);
    aRepl->mRemoteXSN = (smSN) sBigIntData;

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.PEER_REPLICATION_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_PEER_REPL_NAME))->column.offset);
    idlOS::memcpy( aRepl->mPeerRepName,
                   sCharData->value,
                   sCharData->length );
    aRepl->mPeerRepName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPLICATIONS_.REMOTE_LAST_DDL_XSN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_REMOTE_LAST_DDL_XSN))->column.offset);
    aRepl->mRemoteLastDDLXSN = (smSN) sBigIntData;

    //-----------------------------------------------------
    // set by SYS_REPLICATIONS_.XLOGFILE_CURRENT_LSN_FILENO
    //-----------------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_XLOGFILE_CURRENT_LSN_FILENO))->column.offset);
    aRepl->mCurrentReadLSNFromXLogfile.mFileNo = (UInt)sIntData;

    //-----------------------------------------------------
    // set by SYS_REPLICATIONS_.XLOGFILE_CURRENT_LSN_OFFSET
    //-----------------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
            QCM_REPLICATION_XLOGFILE_CURRENT_LSN_OFFSET))->column.offset);
    aRepl->mCurrentReadLSNFromXLogfile.mOffset = (UInt)sIntData;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::selectReplReceiver( smiStatement * aSmiStmt,
                                       rpdReplications * aRepl )
{
    SChar         sSqlStr[ QD_MAX_SQL_LENGTH ];
    idBool        sRecordExist = ID_FALSE;
    mtdBigintType sBigIntData = 0;
    mtdIntegerType sIntData   = 0;
    //UChar  sRow[ID_SIZEOF(mtdBigintType) + ID_SIZEOF(mtdIntegerType) + ID_SIZEOF(mtdIntegerType)];

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH, "SELECT REMOTE_XSN "
                    "FROM SYS_REPL_RECEIVER_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                     aRepl->mRepName);

    IDE_TEST( qcg::runSelectOneRowforDDL( aSmiStmt,
                                          sSqlStr,
                                          (void*)&sBigIntData,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_FALSE, ERR_META_CRASH );

    aRepl->mRemoteXSN = (smSN) sBigIntData;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH, "SELECT "
                    "CURRENT_READ_XLOGFILE_NO "
                    "FROM SYS_REPL_RECEIVER_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                     aRepl->mRepName);

    IDE_TEST( qcg::runSelectOneRowforDDL( aSmiStmt,
                                          sSqlStr,
                                          (void*)&sIntData,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_FALSE, ERR_META_CRASH );

    aRepl->mCurrentReadLSNFromXLogfile.mFileNo = (UInt)sIntData;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH, "SELECT "
                    "CURRENT_READ_XLOGFILE_OFFSET "
                    "FROM SYS_REPL_RECEIVER_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                     aRepl->mRepName);

    IDE_TEST( qcg::runSelectOneRowforDDL( aSmiStmt,
                                          sSqlStr,
                                          (void*)&sIntData,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_FALSE, ERR_META_CRASH );

    aRepl->mCurrentReadLSNFromXLogfile.mOffset = (UInt)sIntData;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode ( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void
rpdCatalog::setReplHostMember( rpdReplHosts * aReplHost,
                               const void   * aRow )
{
    //----- SYS_REPL_HOSTS_ -----
    // HOST_NO            INTEGER
    // REPLICATION_NAME   CHAR(40)
    // HOST_IP            VARCHAR(40)
    // PORT_NO            INTEGER
    // CONN_TYPE          VARCHAR(20)
    // IB_LATENCY         VARCHAR(10)
    //---------------------------

    mtdCharType  * sCharData;
    mtdIntegerType sIntData;
    RP_SOCKET_TYPE sConnType;
    rpIBLatency    sIBLatency;

    //-------------------------------------------
    // set by SYS_REPL_HOSTS_.HOST_NO
    //-------------------------------------------

    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_HOST_NO))->column.offset );
    aReplHost->mHostNo = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPL_HOSTS_.REPLICATION_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_REPL_NAME))->column.offset);
    idlOS::memcpy( aReplHost->mRepName,
                   sCharData->value,
                   sCharData->length );
    aReplHost->mRepName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_HOSTS_.HOST_IP
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_HOST_IP))->column.offset);
    idlOS::memcpy( aReplHost->mHostIp,
                   sCharData->value,
                   sCharData->length );
    aReplHost->mHostIp[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_HOSTS_.PORT_NO
    //-------------------------------------------

    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_PORT_NO))->column.offset );
    aReplHost->mPortNo = (UInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPL_HOSTS_.CONN_TYPE
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_CONN_TYPE))->column.offset);
    sConnType = ConnTypeStrToEnum( sCharData );
    aReplHost->mConnType = sConnType;


    //-------------------------------------------
    // set by SYS_REPL_HOSTS_.IB_LATENCY
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_IB_LATENCY))->column.offset);
    sIBLatency = IBLatencyStrToEnum( sCharData );
    aReplHost->mIBLatency = sIBLatency;


}

/* PROJ-1915 */
void
rpdCatalog::setReplOfflineDirMember( rpdReplOfflineDirs * aReplOfflineDirs,
                                     const void         * aRow )
{
    //----- SYS_REPL_OFFLINE_DIR_ -----
    // REPLICATION_NAME   VARCHAR(40)
    // LFG_ID             INTEGER
    // PATH               VARCHAR(512)
    //---------------------------

    mtdCharType  * sCharData;
    mtdIntegerType sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OFFLINE_DIR_.REPLICATION_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + 
        ((mtcColumn*)rpdGetTableColumns(gQcmReplOfflineDirs,
                                        QCM_REPL_OFFLINE_DIR_REPL_NAME))->column.offset);
    idlOS::memcpy( aReplOfflineDirs->mRepName,
                   sCharData->value,
                   sCharData->length );
    aReplOfflineDirs->mRepName[sCharData->length] = '\0';

    
    //-------------------------------------------
    // set by SYS_REPL_OFFLINE_DIR_.LFG_ID
    //-------------------------------------------

    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + 
        ((mtcColumn*)rpdGetTableColumns(gQcmReplOfflineDirs,
                                        QCM_REPL_OFFLINE_DIR_LFGID))->column.offset );
    aReplOfflineDirs->mLFG_ID = (SInt) sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OFFLINE_DIR_.PATH
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + 
        ((mtcColumn*)rpdGetTableColumns(gQcmReplOfflineDirs,
                                        QCM_REPL_OFFLINE_DIR_PATH))->column.offset);
    idlOS::memcpy( aReplOfflineDirs->mLogDirPath,
                   sCharData->value,
                   sCharData->length );
    aReplOfflineDirs->mLogDirPath[sCharData->length] = '\0';

}



void
rpdCatalog::setReplItemMember( rpdReplItems * aReplItem,
                               const void   * aRow )
{
    //----- SYS_REPL_ITEMS_ -----
    // REPLICATION_NAME         CHAR(40)
    // TABLE_OID                BIGINT
    // LOCAL_USER_NAME          CHAR(40)
    // LOCAL_TABLE_NAME         CHAR(40)
    // LOCAL_PARTITION_NAME     CHAR(40)
    // REMOTE_USER_NAME         CHAR(40)
    // REMOTE_TABLE_NAME        CHAR(40)
    // REMOTE_PARTITION_NAME    CHAR(40)
    // IS_PARTITION             VARCHAR(1)
    // INVALID_MAX_SN           BIGINT
    // CONDITION                VARCHAR(4000)
    // REPLICATION_UNIT         CHAR(1)
    // IS_CONDITION_SYNC        INTEGER
    //---------------------------

    mtdCharType  * sCharData;
    mtdBigintType  sBigIntData;
    mtdIntegerType sIntData;
    
    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.REPLICATION_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REPL_NAME))->column.offset);
    idlOS::memcpy( aReplItem->mRepName,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mRepName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.TABLE_OID
    //-------------------------------------------

    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_TABLE_OID))->column.offset);
    aReplItem->mTableOID = (ULong) sBigIntData;

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.LOCAL_USER_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_LOCAL_USER))->column.offset);
    idlOS::memcpy( aReplItem->mLocalUsername,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mLocalUsername[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.LOCAL_TABLE_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_LOCAL_TABLE))->column.offset);
    idlOS::memcpy( aReplItem->mLocalTablename,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mLocalTablename[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.LOCAL_PARTITION_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_LOCAL_PARTITION))->column.offset);
    idlOS::memcpy( aReplItem->mLocalPartname,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mLocalPartname[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.REMOTE_USER_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REMOTE_USER))->column.offset);
    idlOS::memcpy( aReplItem->mRemoteUsername,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mRemoteUsername[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.REMOTE_TABLE_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REMOTE_TABLE))->column.offset);
    idlOS::memcpy( aReplItem->mRemoteTablename,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mRemoteTablename[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.REMOTE_PARTITION_NAME
    //-------------------------------------------

    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REMOTE_PARTITION))->column.offset);
    idlOS::memcpy( aReplItem->mRemotePartname,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mRemotePartname[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.IS_PARTITION
    //-------------------------------------------
    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_IS_PARTITION))->column.offset);
    idlOS::memcpy( aReplItem->mIsPartition,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mIsPartition[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.INVALID_MAX_SN
    //-------------------------------------------

    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_INVALID_MAX_SN))->column.offset);
    aReplItem->mInvalidMaxSN = (vULong) sBigIntData;

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.REPLICATION_UNIT
    //------------------------------------------
    /* PROJ-2336 */
    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems, QCM_REPLITEM_REPLICATION_UNIT))->column.offset);
    idlOS::memcpy( aReplItem->mReplicationUnit,
                   sCharData->value,
                   sCharData->length );
    aReplItem->mReplicationUnit[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_ITEMS_.IS_CONDITION_SYNC
    //-------------------------------------------

    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItems, QCM_REPLITEM_IS_CONDITION_SYNCED))->column.offset );
    aReplItem->mIsConditionSynced = (idBool) sIntData;

}

IDE_RC
rpdCatalog::increaseReplItemCount( smiStatement     * aSmiStmt,
                                rpdReplications  * aReplications )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET ITEM_COUNT = ITEM_COUNT + 1 "
                     "WHERE REPLICATION_NAME = '%s'",
                     aReplications->mRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::increaseReplItemCount] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::decreaseReplItemCount( smiStatement     * aSmiStmt,
                                   rpdReplications  * aReplications )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET ITEM_COUNT = ITEM_COUNT - 1 "
                     "WHERE REPLICATION_NAME = '%s'",
                     aReplications->mRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::decreaseReplItemCount] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
rpdCatalog::addReplItemCount( smiStatement     * aSmiStmt,
                              rpdReplications  * aReplications,
                              UInt               aAddCount )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET ITEM_COUNT = ITEM_COUNT + %"ID_UINT32_FMT
                     "WHERE REPLICATION_NAME = '%s'",
                     aAddCount,
                     aReplications->mRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::addReplItemCount] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::minusReplItemCount( smiStatement     * aSmiStmt,
                                rpdReplications  * aReplications,
                                UInt               aMinusCount )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET ITEM_COUNT = ITEM_COUNT - %"ID_UINT32_FMT
                     "WHERE REPLICATION_NAME = '%s'",
                     aMinusCount,
                     aReplications->mRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::minusReplItemCount] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::increaseReplHostCount( smiStatement     * aSmiStmt,
                                   rpdReplications  * aReplications )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET HOST_COUNT = HOST_COUNT + 1 "
                     "WHERE REPLICATION_NAME = '%s'",
                     aReplications->mRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::increaseReplHostCount] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::decreaseReplHostCount( smiStatement     * aSmiStmt,
                                   rpdReplications  * aReplications )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET HOST_COUNT = HOST_COUNT - 1 "
                     "WHERE REPLICATION_NAME = '%s'",
                     aReplications->mRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::decreaseReplHostCount] "
                              "[UPDATE SYS_REPLICATIONS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::checkReplItemExistByName( void          * aQcStatement,
                                      qciNamePosition aReplName,
                                      qciNamePosition aReplTableName,
                                      SChar         * aReplPartitionName,
                                      idBool        * aIsTrue)
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    rpdReplItems      * sReplItem;
    scGRID               sRid;
    smiCursorProperties sProperty;

    *aIsTrue = ID_FALSE;

    sCursor.initialize();

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName.stmtText + aReplName.offset,
                          aReplName.size );

    qciMisc::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REPL_NAME),
                                    (const void *) sReplName,
                                    & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplItems,
                            gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID],
                            smiGetRowSCN( gQcmReplItems ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDU_FIT_POINT( "rpdCatalog::checkReplItemExistByName::malloc::ReplItems",
                   rpERR_ABORT_MEMORY_ALLOC,
                   "rpdCatalog::checkReplItemExistByName",
                   "ReplItems" );
    IDE_TEST(STRUCT_ALLOC(QCI_QMP_MEM(aQcStatement), rpdReplItems, &sReplItem)
             != IDE_SUCCESS);

    while ( sRow != NULL )
    {
        setReplItemMember(sReplItem,  sRow);

        if ( ( idlOS::strMatch( aReplTableName.stmtText + aReplTableName.offset,
                                aReplTableName.size,
                                sReplItem->mLocalTablename,
                                idlOS::strlen( sReplItem->mLocalTablename) ) == 0 ) &&
             ( idlOS::strncmp( aReplPartitionName,
                               sReplItem->mLocalPartname,
                               QC_MAX_OBJECT_NAME_LEN )
               == 0 ) )
        {
            *aIsTrue = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

/*
 * PROJ-2336
 */
IDE_RC rpdCatalog::checkReplItemUnitByName( void          * aQcStatement,
                                            qciNamePosition aReplName,
                                            qciNamePosition aReplTableName,
                                            SChar         * aReplPartitionName,
                                            SChar         * aReplicationUnit,
                                            idBool        * aIsTrue )
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    rpdReplItems      * sReplItem;
    scGRID               sRid;
    smiCursorProperties sProperty;

    *aIsTrue = ID_FALSE;

    sCursor.initialize();

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName.stmtText + aReplName.offset,
                              aReplName.size );

    qciMisc::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplItems, QCM_REPLITEM_REPL_NAME ),
                                        (const void *) sReplName,
                                        & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplItems,
                            gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID],
                            smiGetRowSCN( gQcmReplItems ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDU_FIT_POINT( "rpdCatalog::checkReplItemUnitByName::malloc::ReplItems",
                   rpERR_ABORT_MEMORY_ALLOC,
                   "rpdCatalog::checkReplItemUnitByName",
                   "sReplItems" );
    IDE_TEST( STRUCT_ALLOC( QCI_QMP_MEM( aQcStatement ), rpdReplItems, &sReplItem )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        setReplItemMember( sReplItem, sRow );

        if ( ( idlOS::strMatch( aReplTableName.stmtText + aReplTableName.offset,
                                aReplTableName.size,
                                sReplItem->mLocalTablename,
                                idlOS::strlen( sReplItem->mLocalTablename) ) == 0 ) &&
             ( idlOS::strncmp( aReplPartitionName,
                               sReplItem->mLocalPartname,
                               QC_MAX_OBJECT_NAME_LEN )
               == 0 ) &&
             ( idlOS::strncmp( aReplicationUnit,
                               sReplItem->mReplicationUnit,
                               QC_MAX_OBJECT_NAME_LEN )
               == 0 ) )
        {
            *aIsTrue = ID_TRUE;
            break;
        }
        else
        {
            /* To do nothing */
        }


        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::checkReplItemExistByOID( smiStatement  * aSmiStatement,
                                            smOID           aTableOID,
                                            idBool        * aIsExist )
{
    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    rpdReplItems        sReplItem;
    scGRID              sRid;
    smiCursorProperties sProperty;

    *aIsExist = ID_FALSE;

    sCursor.initialize();

    /* BUG-38088 */
    SMI_CURSOR_PROP_INIT( &sProperty, NULL, NULL );

    IDE_TEST( sCursor.open( aSmiStatement,
                            gQcmReplItems,
                            NULL,
                            smiGetRowSCN( gQcmReplItems ),
                            NULL,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        setReplItemMember(&sReplItem,  sRow);

        if ( sReplItem.mTableOID == aTableOID )
        {
            *aIsExist = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::checkHostIPExistByNameAndHostIP(void           * aQcStatement,
                                                   qciNamePosition  aReplName,
                                                   SChar          * aHostIP,
                                                   idBool         * aIsTrue)
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *) &sReplNameBuffer;
    rpdReplHosts       * sReplHost;

    smiTableCursor       sCursor;
    SInt                 sStage = 0;
    const void         * sRow;
    scGRID               sRid;
    smiCursorProperties  sProperty;

    *aIsTrue = ID_FALSE;

    sCursor.initialize();

    qciMisc::setVarcharValue(sReplName,
                         NULL,
                         aReplName.stmtText + aReplName.offset,
                         aReplName.size);

    qciMisc::makeMetaRangeSingleColumn(&sFirstMetaRange,
                                   (mtcColumn*) rpdGetTableColumns(gQcmReplHosts,
                                                                   QCM_REPLHOST_REPL_NAME),
                                   (const void *) sReplName,
                                   &sRange);

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplHostsIndex[QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT] );

    IDE_TEST(sCursor.open(QCI_SMI_STMT(aQcStatement),
                          gQcmReplHosts,
                          gQcmReplHostsIndex[QCM_REPLHOST_INDEX_NAME_N_IP_N_PORT],
                          smiGetRowSCN(gQcmReplHosts),
                          NULL,
                          &sRange,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &sProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    IDE_TEST(STRUCT_ALLOC(QCI_QMP_MEM(aQcStatement), rpdReplHosts, &sReplHost)
             != IDE_SUCCESS);

    while(sRow != NULL)
    {
        setReplHostMember(sReplHost, sRow);

        if(idlOS::strMatch(aHostIP,
                           idlOS::strlen(aHostIP),
                           sReplHost->mHostIp,
                           idlOS::strlen(sReplHost->mHostIp)) == 0)
        {
            *aIsTrue = ID_TRUE;
            break;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sStage == 1)
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::getNextHostNo(void        * aQcStatement,
                                 SInt        * aHostNo)
{
    void   * sSequenceHandle = NULL;
    idBool   sExist;
    idBool   sFirst;
    SLong    sSeqVal = 0;
    SLong    sSeqValFirst = 0;

    sExist = ID_TRUE;
    sFirst = ID_TRUE;

    IDE_TEST(qciMisc::getSequenceHandleByName(
                 QCI_SMI_STMT( aQcStatement ),
                 QC_SYSTEM_USER_ID,
                 (UChar*)"NEXT_REPL_HOST_NO",
                 idlOS::strlen("NEXT_REPL_HOST_NO"),
                 &sSequenceHandle)
             != IDE_SUCCESS);

    while( sExist == ID_TRUE )
    {
        IDE_TEST(smiTable::readSequence(QCI_SMI_STMT( aQcStatement ),
                                        sSequenceHandle,
                                        SMI_SEQUENCE_NEXT,
                                        &sSeqVal,
                                        NULL)
                 != IDE_SUCCESS);

        // sSeqVal은 비록 SLong이지만, sequence를 생성할 때
        // max를 integer max를 안넘도록 하였기 때문에
        // 여기서 overflow체크는 하지 않는다.
        IDE_TEST( searchReplHostNo( QCI_SMI_STMT( aQcStatement ),
                                    (SInt)sSeqVal,
                                    &sExist )
                  != IDE_SUCCESS );
        
        if( sFirst == ID_TRUE )
        {
            sSeqValFirst = sSeqVal;
            sFirst = ID_FALSE;
        }
        else
        {
            // 찾다찾다 한바퀴 돈 경우.
            // 이는 object가 꽉 찬 것을 의미함.
            IDE_TEST_RAISE( sSeqVal == sSeqValFirst, ERR_OBJECTS_OVERFLOW );    
        }
    }
    
    *aHostNo = sSeqVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_OBJECTS_OVERFLOW);
    {    
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                "replication hosts",
                                QCM_META_SEQ_MAXVALUE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::searchReplHostNo( smiStatement * aSmiStmt,
                                     SInt           aReplHostNo,
                                     idBool       * aExist )
{
/***********************************************************************
 *
 * Description : replication host no가 존재하는지 검사.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    UInt                sStage = 0;
    smiRange            sRange;
    smiTableCursor      sCursor;
    const void          *sRow;
    UInt                sFlag = QCM_META_CURSOR_FLAG;
    mtcColumn          *sQcmReplHostsIndexColumn;
    qriMetaRangeColumn  sRangeColumn;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    if( gQcmReplHostsIndex[QCM_REPLHOST_INDEX_HOSTNO] == NULL )
    {
        // createdb하는 경우임.
        // 이때는 검사 할 필요가 없다
        *aExist = ID_FALSE;
    }
    else
    {
        sCursor.initialize();

        sQcmReplHostsIndexColumn = (mtcColumn*) rpdGetTableColumns(gQcmReplHosts,QCM_REPLHOST_HOST_NO);

        qciMisc::makeMetaRangeSingleColumn(
            &sRangeColumn,
            (const mtcColumn *) sQcmReplHostsIndexColumn,
            (const void *) &aReplHostNo,
            &sRange);

        SMI_CURSOR_PROP_INIT( &sCursorProperty, NULL, gQcmReplHostsIndex[QCM_REPLHOST_INDEX_HOSTNO]);

        IDE_TEST(sCursor.open(
                     aSmiStmt,
                     gQcmReplHosts,
                     gQcmReplHostsIndex[QCM_REPLHOST_INDEX_HOSTNO],
                     smiGetRowSCN(gQcmReplHosts),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     sFlag,
                     SMI_SELECT_CURSOR,
                     &sCursorProperty) != IDE_SUCCESS);
        sStage = 1;

        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        if( sRow == NULL )
        {
            *aExist = ID_FALSE;
        }
        else
        {
            *aExist = ID_TRUE;
        }

        sStage = 0;
        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}


IDE_RC
rpdCatalog::getIndexByAddr( SInt          aLastUsedIP,
                            rpdReplHosts *aReplHosts,
                            SInt          aHostNumber,
                            SInt         *aIndex )
{
    SInt i;

    for( i = 0; i < aHostNumber; i++ )
    {
        if( aReplHosts[i].mHostNo == aLastUsedIP )
        {
            *aIndex = i;
            break;
        }
    }

    IDU_FIT_POINT_RAISE( "rpdCatalog::getIndexByAddr::ERR_HOST_NOT_FOUND", ERR_HOST_NOT_FOUND );
    // BUG-24102
    IDE_TEST_RAISE(i >= aHostNumber, ERR_HOST_NOT_FOUND);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_HOST_NOT_FOUND);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_NOT_EXIST_REPL_HOST));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::selectXLogfileCurrentLSNByName( smiStatement      * aSmiStatement,
                                                   SChar             * aReplName,
                                                   smLSN             * aLSN )
{
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;

    smiTableCursor      sCursor;

    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName,
                              idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn( & sMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplReceiver,QCM_REPL_RECEIVER_REPL_NAME),
                                    (const void*) sReplName,
                                    & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplReceiverIndex[QCM_REPL_RECEIVER_INDEX_REPL_NAME] );

    IDE_TEST( sCursor.open( aSmiStatement,
                            gQcmReplReceiver,
                            gQcmReplReceiverIndex[QCM_REPL_RECEIVER_INDEX_REPL_NAME],
                            smiGetRowSCN( gQcmReplReceiver ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, err_no_rows_found );

    setXLogfileCurrentLSN( aLSN, sRow );

    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    IDU_FIT_POINT_RAISE( "rpdCatalog::selectRepl::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                         err_too_many_rows );
    IDE_TEST_RAISE( sRow != NULL, err_too_many_rows );

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_no_rows_found);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_REPL_NOT_FOUND));
    }
    IDE_EXCEPTION(err_too_many_rows);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::selectRepl] err_too_many_rows"));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

void rpdCatalog::setXLogfileCurrentLSN( smLSN * aXLogfileCurrentLSN,
                                          const void  * aRow)
{
    mtdIntegerType  sIntData;

    //-----------------------------------------------------
    // set by SYS_REPL_RECEIVER_.XLOGFILE_CURRENT_LSN_FILENO
    //-----------------------------------------------------
    sIntData = *(mtdIntegerType*) ((UChar*) aRow +
                    ((mtcColumn*)rpdGetTableColumns(gQcmReplReceiver,
                                                    QCM_REPL_RECEIVER_XLOGFILE_CURRENT_LSN_FILENO))->column.offset);
    aXLogfileCurrentLSN->mFileNo = (UInt)sIntData;

    //-----------------------------------------------------
    // set by SYS_REPL_RECEIVER_.XLOGFILE_CURRENT_LSN_OFFSET
    //-----------------------------------------------------
    sIntData = *(mtdIntegerType*) ((UChar*) aRow +
                    ((mtcColumn*)rpdGetTableColumns(gQcmReplReceiver,
                                                    QCM_REPL_RECEIVER_XLOGFILE_CURRENT_LSN_OFFSET))->column.offset);
    aXLogfileCurrentLSN->mOffset = (UInt)sIntData;
}

IDE_RC rpdCatalog::isConsistentReplication( void          * aQcStatement,
                                            qciNamePosition aReplName,
                                            idBool        * aIsTrue)
{

    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;

    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    mtdIntegerType      sIntData;

    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;

    UInt                sReplicationMode;

    sCursor.initialize();

    qciMisc::setVarcharValue( (mtdCharType*) sReplName,
                              NULL,
                              aReplName.stmtText + aReplName.offset,
                              aReplName.size );

    qciMisc::makeMetaRangeSingleColumn(&sMetaRange,
                                   (mtcColumn*)rpdGetTableColumns(gQcmReplications,QCM_REPLICATION_REPL_NAME),
                                   (const void*) sReplName,
                                   & sRange);

    SMI_CURSOR_PROP_INIT( &sProperty,
                          QCI_STATISTIC( aQcStatement ),
                          gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME] );

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplications,
                            gQcmReplicationsIndex[QCM_REPL_INDEX_REPL_NAME],
                            smiGetRowSCN( gQcmReplications ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        sIntData = *(mtdIntegerType*)
            ((UChar*) sRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplications,
                QCM_REPLICATION_REPL_MODE))->column.offset);
        sReplicationMode = (SInt) sIntData;

        if ( sReplicationMode == RP_CONSISTENT_MODE )
        {
            *aIsTrue = ID_TRUE;
        }
        else
        {
            *aIsTrue = ID_FALSE;
        }

    }
    else
    {
        *aIsTrue = ID_FALSE;
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}

/* SYS_REPL_OLD_ITEMS_ */
IDE_RC rpdCatalog::insertReplOldItem(smiStatement * aSmiStmt,
                                     SChar        * aReplName,
                                     rpdOldItem   * aItem)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_OLD_ITEMS_ VALUES ( "
                     "CHAR'%s', BIGINT'%"ID_INT64_FMT"', "
                     "CHAR'%s', CHAR'%s', CHAR'%s', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "CHAR'%s', "
                     "CHAR'%s', "
                     "CHAR'%s', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "VARCHAR'%s', "
                     "VARCHAR'%s', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "CHAR'%s', "
                     "CHAR'%s', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"' )",
                     aReplName,
                     (SLong)aItem->mTableOID,
                     aItem->mUserName,
                     aItem->mTableName,
                     aItem->mPartName,
                     (SInt)aItem->mPKIndexID,
                     aItem->mRemoteUserName,
                     aItem->mRemoteTableName,
                     aItem->mRemotePartName,
                     aItem->mPartitionOrder,
                     aItem->mPartCondMinValues,
                     aItem->mPartCondMaxValues,
                     aItem->mInvalidMaxSN,
                     (SInt)aItem->mTableID,
                     (SInt)aItem->mTablePartitionType,
                     aItem->mIsPartition,
                     aItem->mReplicationUnit,
                     (SInt)aItem->mTBSType,
                     (SInt)aItem->mPartitionMethod,
                     (SInt)aItem->mPartitionCount );

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_EXECUTE_ITEM);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXECUTE_ITEM);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplOldItem] "
                              "[INSERT SYS_REPL_OLD_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::deleteReplOldItem(smiStatement * aSmiStmt,
                                     SChar        * aReplName,
                                     ULong          aTableOID)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_ITEMS_ "
                    "WHERE REPLICATION_NAME = CHAR'%s' "
                    "AND TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                    aReplName,
                    (SLong)aTableOID);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::removeReplOldItems(smiStatement * aSmiStmt,
                                   SChar        * aReplName)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_ITEMS_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                    aReplName);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::getReplOldItemsCount(smiStatement * aSmiStmt,
                                        SChar        * aReplName,
                                        vSLong       * aItemCount)
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;

    qciMisc::setVarcharValue(sReplName,
                         NULL,
                         aReplName,
                         idlOS::strlen(aReplName));

    qciMisc::makeMetaRangeSingleColumn
            (&sFirstMetaRange,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldItems,
                    QCM_REPLOLDITEMS_REPLICATION_NAME),
             (const void *)sReplName,
             &sRange);

    IDE_TEST(qciMisc::selectCount( aSmiStmt,
                                   gQcmReplOldItems,
                                   aItemCount,
                                   smiGetDefaultFilter(),
                                   &sRange,
                                   gQcmReplOldItemsIndex
                                   [QCM_REPLOLDITEM_INDEX_NAME_OID])
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::selectReplOldItems(smiStatement * aSmiStmt,
                                      SChar        * aReplName,
                                      rpdOldItem   * aItemArr,
                                      vSLong         aItemCount)
{
    smiRange              sRange;
    qriMetaRangeColumn    sFirstMetaRange;

    qriNameCharBuffer     sReplNameBuffer;
    mtdCharType         * sReplName = (mtdCharType *)&sReplNameBuffer;

    smiTableCursor        sCursor;
    SInt                  sStage = 0;
    vSLong                sCount = 0;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sProperty;

    qciMisc::setVarcharValue(sReplName,
                         NULL,
                         aReplName,
                         idlOS::strlen(aReplName));

    qciMisc::makeMetaRangeSingleColumn
            (&sFirstMetaRange,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldItems,
                    QCM_REPLOLDITEMS_REPLICATION_NAME),
             (const void *)sReplName,
             &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT(&sProperty, NULL, gQcmReplOldItemsIndex[QCM_REPLOLDITEM_INDEX_NAME_OID]);

    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmReplOldItems,
                          gQcmReplOldItemsIndex[QCM_REPLOLDITEM_INDEX_NAME_OID],
                          smiGetRowSCN(gQcmReplOldItems),
                          NULL,
                          &sRange,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &sProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while(sRow != NULL)
    {
        IDE_TEST_RAISE(sCount >= aItemCount, ERR_TOO_MANY_REPL_ITEMS);

        setReplOldItemMember(&aItemArr[sCount], sRow);

        sCount++;

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE(sCount != aItemCount, ERR_NOT_ENOUGH_REPL_ITEMS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_MANY_REPL_ITEMS);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_ITEMS,
                                aReplName));
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_REPL_ITEMS);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_ITEMS,
                                aReplName));
    }
    IDE_EXCEPTION_END;

    if(sStage == 1)
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}
void rpdCatalog::setReplOldItemMember(rpdOldItem * aReplOldItem,
                                      const void * aRow)
{
    //----- SYS_REPL_OLD_ITEMS_ -----
    // REPLICATION_NAME         CHAR(40)
    // TABLE_OID                BIGINT
    // USER_NAME                CHAR(40)
    // TABLE_NAME               CHAR(40)
    // PARTITION_NAME           CHAR(40)
    // PRIMARY_KEY_INDEX_ID     INTEGER
    // REMOTE_USER_NAME         CHAR(128)
    // REMOTE_TABLE_NAME        CHAR(128)
    // REMOTE_PARTITION_NAME    CHAR(128)
    // PARTITION_ORDER          INT
    // PARTITION_MIN_VALUE      VARCHAR(4000)
    // PARTITION_MAX_VALUE      VARCHAR(4000)
    // INVALID_MAX_SN           BIGINT
    // TABLE_ID                 INTEGER
    // TABLE_PARTITION_TYPE     INTEGER
    // IS_PARTITION             CHAR(1)
    // REPLICATION_UNIT         CHAR(1)
    // TBS_TYPE                 INTEGER
    // PARTITION_METHOD         INTEGER
    // PARTITION_COUNT          INTEGER
    //-------------------------------

    mtdCharType    * sCharData;
    mtdBigintType    sBigIntData;
    mtdIntegerType   sIntData;
    mtcColumn      * sPartMinValueMtcColumn;
    mtcColumn      * sPartMaxValueMtcColumn;
    const mtdCharType * sPartCondMinVal;
    const mtdCharType * sPartCondMaxVal;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.REPLICATION_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
        ((UChar*)aRow +
         ((mtcColumn*)rpdGetTableColumns(
                 gQcmReplOldItems,
                 QCM_REPLOLDITEMS_REPLICATION_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mRepName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mRepName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.TABLE_OID
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
                  ((UChar*)aRow +
                   ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_TABLE_OID))->column.offset);
    aReplOldItem->mTableOID = (vULong)sBigIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.USER_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_USER_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mUserName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mUserName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.TABLE_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_TABLE_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mTableName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mTableName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PARTITION_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_PARTITION_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mPartName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mPartName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PRIMARY_KEY_INDEX_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_PRIMARY_KEY_INDEX_ID))->column.offset);
    aReplOldItem->mPKIndexID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.REMOTE_USER_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_REMOTE_USER_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mRemoteUserName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mRemoteUserName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.REMOTE_TABLE_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_REMOTE_TABLE_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mRemoteTableName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mRemoteTableName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.REMOTE_PARTITION_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_REMOTE_PARTITION_NAME))->column.offset);
    idlOS::memcpy(aReplOldItem->mRemotePartName,
                  sCharData->value,
                  sCharData->length);
    aReplOldItem->mRemotePartName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PARTITION_ORDER
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_PARTITION_ORDER))->column.offset);
    aReplOldItem->mPartitionOrder = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PARTITION_MIN_VALUE
    //-------------------------------------------
    sPartMinValueMtcColumn = ((mtcColumn*)rpdGetTableColumns(
            gQcmReplOldItems,
            QCM_REPLOLDITEMS_PARTITION_MIN_VALUE) );

    sPartCondMinVal = (const mtdCharType *)mtc::value( sPartMinValueMtcColumn,
                                                       aRow,
                                                       MTD_OFFSET_USE );
    if( sPartCondMinVal->length > 0 )
    {
        idlOS::memcpy( aReplOldItem->mPartCondMinValues,
                       sPartCondMinVal->value,
                       sPartCondMinVal->length );        
        aReplOldItem->mPartCondMinValues[sPartCondMinVal->length] = '\0';
    }

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PARTITION_MAX_VALUE
    //-------------------------------------------
    sPartMaxValueMtcColumn = ((mtcColumn*)rpdGetTableColumns(
            gQcmReplOldItems,
            QCM_REPLOLDITEMS_PARTITION_MAX_VALUE) );

    sPartCondMaxVal = (const mtdCharType *)mtc::value( sPartMaxValueMtcColumn,
                                                       aRow,
                                                       MTD_OFFSET_USE );
    if( sPartCondMaxVal->length > 0 )
    {
        idlOS::memcpy( aReplOldItem->mPartCondMaxValues,
                       sPartCondMaxVal->value,
                       sPartCondMaxVal->length );
        aReplOldItem->mPartCondMaxValues[sPartCondMaxVal->length] = '\0';
    }

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.INVALID_MAX_SN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplOldItems,QCM_REPLOLDITEMS_INVALID_MAX_SN))->column.offset);
    aReplOldItem->mInvalidMaxSN = (vULong) sBigIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.TABLE_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ( (UChar*)aRow +
          ( (mtcColumn*)rpdGetTableColumns(
                  gQcmReplOldItems,
                  QCM_REPLOLDITEMS_TABLE_ID ) )->column.offset );
    aReplOldItem->mTableID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.TABLE_PARTITION_TYPE
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ( (UChar*)aRow +
          ( (mtcColumn*)rpdGetTableColumns(
                  gQcmReplOldItems,
                  QCM_REPLOLDITEMS_TABLE_PARTITION_TYPE ) )->column.offset );
    aReplOldItem->mTablePartitionType = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.IS_PARTITION
    //-------------------------------------------
    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns( gQcmReplOldItems,
                                                          QCM_REPLOLDITEMS_IS_PARTITION ) )->column.offset);
    idlOS::memcpy( aReplOldItem->mIsPartition,
                   sCharData->value,
                   sCharData->length );
    aReplOldItem->mIsPartition[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.REPLICATION_UNIT
    //------------------------------------------
    sCharData = (mtdCharType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns( gQcmReplOldItems, 
                                                          QCM_REPLOLDITEMS_REPLICATION_UNIT) )->column.offset);
    idlOS::memcpy( aReplOldItem->mReplicationUnit,
                   sCharData->value,
                   sCharData->length );
    aReplOldItem->mReplicationUnit[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.TBS_TYPE
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ( (UChar*)aRow +
          ( (mtcColumn*)rpdGetTableColumns(
                  gQcmReplOldItems,
                  QCM_REPLOLDITEMS_TBS_TYPE ) )->column.offset );
    aReplOldItem->mTBSType = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PARTITION_METHOD
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ( (UChar*)aRow +
          ( (mtcColumn*)rpdGetTableColumns(
                  gQcmReplOldItems,
                  QCM_REPLOLDITEMS_PARTITION_METHOD ) )->column.offset );
    aReplOldItem->mPartitionMethod = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_ITEMS_.PARTITION_COUNT
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldItems,
                        QCM_REPLOLDITEMS_PARTITION_COUNT))->column.offset);
    aReplOldItem->mPartitionCount = (UInt)sIntData;
}

IDE_RC rpdCatalog::getStrForMeta( SChar        * aSrcStr,
                                  SInt           aSize,
                                  SChar       ** aStrForMeta )
{
    SInt      i;
    SInt      j;
    SInt      sDefaultValLen = 0;

    for (i = 0; i < aSize; i++)
    {
        sDefaultValLen ++;
        if (aSrcStr[i] == '\'')
        {
            sDefaultValLen ++;
        }
    }

    IDE_DASSERT( *aStrForMeta == NULL );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_RP_RPD,
                                 sDefaultValLen + 1,
                                 ID_SIZEOF(SChar),
                                 (void**)aStrForMeta,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    j = 0;
    for (i = 0; i < aSize; i++)
    {
        if (aSrcStr[i] == '\'')
        {
            (*aStrForMeta)[j] = '\'';
            j++;
            (*aStrForMeta)[j] = '\'';
        }
        else
        {
            (*aStrForMeta)[j] = aSrcStr[i];
        }
        j++;
    }
    (*aStrForMeta)[sDefaultValLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aStrForMeta = NULL;

    return IDE_FAILURE;
}

/* SYS_REPL_OLD_COLUMNS_ */
IDE_RC rpdCatalog::insertReplOldColumn(smiStatement  * aSmiStmt,
                                       SChar         * aReplName,
                                       ULong           aTableOID,
                                       smiColumnMeta * aColumn )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;
    SChar  * sStrForMeta = NULL;
    const SChar  * sDefaultValStr = "";

    if ( aColumn->mDefaultValStr == NULL )
    {
        /* do nothing sDefaultValStr = "" */
    }
    else
    {
        IDE_TEST( getStrForMeta( aColumn->mDefaultValStr, 
                                 idlOS::strlen( aColumn->mDefaultValStr ),
                                 &sStrForMeta ) != IDE_SUCCESS );
        sDefaultValStr = sStrForMeta;
    }

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "INSERT INTO SYS_REPL_OLD_COLUMNS_ VALUES ( "
                    "CHAR'%s', BIGINT'%"ID_INT64_FMT"', CHAR'%s', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "CHAR'%s', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "CHAR'%s' )",
                    aReplName,
                    (SLong)aTableOID,
                    aColumn->mName,
                    (SInt)aColumn->mMTDatatypeID,
                    (SInt)aColumn->mMTLanguageID,
                    (SInt)aColumn->mMTFlag,
                    aColumn->mMTPrecision,
                    aColumn->mMTScale,
                    //BUG-26891 : 보안 컬럼 관련 정보
                    aColumn->mMTColumnAttr.mEncAttr.mEncPrecision,
                    aColumn->mMTColumnAttr.mEncAttr.mPolicy,
                    aColumn->mMTColumnAttr.mSridAttr.mSrid,
                    (SInt)aColumn->mSMID,
                    (SInt)aColumn->mSMFlag,
                    (SInt)aColumn->mSMOffset,
                    (SInt)aColumn->mSMVarOrder,
                    (SInt)aColumn->mSMSize,
                    (smOID)aColumn->mSMDictTblOID,
                    (scSpaceID)aColumn->mSMColSpace,
                    (SInt)aColumn->mQPFlag,
                    sDefaultValStr );

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt) != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_EXECUTE_COLUMN);

    if( sStrForMeta != NULL )
    {
        (void)iduMemMgr::free( sStrForMeta );
        sStrForMeta = NULL;
    }
    else
    {
        /*do nothing*/
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXECUTE_COLUMN);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplOldColumn] "
                              "[INSERT SYS_REPL_OLD_COLUMNS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if( sStrForMeta != NULL )
    {
        (void)iduMemMgr::free( sStrForMeta );
        sStrForMeta = NULL;
    }
    else
    {
        /*do nothing*/
    }
    return IDE_FAILURE;
}

IDE_RC rpdCatalog::deleteReplOldColumns(smiStatement * aSmiStmt,
                                        SChar        * aReplName,
                                        ULong          aTableOID)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_COLUMNS_ "
                    "WHERE REPLICATION_NAME = CHAR'%s' "
                    "AND TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                    aReplName,
                    (SLong)aTableOID);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::removeReplOldColumns(smiStatement * aSmiStmt,
                                        SChar        * aReplName)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_COLUMNS_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                    aReplName);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::getReplOldColumnsCount(smiStatement * aSmiStmt,
                                          SChar        * aReplName,
                                          ULong          aTableOID,
                                          vSLong       * aColumnCount)
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID;

    qciMisc::setVarcharValue(sReplName,
                         NULL,
                         aReplName,
                         idlOS::strlen(aReplName));
    sTableOID = (mtdBigintType)aTableOID;

    qciMisc::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        (mtcColumn*)rpdGetTableColumns(
                                                        gQcmReplOldCols,
                                                        QCM_REPLOLDCOLS_REPLICATION_NAME),
                                        (const void*)sReplName,
                                        (mtcColumn*)rpdGetTableColumns(
                                                        gQcmReplOldCols,
                                                        QCM_REPLOLDCOLS_TABLE_OID),
                                        &sTableOID,
                                        &sRange );

    IDE_TEST(qciMisc::selectCount(aSmiStmt,
                              gQcmReplOldCols,
                              aColumnCount,
                              smiGetDefaultFilter(),
                              &sRange,
                              gQcmReplOldColsIndex
                              [QCM_REPLOLDCOL_INDEX_NAME_OID_COL])
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::selectReplOldColumns(smiStatement  * aSmiStmt,
                                        SChar         * aReplName,
                                        ULong           aTableOID,
                                        smiColumnMeta * aColumnArr,
                                        vSLong          aColumnCount)
{
    smiRange              sRange;
    qriMetaRangeColumn    sFirstMetaRange;
    qriMetaRangeColumn    sSecondMetaRange;

    qriNameCharBuffer     sReplNameBuffer;
    mtdCharType         * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType         sTableOID;

    smiTableCursor        sCursor;
    SInt                  sStage = 0;
    vSLong                sCount = 0;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sProperty;

    UInt                  i = 0;

    qciMisc::setVarcharValue(sReplName,
                             NULL,
                             aReplName,
                             idlOS::strlen(aReplName));
    sTableOID = (mtdBigintType)aTableOID;

    qciMisc::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        (mtcColumn*)rpdGetTableColumns(
                                                        gQcmReplOldCols,
                                                        QCM_REPLOLDCOLS_REPLICATION_NAME),
                                        (const void*)sReplName,
                                        (mtcColumn*)rpdGetTableColumns(
                                                        gQcmReplOldCols,
                                                        QCM_REPLOLDCOLS_TABLE_OID),
                                        &sTableOID,
                                        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplOldColsIndex[QCM_REPLOLDCOL_INDEX_NAME_OID_COL] );

    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmReplOldCols,
                          gQcmReplOldColsIndex[QCM_REPLOLDCOL_INDEX_NAME_OID_COL],
                          smiGetRowSCN(gQcmReplOldCols),
                          NULL,
                          &sRange,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &sProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while(sRow != NULL)
    {
        IDE_TEST_RAISE(sCount >= aColumnCount, ERR_TOO_MANY_REPL_COLUMNS);

        IDE_TEST( setReplOldColumnMember(&aColumnArr[sCount], sRow) != IDE_SUCCESS );

        sCount++;

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE(sCount != aColumnCount, ERR_NOT_ENOUGH_REPL_COLUMNS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_MANY_REPL_COLUMNS);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_COLUMNS,
                                aReplName,
                                aTableOID));
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_REPL_COLUMNS);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_COLUMNS,
                                aReplName,
                                aTableOID));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    for ( i = 0; i < sCount; i++)
    {
        if ( aColumnArr[i].mDefaultValStr != NULL )
        {
            (void)iduMemMgr::free( aColumnArr[i].mDefaultValStr );
            aColumnArr[i].mDefaultValStr = NULL;
        }
        else
        {
            /* do nothing */
        }
    }
    IDE_POP();

    if(sStage == 1)
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::setReplOldColumnMember( smiColumnMeta * aReplOldColumn,
                                           const void    * aRow )
{
    //----- SYS_REPL_OLD_COLUMNS_ -----
    // REPLICATION_NAME         VARCHAR(40)
    // TABLE_OID                BIGINT
    // COLUMN_NAME              VARCHAR(40)
    // MT_DATATYPE_ID           INTEGER
    // MT_LANGUAGE_ID           INTEGER
    // MT_FLAG                  INTEGER
    // MT_PRECISION             INTEGER
    // MT_SCALE                 INTEGER
    // MT_ENCRYPT_PRECISON      INTEGER
    // MT_POLICY_NAME           VARCHAR(16)
    // MT_SRID                  INTEGER
    // SM_ID                    INTEGER
    // SM_FLAG                  INTEGER
    // SM_OFFSET                INTEGER
    // SM_VARORDER              INTEGER
    // SM_SIZE                  INTEGER
    // SM_DIC_TABLE_OID         BIGINT
    // SM_COL_SPACE             INTEGER
    // QP_FLAG                  INTEGER
    // DEFAULT_VAL              VARCHAR(4000)
    //---------------------------------

    mtdCharType    * sCharData;
    mtdIntegerType   sIntData;
    mtdBigintType    sBigIntData;
    scSpaceID        sTempData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.COLUMN_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_COLUMN_NAME))->column.offset);
    idlOS::memcpy(aReplOldColumn->mName,
                  sCharData->value,
                  sCharData->length);
    aReplOldColumn->mName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_DATATYPE_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_DATATYPE_ID))->column.offset);
    aReplOldColumn->mMTDatatypeID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_LANGUAGE_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_LANGUAGE_ID))->column.offset);
    aReplOldColumn->mMTLanguageID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_FLAG
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_FLAG))->column.offset);
    aReplOldColumn->mMTFlag = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_PRECISION
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_PRECISION))->column.offset);
    aReplOldColumn->mMTPrecision = (SInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_SCALE
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_SCALE))->column.offset);
    aReplOldColumn->mMTScale = (SInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_ECCRYPT_PRECISION
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_ECRYPT_PRECISION))->column.offset);
    aReplOldColumn->mMTColumnAttr.mEncAttr.mEncPrecision = (SInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_POLICY_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLODLCOLS_MT_POLICY_NAME))->column.offset);
    idlOS::memcpy(aReplOldColumn->mMTColumnAttr.mEncAttr.mPolicy,
                  sCharData->value,
                  sCharData->length);
    aReplOldColumn->mMTColumnAttr.mEncAttr.mPolicy[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.MT_SRID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_MT_SRID))->column.offset);
    aReplOldColumn->mMTColumnAttr.mSridAttr.mSrid = (SInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_SM_ID))->column.offset);
    aReplOldColumn->mSMID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_FLAG
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_SM_FLAG))->column.offset);
    aReplOldColumn->mSMFlag = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_OFFSET
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_SM_OFFSET))->column.offset);
    aReplOldColumn->mSMOffset = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_VARORDER
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
                ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_SM_VARORDER))->column.offset);
    aReplOldColumn->mSMVarOrder = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_SIZE
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldCols,
                        QCM_REPLOLDCOLS_SM_SIZE))->column.offset);
    aReplOldColumn->mSMSize = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_DIC_TABLE_OID
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplOldCols,
                QCM_REPLOLDCOLS_SM_DIC_TABLE_OID))->column.offset );
    aReplOldColumn->mSMDictTblOID = *( (smOID*) &sBigIntData );

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.SM_COL_SPACE
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplOldCols,
                QCM_REPLOLDCOLS_SM_COL_SPACE))->column.offset );
    sTempData = (scSpaceID) sIntData;
    aReplOldColumn->mSMColSpace = sTempData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.QP_FLAG
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)( (UChar*)aRow + 
                                   ((mtcColumn*)rpdGetTableColumns( 
                                                    gQcmReplOldCols,
                                                    QCM_REPLOLDCOLS_QP_FLAG) )->column.offset );
    aReplOldColumn->mQPFlag = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_COLUMNS_.DEFAULT_VAL_STR
    //-------------------------------------------
    sCharData = (mtdCharType *)mtc::value( (mtcColumn*)rpdGetTableColumns( gQcmReplOldCols,
                                                                           QCM_REPLOLDCOLS_DEFAULT_VAL_STR),
                                           aRow,
                                           MTD_OFFSET_USE );

    if ( sCharData->length != 0 )
    {
        IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPD_META,
                                           sCharData->length + 1,
                                           ID_SIZEOF(SChar),
                                           (void **)&(aReplOldColumn->mDefaultValStr),
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_DEFAULT_VAL_STR );

        idlOS::memcpy(aReplOldColumn->mDefaultValStr,
                      sCharData->value,
                      sCharData->length);
        aReplOldColumn->mDefaultValStr[sCharData->length] = '\0';
    }
    else
    {
        aReplOldColumn->mDefaultValStr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DEFAULT_VAL_STR );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpdCatalog::setReplOldColumnMember"
                                  "aReplOldColumn->mDefaultValStr" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* SYS_REPL_OLD_INDICES_ */
IDE_RC rpdCatalog::insertReplOldIndex(smiStatement * aSmiStmt,
                                   SChar        * aReplName,
                                   ULong          aTableOID,
                                   smiIndexMeta * aIndex)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "INSERT INTO SYS_REPL_OLD_INDICES_ VALUES ( "
                    "CHAR'%s', BIGINT'%"ID_INT64_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "CHAR'%s', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "CHAR'%s', "
                    "CHAR'%s', "
                    "CHAR'%s' )",
                    aReplName,
                    (SLong)aTableOID,
                    (SInt)aIndex->mIndexID,
                    aIndex->mName,
                    (SInt)aIndex->mTypeID,
                    (aIndex->mIsUnique == ID_TRUE) ? "Y" : "N",
                    (aIndex->mIsLocalUnique == ID_TRUE) ? "Y" : "N",
                    (aIndex->mIsRange == ID_TRUE) ? "Y" : "N");

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_EXECUTE_INDEX);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXECUTE_INDEX);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplOldIndex] "
                              "[INSERT SYS_REPL_OLD_INDICES_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::deleteReplOldIndices(smiStatement * aSmiStmt,
                                        SChar        * aReplName,
                                        ULong          aTableOID)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    // SYS_REPL_OLD_INDICES_
    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_INDICES_ "
                    "WHERE REPLICATION_NAME = CHAR'%s' "
                    "AND TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                    aReplName,
                    (SLong)aTableOID);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::removeReplOldIndices(smiStatement * aSmiStmt,
                                        SChar        * aReplName)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_INDICES_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                    aReplName);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::getReplOldIndexCount(smiStatement * aSmiStmt,
                                        SChar        * aReplName,
                                        ULong          aTableOID,
                                        vSLong       * aIndexCount)
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID;

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName,
                              idlOS::strlen(aReplName));
    sTableOID = (mtdBigintType)aTableOID;

    qciMisc::makeMetaRangeDoubleColumn
            (&sFirstMetaRange,
            &sSecondMetaRange,
            (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxs,
            QCM_REPLOLDIDXS_REPLICATION_NAME),
            (const void*)sReplName,
            (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxs,
            QCM_REPLOLDIDXS_TABLE_OID),
            &sTableOID,
            &sRange);

    IDE_TEST(qciMisc::selectCount(aSmiStmt,
                                  gQcmReplOldIdxs,
                                  aIndexCount,
                                  smiGetDefaultFilter(),
                                  &sRange,
                                  gQcmReplOldIdxsIndex
                                      [QCM_REPLOLDIDX_INDEX_NAME_OID_IDX])
            != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::selectReplOldIndices(smiStatement * aSmiStmt,
                                     SChar        * aReplName,
                                     ULong          aTableOID,
                                     smiIndexMeta * aIndexArr,
                                     vSLong         aIndexCount)
{
    smiRange              sRange;
    qriMetaRangeColumn    sFirstMetaRange;
    qriMetaRangeColumn    sSecondMetaRange;

    qriNameCharBuffer     sReplNameBuffer;
    mtdCharType         * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType         sTableOID;

    smiTableCursor        sCursor;
    SInt                  sStage = 0;
    vSLong                sCount = 0;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sProperty;

    qciMisc::setVarcharValue(sReplName,
                             NULL,
                             aReplName,
                             idlOS::strlen(aReplName));
    sTableOID = (mtdBigintType)aTableOID;

    qciMisc::makeMetaRangeDoubleColumn(
                             &sFirstMetaRange,
                             &sSecondMetaRange,
                             (mtcColumn*)rpdGetTableColumns(
                                                 gQcmReplOldIdxs,
                                                 QCM_REPLOLDIDXS_REPLICATION_NAME),
                             (const void*)sReplName,
                             (mtcColumn*)rpdGetTableColumns(
                                                 gQcmReplOldIdxs,
                                                 QCM_REPLOLDIDXS_TABLE_OID),
                             &sTableOID,
                             &sRange);

    SMI_CURSOR_PROP_INIT( &sProperty, NULL , gQcmReplOldIdxsIndex[QCM_REPLOLDIDX_INDEX_NAME_OID_IDX] );

    sCursor.initialize();

    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmReplOldIdxs,
                          gQcmReplOldIdxsIndex[QCM_REPLOLDIDX_INDEX_NAME_OID_IDX],
                          smiGetRowSCN(gQcmReplOldIdxs),
                          NULL,
                          &sRange,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &sProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while(sRow != NULL)
    {
        IDE_TEST_RAISE(sCount >= aIndexCount, ERR_TOO_MANY_REPL_INDICES);

        setReplOldIndexMember(&aIndexArr[sCount], sRow);

        sCount++;

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE(sCount != aIndexCount, ERR_NOT_ENOUGH_REPL_INDICES);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_MANY_REPL_INDICES);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_INDICES,
                                aReplName,
                                aTableOID));
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_REPL_INDICES);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_INDICES,
                                aReplName,
                                aTableOID));
    }
    IDE_EXCEPTION_END;

    if(sStage == 1)
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}
void rpdCatalog::setReplOldIndexMember(smiIndexMeta * aReplOldIndex,
                                       const void   * aRow)
{
    //----- SYS_REPL_OLD_INDICES_ -----
    // REPLICATION_NAME         CHAR(40)
    // TABLE_OID                BIGINT
    // INDEX_ID                 INTEGER
    // INDEX_NAME               CHAR(40)
    // TYPE_ID                  INTEGER
    // IS_UNIQUE                CHAR(1)
    // IS_LOCAL_UNIQUE          CHAR(1)
    // IS_RANGE                 CHAR(1)
    //---------------------------------

    mtdCharType    * sCharData;
    mtdIntegerType   sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDICES_.INDEX_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxs,
                        QCM_REPLOLDIDXS_INDEX_ID))->column.offset);
    aReplOldIndex->mIndexID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDICES_.INDEX_NAME
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxs,
                        QCM_REPLOLDIDXS_INDEX_NAME))->column.offset);
    idlOS::memcpy(aReplOldIndex->mName,
                  sCharData->value,
                  sCharData->length);
    aReplOldIndex->mName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDICES_.TYPE_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxs,
                        QCM_REPLOLDIDXS_TYPE_ID))->column.offset);
    aReplOldIndex->mTypeID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDICES_.IS_UNIQUE
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxs,
                        QCM_REPLOLDIDXS_IS_UNIQUE))->column.offset);
    if(idlOS::strncmp((const SChar *)"Y",
                      (const SChar *)sCharData->value,
                      sCharData->length) == 0)
    {
        aReplOldIndex->mIsUnique = ID_TRUE;
    }
    else
    {
        aReplOldIndex->mIsUnique = ID_FALSE;
    }

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDICES_.IS_LOCAL_UNIQUE
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxs,
                        QCM_REPLOLDIDXS_IS_LOCAL_UNIQUE))->column.offset);
    if(idlOS::strncmp((const SChar *)"Y",
                      (const SChar *)sCharData->value,
                      sCharData->length) == 0)
    {
        aReplOldIndex->mIsLocalUnique = ID_TRUE;
    }
    else
    {
        aReplOldIndex->mIsLocalUnique = ID_FALSE;
    }

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDICES_.IS_RANGE
    //-------------------------------------------
    sCharData = (mtdCharType*)
                ((UChar*)aRow +
                 ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxs,
                        QCM_REPLOLDIDXS_IS_RANGE))->column.offset);
    if(idlOS::strncmp((const SChar *)"Y",
                      (const SChar *)sCharData->value,
                      sCharData->length) == 0)
    {
        aReplOldIndex->mIsRange = ID_TRUE;
    }
    else
    {
        aReplOldIndex->mIsRange = ID_FALSE;
    }
}

/* SYS_REPL_OLD_INDEX_COLUMNS_ */
IDE_RC rpdCatalog::insertReplOldIndexCol(smiStatement       * aSmiStmt,
                                         SChar              * aReplName,
                                         ULong                aTableOID,
                                         UInt                 aIndexID,
                                         smiIndexColumnMeta * aIndexCol)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "INSERT INTO SYS_REPL_OLD_INDEX_COLUMNS_ VALUES ( "
                    "CHAR'%s', BIGINT'%"ID_INT64_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"' )",
                    aReplName,
                    (SLong)aTableOID,
                    (SInt)aIndexID,
                    (SInt)(aIndexCol->mID & SMI_COLUMN_ID_MASK),
                    (SInt)aIndexCol->mFlag,
                    (SInt)aIndexCol->mCompositeOrder);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    IDE_TEST_RAISE(sRowCnt != 1, ERR_EXECUTE_INDEX_COLUMN);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXECUTE_INDEX_COLUMN);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplOldIndexCol] "
                              "[INSERT SYS_REPL_OLD_INDEX_COLUMNS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::deleteReplOldIndexCols(smiStatement * aSmiStmt,
                                          SChar        * aReplName,
                                          ULong          aTableOID)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    // SYS_REPL_OLD_INDEX_COLUMNS_
    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_INDEX_COLUMNS_ "
                    "WHERE REPLICATION_NAME = CHAR'%s' "
                    "AND TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                    aReplName,
                    (SLong)aTableOID);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::removeReplOldIndexCols(smiStatement * aSmiStmt,
                                          SChar        * aReplName)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                    "DELETE FROM SYS_REPL_OLD_INDEX_COLUMNS_ "
                    "WHERE REPLICATION_NAME = CHAR'%s'",
                    aReplName);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::getReplOldIndexColCount(smiStatement * aSmiStmt,
                                           SChar        * aReplName,
                                           ULong          aTableOID,
                                           UInt           aIndexID,
                                           vSLong       * aIndexColCount)
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;
    qriMetaRangeColumn   sThirdMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID;
    mtdIntegerType       sIndexID;

    qciMisc::setVarcharValue(sReplName,
                             NULL,
                             aReplName,
                             idlOS::strlen(aReplName));
    sTableOID = (mtdBigintType)aTableOID;
    sIndexID = (mtdIntegerType)aIndexID;

    qciMisc::makeMetaRangeTripleColumn
            (&sFirstMetaRange,
             &sSecondMetaRange,
             &sThirdMetaRange,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxCols,
                    QCM_REPLOLDIDXCOLS_REPLICATION_NAME),
             (const void*)sReplName,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxCols,
                    QCM_REPLOLDIDXCOLS_TABLE_OID),
             &sTableOID,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxCols,
                    QCM_REPLOLDIDXCOLS_INDEX_ID),
             &sIndexID,
             &sRange);

    IDE_TEST(qciMisc::selectCount(aSmiStmt,
                              gQcmReplOldIdxCols,
                              aIndexColCount,
                              smiGetDefaultFilter(),
                              &sRange,
                              gQcmReplOldIdxColsIndex
                              [QCM_REPLOLDIDXCOL_INDEX_NAME_OID_IDX_COL])
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpdCatalog::selectReplOldIndexCols(smiStatement       * aSmiStmt,
                                          SChar              * aReplName,
                                          ULong                aTableOID,
                                          UInt                 aIndexID,
                                          smiIndexColumnMeta * aIndexColArr,
                                          vSLong               aIndexColCount)
{
    smiRange              sRange;
    qriMetaRangeColumn    sFirstMetaRange;
    qriMetaRangeColumn    sSecondMetaRange;
    qriMetaRangeColumn    sThirdMetaRange;

    qriNameCharBuffer     sReplNameBuffer;
    mtdCharType         * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType         sTableOID;
    mtdIntegerType        sIndexID;

    smiTableCursor        sCursor;
    SInt                  sStage = 0;
    vSLong                sCount = 0;
    const void          * sRow;
    scGRID                sRid;
    smiCursorProperties   sProperty;

    qciMisc::setVarcharValue(sReplName,
                         NULL,
                         aReplName,
                         idlOS::strlen(aReplName));
    sTableOID = (mtdBigintType)aTableOID;
    sIndexID = (mtdIntegerType)aIndexID;

    qciMisc::makeMetaRangeTripleColumn
            (&sFirstMetaRange,
             &sSecondMetaRange,
             &sThirdMetaRange,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxCols,
                    QCM_REPLOLDIDXCOLS_REPLICATION_NAME),
             (const void*)sReplName,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxCols,
                    QCM_REPLOLDIDXCOLS_TABLE_OID),
             &sTableOID,
             (mtcColumn*)rpdGetTableColumns(
                    gQcmReplOldIdxCols,
                    QCM_REPLOLDIDXCOLS_INDEX_ID),
             &sIndexID,
             &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, gQcmReplOldIdxColsIndex[QCM_REPLOLDIDXCOL_INDEX_NAME_OID_IDX_COL] );


    IDE_TEST(sCursor.open(aSmiStmt,
                          gQcmReplOldIdxCols,
                          gQcmReplOldIdxColsIndex[QCM_REPLOLDIDXCOL_INDEX_NAME_OID_IDX_COL],
                          smiGetRowSCN(gQcmReplOldIdxCols),
                          NULL,
                          &sRange,
                          smiGetDefaultKeyRange(),
                          smiGetDefaultFilter(),
                          QCM_META_CURSOR_FLAG,
                          SMI_SELECT_CURSOR,
                          &sProperty)
             != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);
    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while(sRow != NULL)
    {
        IDE_TEST_RAISE(sCount >= aIndexColCount, ERR_TOO_MANY_REPL_INDEX_COLS);

        setReplOldIndexColMember(&aIndexColArr[sCount], sRow);

        sCount++;

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE(sCount != aIndexColCount, ERR_NOT_ENOUGH_REPL_INDEX_COLS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_TOO_MANY_REPL_INDEX_COLS);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_TOO_MANY_REPLICATION_OLD_INDEX_COLS,
                                aReplName,
                                aTableOID,
                                aIndexID));
    }
    IDE_EXCEPTION(ERR_NOT_ENOUGH_REPL_INDEX_COLS);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_NOT_ENOUGH_REPLICATION_OLD_INDEX_COLS,
                                aReplName,
                                aTableOID,
                                aIndexID));
    }
    IDE_EXCEPTION_END;

    if(sStage == 1)
    {
        (void) sCursor.close();
        sStage = 0;
    }

    return IDE_FAILURE;
}
void rpdCatalog::setReplOldIndexColMember(smiIndexColumnMeta * aReplOldIndexCol,
                                          const void         * aRow)
{
    //----- SYS_REPL_OLD_INDEX_COLUMNS_ -----
    // REPLICATION_NAME         CHAR(40)
    // TABLE_OID                BIGINT
    // INDEX_ID                 INTEGER
    // KEY_COLUMN_ID            INTEGER
    // KEY_COLUMN_FLAG          INTEGER
    // COMPOSITE_ORDER          INTEGER
    //---------------------------------------

    mtdIntegerType   sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDEX_COLUMNS_.KEY_COLUMN_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxCols,
                        QCM_REPLOLDIDXCOLS_KEY_COLUMN_ID))->column.offset);
    aReplOldIndexCol->mID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDEX_COLUMNS_.KEY_COLUMN_FLAG
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxCols,
                        QCM_REPLOLDIDXCOLS_KEY_COLUMN_FLAG))->column.offset);
    aReplOldIndexCol->mFlag = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_INDEX_COLUMNS_.COMPOSITE_ORDER
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
               ((UChar*)aRow +
                ((mtcColumn*)rpdGetTableColumns(
                        gQcmReplOldIdxCols,
                        QCM_REPLOLDIDXCOLS_COMPOSITE_ORDER))->column.offset);
    aReplOldIndexCol->mCompositeOrder = (UInt)sIntData;
}

// SYS_REPL_OLD_CHECKS_
IDE_RC rpdCatalog::insertReplOldChecks(smiStatement       * aSmiStmt,
                                       SChar              * aReplName,
                                       ULong                aTableOID,
                                       UInt                 aConstraintID,
                                       SChar              * aCheckName,
                                       SChar              * aCondition )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_OLD_CHECKS_ VALUES ( "
                     "CHAR'%s', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "CHAR'%s', "
                     "CHAR'%s') ",
                     aReplName,
                     (SLong)aTableOID,
                     (SInt)aConstraintID,
                     aCheckName,
                     aCondition );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE_OLD_CHECK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE_OLD_CHECK );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::insertReplOldChecks]"
                               "[INSERT SYS_REPL_OLD_CHECKS_ = %ld]\n",
                               sRowCnt);
        IDE_SET( ideSetErrorCode( rpERR_FATAL_RPD_REPL_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::deleteReplOldChecks( smiStatement * aSmiStmt,
                                        SChar        * aReplName,
                                        ULong          aTableOID )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong sRowCnt = 0;

    // SYS_REPL_OLD_CHECKS_
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_OLD_CHECKS_ WHERE "
                     "REPLICATION_NAME = CHAR'%s' AND "
                     "TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                     aReplName,
                     (SLong)aTableOID );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::removeReplOldChecks( smiStatement * aSmiStmt,
                                        SChar        * aRepName )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong sRowCnt = 0;

    // SYS_REPL_OLD_CHECKS_
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_OLD_CHECKS_ WHERE "
                     "REPLICATION_NAME = CHAR'%s'",
                     aRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::getReplOldChecksCount( smiStatement * aSmiStmt,
                                          SChar        * aReplName,
                                          ULong          aTableOID,
                                          vSLong       * aCheckCount )
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID = 0;

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName,
                              (UInt)idlOS::strlen(aReplName) );
    sTableOID = (mtdBigintType)aTableOID;

    qciMisc::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                                        QCM_REPLOLDCHECKS_REPLICATION_NAME ),
                                        (const void*)sReplName,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                                        QCM_REPLOLDCHECKS_TABLE_OID ),
                                        &sTableOID,
                                        &sRange );

    IDE_TEST( qciMisc::selectCount( aSmiStmt,
                                    gQcmReplOldChecks,
                                    aCheckCount,
                                    smiGetDefaultFilter(),
                                    &sRange,
                                    gQcmReplOldChecksIndex[QCM_REPLOLDCHECKS_INDEX_NAME_OID_CID] )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::selectReplOldChecks( smiStatement  * aSmiStmt,
                                        SChar         * aReplName,
                                        ULong           aTableOID,
                                        smiCheckMeta  * aCheckMeta,
                                        vSLong          aCheckMetaCount )
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID = 0;

    smiTableCursor       sCursor;
    idBool               sIsCursorOpen = ID_FALSE;
    UInt                 i = 0;
    vSLong               sCount = 0;
    const void         * sRow = NULL;
    scGRID               sRid;
    smiCursorProperties  sProperty;

    SChar                sErrorMessage[256] = { 0, };

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName,
                              (UInt)idlOS::strlen(aReplName) );
    sTableOID = (mtdBigintType)aTableOID;

    qciMisc::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                                        QCM_REPLOLDCHECKS_REPLICATION_NAME ),
                                        (const void*)sReplName,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                                        QCM_REPLOLDCHECKS_TABLE_OID ),
                                        &sTableOID,
                                        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sProperty, 
                          NULL, 
                          gQcmReplOldChecksIndex[QCM_REPLOLDCHECKS_INDEX_NAME_OID_CID] );

    IDU_FIT_POINT( "rpdCatalog::selectReplOldChecks::open::sCursor" );
    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplOldChecks,
                            gQcmReplOldChecksIndex[QCM_REPLOLDCHECKS_INDEX_NAME_OID_CID],
                            smiGetRowSCN(gQcmReplOldChecks),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST_RAISE( sCount >= aCheckMetaCount, ERR_TOO_MANY_OLD_CHECKS );

        /* 아래 함수 호출시 aCheckMeta[sCount].mCondition 이 할당됨 */
        IDE_TEST( setReplOldCheckMember( &aCheckMeta[sCount], sRow ) != IDE_SUCCESS );
        sCount++;

        IDU_FIT_POINT( "rpdCatalog::selectReplOldChecks::readRow::sCursor" );
        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDE_TEST_RAISE( sCount != aCheckMetaCount, ERR_NOT_ENOUGH_OLD_CHECKS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_OLD_CHECKS )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "There are too many checks meta for replication. (RepName : %s, TableOID : %"ID_UINT64_FMT")",
                         aReplName,
                         aTableOID );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_OLD_CHECKS )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "There are not enough checks meta for replication. (RepName : %s, TableOID : %"ID_UINT64_FMT")",
                         aReplName,
                         aTableOID );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    for ( i = 0; i < sCount; i++ )
    {
        if ( aCheckMeta[i].mCondition != NULL )
        {
            (void)iduMemMgr::free( aCheckMeta[i].mCondition );
            aCheckMeta[i].mCondition = NULL;
        }
        else
        {
            /* do nothing */
        }
    }

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
        sIsCursorOpen = ID_FALSE;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::setReplOldCheckMember( smiCheckMeta  * aReplOldChek,
                                          const void    * aRow )
{
    //----- SYS_REPL_OLD_CHECKS_  -----
    // REPLICATION_NAME         CHAR(40)
    // TABLE_OID                BIGINT
    // CONSTRAINT_ID            INTEGER
    // CHECK_NAME               VARCHAR(40)
    // CONDITION                VARCHAR(4000)
    //---------------------------------------

    mtdIntegerType   sIntData = 0;
    mtdCharType    * sCharData = NULL;
    SChar          * sCheckCondition = NULL;

    //-------------------------------------------
    // set by SYS_REPL_OLD_CHECKS_.CONSTRAINT_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
                ( (UChar*)aRow +
                  ( (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                    QCM_REPLOLDCHECKS_CONSTRAINT_ID))->column.offset );
    aReplOldChek->mConstraintID = (UInt)sIntData;

    //-------------------------------------------
    // set by SYS_REPL_OLD_CHECKS_.CHECK_NAME
    //-------------------------------------------
    sCharData = (mtdCharType *)mtc::value( (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                                           QCM_REPLOLDCHECKS_CHECK_NAME ),
                                           aRow,
                                           MTD_OFFSET_USE );
    idlOS::memcpy( aReplOldChek->mCheckName,
                   sCharData->value,
                   sCharData->length );
    aReplOldChek->mCheckName[sCharData->length] = '\0';

    //-------------------------------------------
    // set by SYS_REPL_OLD_CHECKS_.CONDITION
    //-------------------------------------------
    sCharData = (mtdCharType *)mtc::value( (mtcColumn*)rpdGetTableColumns( gQcmReplOldChecks,
                                                                           QCM_REPLOLDCHECKS_CONDITION ),
                                           aRow,
                                           MTD_OFFSET_USE );

    IDU_FIT_POINT( "rpdCatalog::setReplOldCheckMember::calloc::sCheckCondition" );
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_RP_RPD_META,
                                 sCharData->length + 1,
                                 ID_SIZEOF(SChar),
                                 (void **)&sCheckCondition,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    idlOS::memcpy( sCheckCondition,
                   sCharData->value,
                   sCharData->length);
    sCheckCondition[sCharData->length] = '\0';

    aReplOldChek->mCondition = sCheckCondition;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sCheckCondition != NULL )
    {
        (void)iduMemMgr::free( sCheckCondition );
        sCheckCondition = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

// SYS_REPL_OLD_CHECK_COLUMNS_
IDE_RC rpdCatalog::insertReplOldCheckColumns( smiStatement  * aSmiStmt,
                                              SChar         * aReplName,
                                              ULong           aTableOID,
                                              UInt            aConstraintID,
                                              UInt            aColumnID )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_OLD_CHECK_COLUMNS_ VALUES ( "
                     "CHAR'%s', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"', "
                     "INTEGER'%"ID_INT32_FMT"') ",
                     aReplName,
                     (SLong)aTableOID,
                     (SInt)aConstraintID,
                     (SInt)aColumnID );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE_OLD_CHECK_COLUMNS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE_OLD_CHECK_COLUMNS );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::insertReplOldCheckColumns]"
                               "[INSERT SYS_REPL_OLD_CHECK_COLUMNS_ = %ld]\n",
                               sRowCnt);
        IDE_SET( ideSetErrorCode( rpERR_FATAL_RPD_REPL_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::deleteReplOldCheckColumns( smiStatement * aSmiStmt,
                                              SChar        * aReplName,
                                              ULong          aTableOID )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong sRowCnt = 0;

    // SYS_REPL_OLD_CHECK_COLUMNS_
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_OLD_CHECK_COLUMNS_ WHERE "
                     "REPLICATION_NAME = CHAR'%s' AND "
                     "TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                     aReplName,
                     (SLong)aTableOID );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::removeReplOldCheckColumns( smiStatement * aSmiStmt,
                                              SChar        * aReplName )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong sRowCnt = 0;

    // SYS_REPL_OLD_CHECK_COLUMNS_
    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_OLD_CHECK_COLUMNS_ WHERE "
                     "REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::getReplOldCheckColumnsCount( smiStatement * aSmiStmt,
                                                SChar        * aReplName,
                                                ULong          aTableOID,
                                                UInt           aConstraintID,
                                                vSLong       * aCheckColumnCount )
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;
    qriMetaRangeColumn   sThirdMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID = 0;
    mtdIntegerType       sConstraintID = 0;

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName,
                              (UInt)idlOS::strlen(aReplName) );
    sTableOID = (mtdBigintType)aTableOID;
    sConstraintID = (mtdIntegerType)aConstraintID;

    qciMisc::makeMetaRangeTripleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        &sThirdMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                                        QCM_REPLOLDCHECKCOLUMNS_REPLICATION_NAME ),
                                        (const void*)sReplName,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                                        QCM_REPLOLDCHECKCOLUMNS_TABLE_OID ),
                                        &sTableOID,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                                        QCM_REPLOLDCHECKCOLUMNS_CONSTRAINT_ID ),
                                        &sConstraintID,
                                        &sRange );

    IDE_TEST( qciMisc::selectCount( aSmiStmt,
                                    gQcmReplOldCheckColumns,
                                    aCheckColumnCount,
                                    smiGetDefaultFilter(),
                                    &sRange,
                                    gQcmReplOldCheckColumnsIndex[QCM_REPLOLDCHECKCOLUMNS_INDEX_NAME_OID_CID] )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::selectReplOldCheckColumns( smiStatement          * aSmiStmt,
                                              SChar                 * aReplName,
                                              ULong                   aTableOID,
                                              UInt                    aConstraintID,
                                              smiCheckColumnMeta    * aCheckColumnMeta,
                                              vSLong                  aCheckColumnMetaCount )
{
    smiRange             sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;
    qriMetaRangeColumn   sThirdMetaRange;

    qriNameCharBuffer    sReplNameBuffer;
    mtdCharType        * sReplName = (mtdCharType *)&sReplNameBuffer;
    mtdBigintType        sTableOID = 0;
    mtdIntegerType       sConstraintID = 0;

    smiTableCursor       sCursor;
    idBool               sIsCursorOpen = ID_FALSE;
    vSLong               sCount = 0;
    const void         * sRow = NULL;
    scGRID               sRid;
    smiCursorProperties  sProperty;

    SChar                sErrorMessage[256] = { 0, };

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplName,
                              (UInt)idlOS::strlen(aReplName) );
    sTableOID = (mtdBigintType)aTableOID;
    sConstraintID = (mtdIntegerType)aConstraintID;

    qciMisc::makeMetaRangeTripleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        &sThirdMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                                        QCM_REPLOLDCHECKCOLUMNS_REPLICATION_NAME ),
                                        (const void*)sReplName,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                                        QCM_REPLOLDCHECKCOLUMNS_TABLE_OID ),
                                        &sTableOID,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                                        QCM_REPLOLDCHECKCOLUMNS_CONSTRAINT_ID ),
                                        &sConstraintID,    
                                        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sProperty, 
                          NULL, 
                          gQcmReplOldCheckColumnsIndex[QCM_REPLOLDCHECKCOLUMNS_INDEX_NAME_OID_CID] );

    IDU_FIT_POINT( "rpdCatalog::selectReplOldCheckColumns::open::sCursor" );
    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplOldCheckColumns,
                            gQcmReplOldCheckColumnsIndex[QCM_REPLOLDCHECKCOLUMNS_INDEX_NAME_OID_CID],
                            smiGetRowSCN(gQcmReplOldCheckColumns),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDU_FIT_POINT_RAISE( "rpdCatalog::selectReplOldCheckColumns::sCount::TOO_MANY",
                             ERR_TOO_MANY_OLD_CHECK_COLUMNS );
        IDE_TEST_RAISE( sCount >= aCheckColumnMetaCount, ERR_TOO_MANY_OLD_CHECK_COLUMNS );

        setReplOldCheckColumnsMember( &aCheckColumnMeta[sCount], sRow );
        sCount++;

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpdCatalog::selectReplOldCheckColumns::sCount::NOT_ENOUGH",
                         ERR_NOT_ENOUGH_OLD_CHECK_COLUMNS );
    IDE_TEST_RAISE( sCount != aCheckColumnMetaCount, ERR_NOT_ENOUGH_OLD_CHECK_COLUMNS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_OLD_CHECK_COLUMNS )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "There are too many check columns meta for replication."
                         "(RepName : %s, TableOID : %"ID_UINT64_FMT", Constraint ID : %"ID_UINT32_FMT")",
                         aReplName,
                         aTableOID,
                         aConstraintID );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_OLD_CHECK_COLUMNS )
    {
        idlOS::snprintf( sErrorMessage,
                         ID_SIZEOF( sErrorMessage ),
                         "There are not enough check columns meta for replication."
                         "(RepName : %s, TableOID : %"ID_UINT64_FMT", Constraint ID : %"ID_UINT32_FMT")",
                         aReplName,
                         aTableOID,
                         aConstraintID );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorMessage ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
        sIsCursorOpen = ID_FALSE;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdCatalog::setReplOldCheckColumnsMember( smiCheckColumnMeta    * aReplOldChekColumn,
                                               const void            * aRow )
{
    //----- SYS_REPL_OLD_CHECKS_COLUMNS  -----
    // REPLICATION_NAME         CHAR(40)
    // TABLE_OID                BIGINT
    // CONSTRAINT_ID            INTEGER
    // COLUMN_ID                INTEGER
    //---------------------------------------

    mtdIntegerType   sIntData = 0;

    //-------------------------------------------
    // set by SYS_REPL_OLD_CHECKS_COLUMNS_.COLUMN_ID
    //-------------------------------------------
    sIntData = *(mtdIntegerType*)
                ( (UChar*)aRow +
                  ( (mtcColumn*)rpdGetTableColumns( gQcmReplOldCheckColumns,
                                                    QCM_REPLOLDCHECKCOLUMNS_COLUMN_ID ) )->column.offset );
    aReplOldChekColumn->mColumnID = (UInt)sIntData;

}


IDE_RC
rpdCatalog::checkReplItemRecoveryCntByName( void          * aQcStatement,
                                            qciNamePosition aReplName,
                                            SInt            aRecoveryFlag)
{
    smiRange            sRange;
    qriMetaRangeColumn  sFirstMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    rpdReplItems      * sReplItem;
    scGRID              sRid;
    smiCursorProperties sProperty;
    UInt                sLocalUserID = 0;
    qcmTableInfo      * sInfo;
    smSCN               sSCN;
    void              * sTableHandle;
    UInt                sMaxRecoveryCnt= 0;
    smiStatement      * sSmiStmt = QCI_SMI_STMT( aQcStatement );

    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;

    sCursor.initialize();
    
    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName.stmtText + aReplName.offset,
                          aReplName.size );
    
    qciMisc::makeMetaRangeSingleColumn( & sFirstMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplItems,QCM_REPLITEM_REPL_NAME),
                                    (const void *) sReplName,
                                    & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty, QCI_STATISTIC( aQcStatement ), gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID]);

    IDE_TEST( sCursor.open( QCI_SMI_STMT( aQcStatement ),
                            gQcmReplItems,
                            gQcmReplItemsIndex[QCM_REPLITEM_INDEX_NAME_N_OID],
                            smiGetRowSCN( gQcmReplItems ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;
    
    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    
    IDU_FIT_POINT_RAISE( "rpdCatalog::checkReplItemRecoveryCntByName::malloc::ReplItems", 
                         ERR_RECOVERY_MEMORY_ALLOC_ABORT );
    IDE_TEST_RAISE( STRUCT_ALLOC( QCI_QMP_MEM( aQcStatement ), 
                                  rpdReplItems, 
                                  &sReplItem )
                    != IDE_SUCCESS, ERR_RECOVERY_MEMORY_ALLOC_ABORT );
    
    while ( sRow != NULL )
    {
        setReplItemMember(sReplItem,  sRow);
        /*여기서 repname의 replication에 포함된 repl item마다 recovery count를 확인한다*/

        IDE_TEST(qciMisc::getUserID(aQcStatement,
                                    sReplItem->mLocalUsername,
                                    idlOS::strlen(sReplItem->mLocalUsername),
                                    &sLocalUserID)
                 != IDE_SUCCESS);

        IDE_TEST(qciMisc::getTableInfo(aQcStatement,
                                       sLocalUserID,
                                       (UChar*)sReplItem->mLocalTablename,
                                       idlOS::strlen(sReplItem->mLocalTablename),
                                       &sInfo,
                                       &sSCN,
                                       &sTableHandle)
                 != IDE_SUCCESS);

        IDE_TEST(qciMisc::lockTableForDDLValidation(aQcStatement,
                                                sTableHandle,
                                                sSCN)
                 != IDE_SUCCESS);

        /*
         * PROJ-2336 BUGBUG
         */
        if ( sInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {

            IDE_TEST( qciMisc::getPartitionInfoList( aQcStatement,
                                                     sSmiStmt,
                                                     ( iduMemory * )QCI_QMX_MEM( aQcStatement ),
                                                     sInfo->tableID,
                                                     &sPartInfoList )
                      != IDE_SUCCESS );

            sMaxRecoveryCnt = 0;

            for ( sTempPartInfoList = sPartInfoList;
                  sTempPartInfoList != NULL;
                  sTempPartInfoList = sTempPartInfoList->next )
            {
                sMaxRecoveryCnt++;
            }
        }
        else
        {
            sMaxRecoveryCnt = 1;
        }

        if ( ( aRecoveryFlag & RP_OPTION_RECOVERY_MASK ) == RP_OPTION_RECOVERY_SET)
        {
            IDE_TEST_RAISE( sInfo->replicationRecoveryCount >= sMaxRecoveryCnt,
                            ERR_RECOVERY_COUNT_ABORT );
        }
        else //RP_OPTION_RECOVERY_UNSET
        {
            IDE_TEST_RAISE( sInfo->replicationRecoveryCount > sMaxRecoveryCnt,
                            ERR_RECOVERY_COUNT_FATAL );
            IDE_TEST_RAISE( sInfo->replicationRecoveryCount == 0,
                            ERR_RECOVERY_COUNT_FATAL );
        }

        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_RECOVERY_COUNT_ABORT)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_ALREADY_SUPPORT_RECOVERY,
                               sReplItem->mLocalTablename));
    }
    IDE_EXCEPTION(ERR_RECOVERY_COUNT_FATAL)
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::checkReplItemRecoveryCntByName]\n");
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION( ERR_RECOVERY_MEMORY_ALLOC_ABORT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpdCatalog::checkReplItemRecoveryCntByName",
                                  "sReplItem" ) );
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    IDE_POP();

    return IDE_FAILURE;
}

//proj-1608 SYS_REPL_RECOVERY_INFOS_관련 함수들
IDE_RC
rpdCatalog::getReplRecoveryInfosCount( smiStatement * aSmiStmt,
                                       SChar        * aReplName,
                                       vSLong       * aCount )
{
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    vSLong              sRowCount = 0;

    qciMisc::setVarcharValue( sReplName,
                          NULL,
                          aReplName,
                          idlOS::strlen(aReplName) );

    qciMisc::makeMetaRangeSingleColumn( & sMetaRange,
                                    (mtcColumn*)rpdGetTableColumns(gQcmReplRecoveryInfos,
                                        QCM_REPLRECOVERY_REPL_NAME),
                                    (const void*) sReplName,
                                    & sRange );
    IDE_TEST( qciMisc::selectCount(
                  aSmiStmt,
                  gQcmReplRecoveryInfos,
                  &sRowCount,
                  smiGetDefaultFilter(),
                  & sRange,
                  NULL )
              != IDE_SUCCESS );

    *aCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::insertReplRecoveryInfos( smiStatement    * aSmiStmt,
                                     SChar           * aRepName,
                                     rpdRecoveryInfo * aRecoveryInfos )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "INSERT INTO SYS_REPL_RECOVERY_INFOS_ VALUES ( "
                     "CHAR'%s', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"', "
                     "BIGINT'%"ID_INT64_FMT"' )",
                     aRepName,
                     aRecoveryInfos->mMasterBeginSN,
                     aRecoveryInfos->mMasterCommitSN,
                     aRecoveryInfos->mReplicatedBeginSN,
                     aRecoveryInfos->mReplicatedCommitSN );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::insertReplRecoveryInfos] "
                              "[INSERT SYS_REPL_RECOVERY_INFOS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::removeReplRecoveryInfos( smiStatement*  aSmiStmt,
                                     SChar*         aRepName )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_RECOVERY_INFOS_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aRepName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::selectReplRecoveryInfos( smiStatement    * aSmiStmt,
                                     SChar           * aRepName,
                                     rpdRecoveryInfo * aRecoveryInfos,
                                     vSLong            aCount )
{
    smiRange            sRange;
    qriMetaRangeColumn  sMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    
    vSLong              sRowCount = 0;
    smiTableCursor      sCursor;
    const void        * sRow;
    SInt                sStage = 0;
    scGRID              sRid;
    smiCursorProperties sProperty;

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sProperty, NULL, NULL );

    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aRepName,
                              idlOS::strlen(aRepName) );

    qciMisc::makeMetaRangeSingleColumn( &sMetaRange,
                                        (mtcColumn*)rpdGetTableColumns(gQcmReplRecoveryInfos,
                                        QCM_REPLRECOVERY_REPL_NAME),
                                        (const void*) sReplName,
                                        &sRange );

    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplRecoveryInfos,
                            NULL,
                            smiGetRowSCN( gQcmReplRecoveryInfos ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        IDE_TEST_RAISE( sRowCount >= aCount, ERR_RECOVERYINFO_COUNT );

        setReplRecoveryInfoMember( & aRecoveryInfos[sRowCount], sRow );
        sRowCount++;
        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECOVERYINFO_COUNT );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::selectReplRecoveryInfos] "
                              "sRowCount:%ld aCount:%ld\n",
                              sRowCount, aCount);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_RPD_REPL_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void) sCursor.close();
    }

    return IDE_FAILURE;
}
void
rpdCatalog::setReplRecoveryInfoMember( rpdRecoveryInfo * aRepl,
                                       const void      * aRow )
{
    // ------ SYS_REPL_RECOVERY_INFOS_ -------
    // REPLICATION_NAME       CHAR(40) RP에서 사용하지 않으므로 설정하지 않음
    // MASTER_BEGIN_SN        BIGINT
    // MASTER_COMMIT_SN       BIGINT
    // REPLICATED_BEGIN_SN    BIGINT
    // REPLICATED_COMMIT_SN   BIGINT
    // --------------------------------

    mtdBigintType  sBigIntData;

    //-------------------------------------------
    // set by SYS_REPLICATION_.MASTER_BEGIN_SN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplRecoveryInfos,
            QCM_REPLRECOVERY_MASTER_BEGIN_SN))->column.offset );
    aRepl->mMasterBeginSN = *( (smSN*) &sBigIntData );

    //-------------------------------------------
    // set by SYS_REPLICATION_.MASTER_COMMIT_SN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplRecoveryInfos,
            QCM_REPLRECOVERY_MASTER_COMMIT_SN))->column.offset );
    aRepl->mMasterCommitSN = *( (smSN*) &sBigIntData );

    //-------------------------------------------
    // set by SYS_REPLICATION_.REPLICATED_BEGIN_SN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplRecoveryInfos,
            QCM_REPLRECOVERY_REPLICATED_BEGIN_SN))->column.offset );
    aRepl->mReplicatedBeginSN = *( (smSN*) &sBigIntData );

    //-------------------------------------------
    // set by SYS_REPLICATION_.REPLICATED_COMMIT_SN
    //-------------------------------------------
    sBigIntData = *(mtdBigintType*)
        ((UChar*) aRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplRecoveryInfos,
            QCM_REPLRECOVERY_REPLICATED_COMMIT_SN))->column.offset );
    aRepl->mReplicatedCommitSN = *( (smSN*) &sBigIntData );
}

IDE_RC rpdCatalog::updateReplItemsTableOIDArray( void           * aQcStatement,
                                                 smOID          * aBeforeTableOIDArray,
                                                 smOID          * aAfterTableOIDArray,
                                                 UInt             aTableOIDCount )
{
    UInt                       i = 0;
    rpdUpdateTableOID       * sUpdateTableOIDArray = NULL;
    rpdUpdateTableOID       * sUpdateTableOID = NULL;
    
    IDE_TEST( QCI_QMX_MEM( aQcStatement )->alloc( ID_SIZEOF(rpdUpdateTableOID) * aTableOIDCount,
                                                  (void**)&sUpdateTableOIDArray )
              != IDE_SUCCESS );

    for ( i = 0; i < aTableOIDCount; i++ )
    {
        sUpdateTableOIDArray[i].mBeforeTableOID = aBeforeTableOIDArray[i];
        sUpdateTableOIDArray[i].mAfterTableOID = aAfterTableOIDArray[i];
    }

    idlOS::qsort( sUpdateTableOIDArray,
                  aTableOIDCount,
                  ID_SIZEOF(rpdUpdateTableOID),
                  rpdCompareUpdateTableOID );

    for ( i = 0; i < aTableOIDCount; i++ )
    {
        sUpdateTableOID = &sUpdateTableOIDArray[i];
        if ( sUpdateTableOID->mBeforeTableOID != sUpdateTableOID->mAfterTableOID )
        {
            IDE_TEST( updateReplItemsTableOID( QCI_SMI_STMT( aQcStatement ),
                                               sUpdateTableOID->mBeforeTableOID,
                                               sUpdateTableOID->mAfterTableOID )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateReplItemsTableOID(smiStatement * aSmiStmt,
                                           smOID          aBeforeTableOID,
                                           smOID          aAfterTableOID)
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idBool sIsExist = ID_FALSE;

    smiRange                sRange;
    qriMetaRangeColumn      sMetaRange;
    mtdBigintType           sTableOID;

    mtcColumn             * sTableOIDColumn = NULL;
    smiColumnList           sUpdateColumn;
    smiValue                sUpdateValue;

    smiTableCursor          sCursor;
    smiCursorProperties     sProperty;
    idBool                  sIsOpenCursor = ID_FALSE;

    void                  * sRow = NULL;
    scGRID                  sRid;


    /* BUG-38306 Partitioned Replication에서는 table의 모든 partition이 
       SYS_REPL_ITEMS_에 들어가지 않을 수 있다. 이 때 partitioned table에 
       대해서 DDL을 수행한다면 모든 partition에 대해서 이 함수를 호출한다.
       따라서 먼저 OID로 repl item에 존재하는 지 확인하는 로직이 필요하다. */
    IDE_TEST( rpdCatalog::checkReplItemExistByOID( aSmiStmt,
                                                   aBeforeTableOID,
                                                   &sIsExist )
              != IDE_SUCCESS );
    IDE_TEST_CONT( sIsExist == ID_FALSE, NORMAL_EXIT );

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                            "INSERT INTO SYS_REPL_ITEMS_HISTORY_ ( REPLICATION_NAME, USER_NAME, TABLE_NAME, PARTITION_NAME, OLD_OID, NEW_OID ) "
                            "SELECT REPLICATION_NAME, LOCAL_USER_NAME, LOCAL_TABLE_NAME, LOCAL_PARTITION_NAME, TABLE_OID, BIGINT'%"ID_INT64_FMT"' "
                            "FROM SYS_REPL_ITEMS_ "
                            "WHERE TABLE_OID = BIGINT'%"ID_INT64_FMT"' ",
                            (SLong)aAfterTableOID,
                            (SLong)aBeforeTableOID);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);
    IDE_TEST_RAISE( sRowCnt <= 0  , ERR_INVALID_UPDATE );

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                             "UPDATE /*+ INDEX( SYS_REPL_ITEMS_, SYS_REPLITEMS_INDEX1 ) */ "
                             "SYS_REPL_ITEMS_ SET "
                             "TABLE_OID = BIGINT'%"ID_INT64_FMT"' "
                             "WHERE TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                             (SLong)aAfterTableOID,
                             (SLong)aBeforeTableOID);

    /* table oid 로 정령된 순으로 update 하기 위하여 
     * 직접 cursor 를 열어 해당 row 를 update 한다 */
    /* Range */
    sTableOID = (mtdBigintType)aBeforeTableOID;
    qciMisc::makeMetaRangeSingleColumn( &sMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplItems,
                                                                        QCM_REPLITEM_TABLE_OID ),
                                        &sTableOID,
                                        &sRange );

    /* update */
    sTableOIDColumn = (mtcColumn*)rpdGetTableColumns( gQcmReplItems, QCM_REPLITEM_TABLE_OID );
    sUpdateColumn.column = (smiColumn*)sTableOIDColumn;
    sUpdateColumn.next = NULL;

    sUpdateValue.value = &(aAfterTableOID);
    sUpdateValue.length = ID_SIZEOF(mtdBigintType);

    /* execute */
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT( &sProperty,
                          NULL,
                          gQcmReplItemsIndex[QCM_REPLITEM_INDEX_OID] );

    IDE_TEST( sCursor.open( aSmiStmt,
                            gQcmReplItems,
                            gQcmReplItemsIndex[QCM_REPLITEM_INDEX_OID],
                            smiGetRowSCN(gQcmReplItems),
                            &sUpdateColumn,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            SMI_LOCK_WRITE | SMI_TRAVERSE_FORWARD | SMI_PREVIOUS_DISABLE,
                            SMI_UPDATE_CURSOR,
                            &sProperty )
              != IDE_SUCCESS );
    sIsOpenCursor = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( (const void**)&sRow,
                               &sRid,
                               SMI_FIND_NEXT )
              != IDE_SUCCESS );

    /* 하나 이상의 row 가 존재해야 한다. */
    IDE_TEST_RAISE( sRow == NULL, ERR_INVALID_UPDATE );

    while ( sRow != NULL )
    {
        IDU_FIT_POINT_RAISE( "rpdCatalog::updateReplItemsTableOID::Erratic::rpERR_ABORT_RPD_INTERNAL_ARG",
                             ERR_INVALID_UPDATE );
        IDE_TEST( sCursor.updateRow( &sUpdateValue ) != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( (const void**)&sRow,
                                   &sRid,
                                   SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsOpenCursor = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    RP_LABEL(NORMAL_EXIT);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_UPDATE);
    {
        IDE_ERRLOG(IDE_RP_0);
        ideLog::log(IDE_RP_0, "%s", sBuffer);
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateReplItemsTableOID] "
                              "[UPDATE SYS_REPL_ITEMS_ = %ld]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateReplItemsTableOID] "
                                "err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsOpenCursor == ID_TRUE )
    {
        (void)sCursor.close();
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::isExistInReplItemsHistory( smiStatement * aSmiStmt,
                                              SChar  * aRepName,
                                              smOID    aOID,
                                              idBool * aOutIsExist )
{

    SDouble         sCharBuffer[(ID_SIZEOF(UShort) + QC_MAX_NAME_LEN + 7) / 8] = { (SDouble)0.0, };
    mtdCharType   * sResult         = (mtdCharType *) & sCharBuffer;
    SChar           sSqlStr[QD_MAX_SQL_LENGTH];
    idBool          sRecordExist    = ID_FALSE;

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "SELECT REPLICATION_NAME "
                     "  FROM SYS_REPL_ITEMS_HISTORY_ "
                     " WHERE OLD_OID = BIGINT'%"ID_INT64_FMT"' AND "
                     "       REPLICATION_NAME = VARCHAR'%s' ",
                     aOID,
                     aRepName );

    IDE_TEST( qcg::runSelectOneRowforDDL( aSmiStmt,
                                          sSqlStr,
                                          sResult,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    *aOutIsExist = sRecordExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateRemoteDataInit( smiStatement * aSmiStmt,
                                         SChar        * aReplName )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH] = { 0, };
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPLICATIONS_ "
                     "SET REMOTE_LAST_DDL_XSN = BIGINT'%"ID_INT64_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     SM_SN_NULL,
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt > 1, ERR_EXECUTE );

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "UPDATE SYS_REPL_RECEIVER_ "
                     "SET REMOTE_XSN = BIGINT'%"ID_INT64_FMT"' "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     SM_SN_NULL,
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt > 1, ERR_EXECUTE_RECEIVER );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateRemoteDataInit] [%s]",
                     sBuffer );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateRemoteDataInit] "
                                "err_updated_count_is_not_0_OR_1(status)"));
    }
    IDE_EXCEPTION( ERR_EXECUTE_RECEIVER );
    {
        ideLog::log( IDE_RP_0, "[rpdCatalog::updateRemoteDataInit] [%s]",
                     sBuffer );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateRemoteDataInit] "
                                "err_updated_count_is_not_0_OR_1(receiver)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateConditionalSyncedWithOID( smiStatement      * aSmiStmt,
                                                     SChar           * aReplName,
                                                     smOID             aTableOID,
                                                     idBool            aIsConditionSynced )
{
    SChar  sBuffer[QD_MAX_SQL_LENGTH];
    vSLong sRowCnt = 0;

    idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                             "UPDATE SYS_REPL_ITEMS_ SET "
                             "IS_CONDITION_SYNCED = '%"ID_INT32_FMT"' "
                             "WHERE REPLICATION_NAME = '%s' "
                             "AND TABLE_OID = BIGINT'%"ID_INT64_FMT"'",
                             aIsConditionSynced,
                             aReplName,
                             (SLong)aTableOID);

    IDE_TEST(qciMisc::runDMLforDDL(aSmiStmt, sBuffer, &sRowCnt)
             != IDE_SUCCESS);

    // BUG-24497 [RP] SYS_REPL_ITEMS_의 TABLE_OID가 둘 이상 갱신될 수 있습니다
    IDE_TEST_RAISE( sRowCnt != 1 , ERR_INVALID_UPDATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_UPDATE);
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateConditionalSyncedWithOID] "
                              "[UPDATE SYS_REPL_ITEMS_ = "ID_UINT64_FMT"]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateConditionalSyncedWithOID] "
                                "err_updated_count_is_not_1(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdCatalog::updateConditionalSyncedWithItem( smiStatement      * aSmiStmt,
                                                     rpdReplItems     * aReplItems,
                                                     idBool             aIsConditionSynced )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    if ( aReplItems->mLocalPartname[0] == '\0' )
    {
        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                        "UPDATE SYS_REPL_ITEMS_ SET "
                        "IS_CONDITION_SYNCED = '%"ID_INT32_FMT"' "
                        "WHERE REPLICATION_NAME = '%s' "
                        "AND LOCAL_USER_NAME = '%s' "
                        "AND LOCAL_TABLE_NAME = '%s' ",
                        aIsConditionSynced,
                        aReplItems->mRepName,
                        aReplItems->mLocalUsername,
                        aReplItems->mLocalTablename);
    }
    else
    {
        idlOS::snprintf(sBuffer, ID_SIZEOF(sBuffer),
                        "UPDATE SYS_REPL_ITEMS_ SET "
                        "IS_CONDITION_SYNCED = '%"ID_INT32_FMT"' "
                        "WHERE REPLICATION_NAME = '%s' "
                        "AND LOCAL_USER_NAME = '%s' "
                        "AND LOCAL_TABLE_NAME = '%s' "
                        "AND LOCAL_PARTITION_NAME = '%s' ",
                        aIsConditionSynced,
                        aReplItems->mRepName,
                        aReplItems->mLocalUsername,
                        aReplItems->mLocalTablename,
                        aReplItems->mLocalPartname);
    }
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE );
    {
        ideLog::log(IDE_RP_0, "[rpdCatalog::updateConditionalSyncedWithItem] "
                              "[UPDATE SYS_REPL_ITEMS_ = "ID_UINT64_FMT"]\n",
                              sRowCnt);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::updateConditionalSyncedWithItem] err_updated_count_is_0(status)"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::deleteReplItemReplaceHistory( smiStatement  * aSmiStmt,
                                   rpdReplItems  * aReplItems,
                                   idBool          aIsPartition )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    if ( aIsPartition == ID_TRUE )
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "DELETE FROM SYS_REPL_ITEM_REPLACE_HISTORY_ "
                         "WHERE REPLICATION_NAME = '%s' "
                         "AND USER_NAME = '%s' "
                         "AND TABLE_NAME = '%s' "
                         "AND PARTITION_NAME = '%s' ",
                         aReplItems->mRepName,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename,
                         aReplItems->mLocalPartname );
    }
    else
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "DELETE FROM SYS_REPL_ITEM_REPLACE_HISTORY_ "
                         "WHERE REPLICATION_NAME = '%s' "
                         "AND USER_NAME = '%s' "
                         "AND TABLE_NAME = '%s' ",
                         aReplItems->mRepName,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename );
    }
    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt,
                                 sBuffer,
                                 &sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::removeReplItemReplaceHistory( smiStatement      * aSmiStmt,
                                   SChar             * aReplName )
{
    SChar   sBuffer[QD_MAX_SQL_LENGTH];
    vSLong  sRowCnt = 0;

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     "DELETE FROM SYS_REPL_ITEM_REPLACE_HISTORY_ "
                     "WHERE REPLICATION_NAME = CHAR'%s'",
                     aReplName );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt, sBuffer, & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpdCatalog::setTableOIDReferToItemReplaceHistory( smiStatement  * aSmiStmt,
                                                  rpdReplItems  * aReplItems )
{
    smiRange            sRange;
    qriMetaRangeColumn   sFirstMetaRange;
    qriMetaRangeColumn   sSecondMetaRange;
    qriNameCharBuffer   sReplNameBuffer;
    mtdCharType       * sReplName = ( mtdCharType * ) & sReplNameBuffer;
    mtdBigintType       sTableOID = 0;
    
    smiTableCursor      sCursor;
    SInt                sStage = 0;
    const void        * sRow;
    scGRID              sRid;
    smiCursorProperties sProperty;
    mtdBigintType       sOldTableOID = 0;


    qciMisc::setVarcharValue( sReplName,
                              NULL,
                              aReplItems->mRepName,
                              idlOS::strlen(aReplItems->mRepName) );
    sTableOID = (mtdBigintType)aReplItems->mTableOID;

    
    sCursor.initialize();

    qciMisc::makeMetaRangeDoubleColumn( & sFirstMetaRange,
                                        & sSecondMetaRange,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplItemReplaceHistory, QCM_REPL_ITEM_REPLACE_HISTORY_REPL_NAME ),
                                        (const void*) sReplName,
                                        (mtcColumn*)rpdGetTableColumns( gQcmReplItemReplaceHistory, QCM_REPL_ITEM_REPLACE_HISTORY_NEW_OID ),
                                        & sTableOID,
                                        & sRange );

    SMI_CURSOR_PROP_INIT( &sProperty,
                          NULL,
                          gQcmReplItemReplaceHistoryIndex[QCM_REPL_ITEM_REPLACE_HISTORY_INDEX_NAME_N_OID] );

    IDE_TEST( sCursor.open( aSmiStmt, 
                            gQcmReplItemReplaceHistory,
                            gQcmReplItemReplaceHistoryIndex[QCM_REPL_ITEM_REPLACE_HISTORY_INDEX_NAME_N_OID],
                            smiGetRowSCN( gQcmReplItemReplaceHistory ),
                            NULL,
                            & sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sProperty ) != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        sOldTableOID = *(mtdBigintType*)
            ((UChar*) sRow + ((mtcColumn*)rpdGetTableColumns(gQcmReplItemReplaceHistory,QCM_REPL_ITEM_REPLACE_HISTORY_OLD_OID))->column.offset);
        aReplItems->mTableOID = (ULong) sOldTableOID;

        IDE_TEST( sCursor.readRow( & sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        IDE_TEST_RAISE( sRow != NULL, err_too_many_rows );
    }
    else
    {
        /* Nothihg to do */
    }

    sStage = 0;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_too_many_rows);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPD_INTERNAL_ARG,
                                "[rpdCatalog::setTableOIDReferToItemReplaceHistory] err_too_many_rows"));
    }

    if ( sStage == 1 )
    {
        (void) sCursor.close();
        sStage = 0;
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
