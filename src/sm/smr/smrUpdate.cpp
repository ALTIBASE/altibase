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
 * $Id: smrUpdate.cpp 91224 2021-07-14 05:36:12Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smr.h>
#include <smrReq.h>
#include <sct.h>
#include <svmUpdate.h>
#include <sctTBSUpdate.h>

/*
  Log Record Format

  * Normal
  Log Header:Body:Tail
  * Update Log
  Log Header:Before:After:Tail

*/

smrRecFunction gSmrUndoFunction[SM_MAX_RECFUNCMAP_SIZE];
smrRecFunction gSmrRedoFunction[SM_MAX_RECFUNCMAP_SIZE];

// BUG-9640
smrTBSUptRecFunction  gSmrTBSUptRedoFunction[SCT_UPDATE_MAXMAX_TYPE];
smrTBSUptRecFunction  gSmrTBSUptUndoFunction[SCT_UPDATE_MAXMAX_TYPE];


void smrUpdate::initialize()
{
    idlOS::memset(gSmrUndoFunction, 0x00,
                  ID_SIZEOF(smrRecFunction) * SM_MAX_RECFUNCMAP_SIZE);
    idlOS::memset(gSmrRedoFunction, 0x00,
                  ID_SIZEOF(smrRecFunction) * SM_MAX_RECFUNCMAP_SIZE);

    idlOS::memset(gSmrTBSUptRedoFunction, 0x00,
                  ID_SIZEOF(smrTBSUptRecFunction) * SCT_UPDATE_MAXMAX_TYPE);
    idlOS::memset(gSmrTBSUptUndoFunction, 0x00,
                  ID_SIZEOF(smrTBSUptRecFunction) * SCT_UPDATE_MAXMAX_TYPE);

    gSmrUndoFunction[SMR_PHYSICAL]
        = smmUpdate::redo_undo_PHYSICAL;
    gSmrUndoFunction[SMR_SMM_MEMBASE_ALLOC_PERS_LIST]
        = smmUpdate::redo_undo_SMM_MEMBASE_ALLOC_PERS_LIST;
    gSmrUndoFunction[SMR_SMM_PERS_UPDATE_LINK]
        = smLayerCallback::redo_undo_SMM_PERS_UPDATE_LINK;
    gSmrUndoFunction[SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK]
        = smmUpdate::redo_undo_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_INDEX]
        = smLayerCallback::undo_SMC_TABLEHEADER_UPDATE_INDEX;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_COLUMNS]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMNS;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_INFO]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_INFO;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_ALL]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_ALL;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_ALLOCINFO;
    gSmrUndoFunction[SMR_SMC_PERS_INIT_FIXED_ROW]
        = smLayerCallback::undo_SMC_PERS_INIT_FIXED_ROW;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_FLAG]
        = smLayerCallback::undo_SMC_TABLEHEADER_UPDATE_FLAG;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_SET_SEQUENCE]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_SET_SEQUENCE;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT]
        = smLayerCallback::undo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_SET_SEGSTOATTR]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_SET_SEGSTOATTR;
    gSmrUndoFunction[SMR_SMC_TABLEHEADER_SET_INSERTLIMIT]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_SET_INSERTLIMIT;

    // BUG-26695 DPath Insert가 실패하였을 경우 Log가 Undo 되어야 함
    gSmrUndoFunction[ SMR_SMC_TABLEHEADER_SET_INCONSISTENCY ]
        = smLayerCallback::undo_SMC_TABLEHEADER_SET_INCONSISTENCY;

    gSmrUndoFunction[SMR_SMC_INDEX_SET_FLAG]
        = smLayerCallback::undo_SMC_INDEX_SET_FLAG;
    gSmrUndoFunction[SMR_SMC_INDEX_SET_SEGSTOATTR]
        = smLayerCallback::undo_SMC_INDEX_SET_SEGSTOATTR;
    gSmrUndoFunction[SMR_SMC_INDEX_SET_SEGATTR]
        = smLayerCallback::undo_SMC_SET_INDEX_SEGATTR;

    gSmrUndoFunction[SMR_SMC_PERS_UPDATE_FIXED_ROW]
        = smLayerCallback::redo_undo_SMC_PERS_UPDATE_FIXED_ROW;
    gSmrUndoFunction[SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION]
        = smLayerCallback::undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION;
    gSmrUndoFunction[SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG]
        = smLayerCallback::undo_SMC_PERS_SET_FIX_ROW_DROP_FLAG;
    gSmrUndoFunction[SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT]
        = smLayerCallback::undo_SMC_PERS_SET_FIX_ROW_DELETE_BIT;
    gSmrUndoFunction[SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD]
//        = smLayerCallback::redo_undo_SMC_PERS_UPDATE_VAR_ROW_HEAD;
        = NULL;
    gSmrUndoFunction[SMR_SMC_PERS_UPDATE_VAR_ROW]
        = smLayerCallback::redo_undo_SMC_PERS_UPDATE_VAR_ROW;
    gSmrUndoFunction[SMR_SMC_PERS_SET_VAR_ROW_FLAG]
        = smLayerCallback::undo_SMC_PERS_SET_VAR_ROW_FLAG;
    gSmrUndoFunction[SMR_SMC_INDEX_SET_DROP_FLAG]
        = smLayerCallback::redo_undo_SMC_INDEX_SET_DROP_FLAG;
    gSmrUndoFunction[SMR_SMC_PERS_SET_VAR_ROW_NXT_OID]
        = smLayerCallback::redo_undo_SMC_PERS_SET_VAR_ROW_NXT_OID;
    gSmrUndoFunction[SMR_SMC_PERS_WRITE_LOB_PIECE]
        = NULL;
    gSmrUndoFunction[SMR_SMC_PERS_SET_INCONSISTENCY]
        = smLayerCallback::redo_undo_SMC_PERS_SET_INCONSISTENCY;

    //For Replication
    gSmrUndoFunction[SMR_SMC_PERS_INSERT_ROW]
        = smLayerCallback::undo_SMC_PERS_INSERT_ROW;
    gSmrUndoFunction[SMR_SMC_PERS_UPDATE_VERSION_ROW]
        = smLayerCallback::undo_SMC_PERS_UPDATE_VERSION_ROW;
    gSmrUndoFunction[SMR_SMC_PERS_UPDATE_INPLACE_ROW]
        = smLayerCallback::undo_SMC_PERS_UPDATE_INPLACE_ROW;
    gSmrUndoFunction[SMR_SMC_PERS_DELETE_VERSION_ROW]
        = smLayerCallback::undo_SMC_PERS_DELETE_VERSION_ROW;

    gSmrRedoFunction[SMR_PHYSICAL]
        = smmUpdate::redo_undo_PHYSICAL;
    gSmrRedoFunction[SMR_SMM_MEMBASE_INFO]
        = smmUpdate::redo_SMM_MEMBASE_INFO;
    gSmrRedoFunction[SMR_SMM_MEMBASE_SET_SYSTEM_SCN]
        = smmUpdate::redo_SMM_MEMBASE_SET_SYSTEM_SCN;
    gSmrRedoFunction[SMR_SMM_MEMBASE_ALLOC_PERS_LIST]
        = smmUpdate::redo_undo_SMM_MEMBASE_ALLOC_PERS_LIST;
    gSmrRedoFunction[SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK]
        = smmUpdate::redo_SMM_MEMBASE_ALLOC_EXPAND_CHUNK;
    gSmrRedoFunction[SMR_SMM_PERS_UPDATE_LINK]
        = smLayerCallback::redo_undo_SMM_PERS_UPDATE_LINK;
    gSmrRedoFunction[SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK]
        = smmUpdate::redo_undo_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_INIT]
        = smLayerCallback::redo_SMC_TABLEHEADER_INIT;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_INDEX]
        = smLayerCallback::redo_SMC_TABLEHEADER_UPDATE_INDEX;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_COLUMNS]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMNS;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_INFO]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_INFO;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_SET_NULLROW]
        = smLayerCallback::redo_SMC_TABLEHEADER_SET_NULLROW;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_ALL]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_ALL;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_ALLOCINFO;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_FLAG]
        = smLayerCallback::redo_SMC_TABLEHEADER_UPDATE_FLAG;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_SET_SEQUENCE]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_SET_SEQUENCE;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT]
        = smLayerCallback::redo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_SET_SEGSTOATTR]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_SET_SEGSTOATTR;
    gSmrRedoFunction[SMR_SMC_TABLEHEADER_SET_INSERTLIMIT]
        = smLayerCallback::redo_undo_SMC_TABLEHEADER_SET_INSERTLIMIT;
    gSmrRedoFunction[ SMR_SMC_TABLEHEADER_SET_INCONSISTENCY ]
        = smLayerCallback::redo_SMC_TABLEHEADER_SET_INCONSISTENCY;

    gSmrRedoFunction[SMR_SMC_INDEX_SET_FLAG]
        = smLayerCallback::redo_SMC_INDEX_SET_FLAG;
    gSmrRedoFunction[SMR_SMC_INDEX_SET_SEGSTOATTR]
        = smLayerCallback::redo_SMC_INDEX_SET_SEGSTOATTR;
    gSmrRedoFunction[SMR_SMC_INDEX_SET_SEGATTR]
        = smLayerCallback::redo_SMC_SET_INDEX_SEGATTR;

    gSmrRedoFunction[SMR_SMC_PERS_INIT_FIXED_PAGE]
        = smLayerCallback::redo_SMC_PERS_INIT_FIXED_PAGE;
    gSmrRedoFunction[SMR_SMC_PERS_INIT_FIXED_ROW]
        = smLayerCallback::redo_SMC_PERS_INIT_FIXED_ROW;
    gSmrRedoFunction[SMR_SMC_PERS_UPDATE_FIXED_ROW]
        = smLayerCallback::redo_undo_SMC_PERS_UPDATE_FIXED_ROW;
    gSmrRedoFunction[SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION]
        = smLayerCallback::redo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION;
    gSmrRedoFunction[SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG]
        = smLayerCallback::redo_SMC_PERS_SET_FIX_ROW_DROP_FLAG;
    gSmrRedoFunction[SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT]
        = smLayerCallback::redo_SMC_PERS_SET_FIX_ROW_DELETE_BIT;
    gSmrRedoFunction[SMR_SMC_PERS_INIT_VAR_PAGE]
        = smLayerCallback::redo_SMC_PERS_INIT_VAR_PAGE;
    gSmrRedoFunction[SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD]
        = smLayerCallback::redo_undo_SMC_PERS_UPDATE_VAR_ROW_HEAD;
    gSmrRedoFunction[SMR_SMC_PERS_UPDATE_VAR_ROW]
        = smLayerCallback::redo_undo_SMC_PERS_UPDATE_VAR_ROW;
    gSmrRedoFunction[SMR_SMC_PERS_SET_VAR_ROW_FLAG]
        = smLayerCallback::redo_SMC_PERS_SET_VAR_ROW_FLAG;
    gSmrRedoFunction[SMR_SMC_INDEX_SET_DROP_FLAG]
        = smLayerCallback::redo_undo_SMC_INDEX_SET_DROP_FLAG;
    gSmrRedoFunction[SMR_SMC_PERS_SET_VAR_ROW_NXT_OID]
        = smLayerCallback::redo_undo_SMC_PERS_SET_VAR_ROW_NXT_OID;
    gSmrRedoFunction[SMR_SMC_PERS_WRITE_LOB_PIECE]
        = smLayerCallback::redo_SMC_PERS_WRITE_LOB_PIECE;
    gSmrRedoFunction[SMR_SMC_PERS_SET_INCONSISTENCY]
        = smLayerCallback::redo_undo_SMC_PERS_SET_INCONSISTENCY;

    //For Replication
    gSmrRedoFunction[SMR_SMC_PERS_INSERT_ROW]
        = smLayerCallback::redo_SMC_PERS_INSERT_ROW;
    gSmrRedoFunction[SMR_SMC_PERS_UPDATE_INPLACE_ROW]
        = smLayerCallback::redo_SMC_PERS_UPDATE_INPLACE_ROW;
    gSmrRedoFunction[SMR_SMC_PERS_UPDATE_VERSION_ROW]
        = smLayerCallback::redo_SMC_PERS_UPDATE_VERSION_ROW;
    gSmrRedoFunction[SMR_SMC_PERS_DELETE_VERSION_ROW]
        = smLayerCallback::redo_SMC_PERS_DELETE_VERSION_ROW;

    /* PROJ-2429 Dictionary based data compress for on-disk DB */
    gSmrRedoFunction[SMR_SMC_SET_CREATE_SCN]
        = smLayerCallback::redo_SMC_SET_CREATE_SCN;

    // PRJ-1548 User Memory Tablespace
    // 메모리 테이블스페이스 UPDATE 로그에 대한 Redo/Undo 함수 Vector 목록

    gSmrTBSUptRedoFunction[SCT_UPDATE_MRDB_CREATE_TBS]
        = smmUpdate::redo_SMM_UPDATE_MRDB_CREATE_TBS;
    gSmrTBSUptUndoFunction[SCT_UPDATE_MRDB_CREATE_TBS]
        = smmUpdate::undo_SMM_UPDATE_MRDB_CREATE_TBS;

    gSmrTBSUptRedoFunction[SCT_UPDATE_MRDB_CREATE_CIMAGE_FILE]
        = smmUpdate::redo_SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_MRDB_CREATE_CIMAGE_FILE]
        = smmUpdate::undo_SMM_UPDATE_MRDB_CREATE_CIMAGE_FILE;

    gSmrTBSUptRedoFunction[SCT_UPDATE_MRDB_DROP_TBS]
        = smmUpdate::redo_SMM_UPDATE_MRDB_DROP_TBS;
    gSmrTBSUptUndoFunction[SCT_UPDATE_MRDB_DROP_TBS]
        = smmUpdate::undo_SMM_UPDATE_MRDB_DROP_TBS;

    gSmrTBSUptRedoFunction[SCT_UPDATE_MRDB_ALTER_AUTOEXTEND]
        = smmUpdate::redo_SCT_UPDATE_MRDB_ALTER_AUTOEXTEND;
    gSmrTBSUptUndoFunction[SCT_UPDATE_MRDB_ALTER_AUTOEXTEND]
        = smmUpdate::undo_SCT_UPDATE_MRDB_ALTER_AUTOEXTEND;

    gSmrTBSUptRedoFunction[SCT_UPDATE_MRDB_ALTER_TBS_ONLINE]
        = smLayerCallback::redo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_MRDB_ALTER_TBS_ONLINE]
        = smLayerCallback::undo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE;

    gSmrTBSUptRedoFunction[SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE]
        = smLayerCallback::redo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE]
        = smLayerCallback::undo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE;

    // 디스크 테이블스페이스 UPDATE 로그에 대한 Redo/Undo 함수 Vector 목록
    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_CREATE_TBS]
        = sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_TBS;
    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_DROP_TBS]
        = sddUpdate::redo_SCT_UPDATE_DRDB_DROP_TBS;

    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_CREATE_DBF]
        = sddUpdate::redo_SCT_UPDATE_DRDB_CREATE_DBF;
    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_DROP_DBF]
        = sddUpdate::redo_SCT_UPDATE_DRDB_DROP_DBF;
    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_EXTEND_DBF]
        = sddUpdate::redo_SCT_UPDATE_DRDB_EXTEND_DBF;
    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_SHRINK_DBF]
        = sddUpdate::redo_SCT_UPDATE_DRDB_SHRINK_DBF;
    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_AUTOEXTEND_DBF]
        = sddUpdate::redo_SCT_UPDATE_DRDB_AUTOEXTEND_DBF;

    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_CREATE_TBS]
        = sddUpdate::undo_SCT_UPDATE_DRDB_CREATE_TBS;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_DROP_TBS]
        = sddUpdate::undo_SCT_UPDATE_DRDB_DROP_TBS;

    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_CREATE_DBF]
        = sddUpdate::undo_SCT_UPDATE_DRDB_CREATE_DBF;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_DROP_DBF]
        = sddUpdate::undo_SCT_UPDATE_DRDB_DROP_DBF;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_EXTEND_DBF]
        = sddUpdate::undo_SCT_UPDATE_DRDB_EXTEND_DBF;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_SHRINK_DBF]
        = sddUpdate::undo_SCT_UPDATE_DRDB_SHRINK_DBF;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_AUTOEXTEND_DBF]
        = sddUpdate::undo_SCT_UPDATE_DRDB_AUTOEXTEND_DBF;

    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_ALTER_TBS_ONLINE]
        = sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_TBS_ONLINE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_ALTER_TBS_ONLINE]
        = sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_TBS_ONLINE;

    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE]
        = sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE]
        = sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE;

    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_ALTER_DBF_ONLINE]
        = sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_DBF_ONLINE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_ALTER_DBF_ONLINE]
        = sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_DBF_ONLINE;

    gSmrTBSUptRedoFunction[SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE]
        = sddUpdate::redo_SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE;
    gSmrTBSUptUndoFunction[SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE]
        = sddUpdate::undo_SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE;

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS에 대한 update 로그의 redo, undo function */
    gSmrTBSUptRedoFunction[SCT_UPDATE_VRDB_CREATE_TBS]
        = svmUpdate::redo_SCT_UPDATE_VRDB_CREATE_TBS;
    gSmrTBSUptUndoFunction[SCT_UPDATE_VRDB_CREATE_TBS]
        = svmUpdate::undo_SCT_UPDATE_VRDB_CREATE_TBS;

    gSmrTBSUptRedoFunction[SCT_UPDATE_VRDB_DROP_TBS]
        = svmUpdate::redo_SCT_UPDATE_VRDB_DROP_TBS;
    gSmrTBSUptUndoFunction[SCT_UPDATE_VRDB_DROP_TBS]
        = svmUpdate::undo_SCT_UPDATE_VRDB_DROP_TBS;

    gSmrTBSUptRedoFunction[SCT_UPDATE_VRDB_ALTER_AUTOEXTEND]
        = svmUpdate::redo_SCT_UPDATE_VRDB_ALTER_AUTOEXTEND;
    gSmrTBSUptUndoFunction[SCT_UPDATE_VRDB_ALTER_AUTOEXTEND]
        = svmUpdate::undo_SCT_UPDATE_VRDB_ALTER_AUTOEXTEND;

    /* Disk, Memory, Volatile 모두에 적용되는 REDO/UNDO함수 세팅 */
    gSmrTBSUptRedoFunction[SCT_UPDATE_COMMON_ALTER_ATTR_FLAG]
        = sctTBSUpdate::redo_SCT_UPDATE_ALTER_ATTR_FLAG;
    gSmrTBSUptUndoFunction[SCT_UPDATE_COMMON_ALTER_ATTR_FLAG]
        = sctTBSUpdate::undo_SCT_UPDATE_ALTER_ATTR_FLAG;

    return;
}

