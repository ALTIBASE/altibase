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
 * $Id: 
 **********************************************************************/

#include <rpxReceiver.h>
#include <rpsSQLExecutor.h>
#include <rpsSmExecutor.h>
#include <rpdConvertSQL.h>
#include <rpdCatalog.h>

IDE_RC rpsSQLExecutor::executeSQL( smiStatement   * aSmiStatement,
                                   rpdMetaItem    * aRemoteMetaItem,
                                   rpdMetaItem    * aLocalMetaItem,
                                   rpdXLog        * aXLog,
                                   SChar          * aSQLBuffer,
                                   UInt             aSQLBufferLength,
                                   idBool           aCompareBeforeImage,
                                   SLong          * aRowCount )
{
    qciStatement            sQciStatement;
    qciSQLPlanCacheContext  sPlanCacheContext;

    idBool                  sIsInitialized = ID_FALSE;
    SLong                   sRowCount = 0;

    idlOS::memset( &sQciStatement, 0x00, ID_SIZEOF( qciStatement ) );
    idlOS::memset( &sPlanCacheContext, 0x00, ID_SIZEOF( qciSQLPlanCacheContext ) );

    IDE_TEST( qci::initializeStatement( &sQciStatement,
                                        NULL,
                                        NULL,
                                        NULL )
              != IDE_SUCCESS );
    sIsInitialized = ID_TRUE;

    sPlanCacheContext.mPlanCacheInMode = QCI_SQL_PLAN_CACHE_IN_OFF;
    sPlanCacheContext.mSharedPlanMemory = NULL;
    sPlanCacheContext.mPrepPrivateTemplate = NULL;

    IDE_TEST( prepare( &sQciStatement,
                       aSmiStatement,
                       &sPlanCacheContext,
                       aSQLBuffer,
                       aSQLBufferLength )
              != IDE_SUCCESS );

    IDE_TEST( bind( &sQciStatement,
                    aRemoteMetaItem,
                    aLocalMetaItem,
                    aXLog,
                    aCompareBeforeImage )
              != IDE_SUCCESS );

    IDE_TEST( reprepare( &sQciStatement,
                         aSmiStatement,
                         &sPlanCacheContext,
                         aSQLBuffer,
                         aSQLBufferLength )
              != IDE_SUCCESS );

    IDE_TEST( execute( &sQciStatement, 
                       aSmiStatement, 
                       &sPlanCacheContext,
                       aSQLBuffer,
                       aSQLBufferLength,
                       &sRowCount ) != IDE_SUCCESS );

    sIsInitialized = ID_FALSE;
    IDE_TEST( qci::finalizeStatement( &sQciStatement ) != IDE_SUCCESS );

    *aRowCount = sRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsInitialized == ID_TRUE )
    {
        (void)qci::finalizeStatement( &sQciStatement );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::reprepare( qciStatement              * aQciStatement,
                                  smiStatement              * aSmiStatement,
                                  qciSQLPlanCacheContext    * aPlanCacheContext,
                                  SChar                     * aSQLBuffer,
                                  UInt                        aSQLBufferLength )
{
    IDE_TEST( qci::clearStatement4Reprepare( aQciStatement,
                                             aSmiStatement )
              != IDE_SUCCESS );

    IDE_TEST( prepare( aQciStatement,
                       aSmiStatement,
                       aPlanCacheContext,
                       aSQLBuffer,
                       aSQLBufferLength )
              != IDE_SUCCESS );

    IDE_TEST( qci::setBindTuple( aQciStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::prepare( qciStatement              * aQciStatement,
                                smiStatement              * aSmiStatement,
                                qciSQLPlanCacheContext    * aPlanCacheContext,
                                SChar                     * aSQLBuffer,
                                UInt                        aSQLBufferLength )
{
    idBool      sIsDone = ID_FALSE;

    while ( sIsDone != ID_TRUE )
    {
        IDE_TEST( qci::parse( aQciStatement,
                              aSQLBuffer,
                              aSQLBufferLength )
                  != IDE_SUCCESS );

        IDE_TEST( qci::bindParamInfo( aQciStatement,
                                      aPlanCacheContext)
                  != IDE_SUCCESS );

        if ( qci::hardPrepare( aQciStatement,
                               aSmiStatement,
                               aPlanCacheContext )
             == IDE_SUCCESS )
        {
            sIsDone = ID_TRUE;
        }
        else
        {
            IDE_TEST( ideIsRebuild() != IDE_SUCCESS );

            IDE_TEST( qci::clearStatement4Reprepare( aQciStatement,
                                                     aSmiStatement )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::setBindParamInfo( qciStatement       * aQciStatement,
                                         mtcColumn          * aColumn,
                                         UInt                 aId,
                                         UInt                 aLength )
{
    qciBindParam        sBindParam;

    idlOS::memset( &sBindParam, 0x00, ID_SIZEOF( qciBindParam ) );

    sBindParam.id = aId;
    //sBindParam.name 
    sBindParam.type = aColumn->type.dataTypeId;
    sBindParam.language = aColumn->type.languageId;
    sBindParam.arguments = ( aColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK );

    /* BUG-47458 Lob은 column->precision 값이 0이다 */
    if ( (aColumn->column.flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
    {
        sBindParam.precision = aLength - RP_LOB_MTD_HEADER_SIZE;
    }
    else
    {
        sBindParam.precision = aColumn->precision;
    }

    sBindParam.scale = aColumn->scale;;
    sBindParam.inoutType = 1;
    sBindParam.data = NULL;
    sBindParam.dataSize = 0;

    IDE_TEST( qci::setBindParamInfo( aQciStatement,
                                     &sBindParam )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::setBindParamInfoArray( qciStatement      * aQciStatement,
                                              rpdMetaItem       * aMetaItem,
                                              UInt              * aCIDArray,
                                              smiValue          * aValueArray,
                                              UInt                aColumnCount,
                                              rpdXLog           * aXLog,
                                              idBool              aUpdateWere,
                                              UInt                aStartId,
                                              UInt              * aEndId )
{
    UInt                  i = 0;
    UInt                  sCID = 0;
    UInt                  sId = aStartId;
    rpdColumn           * sRpdColumn = NULL;
    mtcColumn           * sColumn = NULL;
    SChar                 sErrorBuffer[RP_MAX_MSG_LEN] = { 0, };
    idBool                sIsNullValue = ID_FALSE;

    for ( i = 0; i < aColumnCount; i++ )
    {
        sCID = aCIDArray[i] & SMI_COLUMN_ID_MASK;
        IDE_TEST_RAISE( sCID >= QCI_MAX_COLUMN_COUNT, ERR_INVALID_COLUMN_ID );

        if ( aMetaItem->mIsReplCol[sCID] == ID_TRUE )
        {
            sRpdColumn = aMetaItem->getRpdColumn( sCID );
            IDE_TEST_RAISE( sRpdColumn == NULL, ERR_NOT_FOUND_COLUMN );

            sColumn = &(sRpdColumn->mColumn);
            IDE_TEST( rpdConvertSQL::isNullValue( sColumn,
                                                  &(aValueArray[i]),
                                                  &sIsNullValue )
                      != IDE_SUCCESS );

            if ( ( ( sRpdColumn->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                   != QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
                 ( sIsNullValue == ID_FALSE ) )
            {
                if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
                {
                    if ( aUpdateWere == ID_FALSE )
                    {
                        IDE_TEST( setBindParamInfo( aQciStatement,
                                                    sColumn,
                                                    sId,
                                                    aValueArray[i].length )
                                    != IDE_SUCCESS );
                          sId++;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    IDE_TEST( setBindParamInfo( aQciStatement,
                                                sColumn,
                                                sId,
                                                aValueArray[i].length )
                              != IDE_SUCCESS );
                    sId++;
                }
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    *aEndId = sId;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_COLUMN_ID )
    {
        idlOS::snprintf( sErrorBuffer, RP_MAX_MSG_LEN,
                         "[rpsSQLExecutor::setBindParamInfoArray] Too Large Column ID [%"ID_UINT32_FMT"] "
                         "at [Remote SN: %"ID_UINT64_FMT", TID: %"ID_UINT32_FMT"]",
                         sCID, aXLog->mSN, aXLog->mTID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorBuffer ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_COLUMN );
    {
        idlOS::snprintf( sErrorBuffer, RP_MAX_MSG_LEN,
                         "[rpsSQLExecutor::setBindParamInfoArray] Column not found [CID: %"ID_UINT32_FMT"] "
                         "at [Remote SN: %"ID_UINT64_FMT", TID: %"ID_UINT32_FMT", TABLEOID: %"ID_vULONG_FMT"]",
                         sCID, aXLog->mSN, aXLog->mTID, aXLog->mTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorBuffer ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::setBindParamDataArray( qciStatement       * aQciStatement,
                                              rpdMetaItem        * aRemoteMetaItem,
                                              rpdMetaItem        * aLocalMetaItem,
                                              UInt               * aCIDArray,
                                              smiValue           * aValueArray,
                                              UInt                 aColumnCount,
                                              rpdXLog            * aXLog,
                                              idBool               aUpdateWere,
                                              UInt                 aStartId,
                                              UInt               * aEndId )
{
    UInt              sId = aStartId;
    UInt              sCID = 0;
    UInt              i = 0;
    SChar             sErrorBuffer[RP_MAX_MSG_LEN] = { 0, };
    rpdColumn       * sRpdColumn    = NULL;
    idBool            sIsNullValue	= ID_FALSE;
    const void      * sTable       	= NULL;
    mtcColumn       * sMtcColumn   	= NULL;

    for ( i = 0; i < aColumnCount; i++ )
    {
        sCID = aCIDArray[i] & SMI_COLUMN_ID_MASK;
        IDE_TEST_RAISE( sCID >= QCI_MAX_COLUMN_COUNT, ERR_INVALID_COLUMN_ID );

        if ( aRemoteMetaItem->mIsReplCol[sCID] == ID_TRUE )
        {
            sRpdColumn = aRemoteMetaItem->getRpdColumn( sCID );
            IDE_TEST_RAISE( sRpdColumn == NULL, ERR_NOT_FOUND_COLUMN );

            IDE_TEST( rpdConvertSQL::isNullValue( &(sRpdColumn->mColumn),
                                                  &(aValueArray[i]),
                                                  &sIsNullValue )
                      != IDE_SUCCESS );

            if ( ( ( sRpdColumn->mQPFlag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                   != QCM_COLUMN_HIDDEN_COLUMN_TRUE ) &&
                 ( sIsNullValue == ID_FALSE ) )
            {
                sTable = smiGetTable( aLocalMetaItem->mItem.mTableOID );

                sMtcColumn = (mtcColumn*)rpdCatalog::rpdGetTableColumns( sTable, sCID );
                IDE_TEST_RAISE( sMtcColumn == NULL, ERR_NOT_FOUND_COLUMN );

                if ( ( sMtcColumn->column.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_LOB )
                {
                    if ( aUpdateWere == ID_FALSE )
                    {
                        IDE_TEST( qci::setBindParamData( aQciStatement,
                                                         sId,
                                                         (void*)aValueArray[i].value,
                                                         aValueArray[i].length )
                                  != IDE_SUCCESS );
                        sId++;
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    IDE_TEST( qci::setBindParamData( aQciStatement,
                                                     sId,
                                                     (void*)aValueArray[i].value,
                                                     aValueArray[i].length )
                              != IDE_SUCCESS );
                    sId++;
                }
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    *aEndId = sId;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_COLUMN_ID );
    {
        idlOS::snprintf( sErrorBuffer, RP_MAX_MSG_LEN,
                         "[rpsSQLExecutor::setBindParamDataArray] Too Large Column ID [%"ID_UINT32_FMT"] "
                         "at [Remote SN: %"ID_UINT64_FMT", TID: %"ID_UINT32_FMT"]",
                         sCID, aXLog->mSN, aXLog->mTID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorBuffer ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_COLUMN );
    {
        idlOS::snprintf( sErrorBuffer, RP_MAX_MSG_LEN,
                         "[rpsSQLExecutor::setBindParamDataArray] Column not found [CID: %"ID_UINT32_FMT"] "
                         "at [Remote SN: %"ID_UINT64_FMT", TID: %"ID_UINT32_FMT", TABLEOID: %"ID_vULONG_FMT"]",
                         sCID, aXLog->mSN, aXLog->mTID, aXLog->mTableOID );

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrorBuffer ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::addtionalBindParamInfoforUpdate( qciStatement     * aQciStatement,
                                                        rpdMetaItem      * aRemoteMetaItem,
                                                        rpdMetaItem      * aLocalMetaItem,
                                                        rpdXLog          * aXLog,
                                                        UInt               aId,
                                                        idBool             aCompareBeforeImage )
{
    UInt              i = 0;
    UInt              sId = aId;
    UInt              sCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    UInt              sCIDIndex = 0;

    /* PK */
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        sCIDArray[i] = aLocalMetaItem->mPKIndex.keyColumns[i].column.id & SMI_COLUMN_ID_MASK;
    }

    IDE_TEST( setBindParamInfoArray( aQciStatement,
                                     aLocalMetaItem,
                                     sCIDArray,
                                     aXLog->mPKCols,
                                     aXLog->mPKColCnt,
                                     aXLog,
                                     ID_TRUE,
                                     sId,
                                     &sId )
              != IDE_SUCCESS );

    if ( aLocalMetaItem->mTsFlag != NULL )
    {
        IDE_TEST( setBindParamInfo( aQciStatement,
                                    aLocalMetaItem->mTsFlag,
                                    sId,
                                    aXLog->mACols[sId].length )
                  != IDE_SUCCESS );
        sId++;
    }
    else
    {
        if ( aCompareBeforeImage == ID_TRUE )
        {
            /* BCols */
            for ( i = 0; i < aXLog->mColCnt; i++ )
            {
                sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
                sCIDArray[i] = aRemoteMetaItem->mMapColID[sCIDIndex];
            }

            IDE_TEST( setBindParamInfoArray( aQciStatement,
                                             aRemoteMetaItem,
                                             sCIDArray,
                                             aXLog->mBCols,
                                             aXLog->mColCnt,
                                             aXLog,
                                             ID_TRUE,
                                             sId,
                                             &sId )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::addtionalBindParamDataforUpdate( qciStatement     * aQciStatement,
                                                        rpdMetaItem      * aRemoteMetaItem,
                                                        rpdMetaItem      * aLocalMetaItem,
                                                        rpdXLog          * aXLog,
                                                        UInt               aId,
                                                        idBool             aCompareBeforeImage )
{
    UInt          i = 0;
    UInt          sId = aId;
    UInt          sCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    UInt          sCIDIndex = 0;

    /* PK */
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        sCIDArray[i] = aLocalMetaItem->mPKIndex.keyColumns[i].column.id;
    }

    IDE_TEST( setBindParamDataArray( aQciStatement,
                                     aRemoteMetaItem,
                                     aLocalMetaItem,
                                     sCIDArray,
                                     aXLog->mPKCols,
                                     aXLog->mPKColCnt,
                                     aXLog,
                                     ID_TRUE,
                                     sId,
                                     &sId )
              != IDE_SUCCESS );

    if ( aLocalMetaItem->mTsFlag != NULL )
    {
        for ( i = 0; i < aXLog->mColCnt; i++ )
        {
            if ( ( aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK ) == 
                 ( ( aLocalMetaItem->mTsFlag->column.id ) & SMI_COLUMN_ID_MASK ) )
            {
                IDE_TEST( qci::setBindParamData( aQciStatement,
                                                 sId,
                                                 (void*)aXLog->mACols[i].value,
                                                 aXLog->mACols[i].length )
                          != IDE_SUCCESS );
                sId++;
                break;
            }
            else
            {
                /* do nothing */
            }
        }
    }
    else
    {
        if ( aCompareBeforeImage == ID_TRUE )
        {
            /* BCols */
            for ( i = 0; i < aXLog->mColCnt; i++ )
            {
                sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
                sCIDArray[i] = aRemoteMetaItem->mMapColID[sCIDIndex];
            }

            IDE_TEST( setBindParamDataArray( aQciStatement,
                                             aRemoteMetaItem,
                                             aLocalMetaItem,
                                             sCIDArray,
                                             aXLog->mBCols,
                                             aXLog->mColCnt,
                                             aXLog,
                                             ID_TRUE,
                                             sId,
                                             &sId )
                      != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::bind( qciStatement     * aQciStatement,
                             rpdMetaItem      * aRemoteMetaItem,
                             rpdMetaItem      * aLocalMetaItem,
                             rpdXLog          * aXLog,
                             idBool             aCompareBeforeImage )
{
    UInt              i = 0;
    UInt              sId = 0;
    UInt              sCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    UInt              sCIDIndex = 0;

    /* ACols */
    for ( i = 0; i < aXLog->mColCnt; i++ )
    {
        sCIDIndex = aXLog->mCIDs[i] & SMI_COLUMN_ID_MASK;
        sCIDArray[i] = aRemoteMetaItem->mMapColID[sCIDIndex];
    }

    IDE_TEST( setBindParamInfoArray( aQciStatement,
                                     aRemoteMetaItem,
                                     sCIDArray,
                                     aXLog->mACols,
                                     aXLog->mColCnt,
                                     aXLog,
                                     ID_FALSE,
                                     0,
                                     &sId )
              != IDE_SUCCESS );

    if ( aXLog->mType == RP_X_UPDATE )
    {
        IDE_TEST( addtionalBindParamInfoforUpdate( aQciStatement,
                                                   aRemoteMetaItem,
                                                   aLocalMetaItem,
                                                   aXLog,
                                                   sId,
                                                   aCompareBeforeImage )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    /* ACols */
    IDE_TEST( setBindParamDataArray( aQciStatement,
                                     aRemoteMetaItem,
                                     aLocalMetaItem,
                                     sCIDArray,
                                     aXLog->mACols,
                                     aXLog->mColCnt,
                                     aXLog,
                                     ID_FALSE,
                                     0,
                                     &sId )
              != IDE_SUCCESS );

    if ( aXLog->mType == RP_X_UPDATE )
    {
        IDE_TEST( addtionalBindParamDataforUpdate( aQciStatement,
                                                   aRemoteMetaItem,
                                                   aLocalMetaItem,
                                                   aXLog,
                                                   sId,
                                                   aCompareBeforeImage )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::hardRebuild( idvSQL                  * aStatistics,
                                    qciStatement            * aQciStatement,
                                    smiStatement            * aParentSmiStatement,
                                    smiStatement            * aSmiStatement,
                                    idBool                  * aIsBegun,
                                    qciSQLPlanCacheContext  * aPlanCacheContext,
                                    SChar                   * aSQLBuffer,
                                    UInt                      aSQLBufferLength,
                                    UInt                      aStmtFlag )
{
    idBool sIsBegun = ID_FALSE;

    idlOS::memset( aSmiStatement, 0x00, ID_SIZEOF( smiStatement ) );
    IDE_TEST( aSmiStatement->begin( aStatistics,
                                    aParentSmiStatement,
                                    aStmtFlag )
              != IDE_SUCCESS );
    sIsBegun = ID_TRUE;

    IDE_TEST( qci::clearStatement4Reprepare( aQciStatement,
                                             aSmiStatement )
              != IDE_SUCCESS );

    while ( qci::hardRebuild( aQciStatement,
                              aSmiStatement,
                              aParentSmiStatement,
                              aPlanCacheContext,
                              aSQLBuffer,
                              aSQLBufferLength )
            != IDE_SUCCESS )
    {
        IDE_TEST( ideIsRebuild() != IDE_SUCCESS );

        sIsBegun = ID_FALSE;
        IDE_TEST( aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE )
                  != IDE_SUCCESS );

        idlOS::memset( aSmiStatement, 0x00, ID_SIZEOF( smiStatement ) );
        IDE_TEST( aSmiStatement->begin( aStatistics,
                                        aParentSmiStatement,
                                        aStmtFlag )
                  != IDE_SUCCESS );
        sIsBegun = ID_TRUE;
    }

    IDE_TEST( qci::setBindTuple( aQciStatement ) != IDE_SUCCESS );

    *aIsBegun = sIsBegun;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegun == ID_TRUE )
    {
        sIsBegun = ID_FALSE;
        (void)aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* do nothing */
    }

    *aIsBegun = sIsBegun;

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::rebuild( qciStatement            * aQciStatement,
                                smiStatement            * aParentSmiStatement,
                                smiStatement            * aSmiStatement,
                                idBool                  * aIsBegun,
                                qciSQLPlanCacheContext  * aPlanCacheContext,
                                SChar                   * aSQLBuffer,
                                UInt                      aSQLBufferLength )
{
    idBool sIsBegun = *aIsBegun;

    IDE_TEST( qci::closeCursor( aQciStatement,
                                aSmiStatement )
              != IDE_SUCCESS );

    sIsBegun = ID_FALSE;
    IDE_TEST( aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE )
              != IDE_SUCCESS );

    IDE_TEST( hardRebuild( NULL,
                           aQciStatement,
                           aParentSmiStatement,
                           aSmiStatement,
                           &sIsBegun,
                           aPlanCacheContext,
                           aSQLBuffer,
                           aSQLBufferLength,
                           SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );

    *aIsBegun = sIsBegun;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsBegun = sIsBegun;

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::retry( qciStatement      * aQciStatement,
                              smiStatement      * aParentSmiStatement,
                              smiStatement      * aSmiStatement,
                              idBool            * aIsBegun )
{
    idBool      sIsBegun = *aIsBegun;

    IDE_TEST( qci::retry( aQciStatement, aSmiStatement )
              != IDE_SUCCESS );

    sIsBegun = ID_FALSE;
    IDE_TEST( aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE )
              != IDE_SUCCESS );

    idlOS::memset( aSmiStatement, 0x00, ID_SIZEOF( smiStatement ) );

    IDE_TEST( aSmiStatement->begin( NULL,
                                    aParentSmiStatement,
                                    SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sIsBegun = ID_TRUE;

    *aIsBegun = sIsBegun;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegun == ID_TRUE )
    {
        sIsBegun = ID_FALSE;
        (void)aSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* do nothing */
    }

    *aIsBegun = sIsBegun;

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpsSQLExecutor::execute( qciStatement            * aQciStatement,
                                smiStatement            * aParentSmiStatement,
                                qciSQLPlanCacheContext  * aPlanCacheContext,
                                SChar                   * aSQLBuffer,
                                UInt                      aSQLBufferLength,
                                SLong                   * aRowCount )
{
    smiStatement    sSmiStatement;
    SLong           sAffectedRowCount = 0;
    SLong           sFetchedRowCount = 0;
    idBool          sIsBegun = ID_FALSE;
    idBool          sIsDone = ID_FALSE;

    idlOS::memset( &sSmiStatement, 0x00, ID_SIZEOF( smiStatement ) );
    IDE_TEST( sSmiStatement.begin( NULL,
                                   aParentSmiStatement,
                                   SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sIsBegun = ID_TRUE;

    while ( sIsDone != ID_TRUE )
    {
        while ( qci::execute( aQciStatement, &sSmiStatement ) != IDE_SUCCESS )
        {
            switch( ideGetErrorCode() & E_ACTION_MASK )
            {
                case E_ACTION_RETRY:

                    IDE_TEST( retry( aQciStatement,
                                     aParentSmiStatement,
                                     &sSmiStatement,
                                     &sIsBegun )
                              != IDE_SUCCESS );
                    break;

                case E_ACTION_REBUILD:

                    IDE_TEST( rebuild( aQciStatement,
                                       aParentSmiStatement,
                                       &sSmiStatement,
                                       &sIsBegun,
                                       aPlanCacheContext,
                                       aSQLBuffer,
                                       aSQLBufferLength )
                              != IDE_SUCCESS );
                    break;

                case E_ACTION_IGNORE:
                case E_ACTION_ABORT:
                case E_ACTION_FATAL:
                default:
                    IDE_TEST( ID_TRUE );
                    break;
            }

            IDE_CLEAR();
        }

        if ( qci::closeCursor( aQciStatement,
                               &sSmiStatement )
             != IDE_SUCCESS )
        {
            IDE_TEST( retry( aQciStatement,
                             aParentSmiStatement,
                             &sSmiStatement,
                             &sIsBegun )
                      != IDE_SUCCESS );
        }
        else
        {
            sIsDone = ID_TRUE;
        }
    }

    IDE_TEST( sSmiStatement.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegun = ID_FALSE;

    qci::getRowCount( aQciStatement, &sAffectedRowCount, &sFetchedRowCount);

    *aRowCount = sAffectedRowCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegun == ID_TRUE )
    {
        (void)qci::closeCursor( aQciStatement, &sSmiStatement );

        (void)sSmiStatement.end( SMI_STATEMENT_RESULT_FAILURE );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}
