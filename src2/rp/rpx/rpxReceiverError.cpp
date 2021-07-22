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
 * $Id: rpxReceiverError.cpp 85848 2019-07-15 07:36:08Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>
#include <rpDef.h>
#include <rpcManager.h>
#include <rpxReceiverApply.h>
#include <rpdConvertSQL.h>

void rpxReceiverApply::changeToHex( SChar    * aResult,
                                    SChar    * aSource,
                                    SInt       aLength )
{
    static SChar sTable[256][2] = {
        { '0', '0' }, { '0', '1' }, { '0', '2' }, { '0', '3' },
        { '0', '4' }, { '0', '5' }, { '0', '6' }, { '0', '7' },
        { '0', '8' }, { '0', '9' }, { '0', 'A' }, { '0', 'B' },
        { '0', 'C' }, { '0', 'D' }, { '0', 'E' }, { '0', 'F' },
        { '1', '0' }, { '1', '1' }, { '1', '2' }, { '1', '3' },
        { '1', '4' }, { '1', '5' }, { '1', '6' }, { '1', '7' },
        { '1', '8' }, { '1', '9' }, { '1', 'A' }, { '1', 'B' },
        { '1', 'C' }, { '1', 'D' }, { '1', 'E' }, { '1', 'F' },
        { '2', '0' }, { '2', '1' }, { '2', '2' }, { '2', '3' },
        { '2', '4' }, { '2', '5' }, { '2', '6' }, { '2', '7' },
        { '2', '8' }, { '2', '9' }, { '2', 'A' }, { '2', 'B' },
        { '2', 'C' }, { '2', 'D' }, { '2', 'E' }, { '2', 'F' },
        { '3', '0' }, { '3', '1' }, { '3', '2' }, { '3', '3' },
        { '3', '4' }, { '3', '5' }, { '3', '6' }, { '3', '7' },
        { '3', '8' }, { '3', '9' }, { '3', 'A' }, { '3', 'B' },
        { '3', 'C' }, { '3', 'D' }, { '3', 'E' }, { '3', 'F' },
        { '4', '0' }, { '4', '1' }, { '4', '2' }, { '4', '3' },
        { '4', '4' }, { '4', '5' }, { '4', '6' }, { '4', '7' },
        { '4', '8' }, { '4', '9' }, { '4', 'A' }, { '4', 'B' },
        { '4', 'C' }, { '4', 'D' }, { '4', 'E' }, { '4', 'F' },
        { '5', '0' }, { '5', '1' }, { '5', '2' }, { '5', '3' },
        { '5', '4' }, { '5', '5' }, { '5', '6' }, { '5', '7' },
        { '5', '8' }, { '5', '9' }, { '5', 'A' }, { '5', 'B' },
        { '5', 'C' }, { '5', 'D' }, { '5', 'E' }, { '5', 'F' },
        { '6', '0' }, { '6', '1' }, { '6', '2' }, { '6', '3' },
        { '6', '4' }, { '6', '5' }, { '6', '6' }, { '6', '7' },
        { '6', '8' }, { '6', '9' }, { '6', 'A' }, { '6', 'B' },
        { '6', 'C' }, { '6', 'D' }, { '6', 'E' }, { '6', 'F' },
        { '7', '0' }, { '7', '1' }, { '7', '2' }, { '7', '3' },
        { '7', '4' }, { '7', '5' }, { '7', '6' }, { '7', '7' },
        { '7', '8' }, { '7', '9' }, { '7', 'A' }, { '7', 'B' },
        { '7', 'C' }, { '7', 'D' }, { '7', 'E' }, { '7', 'F' },
        { '8', '0' }, { '8', '1' }, { '8', '2' }, { '8', '3' },
        { '8', '4' }, { '8', '5' }, { '8', '6' }, { '8', '7' },
        { '8', '8' }, { '8', '9' }, { '8', 'A' }, { '8', 'B' },
        { '8', 'C' }, { '8', 'D' }, { '8', 'E' }, { '8', 'F' },
        { '9', '0' }, { '9', '1' }, { '9', '2' }, { '9', '3' },
        { '9', '4' }, { '9', '5' }, { '9', '6' }, { '9', '7' },
        { '9', '8' }, { '9', '9' }, { '9', 'A' }, { '9', 'B' },
        { '9', 'C' }, { '9', 'D' }, { '9', 'E' }, { '9', 'F' },
        { 'A', '0' }, { 'A', '1' }, { 'A', '2' }, { 'A', '3' },
        { 'A', '4' }, { 'A', '5' }, { 'A', '6' }, { 'A', '7' },
        { 'A', '8' }, { 'A', '9' }, { 'A', 'A' }, { 'A', 'B' },
        { 'A', 'C' }, { 'A', 'D' }, { 'A', 'E' }, { 'A', 'F' },
        { 'B', '0' }, { 'B', '1' }, { 'B', '2' }, { 'B', '3' },
        { 'B', '4' }, { 'B', '5' }, { 'B', '6' }, { 'B', '7' },
        { 'B', '8' }, { 'B', '9' }, { 'B', 'A' }, { 'B', 'B' },
        { 'B', 'C' }, { 'B', 'D' }, { 'B', 'E' }, { 'B', 'F' },
        { 'C', '0' }, { 'C', '1' }, { 'C', '2' }, { 'C', '3' },
        { 'C', '4' }, { 'C', '5' }, { 'C', '6' }, { 'C', '7' },
        { 'C', '8' }, { 'C', '9' }, { 'C', 'A' }, { 'C', 'B' },
        { 'C', 'C' }, { 'C', 'D' }, { 'C', 'E' }, { 'C', 'F' },
        { 'D', '0' }, { 'D', '1' }, { 'D', '2' }, { 'D', '3' },
        { 'D', '4' }, { 'D', '5' }, { 'D', '6' }, { 'D', '7' },
        { 'D', '8' }, { 'D', '9' }, { 'D', 'A' }, { 'D', 'B' },
        { 'D', 'C' }, { 'D', 'D' }, { 'D', 'E' }, { 'D', 'F' },
        { 'E', '0' }, { 'E', '1' }, { 'E', '2' }, { 'E', '3' },
        { 'E', '4' }, { 'E', '5' }, { 'E', '6' }, { 'E', '7' },
        { 'E', '8' }, { 'E', '9' }, { 'E', 'A' }, { 'E', 'B' },
        { 'E', 'C' }, { 'E', 'D' }, { 'E', 'E' }, { 'E', 'F' },
        { 'F', '0' }, { 'F', '1' }, { 'F', '2' }, { 'F', '3' },
        { 'F', '4' }, { 'F', '5' }, { 'F', '6' }, { 'F', '7' },
        { 'F', '8' }, { 'F', '9' }, { 'F', 'A' }, { 'F', 'B' },
        { 'F', 'C' }, { 'F', 'D' }, { 'F', 'E' }, { 'F', 'F' }
    };

    SChar    * sCursor;

    for( ; aLength > 0; aLength--, aSource++ )
    {
        sCursor = sTable[(SInt)(*aSource)];
        *aResult = *sCursor;
        aResult++, sCursor++;
        *aResult = *sCursor;
        aResult++;
    }

    *aResult = '\0';
}

