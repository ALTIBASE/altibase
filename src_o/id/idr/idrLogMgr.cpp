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
#include <idu.h>
#include <idw.h>
#include <idrDef.h>
#include <idrShmTxPendingOp.h>
#include <iduVLogShmMgr.h>
#include <idrVLogUpdate.h>
#include <iduVLogShmMemPool.h>
#include <iduVLogShmMemList.h>
#include <iduVLogShmLatch.h>
#include <idwVLogPMMgr.h>
#include <iduVLogShmList.h>
#include <iduShmSet4SXLatch.h>
#include <idrLogMgr.h>

idBool           idrLogMgr::mLoggingEnabled = ID_TRUE;
idrUndoFunc    * idrLogMgr::mArrUndoFuncMap[IDR_MODULE_TYPE_MAX];

SChar idrLogMgr::mStrModuleType[IDR_MODULE_TYPE_MAX][100] =
{
    "IDR_MODULE_TYPE_ID",
    "IDR_MODULE_TYPE_ID_IDU_SHM_LATCH",
    "IDR_MODULE_TYPE_ID_IDU_SHM_MGR",
    "IDR_MODULE_TYPE_ID_IDU_SHM_LIST",
    "IDR_MODULE_TYPE_ID_IDU_SHM_MEMLIST",
    "IDR_MODULE_TYPE_ID_IDU_SHM_HASH",
    "IDR_MODULE_TYPE_ID_IDU_SHM_MEMPOOL",
    "IDR_MODULE_TYPE_ID_IDU_PM_MGR",
    "IDR_MODULE_TYPE_SM",
    "IDR_MODULE_TYPE_SMC_TABLE",
    "IDR_MODULE_TYPE_SMC_RECORD",
    "IDR_MODULE_TYPE_SMM_SLOT_LIST",
    "IDR_MODULE_TYPE_SMM_MANAGER",
    "IDR_MODULE_TYPE_SM_SCC_CACHE_MAPPER",
    "IDR_MODULE_TYPE_SMM_DIRTY_PAGE_MGR",
    "IDR_MODULE_TYPE_SMM_DIRTY_PAGE_LIST",
    "IDR_MODULE_TYPE_SMM_EXPAND_CHUNK",
    "IDR_MODULE_TYPE_SMM_FPL_MGR",
    "IDR_MODULE_TYPE_SMM_FIXED_MEM_MGR",
    "IDR_MODULE_TYPE_SCT_TBS_MGR",
    "IDR_MODULE_TYPE_SCT_TBS_ALTER",
    "IDR_MODULE_TYPE_SMM_DATABASE_FILE",
    "IDR_MODULE_TYPE_SMM_TBS_CREATE",
    "IDR_MODULE_TYPE_SMM_TBS_DROP",
    "IDR_MODULE_TYPE_SMM_TBS_CHKPT_PATH",
    "IDR_MODULE_TYPE_SMM_TBS_ALTER_DISCARD",
    "IDR_MODULE_TYPE_SMP_FIXED_PAGE_LIST",
    "IDR_MODULE_TYPE_SMP_VAR_PAGE_LIST",
    "IDR_MODULE_TYPE_SMP_FREE_PAGE_LIST",
    "IDR_MODULE_TYPE_SMP_TBS_ALTER_ONOFF",
    "IDR_MODULE_TYPE_SM_SMX_TRANS",
    "IDR_MODULE_TYPE_SM_SMX_TRANS_MGR",
    "IDR_MODULE_TYPE_SM_SMX_TRANS_FREELIST",
    "IDR_MODULE_TYPE_SM_SMX_OIDLIST",
    "IDR_MODULE_TYPE_SM_SMX_TABLEINFO_MGR",
    "IDR_MODULE_TYPE_SM_SML_LOCK_MGR",
    "IDR_MODULE_TYPE_SM_SMR_RECOVERY_MGR",
    "IDR_MODULE_TYPE_SM_SMR_LOGFILE",
    "IDR_MODULE_TYPE_SM_SMR_LOGFILE_GROUP",
    "IDR_MODULE_TYPE_SM_SMR_LOGFILE_MGR",
    "IDR_MODULE_TYPE_SM_SMR_DIRTY_PAGELIST",
    "IDR_MODULE_TYPE_SM_SMR_LOGANCHOR_MGR",
    "IDR_MODULE_TYPE_SMN_MANAGER",
    "IDR_MODULE_TYPE_SMN_SMNB_MODULE",
    "IDR_MODULE_TYPE_SMN_SMNB_PBASE_BUILD",
    "IDR_MODULE_TYPE_SMN_SMNB_VBASE_BUILD",
    "IDR_MODULE_TYPE_SMN_SMNB_FT",
    "IDR_MODULE_TYPE_SM_SMA_LOGICAL_AGER",
    "IDR_MODULE_TYPE_SM_SMA_DELETE_THREAD",
    "IDR_MODULE_TYPE_SM_SMI_STATISTICS",
    "IDR_MODULE_TYPE_QP",
    "IDR_MODULE_TYPE_MM",
    "IDR_MODULE_TYPE_MM_MMC_TASKSHMINFO",
    "IDR_MODULE_TYPE_TSM"
};

