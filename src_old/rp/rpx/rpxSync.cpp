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

#include <smi.h>
#include <smErrorCode.h>

#include <qci.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpxSender.h>
#include <rpxSync.h>

IDE_RC rpxSync::syncTable( smiStatement  *aStatement,
                           rpnMessenger  *aMessenger,
                           rpdMetaItem   *aMetaItem,
                           idBool        *aExitFlag,
                           UInt           aChildCount,
                           UInt           aChildNumber,
                           ULong         *aSyncedTuples,
                           idBool         aIsALA )
{
    void               * sRow                = NULL;
    SChar              * sRealRow            = NULL;
    scGRID               sRid;
    void               * sTable              = NULL;
    const void         * sIndex;
    idBool               sOpen               = ID_FALSE;
    smiTableCursor       sCursor;
    smiCursorProperties  sProperty;
    UInt                 sRowSize            = 0;
    UInt                 sColCount;
    mtcColumn          * sMtcCol             = NULL;
    idBool               sNeedXLogTransEnd   = ID_FALSE;    // BUG-17764

    // PROJ-1583 large geometry
    UInt                 i                   = 0;
    UInt                 sVariableRecordSize = 0;
    UInt                 sVarColumnOffset    = 0;
    mtcColumn          * sColumn             = NULL;
    SChar              * sColumnBuffer       = NULL;

    ULong                sNumSendXLog        = 0;
    ULong                sTotalNumSendXLog   = 0;

    smiFetchColumnList * sFetchColumnList    = NULL;
    idBool               sIsExistLobColumn   = ID_FALSE;

    SChar                sLog[512]           = { 0, };

    sTable = (void *)smiGetTable( (smOID)aMetaItem->mItem.mTableOID );

    sIndex = smiGetTablePrimaryKeyIndex( sTable );

    SMI_CURSOR_PROP_INIT(&sProperty, NULL, sIndex);
    
    /* BUG-43394 For Parallel Scan */
    if ( aChildCount > 1 ) 
    {
        /* Parallel Scan */
        sProperty.mIndexTypeID = SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID;

        sProperty.mParallelReadProperties.mThreadCnt = aChildCount;
         /* ThreadID Start 1 To mThreadCnt */
        sProperty.mParallelReadProperties.mThreadID = aChildNumber + 1;

        sIndex = NULL;
    }
    else
    {
        if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
           == SMI_TABLE_DISK )
        {
            sProperty.mIndexTypeID = SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID;

            sIndex = NULL;
        }
        else
        {
            sProperty.mFirstReadRecordPos   = 0;
            sProperty.mReadRecordCount      = ID_ULONG_MAX;
        }
    }

    if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
       == SMI_TABLE_DISK )
    {
        /* PROJ-1705 Fetch Column List를 위한 메모리 할당 */
        IDU_FIT_POINT_RAISE( "rpxSync::syncTable::malloc::FetchColumnList",
                              ERR_MEMORY_ALLOC_FETCH_COLUMN_LIST );
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPX_SYNC,
                                         ID_SIZEOF(smiFetchColumnList) * aMetaItem->mColCount,
                                         (void **)&sFetchColumnList,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_MEMORY_ALLOC_FETCH_COLUMN_LIST);

        // PROJ-1705 Fetch Column List 구성
        IDE_TEST(rpxSender::makeFetchColumnList((smOID)aMetaItem->mItem.mTableOID,
                                                sFetchColumnList)
                 != IDE_SUCCESS);
    }
    else /* Memory Table */
    {
        /* nothing to do */
    }

    // smiCursorProperties에 fetch Column List 정보 설정
    sProperty.mFetchColumnList = sFetchColumnList;

    sCursor.initialize();

    IDE_TEST_RAISE( sCursor.open
                    ( aStatement,
                      sTable,
                      sIndex, 
                      smiGetRowSCN(sTable),
                      NULL,
                      smiGetDefaultKeyRange(),
                      smiGetDefaultKeyRange(),
                      smiGetDefaultFilter(),
                      SMI_LOCK_READ|SMI_TRAVERSE_FORWARD|SMI_PREVIOUS_DISABLE,
                      SMI_SELECT_CURSOR,
                      &sProperty ) != IDE_SUCCESS, ERR_CURSOR );

    sOpen = ID_TRUE;

    IDE_TEST_RAISE(sCursor.beforeFirst( ) != IDE_SUCCESS, ERR_CURSOR);

    if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
       == SMI_TABLE_DISK)
    {
        /* 메모리 할당을 위해서, 현재 읽어야 할 Row의 size를 먼저 얻어와야 함 */
        IDE_TEST(qciMisc::getDiskRowSize(sTable,
                                         &sRowSize)
                 != IDE_SUCCESS);

        /* Row를 저장할 Memory를 할당해야 함 */
        IDU_FIT_POINT_RAISE( "rpxSync::syncTable::calloc::RealRow",
                              ERR_MEMORY_ALLOC_REAL_ROW );
        IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPX_SYNC,
                                         1,
                                         sRowSize,
                                         (void **)&sRealRow,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_MEMORY_ALLOC_REAL_ROW);

        sRow = (void *)sRealRow;
    }

    sColCount = smiGetTableColumnCount(sTable);
    /* mtcColumn을 위한 Memory를 할당해야 함 */
    IDU_FIT_POINT_RAISE( "rpxSync::syncTable::calloc::MtcColumn",
                          ERR_MEMORY_ALLOC_MTC_COL );
    IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPX_SYNC,
                                     sColCount,
                                     ID_SIZEOF(mtcColumn),
                                     (void **)&sMtcCol,
                                     IDU_MEM_IMMEDIATE)
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_MTC_COL);

    /* mtcColumn 정보의 copy본을 생성 */
    IDE_TEST(qciMisc::copyMtcColumns(sTable,
                                     sMtcCol)
             != IDE_SUCCESS);

    if((SMI_MISC_TABLE_HEADER(sTable)->mFlag & SMI_TABLE_TYPE_MASK)
       == SMI_TABLE_MEMORY)
    {
        //PROJ-1583 large geometry
        for (i = 0; i < sColCount; i++)
        {
            sColumn = sMtcCol + i;
            // To fix BUG-24356
            // geometry에 대해서만 buffer할당
            if( ((sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                  == SMI_COLUMN_TYPE_VARIABLE_LARGE) &&
                (sColumn->module->id == MTD_GEOMETRY_ID) )
            {
                sVariableRecordSize = sVariableRecordSize +
                    smiGetVarColumnBufHeadSize(&sColumn->column) +       // PROJ-1705
                    sColumn->column.size;
                sVariableRecordSize = idlOS::align(sVariableRecordSize,
                                                   8);
            }
            else
            {
                // Nothing To Do
            }
        }

        if(sVariableRecordSize > 0)
        {
            IDE_TEST_RAISE(iduMemMgr::calloc(IDU_MEM_RP_RPX_SYNC,
                                             1,
                                             sVariableRecordSize,
                                             (void **)&sColumnBuffer,
                                             IDU_MEM_IMMEDIATE)
                           != IDE_SUCCESS, ERR_MEMORY_ALLOC_COLUMN_BUFFER);

            sVarColumnOffset = 0;

            for(i = 0; i < sColCount; i++)
            {
                sColumn = sMtcCol + i;

                // To fix BUG-24356
                // geometry에 대해서만 value buffer할당
                if ( ((sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                       == SMI_COLUMN_TYPE_VARIABLE_LARGE) &&
                     (sColumn->module->id == MTD_GEOMETRY_ID) )
                {
                    sColumn->column.value =
                        (void *)(sColumnBuffer + sVarColumnOffset);
                    sVarColumnOffset = sVarColumnOffset +
                        smiGetVarColumnBufHeadSize(&sColumn->column) +   // PROJ-1705
                        sColumn->column.size;
                    sVarColumnOffset = idlOS::align(sVarColumnOffset, 8);
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
    }

    for ( i = 0; i < sColCount; i++ )
    {
        sColumn = sMtcCol + i;

        if ( ( sColumn->column.flag & SMI_COLUMN_TYPE_MASK )
             == SMI_COLUMN_TYPE_LOB )
        {
            sIsExistLobColumn = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    while(1)
    {
        IDE_TEST_RAISE(sCursor.readRow((const void **)&sRow, &sRid, SMI_FIND_NEXT)
                       != IDE_SUCCESS, ERR_READ_ROW );

        if(sRow == NULL)
        {
            break;
        }

        IDE_TEST_RAISE(*aExitFlag == ID_TRUE, ERR_EXIT);
        if ( aIsALA == ID_TRUE )
        {
            IDE_TEST_RAISE( aMessenger->sendXLogInsert( RP_TID_NULL,
                                                        (SChar*)sRow,
                                                        sMtcCol,
                                                        sColCount,
                                                        (smOID)aMetaItem->mItem.mTableOID )
                            != IDE_SUCCESS, ERR_MAKE_XLOG_INSERT );
        }
        else
        {
            IDE_TEST_RAISE( aMessenger->syncXLogInsert( RP_TID_NULL,
                                                        (SChar*)sRow,
                                                        sMtcCol,
                                                        sColCount,
                                                        (smOID)aMetaItem->mItem.mTableOID )
                            != IDE_SUCCESS, ERR_MAKE_XLOG_INSERT );

        }
        sNeedXLogTransEnd = ID_TRUE;    // BUG-17764

        if ( sIsExistLobColumn == ID_TRUE )
        {
            IDE_TEST_RAISE( aMessenger->sendXLogLob( RP_TID_NULL,
                                                     (SChar*)sRow,
                                                     sRid,
                                                     sMtcCol,
                                                     (smOID)aMetaItem->mItem.mTableOID,
                                                     &sCursor )
                            != IDE_SUCCESS, ERR_MAKE_XLOG_LOB );
        }

        sNumSendXLog += 1;
        if ( sNumSendXLog == RPU_REPLICATION_SYNC_TUPLE_COUNT )
        {
            IDE_TEST_RAISE( aMessenger->sendSyncXLogTrCommit( RP_TID_NULL )
                            != IDE_SUCCESS, ERR_MAKE_XLOG );

            (void)idCore::acpAtomicAdd64( aSyncedTuples, sNumSendXLog );

            sTotalNumSendXLog += sNumSendXLog;

            sNumSendXLog = 0;

            sNeedXLogTransEnd = ID_FALSE;

            idlOS::snprintf(sLog, 512,
                            "[%"ID_UINT32_FMT"] TABLE: %s, PROCESSED: %"ID_UINT64_FMT", TUPLE: %"ID_UINT64_FMT"",
                            aChildNumber,
                            aMetaItem->mItem.mLocalTablename,
                            sTotalNumSendXLog,
                            *aSyncedTuples );

            ideLog::log( IDE_RP_0, RP_TRC_PJC_NTC_MSG, sLog );
        }
        else
        {
            /* nothing to do */
        } 
    }

    if(sMtcCol != NULL)
    {
        (void)iduMemMgr::free(sMtcCol);
        sMtcCol = NULL;
    }

    if(sRealRow != NULL)
    {
        (void)iduMemMgr::free(sRealRow);
        sRealRow = NULL;
    }

    //PROJ-1586 large geometry
    if(sColumnBuffer != NULL)
    {
        (void)iduMemMgr::free(sColumnBuffer);
        sColumnBuffer = NULL;
    }

    sOpen = ID_FALSE;
    IDE_TEST_RAISE( sCursor.close() != IDE_SUCCESS,
                    ERR_CURSOR );

    if(sFetchColumnList != NULL)
    {
        (void)iduMemMgr::free(sFetchColumnList);
        sFetchColumnList = NULL;
    }

    if(sNeedXLogTransEnd == ID_TRUE)    // BUG-17764
    {
        IDE_TEST_RAISE( aMessenger->sendSyncXLogTrCommit( RP_TID_NULL )
                        != IDE_SUCCESS, ERR_MAKE_XLOG );

        (void)idCore::acpAtomicAdd64( aSyncedTuples, sNumSendXLog );

        sTotalNumSendXLog += sNumSendXLog;

        sNeedXLogTransEnd = ID_FALSE;

        idlOS::snprintf(sLog, 512,
                        "[%"ID_UINT32_FMT"] TABLE: %s, PROCESSED: %"ID_UINT64_FMT", TUPLE: %"ID_UINT64_FMT"",
                        aChildNumber,
                        aMetaItem->mItem.mLocalTablename,
                        sTotalNumSendXLog,
                        *aSyncedTuples );

        ideLog::log( IDE_RP_0, RP_TRC_PJC_NTC_MSG, sLog );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIT);
    {
        IDE_SET(ideSetErrorCode(rpERR_IGNORE_EXIT_FLAG_SET));
    }
    IDE_EXCEPTION(ERR_CURSOR);
    {
        if(sNeedXLogTransEnd == ID_TRUE)    // BUG-17764
        {
            IDE_PUSH();
            (void)aMessenger->sendSyncXLogTrAbort( RP_TID_NULL );
            IDE_POP();
        }
    }
    IDE_EXCEPTION(ERR_MAKE_XLOG_INSERT);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_ADD_INSERT_SYNC_XLOG));

        if(sNeedXLogTransEnd == ID_TRUE)    // BUG-17764
        {
            IDE_PUSH();
            (void)aMessenger->sendSyncXLogTrAbort( RP_TID_NULL );
            IDE_POP();
        }
    }
    IDE_EXCEPTION(ERR_MAKE_XLOG_LOB);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_SEND_FAIL));

        if(sNeedXLogTransEnd == ID_TRUE)    // BUG-17764
        {
            IDE_PUSH();
            (void)aMessenger->sendSyncXLogTrAbort( RP_TID_NULL );
            IDE_POP();
        }
    }
    IDE_EXCEPTION(ERR_MAKE_XLOG);
    {
        if(sNeedXLogTransEnd == ID_TRUE)    // BUG-17764
        {
            IDE_PUSH();
            (void)aMessenger->sendSyncXLogTrAbort( RP_TID_NULL );
            IDE_POP();
        }
    }
    IDE_EXCEPTION(ERR_READ_ROW);
    {
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_FETCH_COLUMN_LIST);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxPJChild::syncTable",
                                "sFetchColumnList"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_REAL_ROW);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxPJChild::syncTable",
                                "sRealRow"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_MTC_COL);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxPJChild::syncTable",
                                "sMtcCol"));
    }
    IDE_EXCEPTION(ERR_MEMORY_ALLOC_COLUMN_BUFFER);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpxPJChild::syncTable",
                                "sColumnBuffer"));
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    IDE_PUSH();

    if(sOpen == ID_TRUE)
    {
        (void)sCursor.close();
    }

    if(sMtcCol != NULL)
    {
        (void)iduMemMgr::free(sMtcCol);
    }

    if(sRealRow != NULL)
    {
        (void)iduMemMgr::free(sRealRow);
    }

    //PROJ-1586 large geometry
    if(sColumnBuffer != NULL)
    {
        (void)iduMemMgr::free(sColumnBuffer);
    }

    if(sFetchColumnList != NULL)
    {
        (void)iduMemMgr::free(sFetchColumnList);
    }

    IDE_POP();

    IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_SYNC_TABLE));
    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

