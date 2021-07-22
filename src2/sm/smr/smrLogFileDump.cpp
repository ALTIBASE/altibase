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
 * $Id: smrLogFileDump.cpp 22903 2007-08-08 01:53:38Z newdaily $
 **********************************************************************/

#include <smErrorCode.h>
#include <smu.h>
#include <smm.h>
#include <sct.h>
#include <smr.h>
#include <smrReq.h>

// BUG-28581 dumplf에서 Log 개수 계산을 위한 sizeof 단위가 잘못되어 있습니다.
// smrLogType는 UChar이기 때문에 UChar MAX만큼 Array를 생성해야 합니다.
SChar smrLogFileDump::mStrLogType[ ID_UCHAR_MAX ][100];

SChar smrLogFileDump::mStrOPType[SMR_OP_MAX+1][100]  = {
    "SMR_OP_NULL",
    "SMR_OP_SMM_PERS_LIST_ALLOC",
    "SMR_OP_SMC_FIXED_SLOT_ALLOC",
    "SMR_OP_CREATE_TABLE",
    "SMR_OP_CREATE_INDEX",
    "SMR_OP_DROP_INDEX",
    "SMR_OP_INIT_INDEX",
    "SMR_OP_SMC_FIXED_SLOT_FREE",
    "SMR_OP_SMC_VAR_SLOT_FREE",
    "SMR_OP_ALTER_TABLE",
    "SMR_OP_SMM_CREATE_TBS",
    "SMR_OP_INSTANT_AGING_AT_ALTER_TABLE",
    "SMR_OP_SMC_TABLEHEADER_ALLOC",
    "SMR_OP_DIRECT_PATH_INSERT",
    "SMR_OP_MAX"
};

SChar smrLogFileDump::mStrLOBOPType[SMR_LOB_OP_MAX+1][100]  = {
    "SMR_MEM_LOB_CURSOR_OPEN",
    "SMR_DISK_LOB_CURSOR_OPEN",
    "SMR_LOB_CURSOR_CLOSE",
    "SMR_PREPARE4WRITE",
    "SMR_FINISH2WRITE",
    "SMR_LOB_ERASE",
    "SMR_LOB_TRIM",
    "SMR_LOB_OP_MAX"
};

SChar smrLogFileDump::mStrDiskOPType[SDR_OP_MAX+1][100] = {
    "SDR_OP_INVALID",
    "SDR_OP_NULL",
    "SDR_OP_SDP_CREATE_TABLE_SEGMENT",
    "SDR_OP_SDP_CREATE_LOB_SEGMENT",
    "SDR_OP_SDP_CREATE_INDEX_SEGMENT",
    "SDR_OP_SDP_LOB_ADD_PAGE_TO_AGINGLIST",
    "SDR_OP_SDP_LOB_APPEND_LEAFNODE",
    "SDR_OP_SDC_ALLOC_UNDO_PAGE",
    "SDR_OP_SDPTB_ALLOCATE_AN_EXTENT_FROM_TBS",
    "SDR_OP_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST",
    "SDR_OP_SDPTB_RESIZE_GG",
    "SDR_OP_SDPST_ALLOC_PAGE",
    "SDR_OP_SDPST_UPDATE_WMINFO_4DPATH",
    "SDR_OP_SDPST_UPDATE_MFNL_4DPATH",
    "SDR_OP_SDPST_UPDATE_BMP_4DPATH",
    "SDR_OP_SDPSF_ALLOC_PAGE",
    "SDR_OP_SDPSF_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH",
    "SDR_OP_SDPSF_MERGE_SEG_4DPATH",
    "SDR_OP_SDPSF_UPDATE_HWMINFO_4DPATH",
    "SDR_OP_SDN_INSERT_KEY_WITH_NTA",
    "SDR_OP_SDN_DELETE_KEY_WITH_NTA",
    "SDR_OP_SDP_DPATH_ADD_SEGINFO",
    "SDR_OP_STNDR_INSERT_KEY_WITH_NTA",
    "SDR_OP_STNDR_DELETE_KEY_WITH_NTA",
    "SDR_OP_MAX"
};

SChar smrLogFileDump::mStrDiskLogType[SDR_LOG_TYPE_MAX+1][128];

SChar smrLogFileDump::mStrUpdateType[SM_MAX_RECFUNCMAP_SIZE][100];

