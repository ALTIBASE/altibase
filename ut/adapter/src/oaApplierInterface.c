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
 
/*
 * Copyright 2016, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 */
#include <acp.h>
#include <ace.h>

#include <oaContext.h>
#include <oaConfig.h>

#include <oaApplierInterface.h>

ace_rc_t initializeApplier( oaContext                * aContext,
                            oaConfigHandle           * aConfigHandle,
                            const ALA_Replication    * aAlaReplication,
                            oaApplierHandle         ** aApplierHandle )
{
#ifdef ALTIADAPTER
    return initializeAltiApplier( aContext,
                                  aConfigHandle,
                                  aAlaReplication->mTableCount,
                                  aApplierHandle );

#elif JDBCADAPTER
    return initializeJDBCApplier( aContext,
                                  aConfigHandle,
                                  aAlaReplication,
                                  aApplierHandle );
#else
    return initializeOciApplier( aContext,
                                 aConfigHandle,
                                 aAlaReplication->mTableCount,
                                 aApplierHandle );
#endif
}

void finalizeImportedLibrary( void )
{
#ifdef ALTIADAPTER
    /* nothing to do */
#elif JDBCADAPTER            
    oaFinalizeJAVAVM();
#else
    oaFinalizeOCILibrary();
#endif
    return;
}

void finalizeApplier( oaApplierHandle *aApplierHandle )
{
#ifdef ALTIADAPTER
    finalizeAltiApplier( aApplierHandle );
#elif JDBCADAPTER
    finalizeJDBCApplier( aApplierHandle );
#else
    finalizeOciApplier( aApplierHandle );
#endif
    return;
}

/*
 * normal Xth placeholder is like ":X". example ":1"
 *
 * if Xth column is date, "TO_DATE( :X, 'YYYY/MM/DD HH24:MI:SS')" is used.
 */
void addPlaceHolderToQuery( acp_str_t              * aQuery,
                            acp_sint32_t             aPlaceHolderIndex,
                            oaLogRecordValueType     aLogRecordValueType )
{
    switch (aLogRecordValueType)
    {
    case OA_LOG_RECORD_VALUE_TYPE_NUMERIC:
    case OA_LOG_RECORD_VALUE_TYPE_FLOAT:
    case OA_LOG_RECORD_VALUE_TYPE_DOUBLE:
    case OA_LOG_RECORD_VALUE_TYPE_REAL:
    case OA_LOG_RECORD_VALUE_TYPE_BIGINT:
    case OA_LOG_RECORD_VALUE_TYPE_INTEGER:
    case OA_LOG_RECORD_VALUE_TYPE_SMALLINT:
    case OA_LOG_RECORD_VALUE_TYPE_CHAR:
    case OA_LOG_RECORD_VALUE_TYPE_VARCHAR:
    case OA_LOG_RECORD_VALUE_TYPE_NCHAR:
    case OA_LOG_RECORD_VALUE_TYPE_NVARCHAR:
#if defined(ALTIADAPTER) || defined(JDBCADAPTER)
        ACP_UNUSED( aPlaceHolderIndex );
        (void)acpStrCatFormat( aQuery, "?" );
#else
        (void)acpStrCatFormat( aQuery, ":%d", aPlaceHolderIndex );
#endif
        break;

    case OA_LOG_RECORD_VALUE_TYPE_DATE:
#ifdef ALTIADAPTER
        (void)acpStrCatFormat( aQuery, "TO_DATE(?, 'YYYY/MM/DD HH24:MI:SS.SSSSSS')" );
        
#elif JDBCADAPTER
        (void)acpStrCatFormat( aQuery, "?" );
#else
        (void)acpStrCatFormat( aQuery, "TO_TIMESTAMP(:%d, 'YYYY/MM/DD HH24:MI:SS.FF6')", aPlaceHolderIndex );
#endif
        break;

    default:
        break;
    }
}

/*
 * example "INSERT INTO tableNames VALUES ( :1, :2, :3, :4 )"
 *
 * if Xth column is date, "TO_DATE( :X, 'YYYY/MM/DD HH24:MI:SS')" is used.
 */
