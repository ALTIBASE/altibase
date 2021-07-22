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

#include <cm.h>
#include <smiFixedTable.h>
#include <rpi.h>
#include <qci.h>
#include <sti.h>
#include <mmm.h>
#include <dki.h>
#include <sdi.h>

/* =======================================================
 * Regist Fixed Table Descriptor
 *  아래의 Section에 모든 Fixed Table의 등록 매크로를
 *  사용하도록 합니다.
 * =====================================================*/
IDU_FIXED_TABLE_DEFINE(gPropertyTable);
IDU_FIXED_TABLE_DEFINE(gTableListTable);
IDU_FIXED_TABLE_DEFINE(gTbsHeaderTableDesc);
IDU_FIXED_TABLE_DEFINE(gColumnListTable);
IDU_FIXED_TABLE_DEFINE(gTableSpaceTableDesc);
IDU_FIXED_TABLE_DEFINE(gDataFileTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemBaseTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemBaseMgrTableDesc);
// BUG-24049 aexport 에서 volatile tablespace 를 처리하지 못합니다.
IDU_FIXED_TABLE_DEFINE(gVolTablespaceDescTableDesc);
IDU_FIXED_TABLE_DEFINE(gTxMgrTableDesc);
IDU_FIXED_TABLE_DEFINE(gTxListTableDesc);
IDU_FIXED_TABLE_DEFINE(gTxPendingTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemTBSFreePageListTableDesc);
IDU_FIXED_TABLE_DEFINE(gVolTBSFreePageListTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemTablespaceDescTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemTablespaceCheckpointPathsTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemTablespaceStatusDescTableDesc);
IDU_FIXED_TABLE_DEFINE(gSESSIONTableDesc);
IDU_FIXED_TABLE_DEFINE(gSTATEMENTTableDesc);
IDU_FIXED_TABLE_DEFINE(gSQLTEXTTableDesc);
IDU_FIXED_TABLE_DEFINE(gPLANTEXTTableDesc);
//fix BUG-22669  XID list performance view need.
IDU_FIXED_TABLE_DEFINE(gXidTableDesc);
IDU_FIXED_TABLE_DEFINE(gPROCTEXTTableDesc);
IDU_FIXED_TABLE_DEFINE(gPkgTEXTTableDesc); // PROJ-1073 Package
IDU_FIXED_TABLE_DEFINE(gSQLPLANCACHETableDesc);
IDU_FIXED_TABLE_DEFINE(gSQLPLANCACHESQLTextTableDesc);
IDU_FIXED_TABLE_DEFINE(gSQLPLANCACHESQLPCOTableDesc);
IDU_FIXED_TABLE_DEFINE(gDataTypeTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemoryMgrTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemAllocTableDesc);
IDU_FIXED_TABLE_DEFINE(gSESSIONMGRTableDesc);
IDU_FIXED_TABLE_DEFINE(gUDSEGS);
IDU_FIXED_TABLE_DEFINE(gTSSEGS);
IDU_FIXED_TABLE_DEFINE(gArchiveTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemLogicalAgerTableDesc);
IDU_FIXED_TABLE_DEFINE(gMemDeleteThrTableDesc);
IDU_FIXED_TABLE_DEFINE(gTableInfoTableDesc);
IDU_FIXED_TABLE_DEFINE(gTempTableInfoTableDesc); // PROJ-1407 Temporary Table
IDU_FIXED_TABLE_DEFINE(gMutexMgrTableDesc);
IDU_FIXED_TABLE_DEFINE(gLatchTableDesc);
IDU_FIXED_TABLE_DEFINE(gThreadContainerTableDesc);
IDU_FIXED_TABLE_DEFINE(gCPUCoreTableDesc);
IDU_FIXED_TABLE_DEFINE(gNUMAStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gCondTableDesc);
IDU_FIXED_TABLE_DEFINE(gServiceThreadTableDesc);
/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   BUG-29335A performance-view about service thread should be strengthened for problem tracking
*/
IDU_FIXED_TABLE_DEFINE(gServiceThreadMgrTableDesc);

// PROJ-1568 Buffer Manager Renewal
IDU_FIXED_TABLE_DEFINE(gBufferPoolStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gBufferPageInfoTableDesc);
IDU_FIXED_TABLE_DEFINE(gBufferBCBStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gBufferFrameDumpStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gBufferFlusherStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gBCBListStat);
IDU_FIXED_TABLE_DEFINE(gFlushMgrStatTableDesc);