void rpxReceiverApply::commitErrLog()
{
    ideLog::log(IDE_RP_3, RP_TRC_RA_CONFLICT_COMMIT);
    ideLog::log(IDE_RP_CONFLICT_3, RP_TRC_RA_CONFLICT_COMMIT);
}

void rpxReceiverApply::abortErrLog()
{
    ideLog::log(IDE_RP_3, RP_TRC_RA_CONFLICT_ABORT);
    ideLog::log(IDE_RP_CONFLICT_3, RP_TRC_RA_CONFLICT_ABORT);
}

void rpxReceiverApply::printPK( ideLogEntry   & aLog,
                                const SChar   * aPrefix,
                                rpdMetaItem   * aMetaItem,
                                smiValue      * aSmiValueArray )
{
    idBool       sIsValid = ID_FALSE;
    UInt         i = 0;
    UInt         sPKCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };

    if ( aPrefix != NULL )
    {
        aLog.appendFormat( "%s, ", aPrefix );
    }
    else
    {
        /* do nothing */
    }

    aLog.appendFormat( "User : %s, Table : %s, ", aMetaItem->mItem.mLocalUsername,
                                                  aMetaItem->mItem.mLocalTablename );

    /* PK */
    for ( i = 0; i < aMetaItem->mPKIndex.keyColCount; i++ )
    {
        sPKCIDArray[i] = aMetaItem->mPKIndex.keyColumns[i].column.id;
    }

    aLog.appendFormat("PK : ( " );

    rpdConvertSQL::getColumnListClause( aMetaItem,
                                        aMetaItem,
                                        aMetaItem->mPKIndex.keyColCount,
                                        sPKCIDArray,
                                        sPKCIDArray,
                                        aSmiValueArray,
                                        ID_TRUE,
                                        ID_TRUE,
                                        ID_TRUE,
                                        (SChar*)",",
                                        mSQLBuffer,
                                        mSQLBufferLength,
                                        &sIsValid );
    (void)aLog.appendFormat( "%s )", mSQLBuffer );
}
                                  

