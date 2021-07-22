/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id:$
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <iduMemList.h>
#include <iduMemMgr.h>
#include <idp.h>
#include <iduMemPoolMgr.h>
#include <idCore.h>
#include <idtContainer.h>

// Altibase내의 각 모듈들의 통계정보를 저장
iduMemClientInfo iduMemMgr::mClientInfo[IDU_MEM_UPPERLIMIT] =
{
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMA, "MM", "Main_Module_DirectAttach"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMC, "MM", "Main_Module_Channel"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MML_MAIN, "MM", "Main_Module_CDBC_MAIN"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MML_QP, "MM", "Main_Module_CDBC_QP"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MML_ST, "MM", "Main_Module_CDBC_STATE_MEMPOOL"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MML_CD, "MM", "Main_Module_CDBC_CURSORDATA_MEMPOOL"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MML_CDB, "MM", "Main_Module_CDBC_CONDITIONBUF_MEMPOOL"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMD, "MM", "Main_Module_Distributed"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMT, "MM", "Main_Module_Thread"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMQ, "MM", "Main_Module_Queue"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMU, "MM", "Main_Module_Utility"),
    //PROJ-1436 SQL-Plan Cache.
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MMPLAN_CACHE_CONTROL, "MM", "SQL_Plan_Cache_Control"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ST_STD, "ST", "GIS_DataType"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ST_STN, "ST", "GIS_Disk_Index"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ST_STF, "ST", "GIS_Function"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ST_TEMP, "ST", "GIS_TEMP_MEMORY"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QCI, "QP", "Query_Common"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QCM, "QP", "Query_Meta"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMC, "QP", "Query_DML"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMSEQ, "QP", "Query_Sequence"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QSC, "QP", "Query_PSM_Concurrent_Execute"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP, "RP", "Replication_Common"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPC, "RP", "Replication_Control"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPD, "RP", "Replication_Data"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPD_META, "RP", "Replication_Met"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPN, "RP", "Replication_Network"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPR, "RP", "Replication_Recovery"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPS, "RP", "Replication_Storage"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPX, "RP", "Replication_Executor"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPX_SENDER, "RP", "Replication_Sender"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPX_RECEIVER, "RP", "Replication_Receiver"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPX_SYNC, "RP", "Replication_Sync"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RP_RPU, "RP", "Replication_Module_Property"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QSN, "QP", "Query_PSM_Node"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QSX, "QP", "Query_PSM_Execute"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMP, "QP", "Query_Prepare"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMX, "QP", "Query_Execute"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMB, "QP", "Query_Binding"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMT, "QP", "Query_Transaction"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QMBC, "QP", "Query_Conversion"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QXC, "QP", "Query_Execute_Cache"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QRC, "QP", "Query_Result_Cache"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QIX, "QP", "Query_PSM_Internal_Execute"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MT, "MT", "Mathematics"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDB, "SM", "Storage_Disk_Buffer"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDC, "SM", "Storage_Disk_Collection"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDD, "SM", "Storage_Disk_Datafile"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDS, "SM", "Storage_Disk_SecondaryBuffer"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SCT, "SM", "Storage_Tablespace"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SCP, "SM", "Storage_DataPort"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDN, "SM", "Storage_Disk_Index"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDP, "SM", "Storage_Disk_Page"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SDR, "SM", "Storage_Disk_Recovery"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SGM, "SM", "Storage_Global_Memory_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMA, "SM", "Storage_Memory_Ager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMA_LOGICAL_AGER, "SM", "Storage_Memory_Logical_Ager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMC, "SM", "Storage_Memory_Collection"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMI, "SM", "Storage_Memory_Interface"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SML, "SM", "Storage_Memory_Locking"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMM, "SM", "Storage_Memory_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMN, "SM", "Storage_Memory_Index"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_FIXED_TABLE, "SM", "Fixed_Table"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMP, "SM", "Storage_Memory_Page"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMR, "SM", "Storage_Memory_Recovery"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMR_CHKPT_THREAD, "SM", "Storage_Memory_Recovery_Chkpt_Thread"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMR_LFG_THREAD, "SM", "Storage_Memory_Recovery_LFG_Thread"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMR_ARCHIVE_THREAD, "SM", "Storage_Memory_Recovery_Archive_Thread"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMU, "SM", "Storage_Memory_Utility"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SMX, "SM", "Storage_Memory_Transaction"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SVR, "SM", "Volatile_Log_Buffer"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SVM, "SM", "Volatile_Memory_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SVP, "SM", "Volatile_Memory_Page"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TEMP, "SM", "Temp_Memory"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TRANSACTION_TABLE, "SM", "Transaction_Table"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TRANSACTION_LEGACY_TRANS, "SM", "Legacy_Transaction_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TRANSACTION_OID_LIST, "SM", "Transaction_OID_List"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TRANSACTION_PRIVATE_BUFFER, "SM", "Transaction_Private_Buffer"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TRANSACTION_SEGMENT_TABLE, "SM", "Transaction_Segment_Table"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TRANSACTION_DISKPAGE_TOUCHED_LIST, "SM", "Transaction_DiskPage_Touched_List"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TABLE_INFO, "SM", "Transaction_Table_Info"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_INDEX, "SM", "Index_Memory"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_LOG, "SM", "LOG_Memory"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_IN_MEMORY_RECOVERY, "SM", "InMemoryRecovery_Memory"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_SCC, "SM", "CatalogCache_Memory"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID, "ID", "OS_Independent"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_IDU, "ID", "Utility_Module"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_ASYNC_IO_MANAGER, "ID", "Async_IO_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_MUTEX_MANAGER, "ID", "Mutex"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_CLOCK_MANAGER, "ID", "Clock_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_TIMER_MANAGER, "ID", "Timer_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_PROFILE_MANAGER, "ID", "Profile_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_SOCKET_MANAGER, "ID", "Socket_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_EXTPROC, "ID", "External_Procedure"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_EXTPROC_AGENT, "ID", "External_Procedure_Agent"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_AUDIT_MANAGER, "ID", "Audit_Manager"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_ALTIWRAP, "ID", "Altiwrap"),   /* PROJ-2550 PSM Encryption */
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_THREAD_INFO, "ID", "Process_ThreadInfo"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_CMB, "CM", "CM_Buffer"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_CMN, "CM", "CM_NetworkInterface"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_CMM, "CM", "CM_Multiplexing"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_CMT, "CM", "CM_DataType"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_CMI, "CM", "CM_Interface"), /* BUG-41909 */
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_DK, "DK", "Database_Link"),    
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_IDUDM, "ID", "Dynamic Module Loader"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SM_TBS_FREE_EXTENT_POOL, "SM", "Tablespace_Free_Extent_Pool"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_COND_MANAGER, "ID", "Condition_Variable"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_WATCHDOG, "ID", "WATCHDOG"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_LATCH, "ID", "Latch"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_THREAD_STACK, "ID", "Thread_Stack"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RCS, "RC", "Remote_Call_Server"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RCC, "RC", "Remote_Call_Client"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_QCR, "QP", "Query_Common_Remote_Call"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SD_STMT, "SD", "Shard_Statement"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SDI, "SD", "Shard_Interface"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_OTHER, "--", "IDU_MEM_OTHER"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_MAPPED, "--", "MMAP"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RAW, "--", "SYSTEM"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SHM_META, "ID", "Shared Meta"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_RESERVED, "--", "Free Block"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MAX_CLIENT_COUNT, "--", "IDU_MEM_MAX"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_CHUNK, "ID", "Memory_Manager_Chunk"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_SENTINEL, "ID", "Sentinel"),
    IDU_MEM_CLIENTINFO_STRUCT(IDU_MEM_ID_MULTI_ERROR_MANAGER, "ID", "Multi_Error_Manager")

};