/***********************************************************************
 * Description : UpdateLog를 Transaction Log Buffer에 구성한후에
 *               SM Log Buffer에 로그를 기록한다.
 *
 * aTrans           - [IN] Transaction Pointer
 * aUpdateLogType   - [IN] Update Log Type.
 * aGRID            - [IN] GRID
 * aData            - [IN] smrUpdateLog의 mData영역에 설정될 값
 * aBfrImgCnt       - [IN] Befor Image갯수
 * aBfrImage        - [IN] Befor Image
 * aAftImgCnt       - [IN] After Image갯수
 * aAftImage        - [IN] After Image
 *
 * aWrittenLogLSN   - [OUT] 기록된 로그의 LSN
 *
 * Related Issue!!
 *  BUG-14778: TX의 Log Buffer를 인터페이스로 사용해야 합니다.
 *   - 모든 코드를 고치기가 힘들어서 하나의 Update로그 기록함수를
 *     만들고 Tx의 Lob Buffer 인터페이스를 사용하도록 하였습니다.
 *
 **********************************************************************/
IDE_RC smrUpdate::writeUpdateLog( idvSQL*           aStatistics,
                                  void*             aTrans,
                                  smrUpdateType     aUpdateLogType,
                                  scGRID            aGRID,
                                  vULong            aData,
                                  UInt              aBfrImgCnt,
                                  smrUptLogImgInfo *aBfrImage,
                                  UInt              aAftImgCnt,
                                  smrUptLogImgInfo *aAftImage,
                                  smLSN            *aWrittenLogLSN)
{
    SChar*         sLogBuffer;
    smTID          sTransID;
    UInt           sLogTypeFlag;
    UInt           sSize;
    UInt           i;
    UInt           sAImgSize;
    UInt           sBImgSize;
    UInt           sOffset = 0;

    static smrLogType sLogType = SMR_LT_UPDATE;

    /* Transction Log Buffer를 초기화 한다. */
    smLayerCallback::initLogBuffer( aTrans );

    sOffset = SMR_LOGREC_SIZE(smrUpdateLog);

    /* Before Image를 로그 버퍼에 기록 */
    sBImgSize = 0;
    for( i = 0; i < aBfrImgCnt ; i++ )
    {
        if( aBfrImage->mSize != 0 )
        {
            sBImgSize += aBfrImage->mSize;

            IDE_ERROR( aBfrImage->mLogImg != NULL );
            IDE_TEST( smLayerCallback::writeLogToBufferAtOffset(
                          aTrans,
                          aBfrImage->mLogImg,
                          sOffset,
                          aBfrImage->mSize )
                      != IDE_SUCCESS );

            sOffset += aBfrImage->mSize;
        }
        aBfrImage++;
    }

    /* After Image를 로그 버퍼에 기록 */
    sAImgSize = 0;
    for( i = 0; i < aAftImgCnt ; i++ )
    {
        if( aAftImage->mSize != 0 )
        {
            sAImgSize += aAftImage->mSize;

            IDE_ERROR( aAftImage->mLogImg != NULL );
            IDE_TEST( smLayerCallback::writeLogToBufferAtOffset(
                          aTrans,
                          aAftImage->mLogImg,
                          sOffset,
                          aAftImage->mSize )
                      != IDE_SUCCESS );

            sOffset += aAftImage->mSize;
        }

        aAftImage++;
    }

    /* LogTail 기록 */
    IDE_TEST( smLayerCallback::writeLogToBufferAtOffset(
                  aTrans,
                  &sLogType,
                  sOffset,
                  ID_SIZEOF(smrLogTail) )
              != IDE_SUCCESS );

    /* 트랜잭션 정보 얻기 */
    (void)smLayerCallback::getTransInfo( aTrans,
                                         &sLogBuffer,
                                         &sTransID,
                                         &sLogTypeFlag );

    sSize = sBImgSize + sAImgSize + SMR_LOGREC_SIZE( smrUpdateLog )
        + ID_SIZEOF(smrLogTail);

    /* Stack변수에 있는 Header변수 초기화 */
    smrUpdate::setUpdateLogHead( (smrUpdateLog*)sLogBuffer,
                                 sTransID,
                                 sLogTypeFlag,
                                 SMR_LT_UPDATE,
                                 sSize,
                                 aUpdateLogType,
                                 aGRID,
                                 aData,
                                 sBImgSize,
                                 sAImgSize );

    IDE_TEST(smrLogMgr::writeLog(aStatistics, /* idvSQL* */
                                 aTrans,
                                 sLogBuffer,
                                 NULL,            // Previous LSN Ptr
                                 aWrittenLogLSN,  // Log LSN Ptr
                                 NULL,            // End LSN Ptr
                                 aData )          // TableOID
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description : UpdateLog를 Stack의 Log Buffer에 구성한후에
 *               SM Log Buffer에 로그를 기록한다.
 *               이는 무조건적인 Redo를 위해 기록되는
 *               Dummy Transaction으로 기록된다. 
 *
 *               또한 이는 반드시 하나의 4Byte짜리 After Image만 와야 한다.
 *
 * aUpdateLogType   - [IN] Update Log Type.
 * aGRID            - [IN] GRID
 * aData            - [IN] smrUpdateLog의 mData영역에 설정될 값
 * aAftImage        - [IN] After Image
 *
 **********************************************************************/
#define SMR_DUMMY_UPDATE_LOG_SIZE ( ID_SIZEOF( UInt ) +                \
                                    SMR_LOGREC_SIZE( smrUpdateLog ) +  \
                                    ID_SIZEOF( smrLogTail ) )
IDE_RC smrUpdate::writeDummyUpdateLog( smrUpdateType     aUpdateLogType,
                                       scGRID            aGRID,
                                       vULong            aData,
                                       UInt              aAftImg )
{
    ULong          sLogBuffer[ (SMR_DUMMY_UPDATE_LOG_SIZE + ID_SIZEOF(ULong))
                                / ID_SIZEOF( ULong ) ];
    smrUpdateLog * sUpdateLog;
    SChar        * sCurLogPtr;

    sUpdateLog = (smrUpdateLog*)sLogBuffer;
    smrLogHeadI::setType(    &sUpdateLog->mHead, SMR_LT_UPDATE );
    smrLogHeadI::setTransID( &sUpdateLog->mHead, SM_NULL_TID );
    smrLogHeadI::setSize(    &sUpdateLog->mHead,
                             SMR_DUMMY_UPDATE_LOG_SIZE );
    smrLogHeadI::setFlag(    &sUpdateLog->mHead, SMR_LOG_TYPE_NORMAL );
    smrLogHeadI::setReplStmtDepth( &sUpdateLog->mHead,
                                   SMI_STATEMENT_DEPTH_NULL );
    SC_COPY_GRID( aGRID, sUpdateLog->mGRID );
    sUpdateLog->mData          = aData;
    sUpdateLog->mAImgSize      = ID_SIZEOF( UInt );
    sUpdateLog->mBImgSize      = 0;
    sUpdateLog->mType          = aUpdateLogType;
    sCurLogPtr                 = (SChar *)sLogBuffer + SMR_LOGREC_SIZE( smrUpdateLog );

    SMR_LOG_APPEND_4( sCurLogPtr, aAftImg );
    smrLogHeadI::copyTail( sCurLogPtr, &sUpdateLog->mHead );

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   NULL,
                                   (SChar *)sLogBuffer,
                                   NULL,  // Previous LSN Ptr
                                   NULL,  // Log LSN Ptr
                                   NULL,  // End LSN Ptr
                                   aData ) // 0아니면 1밖에 안넘어온다. 
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_PHYSICAL */
IDE_RC smrUpdate::physicalUpdate(idvSQL*      aStatistics,
                                 void*        aTrans,
                                 scSpaceID    aSpaceID,
                                 scPageID     aPageID,
                                 scOffset     aOffset,
                                 SChar*       aBeforeImage,
                                 SInt         aBeforeImageSize,
                                 SChar*       aAfterImage1,
                                 SInt         aAfterImageSize1,
                                 SChar*       aAfterImage2,
                                 SInt         aAfterImageSize2)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo[2];

    sBfrImgInfo.mLogImg = aBeforeImage;
    sBfrImgInfo.mSize   = aBeforeImageSize;

    sAftImgInfo[0].mLogImg = aAfterImage1;
    sAftImgInfo[0].mSize   = aAfterImageSize1;
    sAftImgInfo[1].mLogImg = aAfterImage2;
    sAftImgInfo[1].mSize   = aAfterImageSize2;

    SC_MAKE_GRID( sGRID, aSpaceID, aPageID, aOffset );

    IDE_TEST( writeUpdateLog( aStatistics, /* idvSQL* */
                              aTrans,
                              SMR_PHYSICAL,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              2, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMM_MEMBASE_INFO                      */
IDE_RC smrUpdate::setMemBaseInfo( idvSQL*      aStatistics,
                                  void       * aTrans,
                                  scSpaceID    aSpaceID,
                                  smmMemBase * aMemBase )
{
    scGRID           sGRID;
    smrUptLogImgInfo sAftImgInfo;

    sAftImgInfo.mLogImg = (SChar*)aMemBase;
    sAftImgInfo.mSize   = ID_SIZEOF(smmMemBase);

    SC_MAKE_GRID( sGRID, aSpaceID, SMM_MEMBASE_PAGEID, SMM_MEMBASE_OFFSET );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMM_MEMBASE_INFO,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMM_MEMBASE_SET_SYSTEM_SCN                      */
#define SMR_SYSTEM_SCN_LOG_SIZE ( ID_SIZEOF( smSCN ) + SMR_LOGREC_SIZE( smrUpdateLog ) + ID_SIZEOF( smrLogTail ) )

IDE_RC smrUpdate::setSystemSCN(smSCN aSystemSCN)
{
    smrUpdateLog*  sUpdateLog;
    SChar*         sCurLogPtr;
    ULong          sLogBuffer[ (SMR_SYSTEM_SCN_LOG_SIZE + ID_SIZEOF( ULong ) - 1)  / ID_SIZEOF( ULong ) ];

    sUpdateLog = (smrUpdateLog*)sLogBuffer;

    smrLogHeadI::setType( &sUpdateLog->mHead, SMR_LT_UPDATE );
    smrLogHeadI::setTransID( &sUpdateLog->mHead, SM_NULL_TID );
    smrLogHeadI::setSize( &sUpdateLog->mHead,
                          ID_SIZEOF(smSCN)
                          + SMR_LOGREC_SIZE( smrUpdateLog )
                          + ID_SIZEOF( smrLogTail ) );
    smrLogHeadI::setFlag( &sUpdateLog->mHead, SMR_LOG_TYPE_NORMAL );
    smrLogHeadI::setReplStmtDepth( &sUpdateLog->mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    SC_MAKE_GRID( sUpdateLog->mGRID, SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SMM_MEMBASE_PAGEID, SMM_MEMBASE_OFFSET );
    sUpdateLog->mAImgSize      = ID_SIZEOF(smSCN);
    sUpdateLog->mBImgSize      = 0;
    sUpdateLog->mType          = SMR_SMM_MEMBASE_SET_SYSTEM_SCN;

    sCurLogPtr                 = (SChar *)sLogBuffer + SMR_LOGREC_SIZE( smrUpdateLog );

    SMR_LOG_APPEND_8( sCurLogPtr, aSystemSCN );

    smrLogHeadI::copyTail( sCurLogPtr, &sUpdateLog->mHead );

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   NULL,
                                   (SChar *)sLogBuffer,
                                   NULL,  // Previous LSN Ptr
                                   NULL,  // Log LSN Ptr
                                   NULL,  // End LSN Ptr
                                   SM_NULL_OID )
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*>>>>>>>>>>>>>> Redo Undo: SMR_SMM_MEMBASE_ALLOC_PERS_LIST
    before image :
                   aFPLNo
                   aBeforeMembase-> mFreePageLists[ aFPLNo ].mFirstFreePageID
                   aBeforeMembase-> mFreePageLists[ aFPLNo ].mFreePageCount
    after  image :
                   aFPLNo
                   aAfterPageID
                   aAfterPageCount

 */
IDE_RC smrUpdate::allocPersListAtMembase(idvSQL*      aStatistics,
                                         void*        aTrans,
                                         scSpaceID    aSpaceID,
                                         smmMemBase*  aBeforeMemBase,
                                         smmFPLNo     aFPLNo,
                                         scPageID     aAfterPageID,
                                         vULong       aAfterPageCount )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[3];
    smrUptLogImgInfo sAftImgInfo[3];

    IDE_DASSERT( smmFPLManager::isValidFPL( aSpaceID,
                                            aAfterPageID,
                                            aAfterPageCount )
                 == ID_TRUE );

    sBfrImgInfo[0].mLogImg = (SChar*)&aFPLNo;
    sBfrImgInfo[0].mSize   = ID_SIZEOF( smmFPLNo );
    sBfrImgInfo[1].mLogImg = (SChar*)&( aBeforeMemBase->
                                mFreePageLists[ aFPLNo ].mFirstFreePageID );
    sBfrImgInfo[1].mSize   = ID_SIZEOF( scPageID );
    sBfrImgInfo[2].mLogImg = (SChar*)&( aBeforeMemBase->
                                        mFreePageLists[ aFPLNo ].mFreePageCount );
    sBfrImgInfo[2].mSize   = ID_SIZEOF( vULong );

    sAftImgInfo[0].mLogImg = (SChar*)&aFPLNo;
    sAftImgInfo[0].mSize   = ID_SIZEOF( smmFPLNo );
    sAftImgInfo[1].mLogImg = (SChar*)&aAfterPageID;
    sAftImgInfo[1].mSize   = ID_SIZEOF( scPageID );
    sAftImgInfo[2].mLogImg = (SChar*)&aAfterPageCount;
    sAftImgInfo[2].mSize   = ID_SIZEOF( vULong );

    SC_MAKE_GRID( sGRID, aSpaceID, SMM_MEMBASE_PAGEID, SMM_MEMBASE_OFFSET );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMM_MEMBASE_ALLOC_PERS_LIST,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              3, /* Before Image Count */
                              sBfrImgInfo,
                              3, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/* Update type:  SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK

   smmManager::allocNewExpandChunk의 Logical Redo를 위해
   Membase 일부 멤버의 Before 이미지를 저장한다.

   before  image: aBeforeMembase->m_alloc_pers_page_count
                 aBeforeMembase->mCurrentExpandChunkCnt
                 aBeforeMembase->mExpandChunkPageCnt
                 aBeforeMembase->m_nDBFileCount[0]
                 aBeforeMembase->m_nDBFileCount[1]
                 aBeforeMembase->mFreePageListCount
                 [
                      aBeforeMembase-> mFreePageLists[i].mFirstFreePageID
                      aBeforeMembase-> mFreePageLists[i].mFreePageCount
                 ] ( aBeforeMembase->mFreePageListCount 만큼 )
                 aExpandPageListID
                 aAfterChunkFirstPID
                 aAfterChunkLastPID

   이 함수에서 기록되는 모든 필드를 vULong타입으로 통일하여
   로그 기록시 버스오류발생을 막았다.
   * 32비트에서는 4바이트 integer 여러개로 기록되므로
     버스오류가 발생하지 않는다.
   * 64비트에서는 8바이트 integer 여러개로 기록되므로
     버스오류가 발생하지 않는다.

     aExpandPageListID : 확장된 Chunk의 Page가 매달릴 Page List의 ID
                         UINT_MAX일 경우 모든 Page List에 골고루 분배된다
 */
#define SMR_ALLOC_EXPAND_CHUNK_MAX_AFTER_IMG_COUNT (SM_MAX_PAGELIST_COUNT * 2 + 9)

IDE_RC smrUpdate::allocExpandChunkAtMembase(
                                       idvSQL      * aStatistics,
                                       void        * aTrans,
                                       scSpaceID     aSpaceID,
                                       smmMemBase  * aBeforeMembase,
                                       UInt          aExpandPageListID,
                                       scPageID      aAfterChunkFirstPID,
                                       scPageID      aAfterChunkLastPID,
                                       smLSN       * aWrittenLogLSN )
{
    scGRID           sGRID;
    UInt             i, j;
    smrUptLogImgInfo sAftImgInfo[ SMR_ALLOC_EXPAND_CHUNK_MAX_AFTER_IMG_COUNT ];

    sAftImgInfo[0].mLogImg = (SChar*) & ( aBeforeMembase->mAllocPersPageCount );
    sAftImgInfo[0].mSize   = ID_SIZEOF( vULong );

    sAftImgInfo[1].mLogImg = (SChar*) & ( aBeforeMembase->mCurrentExpandChunkCnt );
    sAftImgInfo[1].mSize   = ID_SIZEOF( vULong );

    sAftImgInfo[2].mLogImg = (SChar*) & ( aBeforeMembase->mExpandChunkPageCnt );
    sAftImgInfo[2].mSize   = ID_SIZEOF( vULong );

    sAftImgInfo[3].mLogImg = (SChar*) & ( aBeforeMembase->mDBFileCount[0] );
    sAftImgInfo[3].mSize   = ID_SIZEOF( UInt );

    sAftImgInfo[4].mLogImg = (SChar*) & ( aBeforeMembase->mDBFileCount[1] );
    sAftImgInfo[4].mSize   = ID_SIZEOF( UInt );

    sAftImgInfo[5].mLogImg = (SChar*) & ( aBeforeMembase->mFreePageListCount );
    sAftImgInfo[5].mSize   = ID_SIZEOF( UInt );

    j = 6;

    IDE_ERROR_MSG( aBeforeMembase->mFreePageListCount <= SM_MAX_PAGELIST_COUNT,
                   "aBeforeMembase->mFreePageListCount : %"ID_UINT32_FMT,
                   aBeforeMembase->mFreePageListCount );

    for ( i = 0 ; i< aBeforeMembase->mFreePageListCount; i++ )
    {
        IDE_DASSERT( smmFPLManager::isValidFPL(
                         aSpaceID,
                         & aBeforeMembase->mFreePageLists[i] )
                     == ID_TRUE );

        sAftImgInfo[j].mLogImg = (SChar*) &( aBeforeMembase->
                                             mFreePageLists[i].mFirstFreePageID );
        sAftImgInfo[j].mSize   = ID_SIZEOF( scPageID );

        j++;

        sAftImgInfo[j].mLogImg = (SChar*) &( aBeforeMembase->
                                             mFreePageLists[i].mFreePageCount );
        sAftImgInfo[j].mSize   = ID_SIZEOF( vULong );

        j++;
    }

    sAftImgInfo[j].mLogImg = (SChar*) &( aExpandPageListID );
    sAftImgInfo[j].mSize   = ID_SIZEOF( UInt );
    j++;

    sAftImgInfo[j].mLogImg = (SChar*) &( aAfterChunkFirstPID );
    sAftImgInfo[j].mSize   = ID_SIZEOF( scPageID );
    j++;

    sAftImgInfo[j].mLogImg = (SChar*) &( aAfterChunkLastPID );
    sAftImgInfo[j].mSize   = ID_SIZEOF( scPageID );
    j++;

    /* Stack에 할당된 변수 영역을 넘어서는지 검사한다. */
    IDE_ERROR_MSG( j < SMR_ALLOC_EXPAND_CHUNK_MAX_AFTER_IMG_COUNT + 1,
                   "sImageCount : %"ID_UINT32_FMT,
                   j);

    SC_MAKE_GRID( sGRID, aSpaceID, SMM_MEMBASE_PAGEID, SMM_MEMBASE_OFFSET );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              j, /* After Image Count */
                              sAftImgInfo,
                              aWrittenLogLSN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMM_PERS_UPDATE_LINK  */
/* before image: Prev PageID | Next PageID */
/* after  image: Prev PageID | Next PageID */
IDE_RC smrUpdate::updateLinkAtPersPage(idvSQL*    aStatistics,
                                       void*      aTrans,
                                       scSpaceID  aSpaceID,
                                       scPageID   aPageID,
                                       scPageID   aBeforePrevPageID,
                                       scPageID   aBeforeNextPageID,
                                       scPageID   aAfterPrevPageID,
                                       scPageID   aAfterNextPageID)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[2];
    smrUptLogImgInfo sAftImgInfo[2];

    sBfrImgInfo[0].mLogImg = (SChar*) &aBeforePrevPageID;
    sBfrImgInfo[0].mSize   = ID_SIZEOF(scPageID);
    sBfrImgInfo[1].mLogImg = (SChar*) &aBeforeNextPageID;
    sBfrImgInfo[1].mSize   = ID_SIZEOF(scPageID);

    sAftImgInfo[0].mLogImg = (SChar*) &aAfterPrevPageID;
    sAftImgInfo[0].mSize   = ID_SIZEOF(scPageID);
    sAftImgInfo[1].mLogImg = (SChar*) &aAfterNextPageID;
    sAftImgInfo[1].mSize   = ID_SIZEOF(scPageID);

    SC_MAKE_GRID( sGRID, aSpaceID, aPageID, 0 );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMM_PERS_UPDATE_LINK,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              2, /* Before Image Count */
                              sBfrImgInfo, /* Before Image Info Array */
                              2, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*>>>>>>>>>>>>>> Redo Undo: SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK   */
/* before image: Next Free PageID */
/* after  image: Next Free PageID */
IDE_RC smrUpdate::updateLinkAtFreePage(idvSQL*      aStatistics,
                                       void*        aTrans,
                                       scSpaceID    aSpaceID,
                                       scPageID     aFreeListInfoPID,
                                       UInt         aFreePageSlotOffset,
                                       scPageID     aBeforeNextFreePID,
                                       scPageID     aAfterNextFreePID )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*) &aBeforeNextFreePID;
    sBfrImgInfo.mSize   = ID_SIZEOF(scPageID);

    sAftImgInfo.mLogImg = (SChar*) &aAfterNextFreePID;
    sAftImgInfo.mSize   = ID_SIZEOF(scPageID);

    SC_MAKE_GRID( sGRID,
                  aSpaceID,
                  aFreeListInfoPID,
                  aFreePageSlotOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo, /* Before Image Info Array */
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_INIT             */
/* Redo                                              */
IDE_RC smrUpdate::initTableHeader(idvSQL*      aStatistics,
                                  void*        aTrans,
                                  scPageID     aPageID,
                                  scOffset     aOffset,
                                  void*        aTableHeader,
                                  UInt         aSize)
{
    scGRID           sGRID;
    smrUptLogImgInfo sAftImgInfo;

    sAftImgInfo.mLogImg = (SChar*) aTableHeader;
    sAftImgInfo.mSize   = aSize;

    SC_MAKE_GRID( sGRID, SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  aPageID,
                  aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_INIT,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INDEX                     */
IDE_RC smrUpdate::updateIndexAtTableHead(idvSQL*             aStatistics,
                                         void*               aTrans,
                                         smOID               aOID,
                                         const smVCDesc*     aIndex,
                                         const UInt          aOIDIdx,
                                         smOID               aOIDVar,
                                         UInt                aLength,
                                         UInt                aFlag)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[2];
    smrUptLogImgInfo sAftImgInfo[2];
    smVCDesc         sVarColumnDesc;

    sBfrImgInfo[0].mLogImg = (SChar*) &aOIDIdx;
    sBfrImgInfo[0].mSize   = ID_SIZEOF(UInt);
    sBfrImgInfo[1].mLogImg = (SChar*) aIndex;
    sBfrImgInfo[1].mSize   = ID_SIZEOF(smVCDesc);

    sVarColumnDesc.length      = aLength;
    sVarColumnDesc.flag        = aFlag;
    sVarColumnDesc.fstPieceOID = aOIDVar;

    sAftImgInfo[0].mLogImg = (SChar*) &aOIDIdx;
    sAftImgInfo[0].mSize   = ID_SIZEOF(UInt);
    sAftImgInfo[1].mLogImg = (SChar*) &sVarColumnDesc;
    sAftImgInfo[1].mSize   = ID_SIZEOF(smVCDesc);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aOID),
                  SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_UPDATE_INDEX,
                              sGRID,
                              aOID,
                              2, /* Before Image Count */
                              sBfrImgInfo,
                              2, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_COLUMNS                    */
IDE_RC smrUpdate::updateColumnsAtTableHead(idvSQL*             aStatistics,
                                           void*               aTrans,
                                           scPageID            aPageID,
                                           scOffset            aOffset,
                                           const smVCDesc*     aColumnsVCDesc,
                                           smOID               aFstPieceOID,
                                           UInt                aLength,
                                           UInt                aFlag,
                                           UInt                aBeforeLobColumnCount,
                                           UInt                aAfterLobColumnCount,
                                           UInt                aBeforeColumnCount,
                                           UInt                aAfterColumnCount )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[3];
    smrUptLogImgInfo sAftImgInfo[3];
    smVCDesc         sVCDesc;

    sBfrImgInfo[0].mLogImg = (SChar*) aColumnsVCDesc;
    sBfrImgInfo[0].mSize   = ID_SIZEOF(smVCDesc);
    sBfrImgInfo[1].mLogImg = (SChar*) &aBeforeLobColumnCount;
    sBfrImgInfo[1].mSize   = ID_SIZEOF(UInt);
    sBfrImgInfo[2].mLogImg = (SChar*) &aBeforeColumnCount;
    sBfrImgInfo[2].mSize   = ID_SIZEOF(UInt);

    sVCDesc.length      = aLength;
    sVCDesc.fstPieceOID = aFstPieceOID;
    sVCDesc.flag        = aFlag;

    sAftImgInfo[0].mLogImg = (SChar*) &sVCDesc;
    sAftImgInfo[0].mSize   = ID_SIZEOF(smVCDesc);
    sAftImgInfo[1].mLogImg = (SChar*) &aAfterLobColumnCount;
    sAftImgInfo[1].mSize   = ID_SIZEOF(UInt);
    sAftImgInfo[2].mLogImg = (SChar*) &aAfterColumnCount;
    sAftImgInfo[2].mSize   = ID_SIZEOF(UInt);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  aPageID,
                  aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_UPDATE_COLUMNS,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              3, /* Before Image Count */
                              sBfrImgInfo,
                              3, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INFO                       */
IDE_RC smrUpdate::updateInfoAtTableHead(idvSQL*             aStatistics,
                                        void*               aTrans,
                                        scPageID            aPageID,
                                        scOffset            aOffset,
                                        const smVCDesc*     aInfoVCDesc,
                                        smOID               aOIDVar,
                                        UInt                aLength,
                                        UInt                aFlag)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;
    smVCDesc         sVCDesc;

    sBfrImgInfo.mLogImg = (SChar*) aInfoVCDesc;
    sBfrImgInfo.mSize   = ID_SIZEOF(smVCDesc);

    sVCDesc.length      = aLength;
    sVCDesc.fstPieceOID = aOIDVar;
    sVCDesc.flag        = aFlag;

    sAftImgInfo.mLogImg = (SChar*) &sVCDesc;
    sAftImgInfo.mSize   = ID_SIZEOF(smVCDesc);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  aPageID,
                  aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_UPDATE_INFO,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_SET_NULLROW                    */
IDE_RC smrUpdate::setNullRow(idvSQL*         aStatistics,
                             void*           aTrans,
                             scSpaceID       aSpaceID,
                             scPageID        aPageID,
                             scOffset        aOffset,
                             smOID           aNullOID)
{
    scGRID           sGRID;

    SC_MAKE_GRID( sGRID,
                  aSpaceID,
                  aPageID,
                  aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_SET_NULLROW,
                              sGRID,
                              aNullOID,
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              0, /* After Image Count */
                              NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALL         */
IDE_RC smrUpdate::updateAllAtTableHead(idvSQL*               aStatistics,
                                       void*                 aTrans,
                                       void*                 aTable,
                                       UInt                  aColumnsize,
                                       const smVCDesc*       aColumnVCDesc,
                                       const smVCDesc*       aInfoVCDesc,
                                       UInt                  aFlag,
                                       ULong                 aMaxRow,
                                       UInt                  aParallelDegree)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[6];
    smrUptLogImgInfo sAftImgInfo[6];

    scPageID         sPageID;
    scOffset         sOffset;
    smVCDesc         sTableColumnsVCDesc;
    smVCDesc         sTableInfoVCDesc;
    UInt             sTableColumnSize;
    UInt             sTableFlag;
    ULong            sTableMaxRow;
    UInt             sTableParallelDegree;

    // 테이블 헤더 정보 얻기
    (void)smLayerCallback::getTableHeaderInfo( aTable,
                                               &sPageID,
                                               &sOffset,
                                               &sTableColumnsVCDesc,
                                               &sTableInfoVCDesc,
                                               &sTableColumnSize,
                                               &sTableFlag,
                                               &sTableMaxRow,
                                               &sTableParallelDegree );

    sBfrImgInfo[0].mLogImg = (SChar*) &sTableColumnsVCDesc;
    sBfrImgInfo[0].mSize   = ID_SIZEOF( smVCDesc );
    sBfrImgInfo[1].mLogImg = (SChar*) &sTableInfoVCDesc;
    sBfrImgInfo[1].mSize   = ID_SIZEOF( smVCDesc );
    sBfrImgInfo[2].mLogImg = (SChar*) &sTableColumnSize;
    sBfrImgInfo[2].mSize   = ID_SIZEOF( UInt );
    sBfrImgInfo[3].mLogImg = (SChar*) &sTableFlag;
    sBfrImgInfo[3].mSize   = ID_SIZEOF( UInt );
    sBfrImgInfo[4].mLogImg = (SChar*) &sTableMaxRow;
    sBfrImgInfo[4].mSize   = ID_SIZEOF( ULong );
    sBfrImgInfo[5].mLogImg = (SChar*) &sTableParallelDegree;
    sBfrImgInfo[5].mSize   = ID_SIZEOF( UInt );

    sAftImgInfo[0].mLogImg = (SChar*) aColumnVCDesc;
    sAftImgInfo[0].mSize   = ID_SIZEOF( smVCDesc );
    sAftImgInfo[1].mLogImg = (SChar*) aInfoVCDesc;
    sAftImgInfo[1].mSize   = ID_SIZEOF( smVCDesc );
    sAftImgInfo[2].mLogImg = (SChar*) &aColumnsize;
    sAftImgInfo[2].mSize   = ID_SIZEOF( UInt );
    sAftImgInfo[3].mLogImg = (SChar*) &aFlag;
    sAftImgInfo[3].mSize   = ID_SIZEOF( UInt );
    sAftImgInfo[4].mLogImg = (SChar*) &aMaxRow;
    sAftImgInfo[4].mSize   = ID_SIZEOF( ULong );
    sAftImgInfo[5].mLogImg = (SChar*) &aParallelDegree;
    sAftImgInfo[5].mSize   = ID_SIZEOF( UInt );

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  sOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_UPDATE_ALL,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              6, /* Before Image Count */
                              sBfrImgInfo,
                              6, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO */
/* before image: PageCount | HeadPageID | TailPageID */
/* after  image: PageCount | HeadPageID | TailPageID */
IDE_RC smrUpdate::updateAllocInfoAtTableHead(idvSQL*  aStatistics,
                                             void*    aTrans,
                                             scPageID aPageID,
                                             scOffset aOffset,
                                             void*    aAllocPageList,
                                             vULong   aPageCount,
                                             scPageID aHead,
                                             scPageID aTail)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[3];
    smrUptLogImgInfo sAftImgInfo[3];

    vULong           sPageCount;
    scPageID         sHeadPageID;
    scPageID         sTailPageID;

    // PagelistEntry 정보 얻기
    // 로깅을 위한 실제 작업시 List의 Lock을 잡고 있음.
    smLayerCallback::getAllocPageListInfo( aAllocPageList,
                                           &sPageCount,
                                           &sHeadPageID,
                                           &sTailPageID );

    sBfrImgInfo[0].mLogImg = (SChar*) &sPageCount;
    sBfrImgInfo[0].mSize   = ID_SIZEOF(vULong);
    sBfrImgInfo[1].mLogImg = (SChar*) &sHeadPageID;
    sBfrImgInfo[1].mSize   = ID_SIZEOF(scPageID);
    sBfrImgInfo[2].mLogImg = (SChar*) &sTailPageID;
    sBfrImgInfo[2].mSize   = ID_SIZEOF(scPageID);

    sAftImgInfo[0].mLogImg = (SChar*) &aPageCount;
    sAftImgInfo[0].mSize   = ID_SIZEOF(vULong);
    sAftImgInfo[1].mLogImg = (SChar*) &aHead;
    sAftImgInfo[1].mSize   = ID_SIZEOF(scPageID);
    sAftImgInfo[2].mLogImg = (SChar*) &aTail;
    sAftImgInfo[2].mSize   = ID_SIZEOF(scPageID);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  aPageID,
                  aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              3, /* Before Image Count */
                              sBfrImgInfo,
                              3, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* SMR_SMC_TABLEHEADER_UPDATE_FLAG                 */
IDE_RC smrUpdate::updateFlagAtTableHead(idvSQL* aStatistics,
                                        void*   aTrans,
                                        void*   aTable,
                                        UInt    aFlag )
{
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;
    scGRID           sGRID;
    scPageID         sPageID;
    scOffset         sOffset;
    UInt             sTableFlag;

    // 테이블 헤더의 mFlag 얻기
    (void)smLayerCallback::getTableHeaderFlag( aTable,
                                               &sPageID,
                                               &sOffset,
                                               &sTableFlag );

    sBfrImgInfo.mLogImg = (SChar*) &sTableFlag;
    sBfrImgInfo.mSize   = ID_SIZEOF(UInt);

    sAftImgInfo.mLogImg = (SChar*) &aFlag;
    sAftImgInfo.mSize   = ID_SIZEOF(UInt);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  sOffset );

    IDE_TEST( writeUpdateLog(aStatistics,
                             aTrans,
                             SMR_SMC_TABLEHEADER_UPDATE_FLAG,
                             sGRID,
                             0, /* smrUpdateLog::mData */
                             1, /* Before Image Count */
                             &sBfrImgInfo,
                             1, /* After Image Count */
                             &sAftImgInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_TABLEHEADER_SET_SEQUENCE */
IDE_RC smrUpdate::updateSequenceAtTableHead(idvSQL*      aStatistics,
                                            void*        aTrans,
                                            smOID        aOIDTable,
                                            scOffset     aOffset,
                                            void*        aBeforeSequence,
                                            void*        aAfterSequence,
                                            UInt         aSize)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*)aBeforeSequence;

    if( aBeforeSequence == NULL )
    {
        sBfrImgInfo.mSize = 0;
    }
    else
    {
        sBfrImgInfo.mSize = aSize;
    }

    sAftImgInfo.mLogImg = (SChar*)aAfterSequence;
    sAftImgInfo.mSize   = aSize;

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID( aOIDTable ),
                  aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_SET_SEQUENCE,
                              sGRID,
                              aOIDTable, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* SMR_SMC_TABLEHEADER_SET_SEGSTOATTR         */
IDE_RC smrUpdate::updateSegStoAttrAtTableHead(idvSQL            * aStatistics,
                                              void              * aTrans,
                                              void              * aTable,
                                              smiSegStorageAttr   aBfrSegStoAttr,
                                              smiSegStorageAttr   aAftSegStoAttr )
{
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;
    scGRID           sGRID;
    scPageID         sPageID;
    scOffset         sOffset;
    UInt             sTableFlag;

    // 테이블 헤더의 mFlag 얻기
    (void)smLayerCallback::getTableHeaderFlag( aTable,
                                               &sPageID,
                                               &sOffset,
                                               &sTableFlag );

    sBfrImgInfo.mLogImg = (SChar*) &aBfrSegStoAttr;
    sBfrImgInfo.mSize   = ID_SIZEOF(smiSegStorageAttr);

    sAftImgInfo.mLogImg = (SChar*) &aAftSegStoAttr;
    sAftImgInfo.mSize   = ID_SIZEOF(smiSegStorageAttr);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  sOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_SET_SEGSTOATTR,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


/* SMR_SMC_TABLEHEADER_SET_INSERTLIMIT         */
IDE_RC smrUpdate::updateInsLimitAtTableHead(idvSQL            * aStatistics,
                                            void              * aTrans,
                                            void              * aTable,
                                            smiSegAttr         aBfrSegAttr,
                                            smiSegAttr         aAftSegAttr )
{
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;
    scGRID           sGRID;
    scPageID         sPageID;
    scOffset         sOffset;
    UInt             sTableFlag;

    // 테이블 헤더의 mFlag 얻기
    (void)smLayerCallback::getTableHeaderFlag( aTable,
                                               &sPageID,
                                               &sOffset,
                                               &sTableFlag );

    sBfrImgInfo.mLogImg = (SChar*) &aBfrSegAttr;
    sBfrImgInfo.mSize   = ID_SIZEOF(smiSegAttr);

    sAftImgInfo.mLogImg = (SChar*) &aAftSegAttr;
    sAftImgInfo.mSize   = ID_SIZEOF(smiSegAttr);

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  sOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_SET_INSERTLIMIT,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*************************************************************************
 * PROJ-2162 RestartRiskReduction
 *
 * Description:
 *      해당 Table을 Inconsistent하다고 Logging한다.
 *
 * Usecase:
 *      1) No-logging DML
 *         NoLogging DPathInsert 등을 수행했을때, 해당 영역은 Flush로 Dur-
 *         ability를 보장받는다. 따라서 그 이전부터 Recovery하는 MediaRec-
 *         overy 등의 경우, Restore한 DBImage에도 데이터가 없고 Log에도 없
 *         기 때문에, Table은 Invalid해진다.
 *          따라서 MediaRecovery시에만 동작하는 Redo를 수행한다.
 *      2) Detect table inconsistency.
 *         Table에 Inconsistent함을 발견했을때이다. 이때는 다른 Trans-
 *         action이 건드리는 오류를 막기 위해, Table을 Inconsistent함을
 *         설정한다.
 *          따라서 Restart/Media recovery 언제든 항상 Redo를 수행한다.
 *
 * aStatistics       - [IN] 통계정보
 * aTrans            - [IN] Log를 남기는 Transaction
 * aTable            - [IN] Inconsistent하다고 설정할 테이블
 * aForMediaRecovery - [IN] MediaRecovery시에만 수행할 것인가?
 *
 *************************************************************************/
/* Update type: SMR_SMC_TABLEHEADER_SET_INCONSISTENCY        */
IDE_RC smrUpdate::setInconsistencyAtTableHead( void   * aTable,
                                               idBool   aForMediaRecovery )
{
    scGRID             sGRID;
    scPageID           sPageID;
    scOffset           sOffset;
    idBool             sOldFlag;

    // 테이블 헤더의 mFlag 위치 얻기
    (void)smLayerCallback::getTableIsConsistent( aTable,
                                                 &sPageID,
                                                 &sOffset,
                                                 &sOldFlag );

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  sOffset );

    IDE_TEST( writeDummyUpdateLog( SMR_SMC_TABLEHEADER_SET_INCONSISTENCY,
                                   sGRID,
                                   aForMediaRecovery, /*smrUpdateLog::mData*/
                                   ID_FALSE ) // IsConsistent
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_INDEX_SET_FLAG             */
IDE_RC smrUpdate::setIndexHeaderFlag(idvSQL*     aStatistics,
                                     void*       aTrans,
                                     smOID       aOIDIndex,
                                     scOffset    aOffset,
                                     UInt        aBeforeFlag,
                                     UInt        aAfterFlag)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*)&aBeforeFlag;
    sBfrImgInfo.mSize   = ID_SIZEOF( UInt );

    sAftImgInfo.mLogImg = (SChar*)&aAfterFlag;
    sAftImgInfo.mSize   = ID_SIZEOF( UInt );

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aOIDIndex),
                  SM_MAKE_OFFSET(aOIDIndex) + aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_INDEX_SET_FLAG,
                              sGRID,
                              aOIDIndex, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Update type: SMR_SMC_INDEX_SET_SEGSTOATTR  */
IDE_RC smrUpdate::setIdxHdrSegStoAttr(idvSQL*           aStatistics,
                                      void*             aTrans,
                                      smOID             aOIDIndex,
                                      scOffset          aOffset,
                                      smiSegStorageAttr aBfrSegStoAttr,
                                      smiSegStorageAttr aAftSegStoAttr )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*)&aBfrSegStoAttr;
    sBfrImgInfo.mSize   = ID_SIZEOF( smiSegStorageAttr );

    sAftImgInfo.mLogImg = (SChar*)&aAftSegStoAttr;
    sAftImgInfo.mSize   = ID_SIZEOF( smiSegStorageAttr );

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aOIDIndex),
                  SM_MAKE_OFFSET(aOIDIndex) + aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_INDEX_SET_SEGSTOATTR,
                              sGRID,
                              aOIDIndex, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_INDEX_SET_SEGATTR  */
IDE_RC smrUpdate::setIdxHdrSegAttr(idvSQL     * aStatistics,
                                   void       * aTrans,
                                   smOID        aOIDIndex,
                                   scOffset     aOffset,
                                   smiSegAttr   aBfrSegAttr,
                                   smiSegAttr   aAftSegAttr )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*)&aBfrSegAttr;
    sBfrImgInfo.mSize   = ID_SIZEOF( smiSegAttr );

    sAftImgInfo.mLogImg = (SChar*)&aAftSegAttr;
    sAftImgInfo.mSize   = ID_SIZEOF( smiSegAttr );

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aOIDIndex),
                  SM_MAKE_OFFSET(aOIDIndex) + aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_INDEX_SET_SEGATTR,
                              sGRID,
                              aOIDIndex, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_PERS_SET_INCONSISTENCY         */
IDE_RC smrUpdate::setPersPageInconsistency(scSpaceID    aSpaceID,
                                           scPageID     aPageID,
                                           smpPageType  aBeforePageType )
{
    scGRID           sGRID;
    smpPageType      sAfterPageType;

    /* 이전에 Inconsistent 했건 Consistent했건 상관없이 무조건 Inconsistent
     * Flag를 설정함 */
    sAfterPageType = ( aBeforePageType & ~SMP_PAGEINCONSISTENCY_MASK )
        | SMP_PAGEINCONSISTENCY_TRUE;

    SC_MAKE_GRID( sGRID, aSpaceID, aPageID, 0 );

    IDE_TEST( writeDummyUpdateLog(  SMR_SMC_PERS_SET_INCONSISTENCY,
                                    sGRID,
                                    0, /* smrUpdateLog::mData */
                                    sAfterPageType )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* Update type: SMR_SMC_PERS_INIT_FIXED_PAGE           */
/* Redo Only                                           */
/* After Image: SlotSize | SlotCount | AllocPageListID */
IDE_RC smrUpdate::initFixedPage(idvSQL*   aStatistics,
                                void*     aTrans,
                                scSpaceID aSpaceID,
                                scPageID  aPageID,
                                UInt      aPageListID,
                                vULong    aSlotSize,
                                vULong    aSlotCount,
                                smOID     aTableOID)
{
    scGRID           sGRID;
    smrUptLogImgInfo sAftImgInfo[4];

    sAftImgInfo[0].mLogImg = (SChar*) &aSlotSize;
    sAftImgInfo[0].mSize   = ID_SIZEOF( vULong );
    sAftImgInfo[1].mLogImg = (SChar*) &aSlotCount;
    sAftImgInfo[1].mSize   = ID_SIZEOF( vULong );
    sAftImgInfo[2].mLogImg = (SChar*) &aPageListID;
    sAftImgInfo[2].mSize   = ID_SIZEOF( UInt );
    sAftImgInfo[3].mLogImg = (SChar*) &aTableOID;
    sAftImgInfo[3].mSize   = ID_SIZEOF( smOID );

    SC_MAKE_GRID( sGRID, aSpaceID, aPageID, 0 );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_INIT_FIXED_PAGE,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              4, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW                    */
IDE_RC smrUpdate::updateFixedRowHead(idvSQL*          aStatistics,
                                     void*            aTrans,
                                     scSpaceID        aSpaceID,
                                     smOID            aOID,
                                     void*            aBeforeSlotHeader,
                                     void*            aAfterSlotHeader,
                                     UInt             aSize)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    IDE_ERROR( SM_MAKE_PID(aOID) != 0 );

    sBfrImgInfo.mLogImg = (SChar*) aBeforeSlotHeader;
    sBfrImgInfo.mSize   = aSize;

    sAftImgInfo.mLogImg = (SChar*) aAfterSlotHeader;
    sAftImgInfo.mSize   = aSize;

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_UPDATE_FIXED_ROW,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION     */
IDE_RC smrUpdate::updateNextVersionAtFixedRow(idvSQL*    aStatistics,
                                              void*      aTrans,
                                              scSpaceID  aSpaceID,
                                              smOID      aTableOID,
                                              smOID      aOID,
                                              ULong      aBeforeNext,
                                              ULong      aAfterNext)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*) &aBeforeNext;
    sBfrImgInfo.mSize   = ID_SIZEOF( ULong );

    sAftImgInfo.mLogImg = (SChar*) &aAfterNext;
    sAftImgInfo.mSize   = ID_SIZEOF( ULong );

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION,
                              sGRID,
                              aTableOID, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Update Type: SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG                */
IDE_RC smrUpdate::setDropFlagAtFixedRow( idvSQL*         aStatistics,
                                         void*           aTrans,
                                         scSpaceID       aSpaceID,
                                         smOID           aOID,
                                         idBool          aDrop )
{
    scGRID sGRID;

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG,
                              sGRID,
                              aDrop, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              0, /* After Image Count */
                              NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update Type: SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT                */
IDE_RC smrUpdate::setDeleteBitAtFixRow(idvSQL*    aStatistics,
                                       void*      aTrans,
                                       scSpaceID  aSpaceID,
                                       smOID      aOID)
{
    scGRID sGRID;

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              0, /* After Image Count */
                              NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_PERS_INIT_VAR_PAGE                      */
/* Redo Only                                                    */
/* After Image: VarIdx | SlotSize | SlotCount | AllocPageListID */
IDE_RC smrUpdate::initVarPage(idvSQL*    aStatistics,
                              void*      aTrans,
                              scSpaceID  aSpaceID,
                              scPageID   aPageID,
                              UInt       aPageListID,
                              vULong     aIdx,
                              vULong     aSlotSize,
                              vULong     aSlotCount,
                              smOID      aTableOID)
{
    scGRID           sGRID;
    smrUptLogImgInfo sAftImgInfo[5];

    sAftImgInfo[0].mLogImg = (SChar*) &aIdx;
    sAftImgInfo[0].mSize   = ID_SIZEOF( vULong );
    sAftImgInfo[1].mLogImg = (SChar*) &aSlotSize;
    sAftImgInfo[1].mSize   = ID_SIZEOF( vULong );
    sAftImgInfo[2].mLogImg = (SChar*) &aSlotCount;
    sAftImgInfo[2].mSize   = ID_SIZEOF( vULong );
    sAftImgInfo[3].mLogImg = (SChar*) &aPageListID;
    sAftImgInfo[3].mSize   = ID_SIZEOF( UInt );
    sAftImgInfo[4].mLogImg = (SChar*) &aTableOID;
    sAftImgInfo[4].mSize   = ID_SIZEOF( smOID );

    SC_MAKE_GRID( sGRID, aSpaceID, aPageID, 0 );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_INIT_VAR_PAGE,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              0, /* Before Image Count */
                              NULL, /* Before Image Info Array */
                              5, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD       */
IDE_RC smrUpdate::updateVarRowHead(idvSQL*           aStatistics,
                                   void*             aTrans,
                                   scSpaceID         aSpaceID,
                                   smOID             aOID,
                                   smVCPieceHeader*  aBeforeVCPieceHeader,
                                   smVCPieceHeader*  aAfterVCPieceHeader)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    IDE_ERROR( SM_MAKE_PID(aOID) != 0 );

    sBfrImgInfo.mLogImg = (SChar*) aBeforeVCPieceHeader;
    sBfrImgInfo.mSize   = ID_SIZEOF( smVCPieceHeader );

    sAftImgInfo.mLogImg = (SChar*) aAfterVCPieceHeader;
    sAftImgInfo.mSize   = ID_SIZEOF( smVCPieceHeader );

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_UPDATE_VAR_ROW                 */
IDE_RC smrUpdate::updateVarRow(idvSQL*         aStatistics,
                               void*           aTrans,
                               scSpaceID       aSpaceID,
                               smOID           aOID,
                               SChar*          aAfterVarRowData,
                               UShort          aAfterRowSize)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo[2];
    smrUptLogImgInfo sAftImgInfo[2];

    SChar*           sBeforeImage;
    UShort           sBeforeRowSize;

    IDE_ERROR( SM_MAKE_PID(aOID) != 0 );

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       aOID,
                                       (void**)&sBeforeImage )
                == IDE_SUCCESS );
    sBeforeRowSize = ((smVCPieceHeader*)sBeforeImage)->length;

    sBfrImgInfo[0].mLogImg = (SChar*) &sBeforeRowSize;
    sBfrImgInfo[0].mSize   = ID_SIZEOF(UShort);
    sBfrImgInfo[1].mLogImg = sBeforeImage + ID_SIZEOF( smVCPieceHeader );
    sBfrImgInfo[1].mSize   = sBeforeRowSize;

    sAftImgInfo[0].mLogImg = (SChar*) &aAfterRowSize;
    sAftImgInfo[0].mSize   = ID_SIZEOF(UShort);
    sAftImgInfo[1].mLogImg = aAfterVarRowData;
    sAftImgInfo[1].mSize   = aAfterRowSize;

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_UPDATE_VAR_ROW,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              2, /* Before Image Count */
                              sBfrImgInfo,
                              2, /* After Image Count */
                              sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_SET_VAR_ROW_FLAG         */
IDE_RC smrUpdate::setFlagAtVarRow(idvSQL    * aStatistics,
                                  void      * aTrans,
                                  scSpaceID   aSpaceID,
                                  smOID       aTableOID,
                                  smOID       aOID,
                                  UShort      aBFlag,
                                  UShort      aAFlag)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    IDE_ERROR( SM_MAKE_PID(aOID) != 0 );

    sBfrImgInfo.mLogImg = (SChar*) &aBFlag;
    sBfrImgInfo.mSize   = ID_SIZEOF(UShort);

    sAftImgInfo.mLogImg = (SChar*) &aAFlag;
    sAftImgInfo.mSize   = ID_SIZEOF(UShort);

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_SET_VAR_ROW_FLAG,
                              sGRID,
                              aTableOID, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type: SMR_SMC_INDEX_SET_DROP_FLAG  */
IDE_RC smrUpdate::setIndexDropFlag(idvSQL    * aStatistics,
                                   void      * aTrans,
                                   smOID       aTableOID,
                                   smOID       aIndexOID,
                                   UShort      aBFlag,
                                   UShort      aAFlag )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    sBfrImgInfo.mLogImg = (SChar*)&aBFlag;
    sBfrImgInfo.mSize   = ID_SIZEOF( UShort );

    sAftImgInfo.mLogImg = (SChar*)&aAFlag;
    sAftImgInfo.mSize   = ID_SIZEOF( UShort );

    SC_MAKE_GRID( sGRID,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aIndexOID),
                  SM_MAKE_OFFSET(aIndexOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_INDEX_SET_DROP_FLAG,
                              sGRID,
                              aTableOID, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_PERS_SET_VAR_ROW_NXT_OID      */
IDE_RC smrUpdate::setNxtOIDAtVarRow(idvSQL*   aStatistics,
                                    void*     aTrans,
                                    scSpaceID aSpaceID,
                                    smOID     aTableOID,
                                    smOID     aOID,
                                    smOID     aBOID,
                                    smOID     aAOID)
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    IDE_ERROR( SM_MAKE_PID( aOID ) != 0 );

    sBfrImgInfo.mLogImg = (SChar*) &aBOID;
    sBfrImgInfo.mSize   = ID_SIZEOF( smOID );

    sAftImgInfo.mLogImg = (SChar*) &aAOID;
    sAftImgInfo.mSize   = ID_SIZEOF( smOID );

    SC_MAKE_GRID( sGRID, aSpaceID, SM_MAKE_PID(aOID), SM_MAKE_OFFSET(aOID) );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_PERS_SET_VAR_ROW_NXT_OID,
                              sGRID,
                              aTableOID, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Update type:  SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT         */
IDE_RC smrUpdate::updateTableSegPIDAtTableHead(idvSQL*      aStatistics,
                                               void*        aTrans,
                                               scPageID     aPageID,
                                               scOffset     aOffset,
                                               void*        aPageListEntry,
                                               scPageID     aAfterSegPID )
{
    scGRID           sGRID;
    smrUptLogImgInfo sBfrImgInfo;
    smrUptLogImgInfo sAftImgInfo;

    scPageID         sSegPID;

    /* Before img : segment rid 와 meta page id 정보 얻기 */
    (void)smLayerCallback::getDiskPageEntryInfo( aPageListEntry,
                                                 &sSegPID );

    sBfrImgInfo.mLogImg = (SChar*) &sSegPID;
    sBfrImgInfo.mSize   = ID_SIZEOF( scPageID );

    sAftImgInfo.mLogImg = (SChar*) &aAfterSegPID;
    sAftImgInfo.mSize   = ID_SIZEOF( scPageID );

    SC_MAKE_GRID( sGRID, SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, aPageID, aOffset );

    IDE_TEST( writeUpdateLog( aStatistics,
                              aTrans,
                              SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT,
                              sGRID,
                              0, /* smrUpdateLog::mData */
                              1, /* Before Image Count */
                              &sBfrImgInfo,
                              1, /* After Image Count */
                              &sAftImgInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *DESCRIPTION :
 ***********************************************************************/
IDE_RC smrUpdate::writeNTAForExtendDBF( idvSQL*  aStatistics,
                                        void*    aTrans,
                                        smLSN*   aLsnNTA )
{
    smLSN                sDummyLSN;
    smuDynArrayBase      sBuffer;

    IDE_ERROR( aTrans != NULL );
    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeDiskNTALogRec( aStatistics,
                                             aTrans,
                                             &sBuffer,
                                             SMR_DEFAULT_DISK_LOG_WRITE_OPTION,
                                             SDR_OP_NULL,
                                             aLsnNTA,
                                             SC_NULL_SPACEID,
                                             NULL,
                                             0,
                                             &sDummyLSN,
                                             &sDummyLSN,
                                             SM_NULL_OID )
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*
    Memory Tablespace Create에 대한 로깅

    [IN] aTrans      - 로깅하려는 Transaction 객체
    [IN] aSpaceID    - Create하려는 Tablespace의 ID

// BUGBUG-1548-M3 Media Recovery를 위해 TBS생성시의 LSN을
//                Log Anchor및 Checkpoint Image File에 기록해야함.
 */
IDE_RC smrUpdate::writeMemoryTBSCreate( idvSQL                * aStatistics,
                                        void                  * aTrans,
                                        smiTableSpaceAttr     * aTBSAttr,
                                        smiChkptPathAttrList  * aChkptPathAttrList )
{
    UInt                    sAImgSize       = 0;
    UInt                    sBImgSize       = 0;
    smuDynArrayBase         sBuffer;
    UInt                    sState          = 0;
    UInt                    sChkptPathCount = 0;
    smiChkptPathAttrList  * sCPAttrList     = NULL;

    IDE_ERROR( aTrans     != NULL );

    /* 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
     * 그러나, CREATE Tablespace의 경우 aSpaceID에 해당하는 TBSNode가
     * 아직 없는 상태에서 이 함수를 이용하여 로깅한다.
     *  => 여기에서 Memory Tablespace인지 여부를 검사해서는 안됨 */
    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;

    sAImgSize = ID_SIZEOF(smiTableSpaceAttr);
    sBImgSize = 0;

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 aTBSAttr,
                                 ID_SIZEOF(smiTableSpaceAttr) )
              != IDE_SUCCESS );

    if( aChkptPathAttrList == NULL )
    {
        // 입력한 check point path가 없어, 기본값을 사용할 때
        sChkptPathCount = 0;
    }
    else
    {
        // 입력한 check point path 가 있을 때
        sChkptPathCount = 0;
        sCPAttrList     = aChkptPathAttrList;

        while( sCPAttrList != NULL )
        {
            sChkptPathCount++;

            sCPAttrList = sCPAttrList->mNext;
        } // end of while
    }

    sAImgSize += ID_SIZEOF(UInt);
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 &sChkptPathCount,
                                 ID_SIZEOF(UInt) )
              != IDE_SUCCESS );

    if( aChkptPathAttrList != NULL )
    {
        sCPAttrList = aChkptPathAttrList;

        while( sCPAttrList != NULL )
        {
            sAImgSize += ID_SIZEOF(smiChkptPathAttrList);

            /* BUG-38621
             * 절대경로를 상대경로로 */
            if ( smuProperty::getRELPathInLog() == ID_TRUE )
            {
                IDE_TEST( sctTableSpaceMgr::makeRELPath( sCPAttrList->mCPathAttr.mChkptPath,
                                                         NULL,
                                                         SMI_TBS_MEMORY )
                          != IDE_SUCCESS );
            }

            IDE_TEST( smuDynArray::store(&sBuffer,
                                         sCPAttrList,
                                         ID_SIZEOF(smiChkptPathAttrList) )
                      != IDE_SUCCESS );

            sCPAttrList = sCPAttrList->mNext;
        } // end of while
    }
    else
    {
        // do nothing
    }

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aTBSAttr->mID, // aSpaceID,
                                            ID_UINT_MAX,   // file ID
                                            SCT_UPDATE_MRDB_CREATE_TBS,
                                            sAImgSize,
                                            sBImgSize,
                                            NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
    Memory Tablespace DB File 생성에 대한 로깅

    Create Tablespace의  Undo시 DB File을 지우기 위한 목적이므로
    Create Tablespace도중 생성되는 DB File에 대해서만 로깅한다.

    [IN] aTrans      - 로깅하려는 Transaction 객체
    [IN] aSpaceID    - Create하려는 Tablespace의 ID
    [IN] aPingPongNo - Ping Pong Number
    [IN] aDBFileNo   - DB File Number

    [로그 구조]
    Before Image 기록 ----------------------------------------
      UInt - Ping Pong Number
      UInt - DB File Number
 */
IDE_RC smrUpdate::writeMemoryDBFileCreate( idvSQL*             aStatistics,
                                           void*               aTrans,
                                           scSpaceID           aSpaceID,
                                           UInt                aPingPongNo,
                                           UInt                aDBFileNo )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    // 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
    // 그러나,
    // CREATE Tablespace의 경우 aSpaceID에 해당하는 TBSNode가
    // 아직 없는 상태에서 이 함수를 이용하여 로깅한다.
    // => 여기에서 Memory Tablespace인지 여부를 검사해서는 안됨

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;

    sAImgSize = 0;
    sBImgSize = ID_SIZEOF(aPingPongNo) + ID_SIZEOF(aDBFileNo);

    /*
      Before Image 기록 ----------------------------------------
      UInt - Ping Pong Number
      UInt - DB File Number
    */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 & aPingPongNo,
                                 ID_SIZEOF(aPingPongNo) )
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 & aDBFileNo,
                                 ID_SIZEOF(aDBFileNo) )
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            ID_UINT_MAX, // file ID
                                            SCT_UPDATE_MRDB_CREATE_CIMAGE_FILE,
                                            sAImgSize,
                                            sBImgSize,
                                            NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
    Memory Tablespace Drop에 대한 로깅

    [IN] aTrans      - 로깅하려는 Transaction 객체
    [IN] aSpaceID    - Drop하려는 Tablespace의 ID
    [IN] aTouchMode  - Drop시 Checkpoint Image File까지 삭제할지 여부

    [로그 구조]
    After Image 기록 ----------------------------------------
       smiTouchMode  aTouchMode

*/
IDE_RC smrUpdate::writeMemoryTBSDrop( idvSQL*             aStatistics,
                                      void*               aTrans,
                                      scSpaceID           aSpaceID,
                                      smiTouchMode        aTouchMode )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans != NULL );

    // 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
    IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( aSpaceID )
               == ID_TRUE );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;

    sAImgSize = ID_SIZEOF(aTouchMode);
    sBImgSize = 0;

    //////////////////////////////////////////////////////////////
    // After Image 기록 ----------------------------------------
    //   smiTouchMode  aTouchMode
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aTouchMode)),
                                 ID_SIZEOF(aTouchMode))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            ID_UINT_MAX, // file ID
                                            SCT_UPDATE_MRDB_DROP_TBS,
                                            sAImgSize,
                                            sBImgSize,
                                            NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Volatile Tablespace Create 및 Drop에 대한 로깅

    [IN] aTrans      - 로깅하려는 Transaction 객체
    [IN] aSpaceID    - Drop하려는 Tablespace의 ID
*/
IDE_RC smrUpdate::writeVolatileTBSCreate( idvSQL*           aStatistics,
                                          void*             aTrans,
                                          scSpaceID         aSpaceID )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans != NULL );

    // 여기 들어오는 Tablespace는 항상 Volatile Tablespace여야 한다.
    // CREATE Tablespace의 경우 aSpaceID에 해당하는 TBSNode가
    // 아직 없는 상태에서 이 함수를 이용하여 로깅한다.
    // => 여기에서 Volatile Tablespace인지 여부를 검사해서는 안됨

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;

    sAImgSize = 0;
    sBImgSize = 0;

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            ID_UINT_MAX, // file ID
                                            SCT_UPDATE_VRDB_CREATE_TBS,
                                            sAImgSize,
                                            sBImgSize,
                                            NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Volatile Tablespace Drop에 대한 로깅

    [IN] aTrans      - 로깅하려는 Transaction 객체
    [IN] aSpaceID    - Drop하려는 Tablespace의 ID

    [로그 구조]
    After Image 기록 ----------------------------------------
       smiTouchMode  aTouchMode

*/
IDE_RC smrUpdate::writeVolatileTBSDrop( idvSQL*           aStatistics,
                                        void*             aTrans,
                                        scSpaceID         aSpaceID )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    // 여기 들어오는 Tablespace는 항상 Volatile Tablespace여야 한다.
    IDE_ERROR( sctTableSpaceMgr::isVolatileTableSpace( aSpaceID )
                 == ID_TRUE );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;

    sAImgSize = 0;
    sBImgSize = 0;

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            ID_UINT_MAX, // file ID
                                            SCT_UPDATE_VRDB_DROP_TBS,
                                            sAImgSize,
                                            sBImgSize,
                                            NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Memory Tablespace 의 ALTER AUTO EXTEND ... 에 대한 로깅

    [IN] aTrans          - 로깅하려는 Transaction 객체
    [IN] sSpaceID        - ALTER할 Tablespace의 ID
    [IN] aBIsAutoExtend  - Before Image : AutoExtend 여부
    [IN] aBNextPageCount - Before Image : Next Page Count
    [IN] aBMaxPageCount  - Before Image : Max Page Count
    [IN] aAIsAutoExtend  - After Image : AutoExtend 여부
    [IN] aANextPageCount - After Image : Next Page Count
    [IN] aAMaxPageCount  - After Image : Max Page Count

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      idBool              aBIsAutoExtend
      scPageID            aBNextPageCount
      scPageID            aBMaxPageCount
    After Image   --------------------------------------------
      idBool              aAIsAutoExtend
      scPageID            aANextPageCount
      scPageID            aAMaxPageCount

 */
IDE_RC smrUpdate::writeMemoryTBSAlterAutoExtend(
                      idvSQL*           aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      idBool              aBIsAutoExtend,
                      scPageID            aBNextPageCount,
                      scPageID            aBMaxPageCount,
                      idBool              aAIsAutoExtend,
                      scPageID            aANextPageCount,
                      scPageID            aAMaxPageCount )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    // 여기 들어오는 Tablespace는 항상 Memory Tablespace여야 한다.
    IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( aSpaceID )
               == ID_TRUE );


    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;


    sAImgSize = ID_SIZEOF(aAIsAutoExtend)  +
                ID_SIZEOF(aANextPageCount) +
                ID_SIZEOF(aAMaxPageCount)  ;

    sBImgSize = ID_SIZEOF(aBIsAutoExtend)  +
                ID_SIZEOF(aBNextPageCount) +
                ID_SIZEOF(aBMaxPageCount)  ;

    //////////////////////////////////////////////////////////////
    // Before Image 기록 ----------------------------------------
    //   idBool              aBIsAutoExtend
    //   scPageID            aBNextPageCount
    //   scPageID            aBMaxPageCount

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBIsAutoExtend)),
                                 ID_SIZEOF(aBIsAutoExtend))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBNextPageCount)),
                                 ID_SIZEOF(aBNextPageCount))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBMaxPageCount)),
                                 ID_SIZEOF(aBMaxPageCount))
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////
    // After Image 기록 ----------------------------------------
    //   idBool              aAIsAutoExtend
    //   scPageID            aANextPageCount
    //   scPageID            aAMaxPageCount

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAIsAutoExtend)),
                                 ID_SIZEOF(aAIsAutoExtend))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aANextPageCount)),
                                 ID_SIZEOF(aANextPageCount))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAMaxPageCount)),
                                 ID_SIZEOF(aAMaxPageCount))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec(
                             aStatistics,
                             aTrans,
                             &sBuffer,
                             aSpaceID,
                             ID_UINT_MAX, // file ID
                             SCT_UPDATE_MRDB_ALTER_AUTOEXTEND,
                             sAImgSize,
                             sBImgSize,
                             NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/* PROJ-1594 Volatile TBS */
/* writeMemoryTBSAlterAutoExtend와 동일한 함수임 */
IDE_RC smrUpdate::writeVolatileTBSAlterAutoExtend(
                      idvSQL*             aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      idBool              aBIsAutoExtend,
                      scPageID            aBNextPageCount,
                      scPageID            aBMaxPageCount,
                      idBool              aAIsAutoExtend,
                      scPageID            aANextPageCount,
                      scPageID            aAMaxPageCount )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    // 여기 들어오는 Tablespace는 항상 Volatile Tablespace여야 한다.
    IDE_ERROR( sctTableSpaceMgr::isVolatileTableSpace( aSpaceID )
               == ID_TRUE );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;


    sAImgSize = ID_SIZEOF(aAIsAutoExtend)  +
                ID_SIZEOF(aANextPageCount) +
                ID_SIZEOF(aAMaxPageCount)  ;

    sBImgSize = ID_SIZEOF(aBIsAutoExtend)  +
                ID_SIZEOF(aBNextPageCount) +
                ID_SIZEOF(aBMaxPageCount)  ;

    //////////////////////////////////////////////////////////////
    // Before Image 기록 ----------------------------------------
    //   idBool              aBIsAutoExtend
    //   scPageID            aBNextPageCount
    //   scPageID            aBMaxPageCount

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBIsAutoExtend)),
                                 ID_SIZEOF(aBIsAutoExtend))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBNextPageCount)),
                                 ID_SIZEOF(aBNextPageCount))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBMaxPageCount)),
                                 ID_SIZEOF(aBMaxPageCount))
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////
    // After Image 기록 ----------------------------------------
    //   idBool              aAIsAutoExtend
    //   scPageID            aANextPageCount
    //   scPageID            aAMaxPageCount

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAIsAutoExtend)),
                                 ID_SIZEOF(aAIsAutoExtend))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aANextPageCount)),
                                 ID_SIZEOF(aANextPageCount))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAMaxPageCount)),
                                 ID_SIZEOF(aAMaxPageCount))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec(
                             aStatistics,
                             aTrans,
                             &sBuffer,
                             aSpaceID,
                             ID_UINT_MAX, // file ID
                             SCT_UPDATE_VRDB_ALTER_AUTOEXTEND,
                             sAImgSize,
                             sBImgSize,
                             NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
    Tablespace Attribute Flag의 변경에 대한 로깅
    (Ex> ALTER Tablespace Log Compress ON/OFF )

    [IN] aTrans          - 로깅하려는 Transaction 객체
    [IN] aSpaceID        - ALTER할 Tablespace의 ID
    [IN] aBeforeAttrFlag - Before Image : Tablespace의 Attribute Flag
    [IN] aAfterAttrFlag - Before Image : Tablespace의 Attribute Flag

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBeforeAttrFlag
    After Image   --------------------------------------------
      UInt                aAfterAttrFlag

 */
IDE_RC smrUpdate::writeTBSAlterAttrFlag(
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      UInt                aBeforeAttrFlag,
                      UInt                aAfterAttrFlag )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;


    sAImgSize = ID_SIZEOF(aAfterAttrFlag);

    sBImgSize = ID_SIZEOF(aBeforeAttrFlag);


    //////////////////////////////////////////////////////////////
    // Before Image 기록 ----------------------------------------
    //   UInt                aBeforeAttrFlag
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBeforeAttrFlag)),
                                 ID_SIZEOF(aBeforeAttrFlag))
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////
    // After Image 기록 ----------------------------------------
    //   UInt                aAfterAttrFlag
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAfterAttrFlag)),
                                 ID_SIZEOF(aAfterAttrFlag))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec(
                             NULL, /* idvSQL* */
                             aTrans,
                             &sBuffer,
                             aSpaceID,
                             ID_UINT_MAX, // file ID
                             SCT_UPDATE_COMMON_ALTER_ATTR_FLAG,
                             sAImgSize,
                             sBImgSize,
                             NULL ) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
    Tablespace 의 ALTER ONLINE/OFFLINE ... 에 대한 로깅

    [IN] aTrans          - 로깅하려는 Transaction 객체
    [IN] aSpaceID        - ALTER할 Tablespace의 ID
    [IN] aUpdateType     - Alter Tablespace Online인지 Offline인지 여부
    [IN] aBState         - Before Image : Tablespace의 State
    [IN] aAState         - After Image : Tablespace의 State

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBState
    After Image   --------------------------------------------
      UInt                aAState

 */