IDE_RC idrLogMgr::initialize()
{
    idlOS::memset( mArrUndoFuncMap,
                   0,
                   IDR_MODULE_TYPE_MAX * ID_SIZEOF(vULong) );

    IDE_TEST( iduVLogShmMgr::initialize() != IDE_SUCCESS );

    IDE_TEST( idrVLogUpdate::initialize() != IDE_SUCCESS );

    IDE_TEST( iduVLogShmLatch::initialize() != IDE_SUCCESS );
    IDE_TEST( iduVLogShmHash::initialize() != IDE_SUCCESS );

    IDE_TEST( iduVLogShmMemPool::initialize() != IDE_SUCCESS );
    IDE_TEST( iduVLogShmMemList::initialize() != IDE_SUCCESS );

    IDE_TEST( idwVLogPMMgr::initialize() != IDE_SUCCESS );

    IDE_TEST( iduVLogShmList::initialize() != IDE_SUCCESS );

    IDE_TEST( idrShmTxPendingOp::initialize() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idrLogMgr::destroy()
{
    IDE_TEST( iduVLogShmMgr::destroy() != IDE_SUCCESS );

    IDE_TEST( idrVLogUpdate::destroy() != IDE_SUCCESS );

    IDE_TEST( iduVLogShmLatch::destroy() != IDE_SUCCESS );
    IDE_TEST( iduVLogShmHash::destroy() != IDE_SUCCESS );

    IDE_TEST( iduVLogShmMemPool::destroy() != IDE_SUCCESS );
    IDE_TEST( iduVLogShmMemList::destroy() != IDE_SUCCESS );

    IDE_TEST( idwVLogPMMgr::destroy() != IDE_SUCCESS );

    IDE_TEST( iduVLogShmList::destroy() != IDE_SUCCESS );

    IDE_TEST( idrShmTxPendingOp::destroy() != IDE_SUCCESS );

    idlOS::memset( mArrUndoFuncMap,
                   0,
                   IDR_MODULE_TYPE_MAX * ID_SIZEOF(vULong) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idrLogMgr::registerUndoFunc( idrModuleType  aModuleType,
                                  idrUndoFunc  * aArrUndoFunc )
{
    IDE_ASSERT( aModuleType < IDR_MODULE_TYPE_MAX );

    mArrUndoFuncMap[aModuleType] = aArrUndoFunc;
}

idrLSN idrLogMgr::writeLog( iduShmTxInfo   * aShmTxInfo,
                            idrModuleType    aModuleType,
                            idrModuleLogType aModuleLogType,
                            UInt             aUImgInfoCnt,
                            idrUImgInfo    * aArrUImgInfo )
{
    SChar           * sFstLogPtr;
    SChar           * sCurLogPtr;
    UShort            sLogSize;
    idrLogHead      * sLogHeadPtr;
    idrLogType        sLogType;
    UInt              i;
    idrUImgInfo     * sCurUImgInfo;
    idrLSN            sLSN4WriteLog = IDR_NULL_LSN;

    IDE_DASSERT( aModuleLogType != 0 );

    if( iduProperty::getShmLogging() == 1 )
    {
        if( mLoggingEnabled == ID_TRUE )
        {
            IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

            if( idwPMMgr::isLogEnabled( aShmTxInfo ) == ID_TRUE )
            {
                sLogSize = ID_SIZEOF(idrLogHead) + ID_SIZEOF(idrLogTail);

                sFstLogPtr    = aShmTxInfo->mLogBuffer + aShmTxInfo->mLogOffset;
                sCurLogPtr    = sFstLogPtr;
                sLSN4WriteLog = aShmTxInfo->mLogOffset;

                sLogHeadPtr = (idrLogHead*)sCurLogPtr;

                sCurLogPtr  += ID_SIZEOF(idrLogHead);

                IDE_DASSERT( aModuleType <= IDR_MODULE_TYPE_MAX );
                IDE_DASSERT( aModuleLogType <= IDR_MODULE_LOG_TYPE_MAX );

                /* Set log head */
                sLogType = IDR_MAKE_LOG_TYPE( aModuleType,
                                              aModuleLogType,
                                              ID_FALSE /* Not CLR */ );

                IDE_DASSERT( IDR_GET_MODULE_TYPE( sLogType ) == aModuleType );
                IDE_DASSERT( IDR_GET_MODULE_LOG_TYPE( sLogType ) == aModuleLogType );

                IDE_DASSERT( IDR_GET_MODULE_TYPE( sLogType ) <= IDR_MODULE_TYPE_MAX );
                IDE_DASSERT( IDR_GET_MODULE_LOG_TYPE( sLogType )  <= IDR_MODULE_LOG_TYPE_MAX );

                idlOS::memcpy( &sLogHeadPtr->mType, &sLogType, ID_SIZEOF(idrLogType) );

                sCurUImgInfo = aArrUImgInfo;

                for( i = 0; i < aUImgInfoCnt; i++ )
                {
                    sLogSize += sCurUImgInfo->mSize;

                    IDE_ASSERT( IDR_LOG_BUFFER_SIZE >= sLogSize + aShmTxInfo->mLogOffset );

                    idlOS::memcpy( sCurLogPtr,
                                   sCurUImgInfo->mData,
                                   sCurUImgInfo->mSize );

                    sCurLogPtr += sCurUImgInfo->mSize;

                    sCurUImgInfo++;
                }

                idlOS::memcpy( &sLogHeadPtr->mSize, &sLogSize, ID_SIZEOF(UShort) );

                /* Set log tail */
                idlOS::memcpy( sCurLogPtr,
                               &sLogSize,
                               ID_SIZEOF(idrLogTail) );

                sCurLogPtr += ID_SIZEOF(idrLogTail);
                IDE_ASSERT( (SChar*)sCurLogPtr - sFstLogPtr == sLogSize );

                IDL_MEM_BARRIER;
                aShmTxInfo->mLogOffset += sLogSize;
            }
        }
    }

    return sLSN4WriteLog;
}

idrLSN idrLogMgr::writeLog( iduShmTxInfo   * aShmTxInfo,
                            idrModuleType    aModuleType,
                            idrModuleLogType aModuleLogType,
                            idShmAddr        aAddrObj,
                            UShort           aSize,
                            SChar          * aData )
{
    idrUImgInfo sArrUImgInfo[2];

    IDR_VLOG_SET_UIMG( sArrUImgInfo[0], &aAddrObj, ID_SIZEOF(idShmAddr) );
    IDR_VLOG_SET_UIMG( sArrUImgInfo[1], aData, aSize );

    return writeLog( aShmTxInfo,
                     aModuleType,
                     aModuleLogType,
                     2, /* aUImgInfoCnt */
                     sArrUImgInfo );
}

idrLSN idrLogMgr::writeLog( iduShmTxInfo   * aShmTxInfo,
                            idrModuleType    aModuleType,
                            idrModuleLogType aModuleLogType,
                            UShort           aSize,
                            SChar          * aData )
{
    idrUImgInfo sUImgInfo;

    IDR_VLOG_SET_UIMG( sUImgInfo, aData, aSize );

    return writeLog( aShmTxInfo,
                     aModuleType,
                     aModuleLogType,
                     1, /* aUImgInfoCnt */
                     &sUImgInfo );
}

idrLSN idrLogMgr::writeNullNTALog( iduShmTxInfo   * aShmTxInfo,
                                   idrLSN           aPrvLSN,
                                   idrModuleType    aModuleType,
                                   idrModuleLogType aModuleLogType )
{
    return idrLogMgr::writeNTALog( aShmTxInfo,
                                   aPrvLSN,
                                   aModuleType,
                                   aModuleLogType,
                                   0, /* aSize */
                                   (SChar*)NULL /* aData */ );
}

idrLSN idrLogMgr::writeNTALog( iduShmTxInfo   * aShmTxInfo,
                               idrLSN           aPrvLSN,
                               idrModuleType    aModuleType,
                               idrModuleLogType aModuleLogType,
                               UShort           aSize,
                               SChar          * aData )
{
    idrUImgInfo sUImgInfo;

    IDR_VLOG_SET_UIMG( sUImgInfo, aData, aSize );

    return writeNTALog( aShmTxInfo,
                        aPrvLSN,
                        aModuleType,
                        aModuleLogType,
                        1, /* aUImgInfoCnt */
                        &sUImgInfo );
}

idrLSN idrLogMgr::writeNTALog( iduShmTxInfo   * aShmTxInfo,
                               idrLSN           aPrvLSN,
                               idrModuleType    aModuleType,
                               idrModuleLogType aModuleLogType,
                               UInt             aUImgInfoCnt,
                               idrUImgInfo    * aArrUImgInfo )
{
    SChar         * sFstLogPtr;
    SChar         * sCurLogPtr;
    UShort          sLogSize;
    idrNTALogHead * sNTALogHeadPtr;
    idrUImgInfo   * sCurUImgInfo;
    idrLogType      sLogType;
    UInt            i;
    idrLSN          sLSN4WriteLog = IDR_NULL_LSN;

    IDE_DASSERT( aModuleLogType != 0 );

    if( ( aShmTxInfo != NULL ) && ( mLoggingEnabled == ID_TRUE ))
    {
        IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

        if( idwPMMgr::isLogEnabled( aShmTxInfo ) == ID_TRUE )
        {
            sLogSize      = ID_SIZEOF(idrNTALogHead) + ID_SIZEOF(idrLogTail);
            sLSN4WriteLog = aShmTxInfo->mLogOffset;

            sFstLogPtr  = aShmTxInfo->mLogBuffer + aShmTxInfo->mLogOffset;
            sCurLogPtr  = sFstLogPtr;

            sNTALogHeadPtr = (idrNTALogHead*)sCurLogPtr;

            IDE_ASSERT( IDR_LOG_BUFFER_SIZE >= sLogSize + aShmTxInfo->mLogOffset );

            sLogType = IDR_MAKE_LOG_TYPE( aModuleType,
                                          aModuleLogType,
                                          ID_TRUE /* CLR */);

            /* Set Log Type */
            idlOS::memcpy( &sNTALogHeadPtr->mLogHead.mType,
                           &sLogType,
                           ID_SIZEOF(idrLogType) );

            /* Set Previous Log Pos */
            idlOS::memcpy( &sNTALogHeadPtr->mPrvEndOffset,
                           &aPrvLSN,
                           ID_SIZEOF(idrLSN) );

            sCurLogPtr  += ID_SIZEOF(idrNTALogHead);

            sCurUImgInfo = aArrUImgInfo;

            for( i = 0; i < aUImgInfoCnt; i++ )
            {
                sLogSize += sCurUImgInfo->mSize;

                IDE_ASSERT( IDR_LOG_BUFFER_SIZE >= sLogSize + aShmTxInfo->mLogOffset );

                idlOS::memcpy( sCurLogPtr,
                               sCurUImgInfo->mData,
                               sCurUImgInfo->mSize );

                sCurLogPtr += sCurUImgInfo->mSize;

                sCurUImgInfo++;
            }

            /* NTA Log의 구조가 변경되는 것을 Detect하기 위해서 */
            IDE_DASSERT( ID_SIZEOF(idrNTALogHead) ==
                         ID_SIZEOF(idrLogHead) + ID_SIZEOF(idrLSN) );

            IDE_DASSERT( aModuleType <= IDR_MODULE_TYPE_MAX );
            IDE_DASSERT( aModuleLogType <= IDR_MODULE_LOG_TYPE_MAX );

            IDE_DASSERT( IDR_GET_MODULE_TYPE( sLogType ) <= IDR_MODULE_TYPE_MAX );
            IDE_DASSERT( IDR_GET_MODULE_LOG_TYPE( sLogType )  <= IDR_MODULE_LOG_TYPE_MAX );

            /* Seg Log Size */
            idlOS::memcpy( &sNTALogHeadPtr->mLogHead.mSize,
                           &sLogSize,
                           ID_SIZEOF(UShort) );

            /* Set log tail */
            idlOS::memcpy( sCurLogPtr,
                           &sLogSize,
                           ID_SIZEOF(idrLogTail) );

            sCurLogPtr += ID_SIZEOF(idrLogTail);
            IDE_ASSERT( (SChar*)sCurLogPtr - sFstLogPtr == sLogSize );

            IDL_MEM_BARRIER;
            aShmTxInfo->mLogOffset += sLogSize;
        }
    }

    return sLSN4WriteLog;
}

void idrLogMgr::readLog( iduShmTxInfo  * aShmTxInfo,
                         UInt            aOffset,
                         idrLogHead    * aLogHead,
                         SChar        ** aUndoImg )
{
    SChar * sCurLogPtr;

    sCurLogPtr = aShmTxInfo->mLogBuffer + aOffset;
    idlOS::memcpy( aLogHead, sCurLogPtr, ID_SIZEOF(idrLogHead) );

    *aUndoImg  = sCurLogPtr + ID_SIZEOF(idrLogHead);
}

IDE_RC idrLogMgr::undoProcOrThr( idvSQL        * aStatistics,
                                 iduShmTxInfo  * aShmTxInfo,
                                 idrLSN          aLSN )
{
    SChar           * sCurLogPtr;
    idrLogHead        sLogHead;
    idrNTALogHead     sNTALogHead;
    SChar           * sDataPtr;
    idrModuleType     sModuleType;
    UShort            sPrvLogSize;
    idrModuleLogType  sModuleLogType;
    UShort            sBefImgSize;
    UInt              sCurLogOffset;
    idrUndoFunc     * sArrUndoFunction;

    if( aShmTxInfo->mLogOffset != 0 )
    {
        IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

        sCurLogPtr  = aShmTxInfo->mLogBuffer + aShmTxInfo->mLogOffset;
        idlOS::memcpy( &sPrvLogSize,
                       sCurLogPtr - ID_SIZEOF(idrLogTail),
                       ID_SIZEOF(idrLogTail));

        IDE_ASSERT( aShmTxInfo->mLogOffset >= sPrvLogSize );

        sCurLogPtr -= sPrvLogSize;

        sCurLogOffset = aShmTxInfo->mLogOffset - sPrvLogSize;

        while( sCurLogOffset >= aLSN )
        {
            IDE_DASSERT( ( aShmTxInfo->mLogBuffer + sCurLogOffset ) == sCurLogPtr );

            /* Log type에 따라서 Undo를 수행한다. */
            idlOS::memcpy( &sLogHead, sCurLogPtr, ID_SIZEOF(idrLogHead) );

            sModuleType    = IDR_GET_MODULE_TYPE( sLogHead.mType );
            sModuleLogType = IDR_GET_MODULE_LOG_TYPE( sLogHead.mType );

            if( IDR_IS_MODULE_LOG_CLR( sLogHead.mType ) == ID_FALSE )
            {
                sDataPtr     = sCurLogPtr + ID_SIZEOF( idrLogHead ) ;
                sBefImgSize  =
                    sLogHead.mSize - ID_SIZEOF(idrLogHead) - ID_SIZEOF(idrLogTail);

                IDE_ASSERT( sModuleType < IDR_MODULE_TYPE_MAX );
                IDE_ASSERT( mArrUndoFuncMap[sModuleType] != NULL );

                sArrUndoFunction = mArrUndoFuncMap[sModuleType];

                if( IDR_IS_UNDO_LOG( sLogHead.mType ) == ID_TRUE )
                {
                    IDE_TEST( sArrUndoFunction[sModuleLogType](
                                  aStatistics,
                                  aShmTxInfo,
                                  IDR_NULL_LSN,
                                  sBefImgSize,
                                  sDataPtr )
                              != IDE_SUCCESS );
                }

                aShmTxInfo->mLogOffset = sCurLogOffset;

                if( sCurLogOffset == 0 )
                {
                    break;
                }

                idlOS::memcpy( &sPrvLogSize,
                               sCurLogPtr - ID_SIZEOF(idrLogTail),
                               ID_SIZEOF(UShort) );
            }
            else
            {
                idlOS::memcpy( &sNTALogHead, sCurLogPtr, ID_SIZEOF(idrNTALogHead) );

                sDataPtr     = sCurLogPtr + ID_SIZEOF( idrNTALogHead ) ;
                sBefImgSize  =
                    sLogHead.mSize - ID_SIZEOF(idrNTALogHead) - ID_SIZEOF(idrLogTail);

                if( IDR_IS_UNDO_LOG( sLogHead.mType ) == ID_TRUE )
                {
                    if( mArrUndoFuncMap[sModuleType][sModuleLogType](
                            aStatistics,
                            aShmTxInfo,
                            sNTALogHead.mPrvEndOffset,
                            sBefImgSize,
                            sDataPtr ) != IDE_SUCCESS )
                    {
                        IDE_TEST( ideGetErrorCode() !=
                                  idERR_IGNORE_FAIL_COMMIT_TRANS_DUE_TO_MAX_ROW );

                        aShmTxInfo->mLogOffset = sCurLogOffset;

                    }
                    else
                    {
                        sCurLogOffset = sNTALogHead.mPrvEndOffset;
                    }
                }
                else
                {
                    sCurLogOffset = sNTALogHead.mPrvEndOffset;
                }

                aShmTxInfo->mLogOffset = sCurLogOffset;

                if( sCurLogOffset == 0 )
                {
                    break;
                }

                sCurLogPtr = aShmTxInfo->mLogBuffer + sCurLogOffset;

                IDU_FIT_POINT( "idrLogMgr::undoProcOrThr::memcpy::sPrvLogSize" ); 

                /* 모든 Logical Undo함수는 Undo완료후에 내부에서 Thread의 Log Offset을
                 * 바꾸어야 한다. */
                idlOS::memcpy( &sPrvLogSize,
                               sCurLogPtr - ID_SIZEOF(idrLogTail),
                               ID_SIZEOF(UShort) );

                IDL_MEM_BARRIER;
            }

            IDE_ASSERT( sCurLogOffset >= sPrvLogSize );

            /* Log Undo가 완료되었으므로 mLogOffset을 옮겨서 Undo된 로그가
             * 또 언두되는 것을 방지한다. */
            sCurLogOffset -= sPrvLogSize;

            sCurLogPtr -= sPrvLogSize;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idrLogMgr::updatePrvLSN4NTALog( iduShmTxInfo   * aShmTxInfo,
                                     idrLSN           aWriteLSN,
                                     idrLSN           aPrvLSN )
{
    idrNTALogHead * sNTALogPtr;

    if( ( aShmTxInfo != NULL ) && ( mLoggingEnabled == ID_TRUE ))
    {
        IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

        sNTALogPtr  = (idrNTALogHead*)(aShmTxInfo->mLogBuffer + aWriteLSN);

        /* Set Previous Log Pos */
        idlOS::memcpy( &sNTALogPtr->mPrvEndOffset,
                       &aPrvLSN,
                       ID_SIZEOF(idrLSN) );
    }
}

void idrLogMgr::disableUndoLog( iduShmTxInfo   * aShmTxInfo,
                                idrLSN           aLSN )
{
    idrLogHead    * sLogHead;
    idrLogType      sLogType;

    if( ( aShmTxInfo != NULL ) && ( mLoggingEnabled == ID_TRUE ))
    {
        IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

        sLogHead = (idrLogHead*)(aShmTxInfo->mLogBuffer + aLSN);

        sLogType = sLogHead->mType;

        IDR_SET_UNDO_LOG_BIT( sLogType, ID_FALSE /* No Undo */ );
        idlOS::memcpy( &sLogHead->mType, &sLogType, ID_SIZEOF(idrLogType) );
    }
}

IDE_RC idrLogMgr::commit2Svp( idvSQL         * aStatistics,
                              iduShmTxInfo   * aShmTxInfo,
                              idrSVP         * aSavepoint )
{
    IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );
    
    IDU_FIT_POINT( "idrLogMgr::commit2Svp" );

    IDE_ASSERT( aShmTxInfo->mLogOffset != IDR_NULL_LSN );
    IDE_ASSERT( aSavepoint->mLSN != IDR_NULL_LSN );

    aShmTxInfo->mLogOffset = aSavepoint->mLSN;

    IDE_TEST( idrLogMgr::releaseLatch2Svp( aStatistics,
                                           aShmTxInfo,
                                           aSavepoint )
              != IDE_SUCCESS );

    IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idrLogMgr::abort2Svp( idvSQL         * aStatistics,
                             iduShmTxInfo   * aShmTxInfo,
                             idrSVP         * aSavepoint )
{
    IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

    IDU_FIT_POINT( "idrLogMgr::abort2Svp" );
    
    IDE_ASSERT( undoProcOrThr( aStatistics, aShmTxInfo, aSavepoint->mLSN )
                == IDE_SUCCESS );

    IDE_ASSERT( idrLogMgr::releaseLatch2Svp( aStatistics,
                                             aShmTxInfo,
                                             aSavepoint )
                == IDE_SUCCESS );

    IDE_ASSERT( aShmTxInfo->mLogOffset < IDU_LOG_BUFFER_SIZE );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idrLogMgr::pushLatchOp( iduShmTxInfo        * aShmTxInfo,
                               void                * aObject,
                               iduShmSXLatchMode     aMode,
                               iduShmLatchInfo    ** aNewLatchInfo )
{
    iduShmLatchStack   * sLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    iduShmSXLatch      * sSXLatch;

    IDE_ASSERT( aShmTxInfo != NULL );

    sLatchStack = &aShmTxInfo->mLatchStack;

    IDE_ASSERT( sLatchStack->mCurSize < IDU_MAX_SHM_LATCH_STACK_SIZE );

    sLatchInfo = sLatchStack->mArray + sLatchStack->mCurSize;

    sLatchInfo->mObject        = aObject;
    sLatchInfo->mMode          = aMode;

    sSXLatch = (iduShmSXLatch*)aObject;

    IDU_SHM_VALIDATE_ADDR_PTR( sSXLatch->mLatch.mAddrSelf, &sSXLatch->mLatch );

    sLatchInfo->mAddr4Latch   = sSXLatch->mLatch.mAddrSelf;
    sLatchInfo->mLSN4Latch    = aShmTxInfo->mLogOffset;
    sLatchInfo->mOldSLatchCnt = IDU_SHM_SX_SLATCH_CNT_INVALID;
    sLatchInfo->mLatchOPState = IDU_SHM_LATCH_STATE_NULL;

    IDL_MEM_BARRIER;

    sLatchStack->mCurSize++;

    if( aNewLatchInfo != NULL )
    {
        *aNewLatchInfo = sLatchInfo;
    }

    return IDE_SUCCESS;
}

IDE_RC idrLogMgr::pushSetLatchOp( iduShmTxInfo        * aShmTxInfo,
                                  void                * aObject,
                                  iduShmSXLatchMode     aMode,
                                  iduShmLatchInfo    ** aNewLatchInfo )
{
    iduShmLatchStack   * sSetLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    iduShmSet4SXLatch  * sSetLatch;
    iduShmSXLatch      * sSXLatch;

    IDE_ASSERT( aShmTxInfo != NULL );

    sSetLatchStack = &aShmTxInfo->mSetLatchStack;

    IDE_TEST_RAISE( sSetLatchStack->mCurSize >= IDU_MAX_SHM_LATCH_STACK_SIZE, err_set_latch_stack_overflow );

    sLatchInfo = sSetLatchStack->mArray + sSetLatchStack->mCurSize;

    sLatchInfo->mObject        = aObject;
    sLatchInfo->mMode          = aMode;

    if( aMode == IDU_SET_SX_LATCH_MODE_SHARED )
    {
        sSXLatch = (iduShmSXLatch*)aObject;

        IDU_SHM_VALIDATE_ADDR_PTR( sSXLatch->mLatch.mAddrSelf, &sSXLatch->mLatch );

        sLatchInfo->mAddr4Latch   = sSXLatch->mLatch.mAddrSelf;
    }
    else
    {
        IDE_DASSERT( aMode == IDU_SET_SX_LATCH_MODE_EXCLUSIVE );

        sSetLatch = (iduShmSet4SXLatch*)aObject;

        IDU_SHM_VALIDATE_ADDR_PTR( sSetLatch->mAddrSelf, sSetLatch );

        sLatchInfo->mAddr4Latch   = sSetLatch->mAddrSelf;
    }

    sLatchInfo->mLSN4Latch    = aShmTxInfo->mLogOffset;

    IDL_MEM_BARRIER;

    sSetLatchStack->mCurSize++;

    if( aNewLatchInfo != NULL )
    {
        *aNewLatchInfo = sLatchInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_set_latch_stack_overflow );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_SET_LATCH_STACK_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idrLogMgr::clearLatchStack( iduShmTxInfo   * aShmTxInfo,
                                 UInt             aStackIdx,
                                 idBool           aLatchRelease )
{
    iduShmLatchStack   * sLatchStack;
    iduShmLatchInfo    * sLatchInfo;

    IDE_ASSERT( aShmTxInfo != NULL );

    sLatchStack = &aShmTxInfo->mLatchStack;

    while(1)
    {
        if( aStackIdx > sLatchStack->mCurSize )
        {
            break;
        }

        IDE_ASSERT( sLatchStack->mCurSize != 0 );

        sLatchStack->mCurSize--;

        sLatchInfo = sLatchStack->mArray + sLatchStack->mCurSize;

        if( aStackIdx == sLatchStack->mCurSize )
        {
            break;
        }
        else
        {
            /* do nothing */
        }

        if( aLatchRelease == ID_TRUE )
        {
            IDE_ASSERT( iduShmLatchRelease( aShmTxInfo,
                                            (iduShmLatch*)sLatchInfo->mObject )
                        == IDE_SUCCESS );
        }
    }
}

void idrLogMgr::clearSetLatchStack( iduShmTxInfo   * aShmTxInfo,
                                    UInt             aStackIdx )
{
    iduShmLatchStack   * sSetLatchStack;
    iduShmLatchInfo    * sLatchInfo;

    IDE_ASSERT( aShmTxInfo != NULL );

    sSetLatchStack = &aShmTxInfo->mSetLatchStack;

    while(1)
    {
        if( aStackIdx > sSetLatchStack->mCurSize )
        {
            break;
        }

        IDE_ASSERT( sSetLatchStack->mCurSize != 0 );

        sSetLatchStack->mCurSize--;

        sLatchInfo = sSetLatchStack->mArray + sSetLatchStack->mCurSize;

        if( aStackIdx == sSetLatchStack->mCurSize )
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }
}

IDE_RC idrLogMgr::releaseLatch2Svp( idvSQL         * aStatistics,
                                    iduShmTxInfo   * aShmTxInfo,
                                    idrSVP         * aSavepoint )
{
    iduShmLatchStack   * sLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    iduShmProcState      sState4Proc;
    UInt                 sLatchStackIdx;

    IDE_ASSERT( aShmTxInfo != NULL );

    sLatchStack = &aShmTxInfo->mLatchStack;

    sState4Proc = idwPMMgr::getProcState( aStatistics );

    while( sLatchStack->mCurSize != 0 )
    {
        if( sLatchStack->mCurSize <= aSavepoint->mLatchStackIdx )
        {
            break;
        }

        sLatchStackIdx = sLatchStack->mCurSize - 1;
        sLatchInfo = sLatchStack->mArray + sLatchStackIdx;

        if( sLatchInfo->mLSN4Latch <  aSavepoint->mLSN )
        {
            break;
        }

        if( sState4Proc == IDU_SHM_PROC_STATE_RECOVERY )
        {
            // Watch Dog의 의한 Undo시 비정상 종료한 Process의 ProcInfo의 Stack의
            // LatchInfo의 Latch Pointer는 죽은 Process에서만 유효하고, Watch Dog
            // 이 속한 Daemon Process에서는 Valid하지 않기때문에, 다시 보정해 주어
            // 한다.
            sLatchInfo->mObject =
                IDU_SHM_GET_ADDR_PTR_CHECK( sLatchInfo->mAddr4Latch );
        }

        switch( sLatchInfo->mMode )
        {
        case IDU_SHM_LATCH_MODE_EXCLUSIVE:
            IDE_ASSERT( iduShmLatchRelease( aShmTxInfo,
                                            (iduShmLatch *)sLatchInfo->mObject )
                        == IDE_SUCCESS );
            break;

        case IDU_SX_LATCH_MODE_SHARED:
        case IDU_SX_LATCH_MODE_EXCLUSIVE:
            /* iduShmSXLatchRelease함수내에서 Stack의 mCurSize의 값을 바꾸어 준다. */
            IDE_ASSERT( iduShmSXLatchRelease( aStatistics,
                                              aShmTxInfo,
                                              sLatchInfo,
                                              sLatchInfo->mMode )
                        == IDE_SUCCESS );
            break;

        default:
            IDE_ASSERT( 0 );
            break;
        }

        sLatchStack->mCurSize--;
    }

    return IDE_SUCCESS;
}

IDE_RC idrLogMgr::releaseLatch( idvSQL         * aStatistics,
                                iduShmTxInfo   * aShmTxInfo,
                                void           * aLatch )
{
    iduShmLatchStack   * sLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    UInt                 sStackIdx;

    IDE_ASSERT( aShmTxInfo != NULL );

    sLatchStack = &aShmTxInfo->mLatchStack;

    sStackIdx = sLatchStack->mCurSize;

    while( sStackIdx != 0 )
    {
        IDE_ASSERT( sStackIdx != 0 );
    
        sStackIdx--;

        sLatchInfo = sLatchStack->mArray + sStackIdx;

        if( sLatchInfo->mObject == aLatch )
        {
            switch( sLatchInfo->mMode )
            {
            case IDU_SHM_LATCH_MODE_EXCLUSIVE:
                break;

            case IDU_SX_LATCH_MODE_SHARED:
            case IDU_SX_LATCH_MODE_EXCLUSIVE:
                IDE_ASSERT( iduShmSXLatchRelease( aStatistics,
                                                  aShmTxInfo,
                                                  sLatchInfo,
                                                  sLatchInfo->mMode )
                            == IDE_SUCCESS );
                break;

            default:
                IDE_ASSERT( 0 );
                break;
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC idrLogMgr::releaseSetLatch( idvSQL         * aStatistics,
                                   iduShmTxInfo   * aShmTxInfo )
{
    iduShmLatchStack   * sSetLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    iduShmProcState      sState4Proc;
    UInt                 sLatchStackIdx;

    IDE_ASSERT( aShmTxInfo != NULL );

    sSetLatchStack = &aShmTxInfo->mSetLatchStack;

    sState4Proc = idwPMMgr::getProcState( aStatistics );

    while( sSetLatchStack->mCurSize != 0 )
    {
        sLatchStackIdx = sSetLatchStack->mCurSize - 1;
        sLatchInfo = sSetLatchStack->mArray + sLatchStackIdx;

        if( sState4Proc == IDU_SHM_PROC_STATE_RECOVERY )
        {
            // Watch Dog의 의한 Undo시 비정상 종료한 Process의 ProcInfo의 Stack의
            // LatchInfo의 Latch Pointer는 죽은 Process에서만 유효하고, Watch Dog
            // 이 속한 Daemon Process에서는 Valid하지 않기때문에, 다시 보정해 주어
            // 한다.
            sLatchInfo->mObject =
                IDU_SHM_GET_ADDR_PTR_CHECK( sLatchInfo->mAddr4Latch );
        }

        switch( sLatchInfo->mMode )
        {
        case IDU_SET_SX_LATCH_MODE_SHARED:
            IDE_ASSERT( iduShmLatchRelease( aShmTxInfo,
                                            (iduShmLatch *)sLatchInfo->mObject )
                        == IDE_SUCCESS );
            break;

        case IDU_SET_SX_LATCH_MODE_EXCLUSIVE:
            IDE_ASSERT( iduShmSet4SXLatchRelease( aStatistics,
                                                  aShmTxInfo,
                                                  sLatchInfo )
                        == IDE_SUCCESS );
            break;

        default:
            IDE_ASSERT( 0 );
            break;
        }

        sSetLatchStack->mCurSize--;
    }

    return IDE_SUCCESS;
}

IDE_RC idrLogMgr::commit( idvSQL * aStatistics, iduShmTxInfo  * aShmTxInfo )
{
    idrSVP sSavepoint = {0/*UndoLsn*/, 0 /* Latch Stack Idx */};

    IDE_ASSERT( aStatistics != NULL );

    IDE_TEST( commit2Svp( aStatistics, aShmTxInfo, &sSavepoint )
              != IDE_SUCCESS );

    IDE_TEST( idrShmTxPendingOp::executePendingOpAll( aStatistics, aShmTxInfo )
              != IDE_SUCCESS );

    IDE_TEST( idrShmTxPendingOp::freeAllStFreePendingOpList( aStatistics,
                                                             aShmTxInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idrLogMgr::abort( idvSQL * aStatistics, iduShmTxInfo * aShmTxInfo )
{
    idrLSN             sLSN4Latch;
    UInt               sLatchIndex4Recv;
    iduShmLatchInfo  * sShmLstLatchInfo;
    idrSVP             sSavepoint = {0/*UndoLsn*/, 0 /* Latch Stack Idx */};

    IDE_ASSERT( aShmTxInfo != NULL );

    while( idrLogMgr::getLatchCntInStack( aShmTxInfo ) != 0 )
    {
        sShmLstLatchInfo = getLstLatchInfo( aShmTxInfo );

        sLSN4Latch = sShmLstLatchInfo->mLSN4Latch;

        sLatchIndex4Recv = idrLogMgr::getLatchCntInStack( aShmTxInfo ) - 1;

        idrLogMgr::crtSavepoint( &sSavepoint,
                                 sLatchIndex4Recv,
                                 sLSN4Latch );

        IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                    == IDE_SUCCESS );
    }

    idrLogMgr::crtSavepoint( aShmTxInfo, &sSavepoint, 0 );

    IDE_ASSERT( abort2Svp( aStatistics, aShmTxInfo, &sSavepoint ) == IDE_SUCCESS );

    IDE_ASSERT( idrShmTxPendingOp::executePendingOpAll( aStatistics, aShmTxInfo )
                == IDE_SUCCESS );

    IDE_ASSERT( idrShmTxPendingOp::freeAllStFreePendingOpList( aStatistics,
                                                            aShmTxInfo )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC idrLogMgr::clearLog2Svp( iduShmTxInfo   * aShmTxInfo,
                                idrSVP         * aSavepoint )
{
    IDE_ASSERT( aShmTxInfo->mLogOffset != IDR_NULL_LSN );
    IDE_ASSERT( aSavepoint->mLSN != IDR_NULL_LSN );

    aShmTxInfo->mLogOffset = aSavepoint->mLSN;

    return IDE_SUCCESS;
}

idBool idrLogMgr::isLatchAcquired( iduShmTxInfo      * aShmTxInfo,
                                   void              * aLatch,
                                   iduShmSXLatchMode   aMode )
{
    iduShmLatchStack   * sLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    SInt                 sStackIdx;
    idBool               sIsLocked = ID_FALSE;

    IDE_ASSERT( aShmTxInfo != NULL );

    sLatchStack = &aShmTxInfo->mLatchStack;

    sStackIdx = sLatchStack->mCurSize;

    while( sStackIdx != 0 )
    {
        sStackIdx--;

        sLatchInfo = sLatchStack->mArray + sStackIdx;

        if( ( sLatchInfo->mObject == aLatch ) && ( sLatchInfo->mMode == aMode ) )
        {
            sIsLocked = ID_TRUE;
        }
    }

    return sIsLocked;
}

idBool idrLogMgr::isSetLatchAcquired( iduShmTxInfo      * aShmTxInfo,
                                      void              * aLatch,
                                      iduShmSXLatchMode   aMode )
{
    iduShmLatchStack   * sSetLatchStack;
    iduShmLatchInfo    * sLatchInfo;
    SInt                 sStackIdx;
    idBool               sIsLocked = ID_FALSE;

    IDE_ASSERT( aShmTxInfo != NULL );

    sSetLatchStack = &aShmTxInfo->mSetLatchStack;

    sStackIdx = sSetLatchStack->mCurSize;

    while( sStackIdx != 0 )
    {
        sStackIdx--;

        sLatchInfo = sSetLatchStack->mArray + sStackIdx;

        if( (sLatchInfo->mObject == aLatch ) && ( sLatchInfo->mMode == aMode ) )
        {
            sIsLocked = ID_TRUE;
        }
    }

    return sIsLocked;
}

/*****************************************************************************
 * Description : LPID에 해당하는 Process의 aShmTxID를 가지는 Transaction의 Shared
 *               Memory Log를 화면에 출력한다.
 *
 * [IN] aLPID    - Logical Process ID
 * [IN] aShmTxID - Global Shared Memory Transaction ID
 *
 **************************************************************************/
IDE_RC idrLogMgr::dump( UInt aLPID, UInt aGblShmTxID )
{
    iduShmTxInfo   * sShmTxInfo;
    iduShmProcInfo * sShmProcInfo;

    sShmProcInfo = idwPMMgr::getProcInfo( aLPID );

    IDE_TEST_RAISE( sShmProcInfo->mState == IDU_SHM_PROC_STATE_NULL,
                    err_invalide_process_id );

    sShmTxInfo = idwPMMgr::getTxInfo( aLPID, aGblShmTxID );

    IDE_TEST_RAISE( sShmTxInfo == NULL, err_invalid_thread_id );

    dump( sShmTxInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalide_process_id );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_Invalid_Process_ID ) );
    }
    IDE_EXCEPTION( err_invalid_thread_id );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_Invalid_Thread_ID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************************
 * Description : LPID에 해당하는 Process의 모든 Shared Memory Transaction의 Shared
 *               Memory Log를 화면에 출력한다.
 *
 * [IN] aLPID - Logical Process ID
 *
 **************************************************************************/
IDE_RC idrLogMgr::dumpAllTx4Process( UInt aLPID )
{
    iduShmProcInfo  * sShmProcInfo;
    iduShmTxInfo    * sCurShmTxInfo;
    iduShmListNode  * sThrListNode;

    sShmProcInfo = idwPMMgr::getProcInfo( aLPID );

    IDE_TEST_RAISE( sShmProcInfo->mState == IDU_SHM_PROC_STATE_NULL,
                    err_invalide_process_id );

    sCurShmTxInfo = &sShmProcInfo->mMainTxInfo;

    idlOS::printf( "##### Begin Dump Process ShmTx Log [%"ID_UINT64_FMT"]\n",
                   (ULong)aLPID );

    while(1)
    {
        sThrListNode =
            (iduShmListNode*)IDU_SHM_GET_ADDR_PTR_CHECK( sCurShmTxInfo->mNode.mAddrNext );

        sCurShmTxInfo = (iduShmTxInfo*)iduShmList::getDataPtr( sThrListNode );
        IDE_ASSERT( sCurShmTxInfo != NULL );

        dump( sCurShmTxInfo );

        if( sCurShmTxInfo == &sShmProcInfo->mMainTxInfo )
        {
            break;
        }
    }

    idlOS::printf( "##### End Dump Process ShmTx Log [%"ID_UINT64_FMT"]\n",
                   (ULong)aLPID );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalide_process_id );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_Invalid_Process_ID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************************
 * Description : aShmTxInfo의 Shared Memory Transaction Log를 화면에 찍는다.
 *
 * [IN] aLPID    - Logical Process ID
 *
 **************************************************************************/
void idrLogMgr::dump( iduShmTxInfo * aShmTxInfo )
{
    SChar           * sCurLogPtr;
    idrLogHead        sLogHead;
    idrModuleType     sModuleType;
    UShort            sPrvLogSize;
    idrModuleLogType  sModuleLogType;
    UInt              sCurLogOffset;
    idrNTALogHead     sNTALogHead;

    idlOS::printf( "##### Begin Dump Shared Memory Transaction Log [%"ID_UINT32_FMT"]\n",
                   aShmTxInfo->mThrID );

    if( aShmTxInfo->mLogOffset != 0 )
    {
        sCurLogPtr  = aShmTxInfo->mLogBuffer + aShmTxInfo->mLogOffset;
        idlOS::memcpy( &sPrvLogSize,
                       sCurLogPtr - ID_SIZEOF(idrLogTail),
                       ID_SIZEOF(idrLogTail));

        sCurLogOffset = aShmTxInfo->mLogOffset;

        while( sCurLogOffset != 0 )
        {
            sCurLogPtr     -= sPrvLogSize;
            sCurLogOffset  -= sPrvLogSize;

            /* Log type에 따라서 Undo를 수행한다. */
            idlOS::memcpy( &sLogHead, sCurLogPtr, ID_SIZEOF(idrLogHead) );

            sModuleType    = IDR_GET_MODULE_TYPE( sLogHead.mType );
            sModuleLogType = IDR_GET_MODULE_LOG_TYPE( sLogHead.mType );

            if( IDR_IS_MODULE_LOG_CLR( sLogHead.mType ) == ID_FALSE )
            {
                idlOS::printf( "LSN:%4"ID_UINT32_FMT", VLOG: CLR:N, MT: %s, "
                               "LT: %"ID_UINT32_FMT", SZ: %"ID_UINT32_FMT"\n",
                               sCurLogOffset,
                               mStrModuleType[sModuleType],
                               sModuleLogType,
                               (UInt)sLogHead.mSize );
                idlOS::fflush( NULL );

                idlOS::memcpy( &sPrvLogSize,
                               sCurLogPtr - ID_SIZEOF(idrLogTail),
                               ID_SIZEOF(idrLogTail));
            }
            else
            {
                idlOS::memcpy( &sNTALogHead, sCurLogPtr, ID_SIZEOF(idrNTALogHead) );

                idlOS::printf( "LSN:%4"ID_UINT32_FMT", VLOG: CLR:Y, PrvLSN:%"ID_UINT32_FMT", "
                               "MT: %s, LT: %"ID_UINT32_FMT""
                               ", SZ: %"ID_UINT32_FMT"\n",
                               sCurLogOffset,
                               sNTALogHead.mPrvEndOffset,
                               mStrModuleType[sModuleType],
                               sModuleLogType,
                               (UInt)sLogHead.mSize );
                idlOS::fflush( NULL );

                /* 모든 Logical Undo함수는 Undo완료후에 내부에서 Thread의 Log Offset을
                 * 바꾸어야 한다. */
                idlOS::memcpy( &sPrvLogSize,
                               sCurLogPtr - ID_SIZEOF(idrLogTail),
                               ID_SIZEOF(UShort) );

            }

            if( sCurLogOffset == 0 )
            {
                break;
            }
        }
    }

    idlOS::printf( "##### End Dump Shared Memory Transaction Log [%"ID_UINT32_FMT"]\n",
                   aShmTxInfo->mThrID );
}