ULong iduMemMgr::getTotalMemory()
{
    ULong sTotalAllocSize = 0;
    UInt  i;

    for (i = 0; i < IDU_MAX_CLIENT_COUNT; i++)
    {
        sTotalAllocSize += mClientInfo[i].mAllocSize;
    }

    return sTotalAllocSize;
}

IDE_RC iduMemMgr::logMemoryLeak()
{
#define IDE_FN "iduMemMgr::logStat()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sLeak            = ID_FALSE;
    ULong  sEachAllocSize;
    ULong  sEachAllocCount;
    ULong  sTotalAllocSize  = 0;
    ULong  sTotalAllocCount = 0;
    UInt   i;

    ideLogEntry sLog(IDE_MISC_0);
    sLog.append(ID_TRC_MEMMGR_LEAK_TITLE);

    for (i = 0; i < IDU_MAX_CLIENT_COUNT; i++)
    {
        sEachAllocSize  = mClientInfo[i].mAllocSize;
        sEachAllocCount = mClientInfo[i].mAllocCount;
        if ((sEachAllocSize != 0) || (sEachAllocCount != 0))
        {
            if (sLeak == ID_FALSE)
            {
                sLog.append(ID_TRC_MEMMGR_LEAK_HEAD);
                sLeak = ID_TRUE;
            }

            sLog.appendFormat( ID_TRC_MEMMGR_LEAK_BODY,
                               mClientInfo[i].mName,
                               mClientInfo[i].mAllocSize,
                               mClientInfo[i].mAllocCount );

            sTotalAllocSize  += sEachAllocSize;
            sTotalAllocCount += sEachAllocCount;
        }
    }

    if (sLeak == ID_TRUE)
    {
        sLog.appendFormat(ID_TRC_MEMMGR_LEAK_TAIL,
                          sTotalAllocSize,
                          sTotalAllocCount);
        sLog.write();
    }
    else
    {
        sLog.appendFormat(ID_TRC_MEMMGR_NO_LEAK);
        sLog.write();
    }

    return IDE_SUCCESS;
#undef IDE_FN
}