IDU_FIXED_TABLE_DEFINE(gLockTBLDesc);
IDU_FIXED_TABLE_DEFINE(gLockTBSDesc);
IDU_FIXED_TABLE_DEFINE(gLockModeTableDesc);
IDU_FIXED_TABLE_DEFINE(gVersionTableDesc);
IDU_FIXED_TABLE_DEFINE(gSeqDesc);
IDU_FIXED_TABLE_DEFINE(gInstanceDesc);
IDU_FIXED_TABLE_DEFINE(gLogAnchorDesc);
IDU_FIXED_TABLE_DEFINE(gLFGTableDesc);
IDU_FIXED_TABLE_DEFINE(gStableMemDataFileTableDesc);

// TASK-2356
IDU_FIXED_TABLE_DEFINE(gSESSTATTableDesc);
IDU_FIXED_TABLE_DEFINE(gSYSSTATTableDesc);
IDU_FIXED_TABLE_DEFINE(gStatNameTableDesc);
IDU_FIXED_TABLE_DEFINE(gWaitClassNameTableDesc);
IDU_FIXED_TABLE_DEFINE(gWaitEventNameTableDesc);
IDU_FIXED_TABLE_DEFINE(gSessionEventTableDesc);
IDU_FIXED_TABLE_DEFINE(gSystemEventTableDesc);
IDU_FIXED_TABLE_DEFINE(gSysConflictPageTableDesc);
IDU_FIXED_TABLE_DEFINE(gFileStatTableDesc);

IDU_FIXED_TABLE_DEFINE(gIndexDesc);
IDU_FIXED_TABLE_DEFINE(gSegmentDesc);
IDU_FIXED_TABLE_DEFINE(gLockWaitTableDesc);
IDU_FIXED_TABLE_DEFINE(gTracelogTable);
IDU_FIXED_TABLE_DEFINE(gCatalogDesc);
IDU_FIXED_TABLE_DEFINE(gTempCatalogDesc); // PROJ-1407 Temporary Table
IDU_FIXED_TABLE_DEFINE(gLockRecordDesc);
IDU_FIXED_TABLE_DEFINE(gDistLockWaitDesc);

// PROJ-1618 On-line Dump
IDU_FIXED_TABLE_DEFINE( gDumpDiskBTreeStructureTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskBTreeKeyTableDesc );

// PROJ-1591 Disk Spatial Index
IDU_FIXED_TABLE_DEFINE( gDumpDiskRTreeStructureTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskRTreeKeyTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskIndexRTreeCTSlotTableDesc );

// PROJ-1704 MVCC Renewal
IDU_FIXED_TABLE_DEFINE( gDumpDiskIndexBTreeCTSlotTableDesc );

// PROJ-1671 Bitmap Tablespace And Segment Space Management
IDU_FIXED_TABLE_DEFINE( gDumpDiskTBSFreeEXTListTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableEXTListTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTablePIDListTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableLobAgingListTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableLobMetaTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsSegHdrTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsSegCacheTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsBMPStructureTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsRtBMPHdrTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsRtBMPBodyTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsItBMPHdrTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsItBMPBodyTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsLfBMPHdrTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsLfBMPRangeSlotTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableTmsLfBMPPBSTblTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableFmsPvtFreeLstTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableFmsUFmtFreeLstTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableFmsFreeLstTblDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableFmsSegHdrTblDesc );

IDU_FIXED_TABLE_DEFINE( gTmsTableCacheDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskCmsSegHdrTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskCmsExtDirHdrTableDesc );

// PROJ-1617 INDEX PERFORMANCE ANALYSIS
IDU_FIXED_TABLE_DEFINE( gDumpMemBTreeStructureTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpMemBTreeKeyTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpVolBTreeStructureTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpVolBTreeKeyTableDesc );
IDU_FIXED_TABLE_DEFINE( gMemBTreeHeaderDesc );
IDU_FIXED_TABLE_DEFINE( gMemBTreeStatDesc );
IDU_FIXED_TABLE_DEFINE( gVolBTreeHeaderDesc );  // PROJ-1407 Temporary Table
IDU_FIXED_TABLE_DEFINE( gVolBTreeStatDesc );    // PROJ-1407 Temporary Table
IDU_FIXED_TABLE_DEFINE( gDiskBTreeHeaderDesc );
IDU_FIXED_TABLE_DEFINE( gDiskBTreeStatDesc );
IDU_FIXED_TABLE_DEFINE( gDiskRTreeHeaderDesc ); // PROJ-1591 Disk Spatial Index
IDU_FIXED_TABLE_DEFINE( gDiskRTreeStatDesc );   // PROJ-1591 Disk Spatial Index
IDU_FIXED_TABLE_DEFINE( gMemBTreeNodePoolDesc );
IDU_FIXED_TABLE_DEFINE( gTempBTreeHeaderDesc ); // PROJ-1407 Temporary Table
IDU_FIXED_TABLE_DEFINE( gTempBTreeStatDesc );   // PROJ-1407 Temporary Table