void prepareInsertQuery( oaLogRecordInsert * aLogRecord,
                         acp_str_t         * aQuery,
                         acp_bool_t          aIsDirectPathMode,
                         acp_bool_t          aSetUserToTable,
                         acp_bool_t          aSetColumnToInsert )
{
    acp_sint32_t i = 0;
    acp_bool_t sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t sBindIndex = 0;

    if ( aSetColumnToInsert == ACP_FALSE )
    {
        if ( aIsDirectPathMode == ACP_TRUE )
        {
            /* ~ 10g : APPEND Hint는 INSERT ... SELECT 구문에서만 동작
             * 11gR1 : INSERT ... VALUES ... 구문에서 APPEND Hint가 동작
             * 11gR2 : INSERT ... VALUES ... 구문에서 APPEND_VALUES Hint가 동작
             */
            if ( aSetUserToTable == ACP_TRUE )
            {
                (void)acpStrCatFormat( aQuery, "INSERT /*+ APPEND APPEND_VALUES */ INTO %.s.%s VALUES (", 
                                       aLogRecord->mToUser, aLogRecord->mTableName );
            }
            else
            {
                (void)acpStrCatFormat( aQuery, "INSERT /*+ APPEND APPEND_VALUES */ INTO %s VALUES (", aLogRecord->mTableName );
            }
    
        }
        else
        {
            if ( aSetUserToTable == ACP_TRUE )
            {
                (void)acpStrCatFormat( aQuery, "INSERT INTO %s.%s VALUES (", 
                                       aLogRecord->mToUser, aLogRecord->mTableName );
            }
            else
            {
                (void)acpStrCatFormat( aQuery, "INSERT INTO %s VALUES (", aLogRecord->mTableName );
            }
        }
    }
    else
    {
        if ( aIsDirectPathMode == ACP_TRUE )
        {
            /* ~ 10g : APPEND Hint는 INSERT ... SELECT 구문에서만 동작
             * 11gR1 : INSERT ... VALUES ... 구문에서 APPEND Hint가 동작
             * 11gR2 : INSERT ... VALUES ... 구문에서 APPEND_VALUES Hint가 동작
             */
            if ( aSetUserToTable == ACP_TRUE )
            {
                (void)acpStrCatFormat( aQuery, "INSERT /*+ APPEND APPEND_VALUES */ INTO %.s.%s ( ", 
                                       aLogRecord->mToUser, aLogRecord->mTableName );
            }
            else
            {
                (void)acpStrCatFormat( aQuery, "INSERT /*+ APPEND APPEND_VALUES */ INTO %s ( ", aLogRecord->mTableName );
            }
    
        }
        else
        {
            if ( aSetUserToTable == ACP_TRUE )
            {
                (void)acpStrCatFormat( aQuery, "INSERT INTO %s.%s ( ", 
                                       aLogRecord->mToUser, aLogRecord->mTableName );
            }
            else
            {
                (void)acpStrCatFormat( aQuery, "INSERT INTO %s ( ", aLogRecord->mTableName );
            }
        }
        
        for ( i = 0; i < aLogRecord->mColumnCount; i++ )
        {        
            sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mColumn[i]) );
            if ( sIsHiddenColumn == ACP_FALSE )
            {
                if ( i > 0 ) 
                {
                    (void)acpStrCatFormat( aQuery, ", " );
                }
                
                (void)acpStrCatFormat( aQuery, "%s", aLogRecord->mColumn[i].mName );
            }
        }
        
        acpStrCatFormat( aQuery, " ) VALUES (");
    }
    
    for ( i = 0; i < aLogRecord->mColumnCount; i++ )
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mColumn[i]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            if (i > 0)
            {
                (void) acpStrCatFormat(aQuery, ", ");
            }
            else
            {
                /* do nothing */
            }

            sBindIndex++;
            addPlaceHolderToQuery( aQuery, sBindIndex, aLogRecord->mColumn[i].mType );

        }
        else
        {
            /* do nothing */
        }
    }

    (void)acpStrCatFormat( aQuery, ")" );
}

/*
 * example "UPDATE tableNames SET columnName1 = :1, columnName2 = :2
 *          WHERE columnName3 = :3"
 */
void prepareUpdateQuery( oaLogRecordUpdate * aLogRecord, 
                         acp_str_t         * aQuery,
                         acp_bool_t          aSetUserToTable )
{
    acp_sint32_t i = 0;
    acp_sint32_t j = 0;
    acp_uint32_t sColumnID = 0;
    acp_bool_t sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t sBindIndex = 0;

    if ( aSetUserToTable == ACP_TRUE )
    {
        (void)acpStrCatFormat( aQuery, "UPDATE %s.%s SET ", 
                               aLogRecord->mToUser, aLogRecord->mTableName );
    }
    else
    {
        (void)acpStrCatFormat( aQuery, "UPDATE %s SET ", aLogRecord->mTableName );
    }
    
    for ( i = 0; i < aLogRecord->mColumnCount; i++ )
    {
        sColumnID = aLogRecord->mColumnIDMap[i];

        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mColumn[sColumnID]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            if ( i > 0 )
            {
                (void)acpStrCatFormat( aQuery, ", " );
            }
            else
            {
                /* do nothing */
            }

            (void)acpStrCatFormat( aQuery, "%s = ", aLogRecord->mColumn[sColumnID].mName );

            sBindIndex++;
            addPlaceHolderToQuery( aQuery, sBindIndex, aLogRecord->mColumn[sColumnID].mType );
        }
        else
        {
            /* do nothing */
        }
    }

    (void)acpStrCatFormat( aQuery, " WHERE " );

    for ( j = 0; j < aLogRecord->mPrimaryKeyCount; j++ )
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mPrimaryKey[j]) );

        ACE_ASSERT( sIsHiddenColumn == ACP_FALSE );

        if ( j > 0 )
        {
            (void)acpStrCatFormat( aQuery, " AND " );
        }
        else
        {
            /* do nothing */
        }

        (void)acpStrCatFormat( aQuery, "%s = ", aLogRecord->mPrimaryKey[j].mName );

        sBindIndex++;
        addPlaceHolderToQuery( aQuery, sBindIndex, aLogRecord->mPrimaryKey[j].mType );
    }
}