/*
 * BUG-32464
 * Collect stat update routine into one function
 */
void iduMemMgr::server_statupdate(iduMemoryClientIndex  aIndex,
                                  SLong                 aSize, 
                                  SLong                 aCount)
{
    acp_slong_t         sMaxTotalSize;
    acp_slong_t         sCurTotalSize;
    iduMemClientInfo*   sClientInfo;

    sClientInfo = idtContainer::getMemClientInfo();

    // PROJ-2446 xdb bugbug
    if( sClientInfo != NULL )
    {
        sCurTotalSize = (sClientInfo[aIndex].mAllocSize += aSize);
        sClientInfo[aIndex].mAllocCount += aCount;
 
        //===================================================================
        // To Fix PR-13959
        // 현재까지 사용한 최대 메모리 사용량
        //===================================================================
        sMaxTotalSize = sClientInfo[aIndex].mMaxTotSize;
        if ( sCurTotalSize > sMaxTotalSize )
        {
            sClientInfo[aIndex].mMaxTotSize = sCurTotalSize;
        }
        else
        {
            // Nothing To Do
        }
    }
}

void iduMemMgr::single_statupdate(iduMemoryClientIndex aIndex,
                                  SLong aSize,
                                  SLong aCount)
{
    mClientInfo[aIndex].mAllocSize  += aSize;
    mClientInfo[aIndex].mAllocCount += aCount;

    return;
}

void iduMemMgr::logAllocRequest(iduMemoryClientIndex aIndex, ULong aSize, SLong aTimeOut)
{
#define IDE_FN "iduMemMgr::callbackLogLevel()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    if ((mLogLevel > 0) && (aSize >= mLogLowerSize) && (aSize <= mLogUpperSize))
    {
        ideLog::log(IDE_SERVER_3,ID_TRC_MEMMGR_REQUEST_ACK,
                    mClientInfo[aIndex].mName,
                    aSize,
                    aTimeOut);
        if (mLogLevel > 1)
        {
            ideLog::logCallStack(IDE_SERVER_3);
        }
    }
#undef IDE_FN
}