IDE_RC smrUpdate::writeTBSAlterOnOff(
                      idvSQL*           aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      sctUpdateType       aUpdateType,
                      UInt                aBState,
                      UInt                aAState,
                      smLSN*              aBeginLSN )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;


    sAImgSize = ID_SIZEOF(aBState);


    sBImgSize = ID_SIZEOF(aAState);


    //////////////////////////////////////////////////////////////
    // Before Image 기록 ----------------------------------------
    //   UInt                aBState
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBState)),
                                 ID_SIZEOF(aBState))
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////
    // After Image 기록 ----------------------------------------
    //   UInt                aAState
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAState)),
                                 ID_SIZEOF(aAState))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec(
                             aStatistics,
                             aTrans,
                             &sBuffer,
                             aSpaceID,
                             ID_UINT_MAX, // file ID
                             aUpdateType,
                             sAImgSize,
                             sBImgSize,
                             aBeginLSN ) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
    Disk DataFile의 ALTER ONLINE/OFFLINE ... 에 대한 로깅

    [IN] aTrans          - 로깅하려는 Transaction 객체
    [IN] aSpaceID        - Tablespace의 ID
    [IN] aFileID         - ALTER할 DBF의 ID
    [IN] aUpdateType     - Alter Tablespace Online인지 Offline인지 여부
    [IN] aBState         - Before Image : Tablespace의 State
    [IN] aAState         - After Image : Tablespace의 State

    [ 로그 구조 ]
    Before Image  --------------------------------------------
      UInt                aBState
    After Image   --------------------------------------------
      UInt                aAState

 */