SChar smrLogFileDump::mStrTBSUptType[SCT_UPDATE_MAXMAX_TYPE+1][64] = {
    "SCT_UPDATE_MRDB_CREATE_TBS",
    "SCT_UPDATE_MRDB_CREATE_CIMAGE_FILE",
    "SCT_UPDATE_MRDB_DROP_TBS",
    "SCT_UPDATE_MRDB_ALTER_AUTOEXTEND",
    "SCT_UPDATE_MRDB_ALTER_TBS_ONLINE",
    "SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE",
    "SCT_UPDATE_DRDB_CREATE_TBS",
    "SCT_UPDATE_DRDB_DROP_TBS",
    "SCT_UPDATE_DRDB_ALTER_TBS_ONLINE",
    "SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE",
    "SCT_UPDATE_DRDB_CREATE_DBF",
    "SCT_UPDATE_DRDB_DROP_DBF",
    "SCT_UPDATE_DRDB_EXTEND_DBF",
    "SCT_UPDATE_DRDB_SHRINK_DBF",
    "SCT_UPDATE_DRDB_AUTOEXTEND_DBF",
    "SCT_UPDATE_DRDB_ALTER_DBF_ONLINE",
    "SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE",
    "SCT_UPDATE_VRDB_CREATE_TBS",
    "SCT_UPDATE_VRDB_DROP_TBS",
    "SCT_UPDATE_VRDB_ALTER_AUTOEXTEND",
    "SCT_UPDATE_COMMON_ALTER_ATTR_FLAG",
    "SCT_UPDATE_MAXMAX_TYPE"
};

/*
    Static멤버 초기화
 */
