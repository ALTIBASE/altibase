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
 * $Id: stndrUpdate.cpp 88414 2020-08-25 04:45:02Z justin.kwon $
 *
 * Description :
 *
 * 본 파일은 Disk R-Tree Index 관련 redo/undo 함수에 대한 구현파일이다.
 *
 **********************************************************************/

#include <sdr.h>
#include <sdp.h>
#include <smc.h>
#include <stndrModule.h>
#include <stndrUpdate.h>
#include <sdnIndexCTL.h>

extern smnIndexType* gSmnAllIndex[SMI_INDEXTYPE_COUNT];

/***********************************************************************
 * Description : redo_SDR_STNDR_INSERT_INDEX_KEY
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_INSERT_INDEX_KEY( SChar       * aLogPtr,
                                                     UInt         aSize,
                                                     UChar       * aRecPtr,
                                                     sdrRedoInfo * /*aRedoInfo*/,
                                                     sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr   * sIndexPageHdr = NULL;
    UShort            sAllowedSize;
    scOffset          sSlotOffset;
    UChar           * sKey;
    UShort            sKeyLen;
    SShort            sKeySeq;
    
    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aSize != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sIndexPageHdr = sdpPhyPage::getHdr(sdpPhyPage::getPageStartPtr(aRecPtr));

    sKeyLen = aSize - ID_SIZEOF(SShort);
    IDE_DASSERT( sdpPhyPage::canAllocSlot( sIndexPageHdr,
                                           sKeyLen,
                                           ID_TRUE,
                                           SDP_1BYTE_ALIGN_SIZE )
                 == ID_TRUE );

    idlOS::memcpy( &sKeySeq, aLogPtr,  (UInt)ID_SIZEOF(SShort) );

    IDE_TEST( sdpPhyPage::allocSlot( sIndexPageHdr,
                                     sKeySeq, 
                                     sKeyLen,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     &sKey,
                                     &sSlotOffset,
                                     SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_DASSERT( sKey != NULL );
    IDE_DASSERT( sAllowedSize >= sKeyLen );
    IDE_DASSERT( (UChar*)sKey == aRecPtr );

    idlOS::memcpy( sKey,
                   (UChar*)(aLogPtr + ID_SIZEOF(SShort)),
                   aSize - (UInt)ID_SIZEOF(SShort) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : redo_SDR_STNDR_UPDATE_INDEX_KEY
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_UPDATE_INDEX_KEY( SChar       * aLogPtr,
                                                     UInt          aSize,
                                                     UChar       * aRecPtr,
                                                     sdrRedoInfo * /*aRedoInfo*/,
                                                     sdrMtx      * /* aMtx */ )
{
    stndrIKey  * sIKey = NULL;
    stdMBR       sMBR;

    
    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aSize != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sIKey = (stndrIKey*)aRecPtr;
        
    idlOS::memcpy( &sMBR, aLogPtr, (UInt)ID_SIZEOF(sMBR) );

    STNDR_SET_KEYVALUE_TO_IKEY( sMBR, ID_SIZEOF(sMBR), sIKey );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : redo_SDR_STNDR_INSERT_KEY
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_INSERT_KEY( SChar        * aLogPtr,
                                               UInt           aSize,
                                               UChar        * aRecPtr,
                                               sdrRedoInfo  * /* aRedoInfo */,
                                               sdrMtx       * /* aMtx */ )
{
    sdpPhyPageHdr       * sIndexPageHdr = NULL;
    UShort                sAllowedSize;
    scOffset              sSlotOffset;
    stndrLKey           * sLKey = NULL;
    UShort                sKeySize;
    SShort                sKeySeq;
    stndrRollbackContext  sContext;
    
    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aSize != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sIndexPageHdr = sdpPhyPage::getHdr( sdpPhyPage::getPageStartPtr(aRecPtr) );

    idlOS::memcpy(&sKeySeq, aLogPtr, (UInt)ID_SIZEOF(SShort));
   
    idlOS::memcpy( (UChar*)&sContext,
                   (UChar*)(aLogPtr + ID_SIZEOF(SShort)),
                   ID_SIZEOF(stndrRollbackContext) );

    sKeySize = aSize - (ID_SIZEOF(stndrRollbackContext) + ID_SIZEOF(SShort));

    IDE_DASSERT( sdpPhyPage::canAllocSlot( sIndexPageHdr,
                                           sKeySize,
                                           ID_TRUE,
                                           SDP_1BYTE_ALIGN_SIZE )
                 == ID_TRUE );

    IDE_TEST( sdpPhyPage::allocSlot( sIndexPageHdr,
                                     sKeySeq, 
                                     sKeySize,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     (UChar**)&sLKey,
                                     &sSlotOffset,
                                     SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_DASSERT( sLKey != NULL );
    IDE_DASSERT( sAllowedSize >= sKeySize );
    IDE_DASSERT( (UChar*)sLKey == aRecPtr );
    
    idlOS::memcpy( sLKey,
                   (UChar*)(aLogPtr +
                            ID_SIZEOF(SShort) +
                            ID_SIZEOF(stndrRollbackContext)),
                   aSize - (UInt)(ID_SIZEOF(SShort) +
                                  ID_SIZEOF(stndrRollbackContext) ));

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_STNDR_INSERT_KEY
 ***********************************************************************/
IDE_RC stndrUpdate::undo_SDR_STNDR_INSERT_KEY( idvSQL   * aStatistics,
                                               void     * aTrans,
                                               sdrMtx   * aMtx,
                                               scGRID     /*aGRID*/,
                                               SChar    * aLogPtr,
                                               UInt       aSize )
{
    ULong                  sTempBuf[SD_PAGE_SIZE / (2*ID_SIZEOF(ULong))];
    stndrLKey            * sLKey = NULL;
    stndrRollbackContext   sContext;
    smnIndexHeader       * sIndexHeader = NULL;
    smcTableHeader       * sTableHeader = NULL;
    UInt                   sIndexTypeID;
    UInt                   sTableTypeID;
    smnIndexModule       * sIndexModule = NULL;
    
    idlOS::memcpy( &sContext,
                   aLogPtr + ID_SIZEOF(SShort),
                   ID_SIZEOF(stndrRollbackContext) );
    
    idlOS::memcpy( sTempBuf,
                   aLogPtr + ID_SIZEOF(SShort) + ID_SIZEOF(stndrRollbackContext),
                   aSize - (ID_SIZEOF(SShort) + ID_SIZEOF(stndrRollbackContext)) );


    IDE_ERROR( smcTable::getTableHeaderFromOID( sContext.mTableOID,
                                                (void**)&sTableHeader )
               == IDE_SUCCESS );
    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID( sTableHeader,
                                                                 sContext.mIndexID );
    sLKey = (stndrLKey*)sTempBuf;
    
    sIndexTypeID = sIndexHeader->mType;
    
    /* BUG-27690 Disk Btree for Spatial Undo 및 Redo에서의 오류 탐색 */
    sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(sTableHeader->mFlag); 
    
    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
    
    IDE_TEST( sIndexModule->mInsertKeyRollback( aStatistics,
                                                aMtx,
                                                (void*)sIndexHeader,
                                                (SChar*)(STNDR_LKEY_KEYVALUE_PTR( sLKey )),
                                                SD_MAKE_SID( sLKey->mRowPID,
                                                             sLKey->mRowSlotNum ),
                                                (UChar*)&sContext,
                                                ID_FALSE ) /* aIsDupKey */
              != IDE_SUCCESS );
                                 
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    smrRecoveryMgr::prepareRTOIForUndoFailure( aTrans,
                                               SMR_RTOI_TYPE_INDEX,
                                               sContext.mTableOID,
                                               sContext.mIndexID,
                                               SC_NULL_SPACEID,
                                               SC_NULL_PID );

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_STNDR_FREE_INDEX_KEY
 ***********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_FREE_INDEX_KEY( SChar        * aData,
                                                   UInt           aLength,
                                                   UChar        * aPagePtr,
                                                   sdrRedoInfo  * /*aRedoInfo*/,
                                                   sdrMtx       * /*aMtx*/ )
{
    sdpPhyPageHdr   * sPageHdr = NULL;
    UChar           * sFreeKeyPtr = NULL;
    UShort            sKeyLen = 0;
    

    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aData != NULL );

    IDE_ERROR( aLength == ID_SIZEOF(UShort) );

    sFreeKeyPtr = aPagePtr;
    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sFreeKeyPtr);

    idlOS::memcpy(&sKeyLen, aData, ID_SIZEOF(UShort));

    sdpPhyPage::freeSlot( sPageHdr,
                          sFreeKeyPtr,
                          sKeyLen,
                          ID_TRUE,
                          SDP_1BYTE_ALIGN_SIZE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_STNDR_DELETE_INDEX_KEY_WITH_ROW
 ***********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_DELETE_KEY_WITH_NTA( SChar       * aData,
                                                        UInt          aLength,
                                                        UChar       * aPagePtr,
                                                        sdrRedoInfo * /*aRedoInfo*/,
                                                        sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr   * sPageHdr = NULL;
    stndrLKey       * sLKey = NULL;
    UChar             sRemoveInsert;
    UShort            sKeyLen;
    UShort            sAllowedSize;
    scOffset          sSlotOffset;
    SShort            sKeySeq;
    

    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aData != NULL );
    IDE_DASSERT( aLength != 0 );

    sLKey    = (stndrLKey*)aPagePtr;
    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sLKey);
    
    idlOS::memcpy( &sRemoveInsert,
                   aData + ID_SIZEOF(stndrRollbackContext),
                   ID_SIZEOF(UChar) );
    
    idlOS::memcpy( &sKeySeq,
                   aData + (ID_SIZEOF(stndrRollbackContext) +
                            ID_SIZEOF(UChar)),
                   ID_SIZEOF(SShort) );

    sKeyLen = aLength - (ID_SIZEOF(stndrRollbackContext) +
                         ID_SIZEOF(UChar) +
                         ID_SIZEOF(SShort));

    if( sRemoveInsert == ID_TRUE )
    {
        IDE_DASSERT( sdpPhyPage::canAllocSlot( sPageHdr,
                                               sKeyLen,
                                               ID_TRUE,
                                               SDP_1BYTE_ALIGN_SIZE )
                     == ID_TRUE );

        IDE_TEST( sdpPhyPage::allocSlot( sPageHdr,
                               sKeySeq, 
                               sKeyLen,
                               ID_TRUE,
                               &sAllowedSize,
                               (UChar**)&sLKey,
                               &sSlotOffset,
                               SDP_1BYTE_ALIGN_SIZE ) 
                  != IDE_SUCCESS );

        IDE_DASSERT( sLKey != NULL );
        IDE_DASSERT( (UChar*)sLKey == aPagePtr );
    }

    idlOS::memcpy( sLKey,
                   (UChar*)( aData + (ID_SIZEOF(stndrRollbackContext) +
                                      ID_SIZEOF(UChar) +
                                      ID_SIZEOF(SShort)) ),
                   sKeyLen );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_STNDR_DELETE_KEY_WITH_NTA
 ***********************************************************************/
IDE_RC stndrUpdate::undo_SDR_STNDR_DELETE_KEY_WITH_NTA( idvSQL  * aStatistics,
                                                        void    * aTrans,
                                                        sdrMtx  * aMtx,
                                                        scGRID    /*aGRID*/,
                                                        SChar   * aLogPtr,
                                                        UInt      aSize )
{
    ULong                  sTempBuf[SD_PAGE_SIZE / (2*ID_SIZEOF(ULong))];
    stndrLKey            * sLKey = NULL;
    stndrRollbackContext   sContext;
    smnIndexHeader       * sIndexHeader = NULL;
    smcTableHeader       * sTableHeader = NULL;
    smnIndexModule       * sIndexModule = NULL;
    UInt                   sIndexTypeID;
    UInt                   sTableTypeID;
    
    idlOS::memcpy( &sContext,
                   aLogPtr,
                   ID_SIZEOF(stndrRollbackContext) );
    
    idlOS::memcpy( sTempBuf,
                   aLogPtr + (ID_SIZEOF(stndrRollbackContext) +
                              ID_SIZEOF(UChar) +
                              ID_SIZEOF(SShort)),
                   aSize - (ID_SIZEOF(stndrRollbackContext) +
                            ID_SIZEOF(UChar) +
                            ID_SIZEOF(SShort)) );

    IDE_ERROR( smcTable::getTableHeaderFromOID( sContext.mTableOID,
                                                (void**)&sTableHeader )
               == IDE_SUCCESS );
    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID(sTableHeader,
                                                                sContext.mIndexID );
    sLKey = (stndrLKey*)sTempBuf;
    
    sIndexTypeID = sIndexHeader->mType;
    /* BUG-27690 Disk Btree for Spatial Undo 및 Redo에서의 오류 탐색 */
    sTableTypeID = SMN_GET_BASE_TABLE_TYPE_ID(sTableHeader->mFlag);

    sIndexModule = gSmnAllIndex[sIndexTypeID]->mModule[sTableTypeID];
    IDE_ERROR( sIndexModule != NULL );

    IDE_TEST( sIndexModule->mDeleteKeyRollback( aStatistics,
                                                aMtx,
                                                (void*)sIndexHeader,
                                                (SChar*)(STNDR_LKEY_KEYVALUE_PTR( sLKey )),
                                                SD_MAKE_SID( sLKey->mRowPID,
                                                             sLKey->mRowSlotNum ),
                                                (UChar*)&sContext )
              != IDE_SUCCESS );
                                 
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
        
    smrRecoveryMgr::prepareRTOIForUndoFailure( aTrans,
                                               SMR_RTOI_TYPE_INDEX,
                                               sContext.mTableOID,
                                               sContext.mIndexID,
                                               SC_NULL_SPACEID,
                                               SC_NULL_PID );

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  SDR_STNDR_FREE_KEYS
 ***********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_FREE_KEYS( SChar         * aLogPtr,
                                              UInt            aSize,
                                              UChar         * aRecPtr,
                                              sdrRedoInfo   * /*aRedoInfo*/,
                                              sdrMtx        * /*aMtx*/ )
{
    sdpPhyPageHdr   * sPageHdr = NULL;
    stndrNodeHdr    * sNodeHdr = NULL;
    stndrKeyArray   * sKeyArray = NULL;
    SShort            sCount;
    UShort            sDummy1;
    UShort            sDummy2;

    
    sKeyArray = NULL;
    
    IDE_DASSERT( aRecPtr != NULL );
    IDE_DASSERT( aLogPtr != NULL );

    idlOS::memcpy(&sCount, aLogPtr, ID_SIZEOF(SShort));
    aLogPtr += ID_SIZEOF(SShort);

    IDE_ERROR( aSize == ((ID_SIZEOF(stndrKeyArray) * sCount) + ID_SIZEOF(sCount)) );

    IDE_TEST( iduMemMgr::malloc(IDU_MEM_ST_STN,
                                ID_SIZEOF(stndrKeyArray) * sCount,
                                (void**)&sKeyArray)
              != IDE_SUCCESS );

    idlOS::memcpy(sKeyArray, aLogPtr, ID_SIZEOF(stndrKeyArray)*sCount);

    sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aRecPtr);
    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPageHdr);

    if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        // BUG-29538 split시 TBK count를 조정하지 않고 있습니다.
        IDE_TEST( stndrRTree::freeKeysLeaf( sPageHdr,
                                            sKeyArray,
                                            0,
                                            sCount - 1,
                                            &sDummy1,
                                            &sDummy2 )  // dummy TBK count
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( stndrRTree::freeKeysInternal( sPageHdr, 
                                                sKeyArray, 
                                                0, 
                                                sCount - 1 )
                  != IDE_SUCCESS );
    }

    if( sKeyArray != NULL )
    {
        (void)iduMemMgr::free( sKeyArray );
        sKeyArray = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sKeyArray != NULL )
    {
        (void)iduMemMgr::free(sKeyArray);
    }

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : SDR_STNDR_COMPACT_INDEX_PAGE
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_COMPACT_INDEX_PAGE( SChar        * /*aLogPtr*/,
                                                       UInt           /*aSize*/,
                                                       UChar        * aRecPtr,
                                                       sdrRedoInfo  * /*aRedoInfo*/,
                                                       sdrMtx       * /*aMtx*/ )
{
    sdpPhyPageHdr   * sIdxPageHdr = NULL;
    stndrNodeHdr    * sNodeHdr = NULL;
    

    // IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aRecPtr != NULL );

    sIdxPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)sIdxPageHdr);

    if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        IDE_TEST( stndrRTree::compactPageLeaf( sIdxPageHdr )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( stndrRTree::compactPageInternal( sIdxPageHdr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#if 0
/***********************************************************************
 * Description : SDR_STNDR_MAKE_CHAINED_KEYS
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_MAKE_CHAINED_KEYS( SChar         * aLogPtr,
                                                      UInt            aSize,
                                                      UChar         * aRecPtr,
                                                      sdrRedoInfo   * /* aRedoInfo */,
                                                      sdrMtx        * /* aMtx */ )
{
    sdpPhyPageHdr   * sPageHdr = NULL;
    UChar           * sSlotDirPtr = NULL;
    UShort            sKeyCount;
    UInt              i;
    UChar             sCTSlotNum;
    smSCN             sCommitSCN;
    stndrLKey       * sLKey = NULL;

    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize == (ID_SIZEOF(UChar) + ID_SIZEOF(smSCN)) );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
    sKeyCount  = sdpSlotDirectory::getCount(sSlotDirPtr);
    
    idlOS::memcpy(&sCTSlotNum, aLogPtr, ID_SIZEOF(UChar));
    idlOS::memcpy(&sCommitSCN, aLogPtr + ID_SIZEOF(UChar), ID_SIZEOF(smSCN));

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                        (UChar*)sSlotDirPtr,
                                                        i,
                                                        (UChar**)&sLKey )
                  != IDE_SUCCESS );
        
        if( (STNDR_GET_STATE( sLKey ) == STNDR_KEY_DEAD) ||
            (STNDR_GET_STATE( sLKey ) == STNDR_KEY_STABLE) )
        {
            continue;
        }
        
        if( (STNDR_GET_CCTS_NO( sLKey ) == sCTSlotNum) &&
            (STNDR_GET_CHAINED_CCTS( sLKey ) == SDN_CHAINED_NO) )
        {
            STNDR_SET_CHAINED_CCTS( sLKey, SDN_CHAINED_YES );
            STNDR_SET_CSCN( sLKey, &sCommitSCN );
        }
        
        if( (STNDR_GET_LCTS_NO( sLKey ) == sCTSlotNum) &&
            (STNDR_GET_CHAINED_LCTS( sLKey ) == SDN_CHAINED_NO) )
        {
            STNDR_SET_CHAINED_LCTS( sLKey, SDN_CHAINED_YES );
            STNDR_SET_LSCN( sLKey, &sCommitSCN );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/***********************************************************************
 * Description : SDR_STNDR_MAKE_UNCHAINED_KEYS
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_MAKE_UNCHAINED_KEYS( SChar       * aLogPtr,
                                                        UInt          aSize,
                                                        UChar       * aRecPtr,
                                                        sdrRedoInfo * /* aRedoInfo */,
                                                        sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr   * sPageHdr = NULL;
    UChar           * sSlotDirPtr = NULL;
    stndrLKey       * sLKey = NULL;
    UShort            sUnchainedKeyCount;
    UChar           * sUnchainedKey = NULL;
    UShort            sKeySeq;
    UInt              sCursor = 0;
    UChar             sChainedCCTS;
    UChar             sChainedLCTS;
    

    IDE_DASSERT( aLogPtr != NULL );
    IDE_ERROR( aSize != 0 );
    IDE_DASSERT( aRecPtr != NULL );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
    
    idlOS::memcpy( &sUnchainedKeyCount, aLogPtr, ID_SIZEOF(UShort) );
    sUnchainedKey = (UChar*)(aLogPtr + ID_SIZEOF(UShort));

    while( sUnchainedKeyCount > 0 )
    {
        idlOS::memcpy( &sKeySeq, sUnchainedKey + sCursor, ID_SIZEOF(UShort) );
        sCursor += 2;
        sChainedCCTS = *(sUnchainedKey + sCursor);
        sCursor += 1;
        sChainedLCTS = *(sUnchainedKey + sCursor);
        sCursor += 1;

        IDE_DASSERT( sCursor <= (aSize - ID_SIZEOF(UShort)) );
        
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar*)sSlotDirPtr,
                                                            sKeySeq,
                                                           (UChar**)&sLKey )
                  != IDE_SUCCESS );
        if( sChainedCCTS == SDN_CHAINED_YES )
        {
            sUnchainedKeyCount--;
            STNDR_SET_CHAINED_CCTS( sLKey, SDN_CHAINED_NO );
        }
        
        if( sChainedLCTS == SDN_CHAINED_YES )
        {
            sUnchainedKeyCount--;
            STNDR_SET_CHAINED_LCTS( sLKey, SDN_CHAINED_NO );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
    
/***********************************************************************
 * Description : SDR_STNDR_KEY_STAMPING
 **********************************************************************/
IDE_RC stndrUpdate::redo_SDR_STNDR_KEY_STAMPING( SChar       * aLogPtr,
                                                 UInt          aSize,
                                                 UChar       * aRecPtr,
                                                 sdrRedoInfo * /* aRedoInfo */,
                                                 sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr   * sPageHdr = NULL;
    UChar           * sSlotDirPtr = NULL;
    UShort            sKeyCount;
    UInt              i;
    UChar             sCTSlotNum;
    stndrLKey       * sLKey = NULL;
    UInt              sKeyLen = 0;
    stndrNodeHdr    * sNodeHdr = NULL;
    smSCN             sCIInfinite;

    
    IDE_DASSERT( aLogPtr != NULL );
    IDE_DASSERT( aRecPtr != NULL );

    SM_SET_SCN_CI_INFINITE( &sCIInfinite );

    sPageHdr = sdpPhyPage::getHdr((UChar*)aRecPtr);

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPageHdr);

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr);
    sKeyCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

    idlOS::memcpy(&sCTSlotNum, aLogPtr, ID_SIZEOF(UChar));
    aLogPtr += ID_SIZEOF(UChar);

    IDE_ERROR( aSize == ID_SIZEOF(UChar) );

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar*)sSlotDirPtr,
                                                           i,
                                                           (UChar**)&sLKey )
                  != IDE_SUCCESS );
        if( STNDR_GET_CCTS_NO( sLKey ) == sCTSlotNum )
        {
            STNDR_SET_CCTS_NO( sLKey, SDN_CTS_INFINITE );

            /*
             * To fix BUG-23337
             */
            if( STNDR_GET_STATE( sLKey ) == STNDR_KEY_UNSTABLE )
            {
                STNDR_SET_STATE( sLKey, STNDR_KEY_STABLE );
                STNDR_SET_CSCN( sLKey , &sCIInfinite );
            }
        }

        if( STNDR_GET_LCTS_NO( sLKey ) == sCTSlotNum )
        {
            sKeyLen = stndrRTree::getKeyLength( (UChar*)sLKey,
                                                ID_TRUE );
            
            sNodeHdr->mTotalDeadKeySize += sKeyLen +
                ID_SIZEOF( sdpSlotEntry );

            STNDR_SET_CCTS_NO( sLKey, SDN_CTS_INFINITE );
            STNDR_SET_LCTS_NO( sLKey, SDN_CTS_INFINITE );
            STNDR_SET_STATE( sLKey, STNDR_KEY_DEAD );

            STNDR_SET_LSCN( sLKey, &sCIInfinite );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


