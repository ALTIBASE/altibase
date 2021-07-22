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
 * $Id: sdnUpdate.cpp 89495 2020-12-14 05:19:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 index 관련 redo 함수에 대한 구현파일이다.
 *
 **********************************************************************/

#include <sdr.h>
#include <sdp.h>
#include <sdnReq.h>
#include <sdnbModule.h>
#include <sdnUpdate.h>
#include <sdnIndexCTL.h>
#include <smnManager.h>

extern smnIndexType * gSmnAllIndex[SMI_INDEXTYPE_COUNT];

/***********************************************************************
 * Description : redo_SDR_SDN_INSERT_INDEX_KEY
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_INSERT_INDEX_KEY( SChar       * aLogPtr,
                                                 UInt          aSize,
                                                 UChar       * aRecPtr,
                                                 sdrRedoInfo * /*aRedoInfo*/,
                                                 sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr  * sIndexPageHdr = NULL;
    UShort           sAllowedSize  = 0;
    scOffset         sKeyOffset    = 0;
    sdnbLKey       * sLKey;
    UShort           sKeyLength    = 0;
    SShort           sKeySlotSeq   = 0;

    IDE_ERROR( aLogPtr != NULL );
    IDE_ERROR( aSize   != 0 );
    IDE_ERROR( aRecPtr != NULL );

    sIndexPageHdr = sdpPhyPage::getHdr(sdpPhyPage::getPageStartPtr(aRecPtr));

    sKeyLength = aSize - ID_SIZEOF(SShort);
    IDE_ERROR( sdpPhyPage::canAllocSlot(sIndexPageHdr,
                                        sKeyLength,
                                        ID_TRUE,
                                        SDP_1BYTE_ALIGN_SIZE )
               == ID_TRUE );

    idlOS::memcpy(&sKeySlotSeq, aLogPtr,  (UInt)ID_SIZEOF(SShort));

    IDE_TEST( sdpPhyPage::allocSlot( sIndexPageHdr,
                                     sKeySlotSeq,
                                     sKeyLength,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     (UChar**)&sLKey,
                                     &sKeyOffset,
                                     SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_ERROR( sLKey != NULL );
    IDE_ERROR( sAllowedSize >= sKeyLength );
    IDE_ERROR( (UChar*)sLKey == aRecPtr );

    idlOS::memcpy( sLKey,
                   (UChar*)(aLogPtr + ID_SIZEOF(SShort)),
                   aSize -  (UInt)ID_SIZEOF(SShort) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIndexPageHdr != NULL )
    {
        ideLog::logMem( IDE_DUMP_0, 
                        (UChar *)sIndexPageHdr, 
                        SD_PAGE_SIZE,
                        "===================================================\n"
                        " sAllowedSize      : %u\n"
                        " sKeyLength        : %u\n"
                        " sKeySlotSeq       : %u\n"
                        " sKeyOffset        : %u\n",
                        sAllowedSize,
                        sKeyLength,
                        sKeySlotSeq,
                        sKeyOffset );
    }
    else
    {
        /* nothing to do... */
    }

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : SMR_DLT_REF_NTA 로그로부터 RollbackContextEx 판독
 *
 * aLogType [IN]  - 로그타입
 * aLogPtr  [IN]  - SMR_DLT_REF_NTA 로그의 RefOffset + ID_SIZEOF(sdrLogHdr)
 *                  해당하는 LogPtr
 * aContext [OUT] - RollbackContext
 *
 **********************************************************************/
void sdnUpdate::getRollbackContextEx( sdrLogType              aLogType,
                                      SChar                 * aLogPtr,
                                      sdnbRollbackContextEx * aContextEx )
{
    IDE_DASSERT( aLogPtr    != NULL );
    IDE_DASSERT( aContextEx != NULL );

    switch (aLogType)
    {
        case SDR_SDN_INSERT_DUP_KEY:

            idlOS::memcpy( (UChar*)aContextEx,
                           (UChar*)(aLogPtr +
                                    ID_SIZEOF(SShort) +
                                    ID_SIZEOF(UChar)),
                           ID_SIZEOF( sdnbRollbackContextEx ) );
            break;

        default:
            IDE_ASSERT(0);
    }
}


/***********************************************************************
 *
 * Description : SMR_DLT_REF_NTA 로그로부터 RollbackContext 판독
 *
 * aLogType [IN]  - 로그타입
 * aLogPtr  [IN]  - SMR_DLT_REF_NTA 로그의 RefOffset + ID_SIZEOF(sdrLogHdr)
 *                  해당하는 LogPtr
 * aContext [OUT] - RollbackContext
 *
 **********************************************************************/
void sdnUpdate::getRollbackContext( sdrLogType            aLogType,
                                    SChar               * aLogPtr,
                                    sdnbRollbackContext * aContext )
{
    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aContext != NULL );

    switch (aLogType)
    {
        case SDR_SDN_INSERT_UNIQUE_KEY:

            idlOS::memcpy( (UChar*)aContext,
                           (UChar*)(aLogPtr + ID_SIZEOF(SShort)),
                           ID_SIZEOF( sdnbRollbackContext ) );
            break;

        case SDR_SDN_DELETE_KEY_WITH_NTA:

            idlOS::memcpy( (UChar*)aContext,
                           (UChar*)aLogPtr,
                           ID_SIZEOF( sdnbRollbackContext ) );
            break;
        default:
            IDE_ASSERT(0);
    }
}

/***********************************************************************
 *
 * Description : SMR_DLT_REF_NTA 로그로부터 무결성 검증할 인덱스를
 *               트랜잭션의 VerfiyOIDList에 추가한다.
 *
 * aTrans    [IN]  - 트랜잭션
 * aLogPtr   [IN]  - SMR_DLT_REF_NTA 로그의 RefOffset에 해당하는 LogPtr
 * aTableOID [OUT] - 로그로부터 판독한 TableOID를 반환한다.
 * aIndexOID [OUT] - 로그로부터 판독한 IndexOID를 반환한다.
 * aSpaceID  [OUT] - 로그로부터 판독한 Index의 TableSpace ID를 반환한다.
 *
 **********************************************************************/
IDE_RC sdnUpdate::getIndexInfoToVerify( SChar     * aLogPtr,
                                        smOID     * aTableOID,
                                        smOID     * aIndexOID,
                                        scSpaceID * aSpaceID )
{
    sdrLogHdr             sLogHdr;
    sdnbRollbackContext   sContext;
    sdnbRollbackContextEx sContextEx;
    smnIndexHeader      * sIndexHeader = NULL;
    smcTableHeader      * sTableHeader = NULL;

    IDE_DASSERT( aLogPtr   != NULL );
    IDE_DASSERT( aTableOID != NULL );
    IDE_DASSERT( aIndexOID != NULL );
    IDE_DASSERT( aSpaceID  != NULL );
    
    IDE_DASSERT( smrRecoveryMgr::isVerifyIndexIntegrityLevel2() == ID_TRUE );

    *aTableOID = SM_NULL_OID;
    *aIndexOID = SM_NULL_OID;
    *aSpaceID  = SC_NULL_SPACEID;

    idlOS::memcpy( (UChar*)&sLogHdr, aLogPtr, ID_SIZEOF(sdrLogHdr) );

    if (sLogHdr.mType == SDR_SDN_INSERT_DUP_KEY)  
    {
        getRollbackContextEx( sLogHdr.mType, 
                              aLogPtr + ID_SIZEOF(sdrLogHdr), 
                              &sContextEx );

        sContext = sContextEx.mRollbackContext;
    }
    else
    {
        if ( (sLogHdr.mType == SDR_SDN_INSERT_UNIQUE_KEY) ||
             (sLogHdr.mType == SDR_SDN_DELETE_KEY_WITH_NTA) )
        {
            getRollbackContext( sLogHdr.mType, 
                                aLogPtr + ID_SIZEOF(sdrLogHdr), 
                                &sContext );
        }
        else
        {
            IDE_CONT( no_rollback_context );
        }
    }

    IDE_ERROR( smcTable::getTableHeaderFromOID( sContext.mTableOID,
                                                (void**)&sTableHeader )
               == IDE_SUCCESS );

    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID( sTableHeader,
                                                                 sContext.mIndexID );

    IDE_ERROR( sContext.mTableOID == sIndexHeader->mTableOID );

    *aIndexOID = sIndexHeader->mSelfOID;
    *aTableOID = sIndexHeader->mTableOID;
    *aSpaceID  = SC_MAKE_SPACE(sIndexHeader->mIndexSegDesc);

    IDE_EXCEPTION_CONT( no_rollback_context );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    {
        SChar  * sOutBuffer4Dump;

        ideLog::log( IDE_DUMP_0,
                     "sContext.mTableOID : %lu\n"
                     "sContext.mIndexID  : %u\n"
                     "sContext.mTxInfo[0]: %u\n"
                     "sContext.mTxInfo[1]: %u\n",
                     sContext.mTableOID, 
                     sContext.mIndexID,
                     sContext.mTxInfo[0],
                     sContext.mTxInfo[1] );

        if( iduMemMgr::calloc( IDU_MEM_SM_SDN, 1,
                               ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                               (void**)&sOutBuffer4Dump )
            == IDE_SUCCESS )
        {
            if( sIndexHeader != NULL )
            {
                if( smnManager::dumpCommonHeader( sIndexHeader,
                                                  sOutBuffer4Dump,
                                                  IDE_DUMP_DEST_LIMIT )
                    == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SM_0, "%s\n", sOutBuffer4Dump );
                }
            }
            IDE_ASSERT( iduMemMgr::free( sOutBuffer4Dump ) == IDE_SUCCESS );
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : redo_SDR_SDN_INSERT_UNIQUE_KEY
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_INSERT_UNIQUE_KEY( SChar       * aLogPtr,
                                                  UInt          aSize,
                                                  UChar       * aRecPtr,
                                                  sdrRedoInfo * /* aRedoInfo */,
                                                  sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr       * sIndexPageHdr;
    UShort                sAllowedSize = 0;
    scOffset              sKeyOffset   = 0;
    sdnbLKey            * sLKey;
    UShort                sKeyLength   = 0;
    SShort                sKeySlotSeq  = 0;
    sdnbRollbackContext   sContext;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aSize   != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sIndexPageHdr = sdpPhyPage::getHdr(sdpPhyPage::getPageStartPtr(aRecPtr));

    getRollbackContext( SDR_SDN_INSERT_UNIQUE_KEY,
                        aLogPtr,
                        &sContext );

    idlOS::memcpy(&sKeySlotSeq, aLogPtr, (UInt)ID_SIZEOF(SShort));

    sKeyLength = aSize - (ID_SIZEOF(sdnbRollbackContext) + ID_SIZEOF(SShort));

    IDE_ERROR( sdpPhyPage::canAllocSlot( sIndexPageHdr,
                                         sKeyLength,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE )
               == ID_TRUE );

    IDE_TEST( sdpPhyPage::allocSlot( sIndexPageHdr,
                                     sKeySlotSeq,
                                     sKeyLength,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     (UChar**)&sLKey,
                                     &sKeyOffset,
                                     SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_ERROR( sLKey != NULL );
    IDE_ERROR( sAllowedSize >= sKeyLength );
    IDE_ERROR( (UChar*)sLKey == aRecPtr );

    idlOS::memcpy( sLKey,
                   (UChar*)(aLogPtr +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(sdnbRollbackContext)),
                   aSize -  (UInt)(ID_SIZEOF(SShort) +
                                   ID_SIZEOF(sdnbRollbackContext)));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::logMem( IDE_DUMP_0, 
                    (UChar *)sIndexPageHdr, 
                    SD_PAGE_SIZE,
                    "===================================================\n"
                    " sAllowedSize      : %u\n"
                    " sKeyLength        : %u\n"
                    " sKeySlotSeq       : %u\n"
                    " sKeyOffset        : %u\n",
                    sAllowedSize,
                    sKeyLength,
                    sKeySlotSeq,
                    sKeyOffset );

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_SDN_INSERT_UNIQUE_KEY
 ***********************************************************************/
IDE_RC sdnUpdate::undo_SDR_SDN_INSERT_UNIQUE_KEY( idvSQL   * aStatistics,
                                                  void     * aTrans,
                                                  sdrMtx   * aMtx,
                                                  scGRID     /*aGRID*/,
                                                  SChar    * aLogPtr,
                                                  UInt       aSize )
{
    ULong                 sTempKeyBuf[ SDNB_MAX_KEY_BUFFER_SIZE / ID_SIZEOF(ULong) ];
    sdnbLKey            * sLeafKey;
    sdnbRollbackContext   sContext;
    smnIndexHeader      * sIndexHeader = NULL;
    smcTableHeader      * sTableHeader = NULL;
    UInt                  sIndexTypeID;
    UInt                  sTableTypeID;
    smnIndexModule      * sIndexModule;

    getRollbackContext( SDR_SDN_INSERT_UNIQUE_KEY,
                        aLogPtr,
                        &sContext );

    idlOS::memcpy( sTempKeyBuf,
                   aLogPtr + ID_SIZEOF(SShort) + ID_SIZEOF(sdnbRollbackContext),
                   aSize - (ID_SIZEOF(SShort) + ID_SIZEOF(sdnbRollbackContext)) );

    IDE_ERROR( smcTable::getTableHeaderFromOID( sContext.mTableOID,
                                                (void**)&sTableHeader )
               == IDE_SUCCESS );
    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID( sTableHeader,
                                                                 sContext.mIndexID );

    IDE_ERROR( sContext.mTableOID == sIndexHeader->mTableOID );

    sLeafKey = (sdnbLKey*)sTempKeyBuf;

    sIndexTypeID = sIndexHeader->mType;
    sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(sTableHeader->mFlag);
    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];

    IDE_TEST( sIndexModule->mInsertKeyRollback( aStatistics,
                                                aMtx,
                                                (void*)sIndexHeader,
                                                (SChar*)(SDNB_LKEY_KEY_PTR( sLeafKey )),
                                                SD_MAKE_SID( sLeafKey->mRowPID,
                                                             sLeafKey->mRowSlotNum ),
                                                (UChar*)&sContext,
                                                ID_FALSE ) /* aIsDupKey */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    {
        SChar  * sOutBuffer4Dump;

        ideLog::log( IDE_DUMP_0,
                     "sContext.mTableOID : %lu\n"
                     "sContext.mIndexID  : %u\n"
                     "sContext.mTxInfo[0]: %u\n"
                     "sContext.mTxInfo[1]: %u\n",
                     sContext.mTableOID, 
                     sContext.mIndexID,
                     sContext.mTxInfo[0],
                     sContext.mTxInfo[1] );

        if( iduMemMgr::calloc( IDU_MEM_SM_SDN, 1,
                               ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                               (void**)&sOutBuffer4Dump )
            == IDE_SUCCESS )
        {
            if( sIndexHeader != NULL )
            {
                if( smnManager::dumpCommonHeader( sIndexHeader,
                                                  sOutBuffer4Dump,
                                                  IDE_DUMP_DEST_LIMIT )
                    == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SM_0, "%s\n", sOutBuffer4Dump );
                }
            }
            IDE_ASSERT( iduMemMgr::free( sOutBuffer4Dump ) == IDE_SUCCESS );
        }

    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( aTrans,
                                               SMR_RTOI_TYPE_INDEX,
                                               sContext.mTableOID,
                                               sContext.mIndexID,
                                               SC_NULL_SPACEID,
                                               SC_NULL_PID );

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : redo_SDR_SDN_INSERT_DUP_KEY
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_INSERT_DUP_KEY( SChar       * aLogPtr,
                                               UInt          aSize,
                                               UChar       * aRecPtr,
                                               sdrRedoInfo * /* aRedoInfo */,
                                               sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr       * sIndexPageHdr;
    sdnbLKey            * sLeafKey;
    SShort                sKeySlotSeq;
    UChar               * sSlotDirPtr;
    UShort                sAllowedSize = 0;
    scOffset              sKeyOffset   = 0;
    UShort                sKeyLength   = 0;
    UChar                 sRemoveInsert;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aSize   != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sIndexPageHdr = sdpPhyPage::getHdr(sdpPhyPage::getPageStartPtr(aRecPtr));

    idlOS::memcpy(&sKeySlotSeq, aLogPtr, (UInt)ID_SIZEOF(SShort));
    idlOS::memcpy(&sRemoveInsert,
                  aLogPtr + ID_SIZEOF(SShort),
                  ID_SIZEOF(UChar));

    sKeyLength = aSize - (UInt)( ID_SIZEOF(SShort) +
                                 ID_SIZEOF(UChar) +
                                 ID_SIZEOF(sdnbRollbackContextEx) );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sIndexPageHdr);

    if( sRemoveInsert == ID_TRUE )
    {
        IDE_ERROR( sdpPhyPage::canAllocSlot( sIndexPageHdr,
                                             sKeyLength,
                                             ID_TRUE,
                                             SDP_1BYTE_ALIGN_SIZE )
                   == ID_TRUE );

        IDE_TEST( sdpPhyPage::allocSlot( sIndexPageHdr,
                                         sKeySlotSeq,
                                         sKeyLength,
                                         ID_TRUE,
                                         &sAllowedSize,
                                         (UChar**)&sLeafKey,
                                         &sKeyOffset,
                                         SDP_1BYTE_ALIGN_SIZE )
                  != IDE_SUCCESS );

        IDE_ERROR( sLeafKey != NULL );
        IDE_ERROR( (UChar*)sLeafKey == aRecPtr );
    }
    else
    {
        IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                            sKeySlotSeq,
                                                            (UChar**)&sLeafKey )
                   == IDE_SUCCESS );
    }

    idlOS::memcpy( sLeafKey,
                   (UChar*)( aLogPtr +
                             ID_SIZEOF(SShort) +
                             ID_SIZEOF(UChar) +
                             ID_SIZEOF(sdnbRollbackContextEx) ),
                   sKeyLength );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "sRemoveInsert     : %u\n"
                 "sAllowedSize      : %u\n"
                 "sKeyLength        : %u\n"
                 "sKeySlotSeq       : %u\n"
                 "sKeyOffset        : %u\n",
                 sRemoveInsert,
                 sAllowedSize,
                 sKeyLength,
                 sKeySlotSeq,
                 sKeyOffset );

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_SDN_INSERT_DUP_KEY
 ***********************************************************************/
IDE_RC sdnUpdate::undo_SDR_SDN_INSERT_DUP_KEY( idvSQL   * aStatistics,
                                               void     * aTrans,
                                               sdrMtx   * aMtx,
                                               scGRID     /*aGRID*/,
                                               SChar    * aLogPtr,
                                               UInt       aSize )
{
    ULong                   sTempKeyBuf[ SDNB_MAX_KEY_BUFFER_SIZE / ID_SIZEOF(ULong) ];
    sdnbLKey              * sLeafKey;
    sdnbRollbackContextEx   sContextEx;
    smnIndexHeader        * sIndexHeader = NULL;
    smcTableHeader        * sTableHeader = NULL;
    UInt                    sIndexTypeID;
    UInt                    sTableTypeID;
    smnIndexModule        * sIndexModule;
    UInt                    sKeyLength;

    sKeyLength = aSize - (ID_SIZEOF(SShort) +
                          ID_SIZEOF(UChar) +
                          ID_SIZEOF(sdnbRollbackContextEx));

    getRollbackContextEx( SDR_SDN_INSERT_DUP_KEY,
                          aLogPtr,
                          &sContextEx );

    aLogPtr += ID_SIZEOF(SShort);
    aLogPtr += ID_SIZEOF(UChar);
    aLogPtr += ID_SIZEOF(sdnbRollbackContextEx);

    idlOS::memcpy( sTempKeyBuf, aLogPtr, sKeyLength );

    IDE_ERROR( smcTable::getTableHeaderFromOID(
                                       sContextEx.mRollbackContext.mTableOID,
                                       (void**)&sTableHeader )
               == IDE_SUCCESS );
    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID( sTableHeader,
                                                                 sContextEx.mRollbackContext.mIndexID );

    IDE_ERROR( sContextEx.mRollbackContext.mTableOID ==
               sIndexHeader->mTableOID );

    sLeafKey     = (sdnbLKey*)sTempKeyBuf;
    sIndexTypeID = sIndexHeader->mType;
    sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(sTableHeader->mFlag);

    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];

    IDE_TEST( sIndexModule->mInsertKeyRollback( aStatistics,
                                                aMtx,
                                                (void*)sIndexHeader,
                                                (SChar*)(SDNB_LKEY_KEY_PTR( sLeafKey )),
                                                SD_MAKE_SID( sLeafKey->mRowPID,
                                                             sLeafKey->mRowSlotNum ),
                                                (UChar*)&sContextEx,
                                                ID_TRUE ) /* aIsDupKey */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    {
        SChar  * sOutBuffer4Dump;

        ideLog::log( IDE_DUMP_0,
                     "ContextEx.RollbackContext.TableOID : %lu\n"
                     "ContextEx.RollbackContext.IndexID  : %u\n"
                     "ContextEx.RollbackContext.TxInfo[0]: %u\n"
                     "ContextEx.RollbackContext.TxInfo[1]: %u\n"
                     "ContextEx.TxInfoEx.mCreateCSCN     : %llu\n"
                     "ContextEx.TxInfoEx.mCreateTSS      : %llu\n"
                     "ContextEx.TxInfoEx.mLimitCSCN      : %llu\n"
                     "ContextEx.TxInfoEx.mLimitTSS       : %llu\n",
                     sContextEx.mRollbackContext.mTableOID, 
                     sContextEx.mRollbackContext.mIndexID,
                     sContextEx.mRollbackContext.mTxInfo[0],
                     sContextEx.mRollbackContext.mTxInfo[1],
                     SM_SCN_TO_LONG( sContextEx.mTxInfoEx.mCreateCSCN ),
                     sContextEx.mTxInfoEx.mCreateTSS,
                     SM_SCN_TO_LONG( sContextEx.mTxInfoEx.mLimitCSCN ),
                     sContextEx.mTxInfoEx.mLimitTSS );

        if( sIndexHeader != NULL )
        {
            if( iduMemMgr::calloc( IDU_MEM_SM_SDN, 1,
                                   ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                   (void**)&sOutBuffer4Dump )
                == IDE_SUCCESS )
            {
                if( smnManager::dumpCommonHeader( sIndexHeader,
                                                  sOutBuffer4Dump,
                                                  IDE_DUMP_DEST_LIMIT )
                    == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SM_0, "%s\n", sOutBuffer4Dump );
                }
                IDE_ASSERT( iduMemMgr::free( sOutBuffer4Dump ) == IDE_SUCCESS );
            }
        }
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( aTrans,
                                               SMR_RTOI_TYPE_INDEX,
                                               sContextEx.mRollbackContext.mTableOID, 
                                               sContextEx.mRollbackContext.mIndexID,
                                               SC_NULL_SPACEID,
                                               SC_NULL_PID );

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_SDN_FREE_INDEX_KEY
 ***********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_FREE_INDEX_KEY( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr*  sPageHdr;
    UChar*          sFreeSlotPtr;
    UShort          sKeyLength;

    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aData != NULL );

    IDE_ERROR( aLength == ID_SIZEOF(UShort) );

    sFreeSlotPtr = aPagePtr;
    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sFreeSlotPtr);

    idlOS::memcpy(&sKeyLength, aData, ID_SIZEOF(UShort));

    IDE_TEST( sdpPhyPage::freeSlot(sPageHdr,
                                   sFreeSlotPtr,
                                   sKeyLength,
                                   ID_TRUE,
                                   SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_SDN_DELETE_INDEX_KEY_WITH_ROW
 ***********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_DELETE_KEY_WITH_NTA( SChar       * aData,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr * sPageHdr;
    sdnbLKey      * sLeafKey;
    UChar           sRemoveInsert = 0;
    UShort          sKeyLength    = 0;
    UShort          sAllowedSize  = 0;
    scOffset        sKeyOffset    = 0;
    SShort          sKeySlotSeq   = 0;

    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aData != NULL );
    IDE_DASSERT( aLength != 0 );

    sLeafKey = (sdnbLKey*)aPagePtr;
    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sLeafKey);

    idlOS::memcpy( &sRemoveInsert,
                   aData + ID_SIZEOF(sdnbRollbackContext),
                   ID_SIZEOF(UChar) );
    idlOS::memcpy( &sKeySlotSeq,
                   aData + (ID_SIZEOF(sdnbRollbackContext) +
                            ID_SIZEOF(UChar)),
                   ID_SIZEOF(SShort) );

    sKeyLength = aLength - (ID_SIZEOF(sdnbRollbackContext) +
                           ID_SIZEOF(UChar) +
                           ID_SIZEOF(SShort));

    if( sRemoveInsert == ID_TRUE )
    {
        IDE_ERROR( sdpPhyPage::canAllocSlot( sPageHdr,
                                             sKeyLength,
                                             ID_TRUE,
                                             SDP_1BYTE_ALIGN_SIZE )
                   == ID_TRUE );

        IDE_TEST( sdpPhyPage::allocSlot( sPageHdr,
                                         sKeySlotSeq,
                                         sKeyLength,
                                         ID_TRUE,
                                         &sAllowedSize,
                                         (UChar**)&sLeafKey,
                                         &sKeyOffset,
                                         SDP_1BYTE_ALIGN_SIZE )
                  != IDE_SUCCESS );

        IDE_ERROR( sLeafKey != NULL );
        IDE_ERROR( (UChar*)sLeafKey == aPagePtr );
    }

    idlOS::memcpy( sLeafKey,
                   (UChar*)( aData + (ID_SIZEOF(sdnbRollbackContext) +
                                      ID_SIZEOF(UChar) +
                                      ID_SIZEOF(SShort)) ),
                   sKeyLength );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "sRemoveInsert     : %u\n"
                 "sAllowedSize      : %u\n"
                 "sKeyLength        : %u\n"
                 "sKeySlotSeq       : %u\n"
                 "sKeyOffset        : %u\n",
                 sRemoveInsert,
                 sAllowedSize,
                 sKeyLength,
                 sKeySlotSeq,
                 sKeyOffset );

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_SDN_DELETE_KEY_WITH_NTA
 ***********************************************************************/
IDE_RC sdnUpdate::undo_SDR_SDN_DELETE_KEY_WITH_NTA( idvSQL   * aStatistics,
                                                    void     * aTrans,
                                                    sdrMtx   * aMtx,
                                                    scGRID     /*aGRID*/,
                                                    SChar    * aLogPtr,
                                                    UInt       aSize )
{
    ULong                 sTempKeyBuf[ SDNB_MAX_KEY_BUFFER_SIZE / ID_SIZEOF(ULong) ];
    sdnbLKey            * sLeafKey;
    sdnbRollbackContext   sContext;
    smnIndexHeader      * sIndexHeader = NULL;
    smcTableHeader      * sTableHeader = NULL;
    smnIndexModule      * sIndexModule;
    UInt                  sIndexTypeID;
    UInt                  sTableTypeID;

    getRollbackContext( SDR_SDN_DELETE_KEY_WITH_NTA,
                        aLogPtr,
                        &sContext );

    idlOS::memcpy( sTempKeyBuf,
                   aLogPtr + (ID_SIZEOF(sdnbRollbackContext) +
                              ID_SIZEOF(UChar) +
                              ID_SIZEOF(SShort)),
                   aSize - (ID_SIZEOF(sdnbRollbackContext) +
                            ID_SIZEOF(UChar) +
                            ID_SIZEOF(SShort)) );

    IDE_ERROR( smcTable::getTableHeaderFromOID( 
                    sContext.mTableOID,
                    (void**)&sTableHeader )
                == IDE_SUCCESS );
    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID( sTableHeader,
                                                                 sContext.mIndexID );

    IDE_ERROR( sContext.mTableOID == sIndexHeader->mTableOID );

    sLeafKey = (sdnbLKey*)sTempKeyBuf;

    sIndexTypeID = sIndexHeader->mType;
    sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(sTableHeader->mFlag);

    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
    IDE_ERROR( sIndexModule != NULL );

    IDE_TEST( sIndexModule->mDeleteKeyRollback(
                                   aStatistics,
                                   aMtx,
                                   (void*)sIndexHeader,
                                   (SChar*)(SDNB_LKEY_KEY_PTR( sLeafKey )),
                                   SD_MAKE_SID( sLeafKey->mRowPID,
                                                sLeafKey->mRowSlotNum ),
                                   (UChar*)&sContext )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        SChar  * sOutBuffer4Dump;

        ideLog::log( IDE_DUMP_0,
                     "sContext.mTableOID : %lu\n"
                     "sContext.mIndexID  : %u\n"
                     "sContext.mTxInfo[0]: %u\n"
                     "sContext.mTxInfo[1]: %u\n",
                     sContext.mTableOID, 
                     sContext.mIndexID,
                     sContext.mTxInfo[0],
                     sContext.mTxInfo[1] );

        if( sIndexHeader != NULL )
        {
            if( iduMemMgr::calloc( IDU_MEM_SM_SDN, 1,
                                   ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                   (void**)&sOutBuffer4Dump )
                == IDE_SUCCESS )
            {
                if( smnManager::dumpCommonHeader( sIndexHeader,
                                                  sOutBuffer4Dump,
                                                  IDE_DUMP_DEST_LIMIT )
                    == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SM_0, "%s\n", sOutBuffer4Dump );
                }
                IDE_ASSERT( iduMemMgr::free( sOutBuffer4Dump ) == IDE_SUCCESS );
            }
        }
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( aTrans,
                                               SMR_RTOI_TYPE_INDEX,
                                               sContext.mTableOID,
                                               sContext.mIndexID,
                                               SC_NULL_SPACEID,
                                               SC_NULL_PID );

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_SDN_FREE_KEYS
 ***********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_FREE_KEYS( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr     * sPageHdr;
    sdnbNodeHdr       * sLogicalHdr;
    SShort              i;
    UShort              sKeyLength;
    UChar             * sKey;
    UChar             * sSlotDirPtr;
    UShort              sFromSeq;
    UShort              sToSeq;
    sdnbColLenInfoList  sColLenInfoList;

    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aData != NULL );

    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr);

    sColLenInfoList.mColumnCount = aData[0];

    IDE_ERROR( SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList ) <=
               SMI_MAX_IDX_COLUMNS + 1  );

    idlOS::memcpy(&sColLenInfoList, aData,
                  SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList ) );
    aData += SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList );
    idlOS::memcpy(&sFromSeq, aData, ID_SIZEOF(UShort));
    aData += ID_SIZEOF(UShort);
    idlOS::memcpy(&sToSeq, aData, ID_SIZEOF(UShort));

    IDE_ERROR( aLength == ((ID_SIZEOF(UShort) * 2) +
                           SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList )) );

    sLogicalHdr  = (sdnbNodeHdr*)
                            sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);

    for( i = sToSeq; i >= sFromSeq; i-- )
    {
        IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                            i,
                                                            &sKey )
                   == IDE_SUCCESS );

        sKeyLength = sdnbBTree::getKeyLength( &sColLenInfoList,
                                              sKey,
                                              SDNB_IS_LEAF_NODE(sLogicalHdr) ) ;

        IDE_TEST( sdpPhyPage::freeSlot(sPageHdr,
                                       i,
                                       sKeyLength,
                                       SDP_1BYTE_ALIGN_SIZE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SDR_SDN_COMPACT_INDEX_PAGE
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_COMPACT_INDEX_PAGE( SChar       * aLogPtr,
                                                   UInt          aSize,
                                                   UChar       * aRecPtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr      * sIdxPageHdr;
    sdnbNodeHdr        * sNodeHdr;
    sdnbColLenInfoList   sColLenInfoList;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aRecPtr != NULL );

    sIdxPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sNodeHdr = (sdnbNodeHdr*)
               sdpPhyPage::getLogicalHdrStartPtr((UChar*)sIdxPageHdr);

    sColLenInfoList.mColumnCount = aLogPtr[0];
    IDE_ERROR( SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList )
                <= SMI_MAX_IDX_COLUMNS + 1  );
    idlOS::memcpy(&sColLenInfoList,
                  aLogPtr,
                  SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList ) );

    IDE_ERROR( aSize == SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList ) );

    if( SDNB_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        IDE_ERROR( sdnbBTree::compactPageLeaf( NULL, /* sdnbHeader */
                                               NULL,
                                               sIdxPageHdr,
                                               &sColLenInfoList)
                   == IDE_SUCCESS );
    }
    else
    {
        IDE_ERROR( sdnbBTree::compactPageInternal( NULL, /* sdnbHeader */
                                                   sIdxPageHdr,
                                                   &sColLenInfoList)
                   == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if 0
/***********************************************************************
 * Description : SDR_SDN_MAKE_CHAINED_KEYS
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_MAKE_CHAINED_KEYS( SChar       * aLogPtr,
                                                  UInt          aSize,
                                                  UChar       * aRecPtr,
                                                  sdrRedoInfo * /* aRedoInfo */,
                                                  sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr  * sPageHdr;
    UChar          * sSlotDirPtr;
    UShort           sKeyCount;
    UInt             i;
    UChar            sCTSlotNum;
    sdnbLKey       * sLeafKey;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize == ID_SIZEOF(UChar) );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
    sKeyCount   = sdpSlotDirectory::getCount(sSlotDirPtr);

    idlOS::memcpy(&sCTSlotNum, aLogPtr, ID_SIZEOF(UChar));

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar*)sSlotDirPtr,
                                                            i,
                                                            (UChar**)&sLeafKey )
                   == IDE_SUCCESS );

        if( (SDNB_GET_STATE( sLeafKey ) == SDNB_KEY_DEAD) ||
            (SDNB_GET_STATE( sLeafKey ) == SDNB_KEY_STABLE) )
        {
            continue;
        }

        if( (SDNB_GET_CCTS_NO( sLeafKey ) == sCTSlotNum) &&
            (SDNB_GET_CHAINED_CCTS( sLeafKey ) == SDN_CHAINED_NO) )
        {
            SDNB_SET_CHAINED_CCTS( sLeafKey, SDN_CHAINED_YES );
        }
        if( (SDNB_GET_LCTS_NO( sLeafKey ) == sCTSlotNum) &&
            (SDNB_GET_CHAINED_LCTS( sLeafKey ) == SDN_CHAINED_NO) )
        {
            SDNB_SET_CHAINED_LCTS( sLeafKey, SDN_CHAINED_YES );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/***********************************************************************
 * Description : SDR_SDN_MAKE_UNCHAINED_KEYS
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_MAKE_UNCHAINED_KEYS( SChar       * aLogPtr,
                                                    UInt          aSize,
                                                    UChar       * aRecPtr,
                                                    sdrRedoInfo * /* aRedoInfo */,
                                                    sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr  * sPageHdr;
    UChar          * sSlotDirPtr;
    sdnbLKey       * sLeafKey;
    UShort           sUnchainedKeyCount;
    UChar          * sUnchainedKey;
    UShort           sSlotNum;
    UInt             sCursor = 0;
    UChar            sChainedCCTS;
    UChar            sChainedLCTS;


    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);

    idlOS::memcpy( &sUnchainedKeyCount, aLogPtr, ID_SIZEOF(UShort) );
    sUnchainedKey = (UChar*)(aLogPtr + ID_SIZEOF(UShort));

    while( sUnchainedKeyCount > 0 )
    {
        idlOS::memcpy( &sSlotNum, sUnchainedKey + sCursor, ID_SIZEOF(UShort) );
        sCursor += 2;
        sChainedCCTS = *(sUnchainedKey + sCursor);
        sCursor += 1;
        sChainedLCTS = *(sUnchainedKey + sCursor);
        sCursor += 1;

        IDE_ERROR( sCursor <= (aSize - ID_SIZEOF(UShort)) );

        IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar*)sSlotDirPtr,
                                                             sSlotNum,
                                                             (UChar**)&sLeafKey )
                   == IDE_SUCCESS );
        if( sChainedCCTS == SDN_CHAINED_YES )
        {
            sUnchainedKeyCount--;
            SDNB_SET_CHAINED_CCTS( sLeafKey, SDN_CHAINED_NO );
        }
        if( sChainedLCTS == SDN_CHAINED_YES )
        {
            sUnchainedKeyCount--;
            SDNB_SET_CHAINED_LCTS( sLeafKey, SDN_CHAINED_NO );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

/***********************************************************************
 * Description : SDR_SDN_KEY_STAMPING
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_KEY_STAMPING( SChar       * aLogPtr,
                                             UInt          aSize,
                                             UChar       * aRecPtr,
                                             sdrRedoInfo * /* aRedoInfo */,
                                             sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr      * sPageHdr;
    UChar              * sSlotDirPtr;
    UShort               sSlotCount;
    UInt                 i;
    UChar                sCTSlotNum;
    sdnbLKey           * sLeafKey;
    UInt                 sKeyLen;
    sdnbNodeHdr        * sNodeHdr;
    sdnbColLenInfoList   sColLenInfoList;
    ULong                sTempBuf[SD_PAGE_SIZE / ID_SIZEOF(ULong)];
    UChar              * sKeyList     = (UChar *)sTempBuf;
    UShort               sKeyListSize = 0;
    sdnbTBKStamping      sKeyEntry;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sNodeHdr = (sdnbNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPageHdr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
    sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

    idlOS::memcpy(&sCTSlotNum, aLogPtr, ID_SIZEOF(UChar));
    aLogPtr += ID_SIZEOF(UChar);

    sColLenInfoList.mColumnCount = aLogPtr[0];
    IDE_ERROR( SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList ) <= SMI_MAX_IDX_COLUMNS + 1  );
    idlOS::memcpy(&sColLenInfoList, aLogPtr, SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList ) );

    if ( sCTSlotNum == SDN_CTS_IN_KEY )
    {
        /* 
         * CTS Slot Num이 SDN_CTS_IN_KEY 이면,
         * TBK STAMPING을 수행한다. (BUG-44973)
         */
        aLogPtr += SDNB_COLLENINFO_LIST_SIZE( sColLenInfoList );

        idlOS::memcpy( &sKeyListSize,
                       aLogPtr,
                       ID_SIZEOF(UShort) );
        aLogPtr += ID_SIZEOF(UShort);

        idlOS::memcpy( sKeyList,
                       aLogPtr,
                       sKeyListSize );

        IDE_ERROR( aSize == ( ID_SIZEOF(UChar) + SDNB_COLLENINFO_LIST_SIZE(sColLenInfoList) +
                              ID_SIZEOF(UShort) + sKeyListSize ) );

        for ( i = 0; i < sKeyListSize; i += ID_SIZEOF(sdnbTBKStamping) )
        {
            sKeyEntry = *(sdnbTBKStamping *)(sKeyList + i);

            sLeafKey = (sdnbLKey *)( ((UChar *)sPageHdr) + sKeyEntry.mSlotOffset );

            if ( SDNB_IS_TBK_STAMPING_WITH_CSCN( &sKeyEntry ) == ID_TRUE )
            {
                SDNB_SET_CCTS_NO( sLeafKey, SDN_CTS_INFINITE );

                if ( SDNB_GET_STATE( sLeafKey ) == SDNB_KEY_UNSTABLE )
                {
                    SDNB_SET_STATE( sLeafKey, SDNB_KEY_STABLE );
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* nothing to do */
            }

            if ( SDNB_IS_TBK_STAMPING_WITH_LSCN( &sKeyEntry ) == ID_TRUE )
            {
                SDNB_SET_STATE( sLeafKey, SDNB_KEY_DEAD );
                SDNB_SET_CCTS_NO( sLeafKey, SDN_CTS_INFINITE );
                SDNB_SET_LCTS_NO( sLeafKey, SDN_CTS_INFINITE );
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* 
         * TBT에 대한 SOFT KEY STAMPING을 수행한다.
         */
        IDE_ERROR( aSize == ( ID_SIZEOF(UChar) + SDNB_COLLENINFO_LIST_SIZE(sColLenInfoList) ) );

        for ( i = 0; i < sSlotCount; i++ )
        {
            IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar *)sSlotDirPtr,
                                                                i,
                                                                (UChar **)&sLeafKey )
                    == IDE_SUCCESS );

            if ( SDNB_GET_CCTS_NO( sLeafKey ) == sCTSlotNum )
            {
                SDNB_SET_CCTS_NO( sLeafKey, SDN_CTS_INFINITE );
                SDNB_SET_DUPLICATED( sLeafKey, SDNB_DUPKEY_NO );

                /*
                 * To fix BUG-23337
                 */
                if ( SDNB_GET_STATE( sLeafKey ) == SDNB_KEY_UNSTABLE )
                {
                    SDNB_SET_STATE( sLeafKey, SDNB_KEY_STABLE );
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* nothing to do */
            }

            if ( SDNB_GET_LCTS_NO( sLeafKey ) == sCTSlotNum )
            {
                sKeyLen = sdnbBTree::getKeyLength( &sColLenInfoList,
                                                   (UChar *)sLeafKey,
                                                   ID_TRUE );
                sNodeHdr->mTotalDeadKeySize += sKeyLen + ID_SIZEOF(sdpSlotEntry);

                SDNB_SET_CCTS_NO( sLeafKey, SDN_CTS_INFINITE );
                SDNB_SET_LCTS_NO( sLeafKey, SDN_CTS_INFINITE );
                SDNB_SET_STATE( sLeafKey, SDNB_KEY_DEAD );
                SDNB_SET_DUPLICATED( sLeafKey, SDNB_DUPKEY_NO );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : SDR_SDN_INIT_CTL
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_INIT_CTL( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /* aRedoInfo */,
                                         sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr  * sPageHdr;
    UChar            sInitSize;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize == ID_SIZEOF(UChar) );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    idlOS::memcpy(&sInitSize, aLogPtr, ID_SIZEOF(UChar));

    IDE_ERROR( sdnIndexCTL::initLow( sPageHdr, sInitSize )
               == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SDR_SDN_EXTEND_CTL
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_EXTEND_CTL( SChar       * aLogPtr,
                                           UInt          aSize,
                                           UChar       * aRecPtr,
                                           sdrRedoInfo * /* aRedoInfo */,
                                           sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr  * sPageHdr;
    UChar            sCTSlotNum;
    idBool           sSuccess;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize == ID_SIZEOF(UChar) );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    idlOS::memcpy(&sCTSlotNum, aLogPtr, ID_SIZEOF(UChar));

    IDE_ERROR( sdnIndexCTL::extend( NULL, NULL, sPageHdr, ID_FALSE, &sSuccess )
               == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SDR_SDN_FREE_CTS
 **********************************************************************/
IDE_RC sdnUpdate::redo_SDR_SDN_FREE_CTS( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /* aRedoInfo */,
                                         sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr  * sPageHdr;
    UChar            sCTSlotNum;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize == ID_SIZEOF(UChar) );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    idlOS::memcpy(&sCTSlotNum, aLogPtr, ID_SIZEOF(UChar));

    sdnIndexCTL::freeCTS( NULL, sPageHdr, sCTSlotNum, ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