IDU_FIXED_TABLE_DEFINE( gDBProtocolDesc );

// To fix BUG-20527
IDU_FIXED_TABLE_DEFINE( gMemLobStatisticsTableDesc );
IDU_FIXED_TABLE_DEFINE( gDiskLobStatisticsTableDesc );

// To fix BUG-20805
IDU_FIXED_TABLE_DEFINE( gDumpMemTableRecordTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableRecordTableDesc );
// PROJ-1407 Temporary Table
IDU_FIXED_TABLE_DEFINE( gDumpVolTableRecordTableDesc );

// PROJ-1917
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableCTSlotTableDesc );
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableCTLTableDesc );
IDU_FIXED_TABLE_DEFINE( gDiskTSSRecordsTableDesc );
IDU_FIXED_TABLE_DEFINE( gDiskUndoRecordsTableDesc );
IDU_FIXED_TABLE_DEFINE( gActiveTXSEGSTableDesc );

// PROJ-2083 Dual Table
IDU_FIXED_TABLE_DEFINE( gDualTable );

/* PROJ-2208 Multi Currency */
IDU_FIXED_TABLE_DEFINE( gNlsTerritoryTable );

/* PROJ-2209 DBTIMEZONE */
IDU_FIXED_TABLE_DEFINE( gTimezoneNamesTable );

/*  BUG-24767  [SD] slot direcotry dump하는 기능 추가 */
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableSlotDirTableDesc );

/*  PROJ-1862 [SD] row내의 Lob Descriptor를 dump 하는 기능 추가 */
IDU_FIXED_TABLE_DEFINE( gDumpDiskTableLobInfoTableDesc );

/* TASK-4007 [SM] PBT를 위한 기능 추가 */
IDU_FIXED_TABLE_DEFINE(gDumpDiskDBPageTableDesc);
IDU_FIXED_TABLE_DEFINE(gDumpDiskDBPhyPageHdrTableDesc);
IDU_FIXED_TABLE_DEFINE(gDumpMemDBPageTableDesc);
IDU_FIXED_TABLE_DEFINE(gDumpVolDBPageTableDesc);
IDU_FIXED_TABLE_DEFINE(gDumpMemTBSPCHTableDesc);
IDU_FIXED_TABLE_DEFINE(gDumpVolTBSPCHTableDesc);

// PROJ-2068 [SM] Direct-Path INSERT 성능 개선
IDU_FIXED_TABLE_DEFINE( gDPathInsertDesc );

/* Proj-2059 [SM] DB Upgrade  */
IDU_FIXED_TABLE_DEFINE(gDataPort);
IDU_FIXED_TABLE_DEFINE(gDataPortFileHeader);
IDU_FIXED_TABLE_DEFINE(gDataPortFileCursor);

/*
 *PROJ-2065  한계 상황 테스트
 */
IDU_FIXED_TABLE_DEFINE(gDumpMemPoolDesc);

/* PROJ-2162 [SM] RestartRiskReduction */
IDU_FIXED_TABLE_DEFINE(gRecvFailObjDesc);
IDU_FIXED_TABLE_DEFINE(gDumpMemDBPersPageHdrDesc);
IDU_FIXED_TABLE_DEFINE(gDBMSStatsDesc);

/* PROJ-2133 [SM] Incremental backup */
IDU_FIXED_TABLE_DEFINE(gBackupInfoDesc);
IDU_FIXED_TABLE_DEFINE(gObsoleteBackupInfoDesc);

/* PROJ-1381 Fetch Across Commits */
IDU_FIXED_TABLE_DEFINE(gLegacyTxListTableDesc);

/* PROJ-2201 */
IDU_FIXED_TABLE_DEFINE(gTempTableStatsDesc);
IDU_FIXED_TABLE_DEFINE(gTempInfoDesc);

// PROJ-1685
IDU_FIXED_TABLE_DEFINE(gAgentProcTableDesc);

/* PROJ-2101 Fast Secondary Buffer */
IDU_FIXED_TABLE_DEFINE(gSBufferStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gSBufferBCBStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gSBufferFlushMgrStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gSBufferFlusherStatTableDesc);
IDU_FIXED_TABLE_DEFINE(gSBufferFileTableDesc);

/* PROJ-2451 Concurrent Execute Package */
IDU_FIXED_TABLE_DEFINE(gINTERNALSESSIONTableDesc);

/* PROJ-2626 Snapshot Export */
IDU_FIXED_TABLE_DEFINE(gSnapshotExportTableDesc);

/* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
IDU_FIXED_TABLE_DEFINE(gAccessListDesc);

/* TASK-6780 */
IDU_FIXED_TABLE_DEFINE(gRBHashTableDesc);