IDE_RC smrLogFileDump::initializeStatic()
{
    idlOS::strcpy( mStrLogType[SMR_LT_NULL],
                   "SMR_LT_NULL" );
    idlOS::strcpy( mStrLogType[SMR_LT_DUMMY],
                   "SMR_LT_DUMMY" );
    idlOS::strcpy( mStrLogType[SMR_LT_CHKPT_BEGIN],
                   "SMR_LT_CHKPT_BEGIN" );
    idlOS::strcpy( mStrLogType[SMR_LT_DIRTY_PAGE],
                   "SMR_LT_DIRTY_PAGE" );
    idlOS::strcpy( mStrLogType[SMR_LT_CHKPT_END],
                   "SMR_LT_CHKPT_END" );

    /* Transaction */
    idlOS::strcpy( mStrLogType[SMR_LT_MEMTRANS_GROUPCOMMIT],
                   "SMR_LT_MEMTRANS_GROUPCOMMIT" );
    idlOS::strcpy( mStrLogType[SMR_LT_MEMTRANS_COMMIT],
                   "SMR_LT_MEMTRANS_COMMIT" );
    idlOS::strcpy( mStrLogType[SMR_LT_MEMTRANS_ABORT],
                   "SMR_LT_MEMTRANS_ABORT" );
    idlOS::strcpy( mStrLogType[SMR_LT_DSKTRANS_COMMIT],
                   "SMR_LT_DSKTRANS_COMMIT" );
    idlOS::strcpy( mStrLogType[SMR_LT_DSKTRANS_ABORT],
                   "SMR_LT_DSKTRANS_ABORT" );
    idlOS::strcpy( mStrLogType[SMR_LT_SAVEPOINT_SET],
                   "SMR_LT_SAVEPOINT_SET" );
    idlOS::strcpy( mStrLogType[SMR_LT_SAVEPOINT_ABORT],
                   "SMR_LT_SAVEPOINT_ABORT" );
    idlOS::strcpy( mStrLogType[SMR_LT_XA_PREPARE],
                   "SMR_LT_XA_PREPARE" );
    idlOS::strcpy( mStrLogType[SMR_LT_TRANS_PREABORT],
                   "SMR_LT_TRANS_PREABORT" );
    idlOS::strcpy( mStrLogType[SMR_LT_XA_SEGS],
                   "SMR_LT_XA_SEGS" );
    idlOS::strcpy( mStrLogType[SMR_LT_LOB_FOR_REPL],
                   "SMR_LT_LOB_FOR_REPL" );

    idlOS::strcpy( mStrLogType[SMR_LT_DDL_QUERY_STRING],
                  "SMR_LT_DDL_QUERY_STRING" );
    idlOS::strcpy( mStrLogType[SMR_LT_UPDATE],
                  "SMR_LT_UPDATE" );

    idlOS::strcpy( mStrLogType[SMR_LT_NTA],
                   "SMR_LT_NTA" );
    idlOS::strcpy( mStrLogType[SMR_LT_COMPENSATION],
                   "SMR_LT_COMPENSATION" );
    idlOS::strcpy( mStrLogType[SMR_LT_DUMMY_COMPENSATION],
                   "SMR_LT_DUMMY_COMPENSATION" );
    idlOS::strcpy( mStrLogType[SMR_LT_FILE_BEGIN],
                   "SMR_LT_FILE_BEGIN" );
    idlOS::strcpy( mStrLogType[SMR_LT_TBS_UPDATE],
                   "SMR_LT_TBS_UPDATE" );
    idlOS::strcpy( mStrLogType[SMR_LT_FILE_END],
                   "SMR_LT_FILE_END" );

    // DDL Transaction임을 표시하는 Log Record
    idlOS::strcpy( mStrLogType[SMR_LT_DDL],
                   "SMR_LT_DDL" );

    /* DRDB 로그 타입 */
    idlOS::strcpy( mStrLogType[SMR_DLT_REDOONLY],
                   "SMR_DLT_REDOONLY" );
    idlOS::strcpy( mStrLogType[SMR_DLT_UNDOABLE],
                   "SMR_DLT_UNDOABLE" );
    idlOS::strcpy( mStrLogType[SMR_DLT_NTA],
                   "SMR_DLT_NTA" );
    idlOS::strcpy( mStrLogType[SMR_DLT_REF_NTA],
                   "SMR_DLT_REF_NTA" );
    idlOS::strcpy( mStrLogType[SMR_DLT_COMPENSATION],
                   "SMR_DLT_COMPENSATION" );
    idlOS::strcpy( mStrLogType[SMR_LT_XA_PREPARE_REQ],
                   "SMR_LT_XA_PREPARE_REQ" );
    idlOS::strcpy( mStrLogType[SMR_LT_XA_END],
                   "SMR_LT_XA_END" );
    idlOS::strcpy( mStrLogType[SMR_LT_XA_START_REQ],
                   "SMR_LT_XA_START_REQ" );

    /* Init Log Type Table */
    idlOS::strcpy( mStrUpdateType[SMR_PHYSICAL],
                   "SMR_PHYSICAL" );
    /* SMM */
    idlOS::strcpy( mStrUpdateType[SMR_SMM_MEMBASE_INFO],
                   "SMR_SMM_MEMBASE_INFO" );
    idlOS::strcpy( mStrUpdateType[SMR_SMM_MEMBASE_SET_SYSTEM_SCN],
                   "SMR_SMM_MEMBASE_SET_SYSTEM_SCN" );
    idlOS::strcpy( mStrUpdateType[SMR_SMM_MEMBASE_ALLOC_PERS_LIST],
                   "SMR_SMM_MEMBASE_ALLOC_PERS_LIST" );
    idlOS::strcpy( mStrUpdateType[SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK],
                   "SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK" );
    idlOS::strcpy( mStrUpdateType[SMR_SMM_PERS_UPDATE_LINK],
                   "SMR_SMM_PERS_UPDATE_LINK" );
    idlOS::strcpy( mStrUpdateType[SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK],
                   "SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK" );

    /* Table Header */
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_INIT],
                   "SMR_SMC_TABLEHEADER_INIT" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_INDEX],
                   "SMR_SMC_TABLEHEADER_UPDATE_INDEX" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_COLUMNS],
                   "SMR_SMC_TABLEHEADER_UPDATE_COLUMNS" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_INFO],
                   "SMR_SMC_TABLEHEADER_UPDATE_INFO" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_SET_NULLROW],
                   "SMR_SMC_TABLEHEADER_SET_NULLROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_ALL],
                   "SMR_SMC_TABLEHEADER_UPDATE_ALL" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO],
                   "SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_FLAG],
                   "SMR_SMC_TABLEHEADER_UPDATE_FLAG" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_SET_SEQUENCE],
                   "SMR_SMC_TABLEHEADER_SET_SEQUENCE" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT],
                   "SMR_SMC_TABLEHEADER_UPDATE_TABLE_COLUMN_COUNT" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT],
                   "SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_SET_SEGSTOATTR],
                   "SMR_SMC_TABLEHEADER_SET_SEGSTOATTR" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_SET_INSERTLIMIT],
                   "SMR_SMC_TABLEHEADER_SET_INSERTLIMIT" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_TABLEHEADER_SET_INCONSISTENCY],
                   "SMR_SMC_TABLEHEADER_SET_INCONSISTENCY" );

    /* Index Header */
    idlOS::strcpy( mStrUpdateType[SMR_SMC_INDEX_SET_FLAG],
                   "SMR_SMC_INDEX_SET_FLAG" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_INDEX_SET_SEGATTR],
                   "SMR_SMC_INDEX_SET_SEGATTR" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_INDEX_SET_SEGSTOATTR],
                   "SMR_SMC_INDEX_SET_SEGSTOATTR" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_INDEX_SET_DROP_FLAG],
                   "SMR_SMC_INDEX_SET_DROP_FLAG" );

    /* Pers Page */
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_INIT_FIXED_PAGE],
                   "SMR_SMC_PERS_INIT_FIXED_PAGE" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_INIT_FIXED_ROW],
                   "SMR_SMC_PERS_INIT_FIXED_ROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_UPDATE_FIXED_ROW],
                   "SMR_SMC_PERS_UPDATE_FIXED_ROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION],
                   "SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG],
                   "SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT],
                   "SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_INIT_VAR_PAGE],
                   "SMR_SMC_PERS_INIT_VAR_PAGE" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD],
                   "SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_UPDATE_VAR_ROW],
                   "SMR_SMC_PERS_UPDATE_VAR_ROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_SET_VAR_ROW_FLAG],
                   "SMR_SMC_PERS_SET_VAR_ROW_FLAG" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_SET_VAR_ROW_NXT_OID],
                   "SMR_SMC_PERS_SET_VAR_ROW_NXT_OID" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_WRITE_LOB_PIECE],
                   "SMR_SMC_PERS_WRITE_LOB_PIECE" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_SET_INCONSISTENCY],
                   "SMR_SMC_PERS_SET_INCONSISTENCY" );

    /* Replication */
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_INSERT_ROW],
                   "SMR_SMC_PERS_INSERT_ROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_UPDATE_INPLACE_ROW],
                   "SMR_SMC_PERS_UPDATE_INPLACE_ROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_UPDATE_VERSION_ROW],
                   "SMR_SMC_PERS_UPDATE_VERSION_ROW" );
    idlOS::strcpy( mStrUpdateType[SMR_SMC_PERS_DELETE_VERSION_ROW],
                   "SMR_SMC_PERS_DELETE_VERSION_ROW" );

    /* Meta */
    // Table Meta Log Record
    idlOS::strcpy( mStrLogType[SMR_LT_TABLE_META],
                   "SMR_LT_TABLE_META" );

    /* Disk Log Type */
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_1BYTE],
                   "SDR_SDP_1BYTE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_2BYTE],
                   "SDR_SDP_2BYTE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_4BYTE],
                   "SDR_SDP_4BYTE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_8BYTE],
                   "SDR_SDP_8BYTE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_BINARY],
                   "SDR_SDP_BINARY" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDP_PAGE_CONSISTENT],
                   "SDR_SDP_PAGE_CONSISTENT" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INSERT_ROW_PIECE],
                   "SDR_SDC_INSERT_ROW_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE],
                   "SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO],
                   "SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_UPDATE_ROW_PIECE],
                   "SDR_SDC_UPDATE_ROW_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_OVERWRITE_ROW_PIECE],
                   "SDR_SDC_OVERWRITE_ROW_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_CHANGE_ROW_PIECE_LINK],
                   "SDR_SDC_CHANGE_ROW_PIECE_LINK" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_DELETE_FIRST_COLUMN_PIECE],
                   "SDR_SDC_DELETE_FIRST_COLUMN_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_ADD_FIRST_COLUMN_PIECE],
                   "SDR_SDC_ADD_FIRST_COLUMN_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE],
                   "SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_DELETE_ROW_PIECE],
                   "SDR_SDC_DELETE_ROW_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOCK_ROW],
                   "SDR_SDC_LOCK_ROW" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_PK_LOG],
                   "SDR_SDC_PK_LOG" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDP_INIT_PHYSICAL_PAGE],
                   "SDR_SDP_INIT_PHYSICAL_PAGE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_INIT_LOGICAL_HDR],
                   "SDR_SDP_INIT_LOGICAL_HDR" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_INIT_SLOT_DIRECTORY],
                   "SDR_SDP_INIT_SLOT_DIRECTORY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_FREE_SLOT],
                   "SDR_SDP_FREE_SLOT" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_FREE_SLOT_FOR_SID],
                   "SDR_SDP_FREE_SLOT_FOR_SID" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_RESTORE_FREESPACE_CREDIT],
                   "SDR_SDP_RESTORE_FREESPACE_CREDIT" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_RESET_PAGE],
                   "SDR_SDP_RESET_PAGE" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_INIT_SEGHDR],
                   "SDR_SDPST_INIT_SEGHDR" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_INIT_BMP],
                   "SDR_SDPST_INIT_BMP" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_INIT_LFBMP],
                   "SDR_SDPST_INIT_LFBMP" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_INIT_EXTDIR],
                   "SDR_SDPST_INIT_EXTDIR" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_ADD_RANGESLOT],
                   "SDR_SDPST_ADD_RANGESLOT" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_ADD_SLOTS],
                   "SDR_SDPST_ADD_SLOTS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_ADD_EXTDESC],
                   "SDR_SDPST_ADD_EXTDESC" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_ADD_EXT_TO_SEGHDR],
                   "SDR_SDPST_ADD_EXT_TO_SEGHDR" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_UPDATE_WM],
                   "SDR_SDPST_UPDATE_WM" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_UPDATE_MFNL],
                   "SDR_SDPST_UPDATE_MFNL" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_UPDATE_PBS],
                   "SDR_SDPST_UPDATE_PBS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPST_UPDATE_LFBMP_4DPATH],
                   "SDR_SDPST_UPDATE_LFBMP_4DPATH" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDPSC_INIT_SEGHDR],
                   "SDR_SDPSC_INIT_SEGHDR" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPSC_INIT_EXTDIR],
                   "SDR_SDPSC_INIT_EXTDIR" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR],
                   "SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDPTB_INIT_LGHDR_PAGE],
                   "SDR_SDPTB_INIT_LGHDR_PAGE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPTB_ALLOC_IN_LG],
                   "SDR_SDPTB_ALLOC_IN_LG" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDPTB_FREE_IN_LG],
                   "SDR_SDPTB_FREE_IN_LG" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INIT_CTL],
                   "SDR_SDC_INIT_CTL" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_EXTEND_CTL],
                   "SDR_SDC_EXTEND_CTL" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_BIND_CTS],
                   "SDR_SDC_BIND_CTS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_UNBIND_CTS],
                   "SDR_SDC_UNBIND_CTS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_BIND_ROW],
                   "SDR_SDC_BIND_ROW" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_UNBIND_ROW],
                   "SDR_SDC_UNBIND_ROW" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_ROW_TIMESTAMPING],
                   "SDR_SDC_ROW_TIMESTAMPING" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_DATA_SELFAGING],
                   "SDR_SDC_DATA_SELFAGING" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDC_BIND_TSS],
                   "SDR_SDC_BIND_TSS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_UNBIND_TSS],
                   "SDR_SDC_UNBIND_TSS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_SET_INITSCN_TO_TSS],
                   "SDR_SDC_SET_INITSCN_TO_TSS" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INIT_TSS_PAGE],
                   "SDR_SDC_INIT_TSS_PAGE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INIT_UNDO_PAGE],
                   "SDR_SDC_INIT_UNDO_PAGE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_INSERT_UNDO_REC],
                   "SDR_SDC_INSERT_UNDO_REC" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDN_INSERT_INDEX_KEY],
                   "SDR_SDN_INSERT_INDEX_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_FREE_INDEX_KEY],
                   "SDR_SDN_FREE_INDEX_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_INSERT_UNIQUE_KEY],
                   "SDR_SDN_INSERT_UNIQUE_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_INSERT_DUP_KEY],
                   "SDR_SDN_INSERT_DUP_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_DELETE_KEY_WITH_NTA],
                   "SDR_SDN_DELETE_KEY_WITH_NTA" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_FREE_KEYS],
                   "SDR_SDN_FREE_KEYS" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDN_COMPACT_INDEX_PAGE],
                   "SDR_SDN_COMPACT_INDEX_PAGE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_KEY_STAMPING],
                   "SDR_SDN_KEY_STAMPING" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_INIT_CTL],
                   "SDR_SDN_INIT_CTL" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_EXTEND_CTL],
                   "SDR_SDN_EXTEND_CTL" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDN_FREE_CTS],
                   "SDR_SDN_FREE_CTS" );

    /*
     * PROJ-2047 Strengthening LOB
     */

    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_UPDATE_LOBDESC],
                   "SDR_SDC_LOB_UPDATE_LOBDESC" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_INSERT_INTERNAL_KEY],
                   "SDR_SDC_LOB_INSERT_INTERNAL_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_INSERT_LEAF_KEY],
                   "SDR_SDC_LOB_INSERT_LEAF_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_UPDATE_LEAF_KEY],
                   "SDR_SDC_LOB_UPDATE_LEAF_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_OVERWRITE_LEAF_KEY],
                   "SDR_SDC_LOB_OVERWRITE_LEAF_KEY" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_FREE_INTERNAL_KEY],
                   "SDR_SDC_LOB_FREE_INTERNAL_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_FREE_LEAF_KEY],
                   "SDR_SDC_LOB_FREE_LEAF_KEY" );
    
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_WRITE_PIECE],
                   "SDR_SDC_LOB_WRITE_PIECE" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_WRITE_PIECE4DML],
                   "SDR_SDC_LOB_WRITE_PIECE4DML" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_WRITE_PIECE_PREV],
                   "SDR_SDC_LOB_WRITE_PIECE_PREV" );
    idlOS::strcpy( mStrDiskLogType[SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST],
                   "SDR_SDC_LOB_ADD_PAGE_TO_AGING_LIST" );

    /*
     * PROJ-1591: Disk Spatial Index
     */
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_INSERT_INDEX_KEY],
                   "SDR_STNDR_INSERT_INDEX_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_UPDATE_INDEX_KEY],
                   "SDR_STNDR_UPDATE_INDEX_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_FREE_INDEX_KEY],
                   "SDR_STNDR_FREE_INDEX_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_INSERT_KEY],
                   "SDR_STNDR_INSERT_KEY" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_DELETE_KEY_WITH_NTA],
                   "SDR_STNDR_DELETE_KEY_WITH_NTA" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_FREE_KEYS],
                   "SDR_STNDR_FREE_KEYS" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_COMPACT_INDEX_PAGE],
                   "SDR_STNDR_COMPACT_INDEX_PAGE" );
    idlOS::strcpy( mStrDiskLogType[SDR_STNDR_KEY_STAMPING],
                   "SDR_STNDR_KEY_STAMPING" );

    
    idlOS::strcpy( mStrDiskLogType[SDR_SDP_WRITE_PAGEIMG],
                   "SDR_SDP_WRITE_PAGEIMG" );

    idlOS::strcpy( mStrDiskLogType[SDR_SDP_WRITE_DPATH_INS_PAGE],
                   "SDR_SDP_WRITE_DPATH_INS_PAGE" );
    
    idlOS::strcpy( mStrDiskLogType[SDR_LOG_TYPE_MAX],
                   "SDR_LOG_TYPE_MAX" );

    /* BUG-31114 mismatch between real log type and display log type 
     *           in dumplf. */
    IDE_DASSERT( idlOS::strncmp( mStrOPType[ SMR_OP_MAX ], 
                                 "SMR_OP_MAX", 
                                 100 ) == 0 );
    IDE_DASSERT( idlOS::strncmp( mStrLOBOPType[ SMR_LOB_OP_MAX ], 
                                 "SMR_LOB_OP_MAX", 
                                 100 ) == 0 );
    IDE_DASSERT( idlOS::strncmp( mStrDiskOPType[ SDR_OP_MAX ], 
                                 "SDR_OP_MAX", 
                                 100 ) == 0 );
    IDE_DASSERT( idlOS::strncmp( mStrTBSUptType[ SCT_UPDATE_MAXMAX_TYPE ], 
                                 "SCT_UPDATE_MAXMAX_TYPE", 
                                 64 ) == 0 );
    
    return IDE_SUCCESS;
}