IDE_RC smrUpdate::writeDiskDBFAlterOnOff(
                      idvSQL*           aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      UInt                aFileID,
                      sctUpdateType       aUpdateType,
                      UInt                aBState,
                      UInt                aAState )
{
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    smuDynArrayBase      sBuffer;
    UInt                 sState = 0;

    IDE_ERROR( aTrans     != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;


    sAImgSize = ID_SIZEOF(aBState);
    sBImgSize = ID_SIZEOF(aAState);

    //////////////////////////////////////////////////////////////
    // Before Image 기록 ----------------------------------------
    //   UInt                aBState
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aBState)),
                                 ID_SIZEOF(aBState))
              != IDE_SUCCESS );

    //////////////////////////////////////////////////////////////
    // After Image 기록 ----------------------------------------
    //   UInt                aAState
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAState)),
                                 ID_SIZEOF(aAState))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec(
                             aStatistics,
                             aTrans,
                             &sBuffer,
                             aSpaceID,
                             aFileID,
                             aUpdateType,
                             sAImgSize,
                             sBImgSize,
                             NULL) // Begin LSN
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/***********************************************************************
 * DESCRIPTION : Disk TBS 생성, 제거에 대한 로깅
 ***********************************************************************/
IDE_RC smrUpdate::writeDiskTBSCreateDrop( idvSQL              * aStatistics,
                                          void                * aTrans,
                                          sctUpdateType         aUpdateType,
                                          scSpaceID             aSpaceID,
                                          smiTableSpaceAttr   * aTableSpaceAttr,
                                          smLSN               * aBeginLSN )
{
    UInt                 sState = 0;
    UInt                 sAImgSize;
    UInt                 sBImgSize;
    scSpaceID            sSpaceID;
    smuDynArrayBase      sBuffer;

    IDE_ERROR( aTrans != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sState = 1;

    switch(aUpdateType)
    {
        case SCT_UPDATE_DRDB_CREATE_TBS :
            {
                /* aTableSpaceAttr를 After로 저장 */
                IDE_TEST( smuDynArray::store(&sBuffer,
                                             (void *)aTableSpaceAttr,
                                             ID_SIZEOF(smiTableSpaceAttr) )
                          != IDE_SUCCESS );

                sAImgSize = ID_SIZEOF(smiTableSpaceAttr);   /* PROJ-1923 */
                sBImgSize = 0;
                sSpaceID = aSpaceID;
                break;
            }
        case SCT_UPDATE_DRDB_DROP_TBS :
            {
                sAImgSize = 0;
                sBImgSize = 0;
                sSpaceID = aSpaceID;
                break;
            }
        default :
            {
                IDE_ERROR( 0 );
                break;
            }
    }

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            sSpaceID,
                                            ID_UINT_MAX, // file ID
                                            aUpdateType,
                                            sAImgSize,
                                            sBImgSize,
                                            aBeginLSN )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 1:
            IDE_ASSERT( smuDynArray::destroy(&sBuffer) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeLogCreateDBF(idvSQL            * aStatistics,
                                    void              * aTrans,
                                    scSpaceID           aSpaceID,
                                    sddDataFileNode   * aFileNode,
                                    smiTouchMode        aTouchMode,
                                    smiDataFileAttr   * aFileAttr,  /* PROJ-1923 */
                                    smLSN             * aBeginLSN )
{
    smuDynArrayBase     sBuffer;
    SInt                sBufferInit     = 0;
    UInt                sAImgSize       = 0;

    IDE_ERROR( aFileNode != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sBufferInit = 1;

    /* Before CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)&aTouchMode,
                                 ID_SIZEOF(smiTouchMode))
              != IDE_SUCCESS );

    /* After CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)&aTouchMode,
                                 ID_SIZEOF(smiTouchMode))
              != IDE_SUCCESS );
    sAImgSize += ID_SIZEOF(smiTouchMode);

    /* BUG-38621
     * 절대경로를 상대경로로 */
    if ( smuProperty::getRELPathInLog() == ID_TRUE )
    {
        IDE_TEST( sctTableSpaceMgr::makeRELPath( aFileAttr->mName,
                                                 &aFileAttr->mNameLength,
                                                 SMI_TBS_DISK )
                  != IDE_SUCCESS );
    }

    /* PROJ-1923
     * store aFileAttr and aFileAttrID */
    /* After CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)aFileAttr,
                                 ID_SIZEOF(smiDataFileAttr))
              != IDE_SUCCESS );
    sAImgSize += ID_SIZEOF(smiDataFileAttr);

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            aFileNode->mID,
                                            SCT_UPDATE_DRDB_CREATE_DBF,
                                            sAImgSize, //ID_SIZEOF(smiTouchMode),
                                            ID_SIZEOF(smiTouchMode),
                                            aBeginLSN )
              != IDE_SUCCESS );

    sBufferInit = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if (sBufferInit == 1)
    {
        IDE_ASSERT(smuDynArray::destroy(&sBuffer) == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeLogDropDBF(idvSQL*             aStatistics,
                                  void               *aTrans,
                                  scSpaceID           aSpaceID,
                                  sddDataFileNode    *aFileNode,
                                  smiTouchMode        aTouchMode,
                                  smLSN              *aBeginLSN)
{
    smuDynArrayBase      sBuffer;
    SInt                 sBufferInit = 0;

    IDE_ERROR( aFileNode != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sBufferInit = 1;

    /* After CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)&aTouchMode,
                                 ID_SIZEOF(smiTouchMode))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            aFileNode->mID,
                                            SCT_UPDATE_DRDB_DROP_DBF,
                                            ID_SIZEOF(smiTouchMode),
                                            0,
                                            aBeginLSN )
              != IDE_SUCCESS );

    sBufferInit = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if (sBufferInit == 1)
    {
        IDE_ASSERT(smuDynArray::destroy(&sBuffer) == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeLogExtendDBF(idvSQL*           aStatistics,
                                    void               *aTrans,
                                    scSpaceID           aSpaceID,
                                    sddDataFileNode    *aFileNode,
                                    ULong               aAfterCurrSize,
                                    smLSN              *aBeginLSN)
{
    smuDynArrayBase      sBuffer;
    ULong                sDiffSize;
    SInt                 sBufferInit = 0;

    IDE_ERROR( aFileNode != NULL );
    IDE_ERROR( aFileNode->mCurrSize <= aAfterCurrSize );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sBufferInit = 1;

    sDiffSize = aAfterCurrSize - aFileNode->mCurrSize;

    /* Before CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aFileNode->mCurrSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    /* After CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAfterCurrSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(sDiffSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            aFileNode->mID,
                                            SCT_UPDATE_DRDB_EXTEND_DBF,
                                            ID_SIZEOF(ULong)*2,
                                            ID_SIZEOF(ULong),
                                            aBeginLSN )
              != IDE_SUCCESS );

    sBufferInit = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if (sBufferInit == 1)
    {
        IDE_ASSERT(smuDynArray::destroy(&sBuffer) == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeLogShrinkDBF(idvSQL*           aStatistics,
                                    void               *aTrans,
                                    scSpaceID           aSpaceID,
                                    sddDataFileNode    *aFileNode,
                                    ULong               aAfterInitSize,
                                    ULong               aAfterCurrSize,
                                    smLSN              *aBeginLSN)
{
    smuDynArrayBase      sBuffer;
    ULong                sDiffSize;
    SInt                 sBufferInit = 0;

    IDE_ERROR( aFileNode != NULL );
    IDE_ERROR( aFileNode->mCurrSize >= aAfterCurrSize );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sBufferInit = 1;

    sDiffSize = aFileNode->mCurrSize - aAfterCurrSize;

    /* Before InitSize & CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aFileNode->mInitSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aFileNode->mCurrSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(sDiffSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    /* After InitSize & CurrSize */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAfterInitSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aAfterCurrSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(sDiffSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            aFileNode->mID,
                                            SCT_UPDATE_DRDB_SHRINK_DBF,
                                            ID_SIZEOF(ULong)*3,
                                            ID_SIZEOF(ULong)*3,
                                            aBeginLSN )
              != IDE_SUCCESS );

    sBufferInit = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if (sBufferInit == 1)
    {
        IDE_ASSERT(smuDynArray::destroy(&sBuffer) == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeLogSetAutoExtDBF(idvSQL*           aStatistics,
                                        void               *aTrans,
                                        scSpaceID           aSpaceID,
                                        sddDataFileNode    *aFileNode,
                                        idBool              aAfterAutoExtMode,
                                        ULong               aAfterNextSize,
                                        ULong               aAfterMaxSize,
                                        smLSN              *aBeginLSN)
{
    smuDynArrayBase      sBuffer;
    SInt                 sBufferInit = 0;

    IDE_ERROR( aFileNode != NULL );

    IDE_TEST( smuDynArray::initialize(&sBuffer) != IDE_SUCCESS );
    sBufferInit = 1;

    /* Before */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aFileNode->mIsAutoExtend)),
                                 ID_SIZEOF(idBool))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aFileNode->mNextSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&(aFileNode->mMaxSize)),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    /* After */
    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&aAfterAutoExtMode),
                                 ID_SIZEOF(idBool))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&aAfterNextSize),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smuDynArray::store(&sBuffer,
                                 (void*)(&aAfterMaxSize),
                                 ID_SIZEOF(ULong))
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeTBSUptLogRec( aStatistics,
                                            aTrans,
                                            &sBuffer,
                                            aSpaceID,
                                            aFileNode->mID,
                                            SCT_UPDATE_DRDB_AUTOEXTEND_DBF,
                                            ID_SIZEOF(ULong)*2 + ID_SIZEOF(idBool),
                                            ID_SIZEOF(ULong)*2 + ID_SIZEOF(idBool),
                                            aBeginLSN )
              != IDE_SUCCESS );

    sBufferInit = 0;
    IDE_TEST( smuDynArray::destroy(&sBuffer) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if (sBufferInit == 1)
    {
        IDE_ASSERT(smuDynArray::destroy(&sBuffer) == IDE_SUCCESS);
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeXaStartReqLog( ID_XID * aGlobalXID,
                                      smTID    aTID,
                                      smLSN  * aLSN )
{
    smrXaStartReqLog   * sXaStartReqLog = NULL;
    SChar              * sLogBuffer = NULL;
    void               * sTrans = NULL;
    UInt                 sLogTypeFlag = 0;
    smLSN                sLSN;
    static smrLogType sLogType = SMR_LT_XA_START_REQ;

    sTrans       = smLayerCallback::getTransByTID( aTID );
    sLogBuffer   = smLayerCallback::getLogBufferOfTrans( sTrans );
    smLayerCallback::initLogBuffer( sTrans );
    sLogTypeFlag = smLayerCallback::getLogTypeFlagOfTrans( sTrans );

    sXaStartReqLog = (smrXaStartReqLog*)sLogBuffer;

    smrLogHeadI::setTransID( &sXaStartReqLog->mHead, aTID );
    smrLogHeadI::setFlag( &sXaStartReqLog->mHead, sLogTypeFlag );
    smrLogHeadI::setType( &sXaStartReqLog->mHead, sLogType );
    smrLogHeadI::setSize( &sXaStartReqLog->mHead, SMR_LOGREC_SIZE( smrXaStartReqLog ) );
    sXaStartReqLog->mTail = SMR_LT_XA_START_REQ;
    idlOS::memcpy( &(sXaStartReqLog->mGlobalXID), aGlobalXID, ID_SIZEOF(ID_XID) );

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   sTrans,
                                   (SChar*)sLogBuffer,
                                   NULL,      // Previous LSN Ptr
                                   &sLSN,     // Log LSN Ptr
                                   NULL,      // End LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );

    *aLSN = sLSN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Prepare 트랜잭션이 사용하던 트랜잭션 세그먼트 정보 복원
 *
 * 트랜잭션 세그먼트의 정보는 원래 Volatile 특성을 가지므로 서버 재구동시에는
 * 초기화되는 것이 일반적이지만, 만약 Prepare 트랜잭션이 존재하는 경우에는
 * 사용하던 그대로 복원해주어야 하기 때문에 로깅을 수행한다.
 *
 * aStatistics         - [IN] 통계정보
 * aTrans              - [IN] 트랜잭션 포인터
 * aXID                - [IN] In-Doubt 트랜잭션 Global ID
 * aLogBuffer          - [IN] 트랜잭션 LogBuffer 포인터
 * aTXSegEntryIdx      - [IN] 트랜잭션 Segment Entry ID
 * aExtRID4TSS         - [IN] TSS를 할당한 Extent RID
 * aFstPIDOfLstExt4TSS - [IN] TSS를 할당한 Extent의 첫번째 PID
 * aFstExtRID4UDS      - [IN] UDS에서 트랜잭션이 사용한 첫번째 Extent RID
 * aLstExtRID4UDS      - [IN] UDS에서 트랜잭션이 사용한 마지막 Extent RID
 * aFstPIDOfLstExt4UDS - [IN] UDS를 할당한 Extent의 첫번째 PID
 * aFstUndoPID         - [IN] 트랜잭션이 사용한 첫번째 Undo PID
 * aLstUndoPID         - [IN] 트랜잭션이 사용한 마지막 Undo PID
 *
 ***********************************************************************/
IDE_RC smrUpdate::writeXaSegsLog( idvSQL      * aStatistics,
                                  void        * aTrans,
                                  ID_XID      * aXID,
                                  SChar       * aLogBuffer,
                                  UInt          aTxSegEntryIdx,
                                  sdRID         aExtRID4TSS,
                                  scPageID      aFstPIDOfLstExt4TSS,
                                  sdRID         aFstExtRID4UDS,
                                  sdRID         aLstExtRID4UDS,
                                  scPageID      aFstPIDOfLstExt4UDS,
                                  scPageID      aFstUndoPID,
                                  scPageID      aLstUndoPID )
{
    smrXaSegsLog   * sXaSegsLog;
    SChar          * sLogBuffer;
    smTID            sTransID;
    UInt             sLogTypeFlag;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aLogBuffer != NULL );

    sLogBuffer = aLogBuffer;
    sXaSegsLog = (smrXaSegsLog*)sLogBuffer;

    smLayerCallback::getTxIDAnLogType( aTrans,
                                       &sTransID,
                                       &sLogTypeFlag);

    smrLogHeadI::setTransID( &sXaSegsLog->mHead, sTransID );
    smrLogHeadI::setSize( &sXaSegsLog->mHead,
                          SMR_LOGREC_SIZE( smrXaSegsLog ) );
    smrLogHeadI::setFlag( &sXaSegsLog->mHead, sLogTypeFlag );
    smrLogHeadI::setType( &sXaSegsLog->mHead, SMR_LT_XA_SEGS );
    if( (smrLogHeadI::getFlag(&sXaSegsLog->mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth(
                        &sXaSegsLog->mHead,
                        smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sXaSegsLog->mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    idlOS::memcpy( &(sXaSegsLog->mXaTransID), aXID, ID_SIZEOF(ID_XID) );

    sXaSegsLog->mTxSegEntryIdx      = aTxSegEntryIdx;
    sXaSegsLog->mExtRID4TSS         = aExtRID4TSS;
    sXaSegsLog->mFstPIDOfLstExt4TSS = aFstPIDOfLstExt4TSS;
    sXaSegsLog->mFstExtRID4UDS      = aFstExtRID4UDS;
    sXaSegsLog->mLstExtRID4UDS      = aLstExtRID4UDS;
    sXaSegsLog->mFstPIDOfLstExt4UDS = aFstPIDOfLstExt4UDS;
    sXaSegsLog->mFstUndoPID         = aFstUndoPID;
    sXaSegsLog->mLstUndoPID         = aLstUndoPID;
    sXaSegsLog->mTail               = SMR_LT_XA_SEGS;

    IDE_TEST( smrLogMgr::writeLog( aStatistics, /* idvSQL* */
                                   aTrans,
                                   (SChar*)sLogBuffer,
                                   NULL,      // Previous LSN Ptr
                                   NULL,      // Log LSN Ptr
                                   NULL,      // End LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#define SMR_XA_PREPARE_REQ_LOG_BUFFER_SIZE (2048)

IDE_RC smrUpdate::writeXaPrepareReqLog( ID_XID * aGlobalXID,
                                        smTID    aTID,
                                        UInt     aGlobalTxId,
                                        UChar  * aBranchTx,
                                        UInt     aBranchTxSize,
                                        smLSN  * aLSN )
{
    smrXaPrepareReqLog   sXaPrepareReqLog;
    void               * sTrans = NULL;
    UInt                 sLogTypeFlag = 0;
    UInt                 sOffset = 0;
    UInt                 sSize = 0;
    smLSN                sLSN;
    SChar                sLocalBuffer[SMR_XA_PREPARE_REQ_LOG_BUFFER_SIZE];
    SChar              * sDynBuffer = NULL;
    SChar              * sBuffer    = NULL;

    static smrLogType sLogType = SMR_LT_XA_PREPARE_REQ;

    sSize = SMR_LOGREC_SIZE( smrXaPrepareReqLog ) +
                                            aBranchTxSize +
                                            ID_SIZEOF( smrLogTail );

    if ( sSize <= SMR_XA_PREPARE_REQ_LOG_BUFFER_SIZE )
    {
        sBuffer = sLocalBuffer;
    }
    else
    {
        IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_SM_SMR,
                                       sSize,
                                       (void **)&sDynBuffer) == IDE_SUCCESS );

        sBuffer = sDynBuffer;
    }

    sTrans       = smLayerCallback::getTransByTID( aTID );
    sLogTypeFlag = smLayerCallback::getLogTypeFlagOfTrans( sTrans );

    /* LOG HEAD */
    smrLogHeadI::setTransID( &sXaPrepareReqLog.mHead, aTID );
    smrLogHeadI::setFlag( &sXaPrepareReqLog.mHead, sLogTypeFlag );
    smrLogHeadI::setType( &sXaPrepareReqLog.mHead, sLogType );
    smrLogHeadI::setSize( &sXaPrepareReqLog.mHead, sSize );
    sXaPrepareReqLog.mGlobalTxId = aGlobalTxId;
    sXaPrepareReqLog.mBranchTxSize = aBranchTxSize;

    idlOS::memcpy( &(sXaPrepareReqLog.mGlobalXID), aGlobalXID, ID_SIZEOF(ID_XID) );

    idlOS::memcpy( sBuffer,
                   &sXaPrepareReqLog,
                   SMR_LOGREC_SIZE( smrXaPrepareReqLog ) );

    sOffset += SMR_LOGREC_SIZE( smrXaPrepareReqLog );

    /* LOG BODY */
    idlOS::memcpy( sBuffer + sOffset,
                   aBranchTx,
                   aBranchTxSize );

    sOffset += aBranchTxSize;

    /* LOG TAIL */
    idlOS::memcpy( sBuffer + sOffset,
                   &sLogType,
                   ID_SIZEOF( smrLogTail ) );

    /* WRITE LOG */
    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   sTrans,
                                   sBuffer,
                                   NULL,      // Previous LSN Ptr
                                   &sLSN,     // Log LSN Ptr
                                   NULL,      // End LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );

    IDE_TEST( smrRecoveryMgr::mIsReplWaitGlobalTxAfterPrepareFunc( NULL, /* idvSQL* */
                                                                   ID_TRUE, /* isRequestNode */
                                                                   aTID,
                                                                   SM_MAKE_SN(sLSN) ) 
              != IDE_SUCCESS );


    *aLSN = sLSN;

    if ( sDynBuffer != NULL )
    {
        iduMemMgr::free( sDynBuffer );
        sDynBuffer = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDynBuffer != NULL )
    {
        iduMemMgr::free( sDynBuffer );
        sDynBuffer = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC smrUpdate::writeXaEndLog( smTID aTID, UInt aGlobalTxId )
{
    smrXaEndLog   sXaEndLog;

    smrLogHeadI::setType(&sXaEndLog.mHead, SMR_LT_XA_END);
    smrLogHeadI::setTransID(&sXaEndLog.mHead, aTID);
    smrLogHeadI::setSize(&sXaEndLog.mHead, SMR_LOGREC_SIZE(smrXaEndLog));
    smrLogHeadI::setFlag(&sXaEndLog.mHead, SMR_LOG_TYPE_NORMAL);
    smrLogHeadI::setPrevLSN(&sXaEndLog.mHead,
                            ID_UINT_MAX,  // FILENO
                            ID_UINT_MAX); // OFFSET
    smrLogHeadI::setReplStmtDepth( &sXaEndLog.mHead, 
                                   SMI_STATEMENT_DEPTH_NULL );
    sXaEndLog.mGlobalTxId = aGlobalTxId;
    sXaEndLog.mTail = SMR_LT_XA_END;

    IDE_TEST( smrLogMgr::writeLog( NULL, /* idvSQL* */
                                   NULL,
                                   (SChar*)&sXaEndLog,
                                   NULL,      // Previous LSN Ptr
                                   NULL,      // Log LSN Ptr
                                   NULL,      // End LSN Ptr
                                   SM_NULL_OID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