IDU_FIXED_TABLE_DEFINE(gReservedTableDesc);

IDU_FIXED_TABLE_DEFINE(gLibraryTableDesc);
IDU_FIXED_TABLE_DEFINE(gPROCInfoDesc);

/* PROJ-2733 */
IDU_FIXED_TABLE_DEFINE(gTimeSCNListTableDesc); 
IDU_FIXED_TABLE_DEFINE(gPendingWaitTableDesc);

/* =======================================================
 * Action Function
 * => Setup All Callback Functions
 * =====================================================*/

static IDE_RC mmmPhaseActionFixedTable(mmmPhase         /*aPhase*/,
                                       UInt             /*aOptionflag*/,
                                       mmmPhaseAction * /*aAction*/)
{
    //fix PROJ-1822
    IDE_TEST(qciMisc::initializePerformanceViewManager() != IDE_SUCCESS);

    IDE_TEST( dkiInitializePerformanceView() != IDE_SUCCESS );

    IDE_TEST(rpi::initSystemTables() != IDE_SUCCESS);

    IDE_TEST(sti::initSystemTables() != IDE_SUCCESS);

    /* PROJ-2638 shard native linker */
    IDE_TEST(sdi::initSystemTables() != IDE_SUCCESS);

    IDE_TEST(smiFixedTable::initializeStatic((SChar *)"X$") != IDE_SUCCESS);

    // PROJ-1618
    IDE_TEST(smiFixedTable::initializeStatic((SChar *)"D$") != IDE_SUCCESS);

    IDE_TEST(qciMisc::buildPerformanceView(NULL) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmmBuildRecordForVersion(idvSQL              * /*aStatistics*/,
                                void                *aHeader,
                                void                * /* aDumpObj */,
                                iduFixedTableMemory *aMemory)
{
    mmmVersionInfo sVersionInfo;

    sVersionInfo.mProdVersion = iduVersionString;
    idlOS::snprintf(sVersionInfo.mPkgPlatFormInfo, MMM_VERSION_STRLEN,
                    "%s", iduGetSystemInfoString() );

    idlOS::snprintf(sVersionInfo.mProductTime, MMM_VERSION_STRLEN,
                    "%s", iduGetProductionTimeString() );

    /* BUG-43026 v$version */
    sVersionInfo.mSmVersion = smVersionString;
    idlOS::snprintf(sVersionInfo.mMetaVersion, MMM_VERSION_STRLEN,
                    "%"ID_UINT32_FMT".%"ID_UINT32_FMT".%"ID_UINT32_FMT"  ",
                    QCM_META_MAJOR_VER,
                    QCM_META_MINOR_VER,
                    QCM_META_PATCH_VER);
    idlOS::snprintf(sVersionInfo.mProtocolVersion, MMM_VERSION_STRLEN,
                    "%"ID_INT32_FMT".%"ID_INT32_FMT".%"ID_INT32_FMT"",
                    CM_MAJOR_VERSION,
                    CM_MINOR_VERSION,
                    CM_PATCH_VERSION);

    idlOS::snprintf(sVersionInfo.mReplProtocolVersion, MMM_VERSION_STRLEN,
                    "%"ID_UINT32_FMT".%"ID_UINT32_FMT".%"ID_UINT32_FMT"",
                    REPLICATION_MAJOR_VERSION, // BUGBUG
                    REPLICATION_MINOR_VERSION,
                    REPLICATION_FIX_VERSION);

    // [2] Alloc Buffer
    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *)&sVersionInfo)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static iduFixedTableColDesc gVersionTableColDesc[]=
{

    {
        (SChar*)"PRODUCT_VERSION",
        offsetof(mmmVersionInfo,mProdVersion),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PKG_BUILD_PLATFORM_INFO",
        offsetof(mmmVersionInfo,mPkgPlatFormInfo),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PRODUCT_TIME",
        offsetof(mmmVersionInfo,mProductTime),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"SM_VERSION",
        offsetof(mmmVersionInfo,mSmVersion),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"META_VERSION",
        offsetof(mmmVersionInfo,mMetaVersion),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR  ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"PROTOCOL_VERSION",
        offsetof(mmmVersionInfo,mProtocolVersion),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR  ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"REPL_PROTOCOL_VERSION",
        offsetof(mmmVersionInfo,mReplProtocolVersion),
        MMM_VERSION_STRLEN,
        IDU_FT_TYPE_VARCHAR  ,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gVersionTableDesc =
{
    (SChar*)"X$VERSION",
    mmmBuildRecordForVersion,
    gVersionTableColDesc,
    IDU_STARTUP_PROCESS,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActFixedTable =
{
    (SChar *)"Initialize Performance View",
    MMM_ACTION_NO_LOG,
    mmmPhaseActionFixedTable
};