/*
    Static멤버 파괴
    - initializeStatic과 짝을 맞추기 위해 만들어둠.
    - 현재 아무런일도 하지 않는다.
 */
IDE_RC smrLogFileDump::destroyStatic()
{
    return IDE_SUCCESS;
}

/****************************************************************
 * Description : 
 ***************************************************************/
IDE_RC smrLogFileDump::initFile()
{
    mFileBuffer = NULL;

    return IDE_SUCCESS;
}

/****************************************************************
 * Description : 
 ***************************************************************/
IDE_RC smrLogFileDump::destroyFile()
{
    IDE_TEST( freeBuffer() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************************
 * Description : 
 ***************************************************************/
IDE_RC smrLogFileDump::allocBuffer()
{
    if ( mFileBuffer == NULL )
    {
        mDecompBuffer.initialize( IDU_MEM_SM_SMR );

        IDE_TEST(iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                    mFileSize,
                                    (void **)&mFileBuffer,
                                    IDU_MEM_FORCE ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_ERROR( mFileBuffer != NULL);
    IDE_ERROR( mFileSize != 0 );

    idlOS::memset( mFileBuffer, 0, mFileSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description : 
 ***************************************************************/
IDE_RC smrLogFileDump::freeBuffer()
{
    if ( mFileBuffer != NULL )
    {
        IDE_TEST(iduMemMgr::free( mFileBuffer ) != IDE_SUCCESS );
        mFileBuffer = NULL;

        IDE_TEST( mDecompBuffer.destroy() != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description : 
 ***************************************************************/
IDE_RC smrLogFileDump::openFile( SChar * aStrLogFile )
{
    IDE_TEST( mLogFile.initialize( IDU_MEM_SM_SMR,
                                   1, /* Max Open FD Count */
                                   IDU_FIO_STAT_OFF,
                                   IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( mLogFile.setFileName( aStrLogFile ) != IDE_SUCCESS );
    IDE_TEST( mLogFile.open() != IDE_SUCCESS );

    IDE_TEST_RAISE( mLogFile.getFileSize( &mFileSize ) != IDE_SUCCESS,
                    error_file_fstat );

    IDE_TEST_RAISE( allocBuffer() != IDE_SUCCESS,
                    error_file_allocBuffer );

    IDE_TEST( mLogFile.read( NULL, /* idvSQL* */
                             0,    /* aWhere  */
                             mFileBuffer,
                             mFileSize) != IDE_SUCCESS );

    IDE_TEST( getFileNo( mFileBuffer,
                         & mFileNo ) != IDE_SUCCESS );
    mIsCompressed = ID_FALSE;
    mOffset     = 0;
    mNextOffset = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_file_fstat );
    {
        idlOS::printf( "Unable to get the size of the logfile.\n" );
    }
    IDE_EXCEPTION( error_file_allocBuffer );
    {
        idlOS::printf( "Insufficient memory.\n" );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogFileDump::closeFile()
{
    IDE_TEST( mLogFile.close() != 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogFileDump::dumpLog( idBool      * aEOF )
{
    UInt                   sLogSizeAtDisk = 0;

    IDE_TEST( readLog( & mDecompBuffer,
                       mFileBuffer,
                       mFileNo,
                       mNextOffset,
                       & mLogHead,
                       & mLogPtr,
                       & sLogSizeAtDisk,
                       & mIsCompressed ) != IDE_SUCCESS );

    mOffset      = mNextOffset;
    mNextOffset += sLogSizeAtDisk;

    if ( ( smrLogHeadI::getSize(&mLogHead) == 0 ) ||
         ( smrLogHeadI::getType(&mLogHead) == SMR_LT_FILE_END ) ||
         ( smrLogHeadI::getType(&mLogHead) == SMR_LT_NULL ) ) // BUG-47754
    {
        (*aEOF) = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Log File Header로부터 File Begin로그를 얻어낸다

   [IN] aFileBeginLog - File Begin Log ( 파일의 헤더와 같음 )
   [IN] aFileNo       - 파일의 번호
 */
IDE_RC smrLogFileDump::getFileNo( SChar * aFileBeginLog,
                                  UInt  * aFileNo )
{
    IDE_DASSERT( aFileBeginLog != NULL );
    IDE_DASSERT( aFileNo != NULL );

    // 첫번째 로그는 File Begin Log이며, 압축하지 않은채로 저장됨
    IDE_ASSERT( smrLogComp::isCompressedLog( aFileBeginLog )
                == ID_FALSE );

    *aFileNo = ((smrFileBeginLog*)aFileBeginLog)->mFileNo;

    return IDE_SUCCESS;
}

/* 로그메모리의 특정 Offset에서 로그 레코드를 읽어온다.
   압축된 로그의 경우, 로그 압축해제를 수행한다.

   [IN] aDecompBufferHandle - 압축 해제 버퍼의 핸들
   [IN] aFileBuffer         - 로그 파일을 읽을때 임시적으로 쓰이는 버퍼
   [IN] aFileNo             - 로그 파일의 번호
   [IN] aLogOffset          - 로그를 읽어올 오프셋
   [OUT] aRawLogHead        - 로그의 Head
   [OUT] aRawLogPtr         - 읽어낸 로그 (압축해제된 로그)
   [OUT] aLogSizeAtDisk     - 파일에서 읽어낸 로그 데이터의 양
*/

IDE_RC smrLogFileDump::readLog( iduMemoryHandle    * aDecompBufferHandle,
                                SChar              * aFileBuffer,
                                UInt                 aFileNo,
                                UInt                 aLogOffset,
                                smrLogHead         * aRawLogHead,
                                SChar             ** aRawLogPtr,
                                UInt               * aLogSizeAtDisk,
                                idBool             * aIsCompressed )
{
    IDE_DASSERT( aDecompBufferHandle != NULL );
    IDE_DASSERT( aFileBuffer    != NULL );
    IDE_DASSERT( aRawLogHead    != NULL );
    IDE_DASSERT( aRawLogPtr     != NULL );
    IDE_DASSERT( aLogSizeAtDisk != NULL );

    smMagic     sValidLogMagic ;
    SChar     * sRawOrCompLog ;

    sRawOrCompLog = aFileBuffer + aLogOffset;

    *aIsCompressed = smrLogComp::isCompressedLog( sRawOrCompLog );

    // File No와 Offset으로부터 Magic값 계산
    sValidLogMagic = smrLogFile::makeMagicNumber( aFileNo,
                                                  aLogOffset );

    IDE_TEST( smrLogComp::getRawLog( aDecompBufferHandle,
                                     aLogOffset,
                                     sRawOrCompLog,
                                     sValidLogMagic,
                                     aRawLogHead,
                                     aRawLogPtr,
                                     aLogSizeAtDisk ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smrLogFileDump::dumpPrepareReqBranchTx( SChar * aBranchTxStr, UInt aSize )
{
    UInt           sBranchTxSize = 0;
    UInt           sBranchTxCount = 0;
    SChar        * sBuffer = NULL;
    UChar          sLen = 0;
    SChar          sLinkerType = 0;
    UChar          sCoordinatorType = 0;
    SChar        * sTargetName = NULL;
    SChar        * sNodeName = NULL;
    SChar        * sDataServerIP = NULL;
    UShort         sDataPortNo = 0;
    UShort         sConnectType = 0;
    ID_XID         sXID;
    UChar          sXidLen;
    UInt           i;
    UInt           sShardBranchCounter = 0;
    UChar          sXidString[SMR_XID_DATA_MAX_LEN];

    sBuffer = aBranchTxStr;

    /* 길이 4byte */
    ID_4_BYTE_ASSIGN( &sBranchTxSize, sBuffer );
    sBuffer += 4;

    IDE_TEST_RAISE( sBranchTxSize != aSize, ERR_INVALID_BRANCH_TX_INFO );

    /* 갯수 4byte */
    ID_4_BYTE_ASSIGN( &sBranchTxCount, sBuffer );
    sBuffer += 4;

    idlOS::printf( "BranchTx Count: %"ID_UINT32_FMT"\n", sBranchTxCount );

    for ( i = 0; i < sBranchTxCount; i++ )
    {
        idlOS::printf("[ %"ID_UINT32_FMT" ] ", i );

        /* XID 길이 1byte */
        ID_1_BYTE_ASSIGN( &sXidLen, sBuffer );
        sBuffer += 1;

        /* XID */
        idlOS::memcpy( &sXID, sBuffer, sXidLen );
        sBuffer += sXidLen;
        (void)idaXaConvertXIDToString(NULL, &sXID, sXidString, SMR_XID_DATA_MAX_LEN);
        idlOS::printf( "XID: %s, ", sXidString );

        /* linker type 1byte */
        ID_1_BYTE_ASSIGN( &sLinkerType, sBuffer );
        sBuffer += 1;

        idlOS::printf( "LinkerType: %c, ", sLinkerType );

        if ( sLinkerType == 'D' )
        {
            /* target name string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sTargetName = (SChar*)sBuffer;
            sBuffer += sLen;

            idlOS::printf( "TargetName: %s\n", sTargetName );
        }
        else
        {
            IDE_TEST_RAISE( sLinkerType != 'S', ERR_INVALID_BRANCH_TX_INFO );

            if ( sShardBranchCounter == 0 )
            {
                /* XID 길이 1byte */
                ID_1_BYTE_ASSIGN( &sXidLen, sBuffer );
                sBuffer += 1;

                /* XID */
                idlOS::memcpy( &sXID, sBuffer, sXidLen );
                sBuffer += sXidLen;

                (void)idaXaConvertXIDToString(NULL, &sXID, sXidString, SMR_XID_DATA_MAX_LEN);
                idlOS::printf( "FromXID: %s, ", sXidString );
            }
            ++sShardBranchCounter;

            /* node type 1byte */
            ID_1_BYTE_ASSIGN( &sCoordinatorType, sBuffer );
            sBuffer += 1;
            idlOS::printf( "CoordinatorType: %"ID_UINT32_FMT", ", sCoordinatorType ); /* sdiCoordinatorType */

            /* node name string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sNodeName = (SChar*)sBuffer;
            sBuffer += sLen;

            idlOS::printf( "NodeName: %s, ", sNodeName );

            /* user name string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            //sUserName = (SChar*)sBuffer;
            sBuffer += sLen;

            /* user password string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            //sUserPassword = (SChar*)sBuffer;
            sBuffer += sLen;

            /* ip string */
            ID_1_BYTE_ASSIGN( &sLen, sBuffer );
            sBuffer += 1;
            sDataServerIP = (SChar*)sBuffer;
            sBuffer += sLen;

            idlOS::printf( "IP: %s, ", sDataServerIP );

            /* port no 2byte */
            ID_2_BYTE_ASSIGN( &sDataPortNo, sBuffer );
            sBuffer += 2;

            idlOS::printf( "PORT: %"ID_UINT32_FMT", ", sDataPortNo );

            /* connect type 2byte */
            ID_2_BYTE_ASSIGN( &sConnectType, sBuffer );
            sBuffer += 2;

            idlOS::printf( "ConnnectType: %"ID_UINT32_FMT"\n", sConnectType ); /* sdiInternalNodeConnectType */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_BRANCH_TX_INFO )
    {
        idlOS::printf( " BranchTx Parsing ERROR...\n" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