IDE_RC rpxReceiverApply::insertErrLog( ideLogEntry    &aLog,
                                       rpdMetaItem    *aMetaItem,
                                       rpdXLog        *aXLog,
                                       rpApplyFailType aFailType,
                                       const SChar    *aPrefix )
{
    idBool       sIsValid = ID_FALSE;

    aLog.appendFormat("%sINSERT INTO %s.%s VALUES ( ",
                      aPrefix,
                      aMetaItem->mItem.mLocalUsername,
                      aMetaItem->mItem.mLocalTablename);

    IDE_TEST_RAISE( aXLog->mColCnt != (UInt)aMetaItem->mColCount,
                    ERR_COL_COUNT_MISMATCH );

    rpdConvertSQL::getColumnListClause( aMetaItem,
                                        aMetaItem,
                                        aXLog->mColCnt,
                                        aXLog->mCIDs,
                                        aXLog->mCIDs,
                                        aXLog->mACols,
                                        ID_FALSE,
                                        ID_FALSE,
                                        ID_FALSE,
                                        (SChar*)", ",
                                        mSQLBuffer,
                                        mSQLBufferLength,
                                        &sIsValid );

    (void)aLog.appendFormat("%s ); (TID : %u)", mSQLBuffer, aXLog->mTID);


    if ( aFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE )
    {
        aLog.append(" (CORRUPTED)\n");
    }
    else
    {
        aLog.append("\n");
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COL_COUNT_MISMATCH );
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_COLUMN_COUNT_MISMATCH,
                                "[Insert Error Log]",
                                aMetaItem->mItem.mLocalTablename,
                                aXLog->mColCnt,
                                aMetaItem->mColCount)
                );
        ideLog::logErrorMsgInternal(aLog);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::updateErrLog(ideLogEntry   & aLog,
                                      rpdMetaItem   * aMetaItem,
                                      rpdMetaItem   * aMetaItemForPK,
                                      rpdXLog       * aXLog,
                                      idBool          aIsCmpBeforeImg,
                                      rpApplyFailType aFailType,
                                      const SChar   * aPrefix)
{
    UInt          i = 0;
    UInt          sPKCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    idBool        sIsValid = ID_FALSE;

    // for making shortest message
    aLog.appendFormat("%sUPDATE %s.%s SET ",
                      aPrefix,
                      aMetaItem->mItem.mLocalUsername,
                      aMetaItem->mItem.mLocalTablename);

    // write update column 
    rpdConvertSQL::getColumnListClause( aMetaItem,
                                        aMetaItem,
                                        aXLog->mColCnt,
                                        aXLog->mCIDs,
                                        aXLog->mCIDs,
                                        aXLog->mACols,
                                        ID_TRUE,
                                        ID_FALSE,
                                        ID_FALSE,
                                        (SChar*)", ",
                                        mSQLBuffer,
                                        mSQLBufferLength,
                                        &sIsValid );

    (void)aLog.appendFormat( "%s", mSQLBuffer );

    IDE_TEST_RAISE( aXLog->mPKColCnt != aMetaItemForPK->mPKIndex.keyColCount,
                    ERR_PK_COL_COUNT_MISMATCH );

    (void)aLog.appendFormat( " WHERE " );

    /* PK */
    for ( i = 0; i < aMetaItemForPK->mPKIndex.keyColCount; i++ )
    {
        sPKCIDArray[i] = aMetaItemForPK->mPKIndex.keyColumns[i].column.id;
    }

    rpdConvertSQL::getColumnListClause( aMetaItemForPK,
                                        aMetaItemForPK,
                                        aMetaItemForPK->mPKIndex.keyColCount,
                                        sPKCIDArray,
                                        sPKCIDArray,
                                        aXLog->mPKCols,
                                        ID_TRUE,
                                        ID_TRUE,
                                        ID_TRUE,
                                        (SChar*)"AND",
                                        mSQLBuffer,
                                        mSQLBufferLength,
                                        &sIsValid );

    (void)aLog.appendFormat( "%s", mSQLBuffer );

    /* BUG-36555 : Before image logging */
    if ( RPU_REPLICATION_BEFORE_IMAGE_LOG_ENABLE == 1 )
    {
        if ( aIsCmpBeforeImg == ID_TRUE )
        {
            aLog.append( "; (before : " );

            rpdConvertSQL::getColumnListClause( aMetaItem,
                                                aMetaItem,
                                                aXLog->mColCnt,
                                                aXLog->mCIDs,
                                                aXLog->mCIDs,
                                                aXLog->mBCols,
                                                ID_TRUE,
                                                ID_TRUE,
                                                ID_FALSE,
                                                (SChar*)", ",
                                                mSQLBuffer,
                                                mSQLBufferLength,
                                                &sIsValid );

            (void)aLog.appendFormat( "%s )", mSQLBuffer );
        }
        else
        {
            aLog.append(";");
        }
    }
    else
    {
        aLog.append(";");
    }

    aLog.appendFormat(" (TID : %u)", aXLog->mTID);

    if ( aFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE )
    {
        aLog.append(" (CORRUPTED)\n");
    }
    else
    {
        aLog.append("\n");
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PK_COL_COUNT_MISMATCH );
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_COLUMN_COUNT_MISMATCH,
                                "[Update Error Log] PK",
                                aMetaItem->mItem.mLocalTablename,
                                aXLog->mPKColCnt,
                                aMetaItem->mPKIndex.keyColCount)
               );
        ideLog::logErrorMsgInternal(aLog);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxReceiverApply::deleteErrLog( ideLogEntry    &aLog,
                                       rpdMetaItem    *aMetaItem,
                                       rpdXLog        *aXLog,
                                       rpApplyFailType aFailType,
                                       const SChar    *aPrefix )
{
    UInt       i = 0;
    UInt       sPKCIDArray[QCI_MAX_COLUMN_COUNT] = { 0, };
    idBool     sIsValid = ID_FALSE;

    IDE_TEST_RAISE( aXLog->mPKColCnt != aMetaItem->mPKIndex.keyColCount,
                    ERR_COL_COUNT_MISMATCH );

    (void)aLog.appendFormat( "%sDELETE FROM %s.%s WHERE ", aPrefix,
                                                           aMetaItem->mItem.mLocalUsername,
                                                           aMetaItem->mItem.mLocalTablename );
    /* PK */
    for ( i = 0; i < aMetaItem->mPKIndex.keyColCount; i++ )
    {
        sPKCIDArray[i] = aMetaItem->mPKIndex.keyColumns[i].column.id;
    }

    rpdConvertSQL::getColumnListClause( aMetaItem,
                                        aMetaItem,
                                        aMetaItem->mPKIndex.keyColCount,
                                        sPKCIDArray,
                                        sPKCIDArray,
                                        aXLog->mPKCols,
                                        ID_TRUE,
                                        ID_TRUE,
                                        ID_FALSE,
                                        (SChar*)"AND",
                                        mSQLBuffer,
                                        mSQLBufferLength,
                                        &sIsValid );

    (void)aLog.appendFormat( "%s; (TID : %u)", mSQLBuffer, aXLog->mTID );

    if ( aFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE )
    {
        aLog.append(" (CORRUPTED)\n");
    }
    else
    {
        aLog.append("\n");
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COL_COUNT_MISMATCH );
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_COLUMN_COUNT_MISMATCH,
                                "[Delete Error Log] PK",
                                aMetaItem->mItem.mLocalTablename,
                                aXLog->mPKColCnt,
                                aMetaItem->mPKIndex.keyColCount)
                );
        ideLog::logErrorMsgInternal(aLog);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxReceiverApply::lobErrLog( ideLogEntry    &aLog,
                                  rpdMetaItem    *aMetaItem,
                                  rpdXLog        *aXLog,
                                  rpApplyFailType aFailType,
                                  const SChar    *aPrefix )
{
    aLog.appendFormat("%sA failure occurred while opening LOB Cursor (Table : %s.%s) (TID : %u) ",
                      aPrefix,
                      aMetaItem->mItem.mLocalUsername,
                      aMetaItem->mItem.mLocalTablename,
                      aXLog->mTID );

    if ( aFailType == RP_APPLY_FAIL_BY_CORRUPTED_PAGE )
    {
        aLog.append(" (CORRUPTED)\n");
    }
    else
    {
        aLog.append("\n");
    }
}


void rpxReceiverApply::abortConflictLog( ideLogEntry &aLog,
                                         rpdXLog * aXLog )
{
    aLog.appendFormat( "ROLLBACK (TID : %u)\n", aXLog->mTID );
    return;
}

void rpxReceiverApply::commitConflictLog( ideLogEntry &aLog,
                                          rpdXLog * aXLog )
{
    aLog.appendFormat( "COMMIT (TID : %u)\n", aXLog->mTID );
    return;
}

void rpxReceiverApply::abortToSavepointConflictLog( ideLogEntry &aLog,
                                                    rpdXLog * aXLog,
                                                    SChar * aSavepointName )
{
    aLog.appendFormat( "ROLLBACK TO SAVEPOINT %s (TID : %u)\n",
                       aSavepointName, aXLog->mTID );
    return;
}