/*
 * example "DELETE FROM tableNames WHERE column1 = :1 AND column2 = :2"
 *
 * if Xth column is date, "TO_DATE( :X, 'YYYY/MM/DD HH24:MI:SS')" is used.
 */
void prepareDeleteQuery( oaLogRecordDelete * aLogRecord,
                         acp_str_t         * aQuery,
                         acp_bool_t          aSetUserToTable )
{
    acp_sint32_t i = 0;
    acp_bool_t sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t sBindIndex = 0;

    if ( aSetUserToTable == ACP_TRUE )
    {
        (void)acpStrCatFormat( aQuery, "DELETE FROM %s.%s WHERE ", 
                               aLogRecord->mToUser, aLogRecord->mTableName );
    }
    else
    {
        (void)acpStrCatFormat( aQuery, "DELETE FROM %s WHERE ", aLogRecord->mTableName );
    }
    
    for ( i = 0; i < aLogRecord->mPrimaryKeyCount; i++ )
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mPrimaryKey[i]) );

        ACE_ASSERT( sIsHiddenColumn == ACP_FALSE );

        if (i > 0)
        {
            (void)acpStrCatFormat( aQuery, " AND " );
        }
        else
        {
            /* do nothing */
        }

        (void)acpStrCatFormat( aQuery, "%s = ", aLogRecord->mPrimaryKey[i].mName );

        sBindIndex++;
        addPlaceHolderToQuery( aQuery, sBindIndex, aLogRecord->mPrimaryKey[i].mType );
    }
}

/**
 * @breif  Array DML Max Size를 얻는다.
 *
 * @param  aHandle    Oracle Applier Handle
 *
 * @return Array DML Max Size
 */
acp_uint32_t oaApplierGetArrayDMLMaxSize( oaApplierHandle * aHandle )
{
#ifdef JDBCADAPTER    
    return aHandle->mBatchDMLMaxSize;
#else
    return aHandle->mArrayDMLMaxSize;
#endif    
}

/**
 * @breif  Group Commit 여부를 얻는다.
 *
 * @param  aHandle    Oracle Applier Handle
 *
 * @return Group Commit 여부
 */
acp_bool_t oaApplierIsGroupCommit( oaApplierHandle * aHandle )
{
    return aHandle->mIsGroupCommit;
}

ace_rc_t oaApplierApplyLogRecordList( oaContext       * aContext,
                                      oaApplierHandle * aHandle,
                                      acp_list_t      * aLogRecordList,
                                      oaLogSN           aPrevLastProcessedSN,
                                      oaLogSN         * aLastProcessedSN )
{
#ifdef ALTIADAPTER
    return oaAltiApplierApplyLogRecordList( aContext,
                                             aHandle,
                                             aLogRecordList,
                                             aPrevLastProcessedSN,
                                             aLastProcessedSN );
#elif JDBCADAPTER
    return oaJDBCApplierApplyLogRecordList( aContext,
                                             aHandle,
                                             aLogRecordList,
                                             aPrevLastProcessedSN,
                                             aLastProcessedSN );
#else
    return oaOciApplierApplyLogRecordList( aContext,
                                            aHandle,
                                            aLogRecordList,
                                            aPrevLastProcessedSN,
                                            aLastProcessedSN );
#endif
}

/**
 * @breif  ACK 전송이 필요한지 확인한다.
 *
 * @param  aHandle    Adapter Applier Handle
 * @param  aLogRecord Log Record
 *
 * @return ACK 전송 필요 여부
 */
acp_bool_t oaApplierIsAckNeeded( oaApplierHandle    * aHandle,
                                 oaLogRecord        * aLogRecord )
{
    acp_bool_t sAckNeededFlag = ACP_TRUE;

    if ( aHandle->mIsGroupCommit == ACP_TRUE )
    {
        switch ( aLogRecord->mCommon.mType )
        {
            case OA_LOG_RECORD_TYPE_KEEP_ALIVE :
            case OA_LOG_RECORD_TYPE_STOP_REPLICATION :
                break;

            default :
                sAckNeededFlag = ACP_FALSE;
                break;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return sAckNeededFlag;
}
