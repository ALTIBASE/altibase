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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#if !defined(_O_IDR_DEF_H_)
#define _O_IDR_DEF_H_ 1

#include <idl.h>

#define IDR_LOG_BUFFER_SIZE     (32*1024) /* 32k */

#define idrLSN     UInt
#define idrLogType UShort /* Upper 3bit is about Module Type,
                           * other bits are Module Log Types */

#define IDR_NULL_LSN ID_UINT_MAX

#define IDR_MODULE_TYPE_BIT               7
#define IDR_MODULE_TYPE_BIT_MASK          ((UShort)0x3F80)
#define IDR_MODULE_LOG_TYPE_BIT           7
#define IDR_MODULE_LOG_TYPE_MAX           (1 << IDR_MODULE_LOG_TYPE_BIT)
#define IDR_MODULE_LOG_TYPE_BIT_MASK      ((UShort)0x007F)

#define IDR_MODULE_LOG_CLR_TYPE_BIT_MASK  ((UShort)0x8000)
#define IDR_MODULE_LOG_CLR_TYPE_TRUE      ((UShort)0x8000)
#define IDR_MODULE_LOG_CLR_TYPE_FALSE     ((UShort)0x0000)

#define IDR_MODULE_LOG_UNDO_BIT_MASK      ((UShort)0x4000)
#define IDR_MODULE_LOG_UNDO_BIT_TRUE      ((UShort)0x4000)
#define IDR_MODULE_LOG_UNDO_BIT_FALSE     ((UShort)0x0000)

typedef enum idrModuleType
{
    IDR_MODULE_TYPE_ID = 0,
    IDR_MODULE_TYPE_ID_IDU_SHM_LATCH,
    IDR_MODULE_TYPE_ID_IDU_SHM_MGR,
    IDR_MODULE_TYPE_ID_IDU_SHM_PERS_MGR,
    IDR_MODULE_TYPE_ID_IDU_SHM_LIST,
    IDR_MODULE_TYPE_ID_IDU_SHM_MEMLIST,
    IDR_MODULE_TYPE_ID_IDU_SHM_HASH,
    IDR_MODULE_TYPE_ID_IDU_SHM_MEMPOOL,
    IDR_MODULE_TYPE_ID_IDU_SHM_PERSLIST,
    IDR_MODULE_TYPE_ID_IDU_SHM_PERSPOOL,
    IDR_MODULE_TYPE_ID_IDU_PM_MGR,
    IDR_MODULE_TYPE_SM,
    IDR_MODULE_TYPE_SMC_TABLE,
    IDR_MODULE_TYPE_SMC_RECORD,
    IDR_MODULE_TYPE_SMM_SLOT_LIST,
    IDR_MODULE_TYPE_SMM_MANAGER,
    IDR_MODULE_TYPE_SM_SCC_CACHE_MAPPER,
    IDR_MODULE_TYPE_SMM_DIRTY_PAGE_MGR,
    IDR_MODULE_TYPE_SMM_DIRTY_PAGE_LIST,
    IDR_MODULE_TYPE_SMM_EXPAND_CHUNK,
    IDR_MODULE_TYPE_SMM_FPL_MGR,
    IDR_MODULE_TYPE_SMM_FIXED_MEM_MGR,
    IDR_MODULE_TYPE_SCT_TBS_MGR,
    IDR_MODULE_TYPE_SCT_TBS_ALTER,
    IDR_MODULE_TYPE_SMM_DATABASE_FILE,
    IDR_MODULE_TYPE_SMM_TBS_CREATE,
    IDR_MODULE_TYPE_SMM_TBS_DROP,
    IDR_MODULE_TYPE_SMM_TBS_CHKPT_PATH,
    IDR_MODULE_TYPE_SMM_TBS_ALTER_DISCARD,
    IDR_MODULE_TYPE_SMP_FIXED_PAGE_LIST,
    IDR_MODULE_TYPE_SMP_VAR_PAGE_LIST,
    IDR_MODULE_TYPE_SMP_FREE_PAGE_LIST,
    IDR_MODULE_TYPE_SMP_TBS_ALTER_ONOFF,
    IDR_MODULE_TYPE_SM_SMX_TRANS,
    IDR_MODULE_TYPE_SM_SMX_TRANS_MGR,
    IDR_MODULE_TYPE_SM_SMX_TRANS_FREELIST,
    IDR_MODULE_TYPE_SM_SMX_OIDLIST,
    IDR_MODULE_TYPE_SM_SMX_TABLEINFO_MGR,
    IDR_MODULE_TYPE_SM_SML_LOCK_MGR,
    IDR_MODULE_TYPE_SM_SMR_RECOVERY_MGR,
    IDR_MODULE_TYPE_SM_SMR_LOGFILE,
    IDR_MODULE_TYPE_SM_SMR_LOGFILE_GROUP,
    IDR_MODULE_TYPE_SM_SMR_LOGFILE_MGR,
    IDR_MODULE_TYPE_SM_SMR_DIRTY_PAGELIST,
    IDR_MODULE_TYPE_SM_SMR_LOGANCHOR_MGR,
    IDR_MODULE_TYPE_SMN_MANAGER,
    IDR_MODULE_TYPE_SMN_SMNB_MODULE,
    IDR_MODULE_TYPE_SMN_SMNB_PBASE_BUILD,
    IDR_MODULE_TYPE_SMN_SMNB_VBASE_BUILD,
    IDR_MODULE_TYPE_SMN_SMNB_FT,
    IDR_MODULE_TYPE_SM_SMA_LOGICAL_AGER,
    IDR_MODULE_TYPE_SM_SMA_DELETE_THREAD,
    IDR_MODULE_TYPE_SM_SMI_STATISTICS,
    IDR_MODULE_TYPE_SVM_FPL_MGR,
    IDR_MODULE_TYPE_SVM_MANAGER,
    IDR_MODULE_TYPE_SVM_TBS_CREATE,
    IDR_MODULE_TYPE_SVM_TBS_DROP,
    IDR_MODULE_TYPE_SVP_ALLOC_PAGE_LIST,
    IDR_MODULE_TYPE_SVP_FIXED_PAGE_LIST,
    IDR_MODULE_TYPE_SVP_VAR_PAGE_LIST,
    IDR_MODULE_TYPE_SVP_FREE_PAGE_LIST,
    IDR_MODULE_TYPE_SVR_LOG_MGR,
    IDR_MODULE_TYPE_QP,
    IDR_MODULE_TYPE_MM,
    IDR_MODULE_TYPE_MM_MMC_TASKSHMINFO,
    IDR_MODULE_TYPE_TSM,
    IDR_MODULE_TYPE_MAX
} idrModuleType;

typedef struct idrUImgInfo
{
    UInt    mSize;
    void  * mData;
} idrUImgInfo;

typedef struct idrDummyNTALogInfo
{
    idrLSN   mLSN;
    idrLSN   mNewPrvLSN;
} idrUpdateInfo4Log;

#define idrModuleLogType idrLogType

#define IDR_MAKE_LOG_TYPE( aModuleType, aModuleLogType, aIsCLR )                        \
    (((aIsCLR) == ID_TRUE) ? IDR_MODULE_LOG_CLR_TYPE_TRUE |                                      \
     ((aModuleType) << IDR_MODULE_LOG_TYPE_BIT | ((aModuleLogType)|IDR_MODULE_LOG_UNDO_BIT_TRUE)) : \
     ((aModuleType) << IDR_MODULE_LOG_TYPE_BIT | ((aModuleLogType)|IDR_MODULE_LOG_UNDO_BIT_TRUE)))

#define IDR_GET_MODULE_TYPE( aLogType )                            \
    ((idrModuleType)((((UShort)(aLogType) & IDR_MODULE_TYPE_BIT_MASK)) >> IDR_MODULE_LOG_TYPE_BIT ))

#define IDR_GET_MODULE_LOG_TYPE( aLogType )                        \
    ((UShort)(aLogType) & IDR_MODULE_LOG_TYPE_BIT_MASK )

#define IDR_IS_MODULE_LOG_CLR( aLogType ) \
    (( ((aLogType) & IDR_MODULE_LOG_CLR_TYPE_BIT_MASK ) == IDR_MODULE_LOG_CLR_TYPE_TRUE) ? \
    ID_TRUE : ID_FALSE )

#define IDR_IS_UNDO_LOG( aLogType ) \
    (( ((aLogType) & IDR_MODULE_LOG_UNDO_BIT_MASK ) == IDR_MODULE_LOG_UNDO_BIT_TRUE) ? \
    ID_TRUE : ID_FALSE )

#define IDR_SET_UNDO_LOG_BIT( aLogType, aIsUndo )                       \
{                                                                       \
    (aLogType) &= ~IDR_MODULE_LOG_UNDO_BIT_MASK;                        \
    if( (aIsUndo) == ID_TRUE )                                          \
    {                                                                   \
        (aLogType) |= IDR_MODULE_LOG_UNDO_BIT_TRUE;                     \
    }                                                                   \
}

#define IDR_VLOG_SET_UIMG( aUImg, aData, aSize ) { \
    ((aUImg).mSize = aSize);                       \
    ((aUImg).mData = (void*)(aData));              \
}

#define IDR_VLOG_GET_UIMG( aUImg, aTgt, aSize ) {         \
    idlOS::memcpy( (aTgt), (aUImg), (aSize) );            \
}
#define IDR_VLOG_MOVE_NEXT_UIMG( aUImg, aSize )           \
    ( (aUImg) += (aSize) )

#define IDR_VLOG_GET_UIMG_N_NEXT( aUImg, aTgt, aSize ) {     \
    IDR_VLOG_GET_UIMG( (aUImg), (aTgt), (aSize) );           \
    IDR_VLOG_MOVE_NEXT_UIMG( (aUImg), (aSize) );             \
}

/* Log Head Format
 *
 * +----------+----------------+
 * | Type(16bit) | Size(16bit) |
 * +----------+----------------+
 *
 * Log Tail Format
 * +------------+
 * | Size(16bit) |
 * +------------+
 *
 * */
typedef struct idrLogHead
{
    idrLogType mType;
    UShort     mSize;
} idrLogHead;

#define idrLogTail UShort /* Tail에는 LogHeader에 mSize와 동일한 값을 설정한다. */

typedef struct idrNTALogHead
{
    idrLogHead  mLogHead;
    idrLSN      mPrvEndOffset;
} idrNTALogHead;

struct iduShmTxInfo;
struct idvSQL;

typedef IDE_RC (*idrUndoFunc)( idvSQL       * aStatistics,
                               iduShmTxInfo * aShmTxInfo,
                               idrLSN         aNTALsn,
                               UShort         aSize,
                               SChar        * aImage );

typedef struct idrSVP
{
    idrLSN     mLSN;
    UInt       mLatchStackIdx;
} idrSVP;

#define IDR_INIT_SVP( aSVP )                                \
    {                                                       \
        (aSVP)->mLSN           = IDR_NULL_LSN;              \
        (aSVP)->mLatchStackIdx = IDU_LATCH_STACK_NULL_IDX;  \
    }

#define IDR_IS_INIT_SVP( aSVP )                             \
   ( ((aSVP)->mLSN == IDR_NULL_LSN) && ((aSVP)->mLatchStackIdx == IDU_LATCH_STACK_NULL_IDX) ) 

#endif /* _O_IDR_DEF_H_ */
