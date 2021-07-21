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
 * $Id: smrUpdate.h 91224 2021-07-14 05:36:12Z minku.kang $
 **********************************************************************/

#ifndef _O_SMR_UPDATE_H_
#define _O_SMR_UPDATE_H_ 1

#include <smm.h>
#include <smmFPLManager.h>
#include <smDef.h>

class smrUpdate
{
//For Operation
public:
    static void initialize();

    static IDE_RC writeUpdateLog( idvSQL*           aStatistics,
                                  void*             aTrans,
                                  smrUpdateType     aUpdateLogType,
                                  scGRID            aGRID,
                                  vULong            aData,
                                  UInt              aBfrImgCnt,
                                  smrUptLogImgInfo *aBfrImage,
                                  UInt              aAftImgCnt,
                                  smrUptLogImgInfo *aAftImage,
                                  smLSN            *aWrittenLogLSN = NULL);

    /* NullTransaction (ID_SINT_MAX)로 Log를 기록함 */
    static IDE_RC writeDummyUpdateLog( smrUpdateType     aUpdateLogType,
                                       scGRID            aGRID,
                                       vULong            aData,
                                       UInt              aAftImg );


    /* Update type:  SMR_PHYSICAL                                   */
    static IDE_RC physicalUpdate( idvSQL*      aStatistics,
                                  void*        aTrans,
                                  scSpaceID    aSpaceID,
                                  scPageID     aPageID,
                                  scOffset     aOffset,
                                  SChar*       aBeforeImage,
                                  SInt         aBeforeImageSize,
                                  SChar*       aAfterImage1,
                                  SInt         aAfterImageSize,
                                  SChar*       aAfterImage,
                                  SInt         aAfterImageSize2 );
    
    /* Update type:  SMR_SMM_MEMBASE_INFO                      */
    static IDE_RC setMemBaseInfo(idvSQL*      aStatistics,
                                 void       * aTrans,
                                 scSpaceID    aSpaceID,
                                 smmMemBase * aMemBase);
    
    /* Update type:  SMR_SMM_MEMBASE_SET_SYSTEM_SCN                      */
    static IDE_RC setSystemSCN(smSCN        aSystemSCN);


    /* Update type:   SMR_SMM_MEMBASE_ALLOC_PERS_LIST                    */
    static IDE_RC allocPersListAtMembase(idvSQL*      aStatistics,
                                         void*        aTrans,
                                         scSpaceID    aSpaceID,
                                         smmMemBase*  aBeforeMemBase,
                                         smmFPLNo     aFPLNo,
                                         scPageID     aAfterPageID,
                                         vULong       aAfterPageCount );
    
    /* Update type:  SMR_SMM_MEMBASE_ALLOC_PERS                          */
    static IDE_RC allocExpandChunkAtMembase(
                                  idvSQL*      aStatistics,
                                  void*        aTrans,
                                  scSpaceID    aSpaceID,
                                  smmMemBase*  aMemBase,
                                  UInt         aExpandPageListID,
                                  scPageID     aAfterChunkFirstPID,
                                  scPageID     aAfterChunkLastPID,
                                  smLSN      * aBeginLSN );
    
    /* Update type:  SMR_SMM_PERS_UPDATE_LINK                       */
    static IDE_RC updateLinkAtPersPage(idvSQL*    aStatistics,
                                       void*      aTrans,
                                       scSpaceID  aSpaceID,
                                       scPageID   aPageID,
                                       scPageID   aBeforePrevPageID,
                                       scPageID   aBeforeNextPageID,
                                       scPageID   aAfterPrevPageID,
                                       scPageID   aAfterNextPageID);

    /* Update type: SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK         */
    static IDE_RC updateLinkAtFreePage(idvSQL*      aStatistics,
                                       void*        aTrans,
                                       scSpaceID    aSpaceID,
                                       scPageID     aFreeListInfoPID,
                                       UInt         aFreePageSlotOffset,
                                       scPageID     aBeforeNextFreePID,
                                       scPageID     aAfterNextFreePID );

    
    /* Update Type: SMR_SMC_TABLEHEADER_INIT                           */ 
    static IDE_RC initTableHeader(idvSQL*      aStatistics,
                                  void*        aTrans,
                                  scPageID     aPageID,
                                  scOffset     aOffset,
                                  void*        aTableHeader,
                                  UInt         aSize);
    
    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INDEX                   */
    static IDE_RC updateIndexAtTableHead(idvSQL*      aStatistics,
                                         void*        aTrans,
                                         smOID        aOID,
                                         const smVCDesc*  aIndex,
                                         //table헤더의 mIndexes[] 첨자
                                         const UInt   aOIDIdx,
                                         smOID        aOIDVar,
                                         UInt         aLength,
                                         UInt         aFlag);

    
    static IDE_RC updateColumnsAtTableHead(idvSQL*             aStatistics,
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
                                           UInt                aAfterColumnCount);


    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_INFO                       */
    static IDE_RC updateInfoAtTableHead(idvSQL*             aStatistics,
                                        void*               aTrans,
                                        scPageID            aPageID,
                                        scOffset            aOffset,
                                        const smVCDesc*     aInfo,
                                        smOID               aOIDVar,
                                        UInt                aLength,
                                        UInt                aFlag);


    // PROJ-1671
    /* SMR_SMC_TABLEHEADER_SET_SEGSTOATTR         */
    static IDE_RC updateSegStoAttrAtTableHead(idvSQL            * aStatistics,
                                              void              * aTrans,
                                              void              * aTable,
                                              smiSegStorageAttr   aBfrSegStoAttr,
                                              smiSegStorageAttr   aAftSegStoAttr );



    /* SMR_SMC_TABLEHEADER_SET_INSERTLIMIT         */
    static IDE_RC updateInsLimitAtTableHead(idvSQL            * aStatistics,
                                            void              * aTrans,
                                            void              * aTable,
                                            smiSegAttr         aBfrSegAttr,
                                            smiSegAttr         aAftSegAttr );

    /* Update Type: SMR_SMC_TABLEHEADER_SET_NULLROW                       */
    static IDE_RC setNullRow(idvSQL*             aStatistics,
                             void*           aTrans,
                             scSpaceID       aSpaceID,
                             scPageID        aPageID,
                             scOffset        aOffset,
                             smOID           aNullOID);

    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALL                       */
    static IDE_RC updateAllAtTableHead(idvSQL*               aStatistics,
                                       void*                 aTrans,
                                       void*                 aTable,
                                       UInt                  aColumnsize,
                                       const smVCDesc*       aColumn,
                                       const smVCDesc*       aInfo,
                                       UInt                  aFlag,
                                       ULong                 aMaxRow,
                                       UInt                  aParallelDegree);

    /* Update Type: SMR_SMC_TABLEHEADER_UPDATE_ALLOCPAGELIST             */
    static IDE_RC updateAllocInfoAtTableHead(idvSQL*           aStatistics,
                                             void*             aTrans,
                                             scPageID          aPageID,
                                             scOffset          aOffset,
                                             void*             aAllocPageList,
                                             vULong            aPageCount,
                                             scPageID          aHead,
                                             scPageID          aTail);
        
    /* SMR_SMC_TABLEHEADER_UPDATE_FLAG                                */
    static IDE_RC updateFlagAtTableHead( idvSQL* aStatistics,
                                         void*   aTrans,
                                         void*   aTable,
                                         UInt    aFlag );
    
    /* Update type: SMR_SMC_TABLEHEADER_SET_SEQUENCE */
    static IDE_RC updateSequenceAtTableHead(idvSQL*      aStatistics,
                                            void*        aTrans,
                                            smOID        aOIDTable,
                                            scOffset     aOffset,
                                            void*        aBeforeSequence,
                                            void*        aAfterSequence,
                                            UInt         aSize);

    /* Update type: SMR_SMC_TABLEHEADER_SET_INCONSISTENCY       */
    static IDE_RC setInconsistencyAtTableHead( void   * aTable,
                                               idBool   aForMediaRecovery );

    /* Update type: SMR_SMC_PERS_INIT_FIXED_PAGE                         */
    static IDE_RC initFixedPage(idvSQL*   aStatistics,
                                void*     aTrans,
                                scSpaceID aSpaceID,
                                scPageID  aPageID,
                                UInt      aPageListID,
                                vULong    aSlotSize,
                                vULong    aSlotCount,
                                smOID     aTableOID);

    /* Update type: SMR_SMC_PERS_SET_INCONSISTENCY         */
    static IDE_RC setPersPageInconsistency(scSpaceID    aSpaceID,
                                           scPageID     aPageID,
                                           smpPageType  aBeforePageType );

    /* Update type: SMR_SMC_INDEX_SET_FLAG                      */
    static IDE_RC setIndexHeaderFlag(idvSQL*     aStatistics,
                                     void*       aTrans,
                                     smOID       aOIDIndex,
                                     scOffset    aOffset,
                                     UInt        aBeforeFlag,
                                     UInt        aAfterFlag);

    /* Update type: SMR_SMC_INDEX_SET_SEGSTOATTR  */
    static IDE_RC setIdxHdrSegStoAttr(idvSQL*           aStatistics,
                                      void*             aTrans,
                                      smOID             aOIDIndex,
                                      scOffset          aOffset,
                                      smiSegStorageAttr aBfrSegStoAttr,
                                      smiSegStorageAttr aAftSegStoAttr );
        
    /* Update type: SMR_SMC_INDEX_SET_SEGATTR  */
    static IDE_RC setIdxHdrSegAttr(idvSQL*         aStatistics,
                                      void*        aTrans,
                                      smOID        aOIDIndex,
                                      scOffset     aOffset,
                                      smiSegAttr   aBfrSegAttr,
                                      smiSegAttr   aAftSegAttr );
        
    /* Update type: SMR_SMC_INDEX_SET_DROP_FLAG  */
    static IDE_RC setIndexDropFlag(idvSQL    * aStatistics,
                                   void      * aTrans,
                                   smOID       aTableOID,
                                   smOID       aIndexOID,
                                   UShort      aBFlag,
                                   UShort      aAFlag );

    /* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW                     */
    static IDE_RC updateFixedRowHead(idvSQL*          aStatistics,
                                     void*            aTrans,
                                     scSpaceID        aSpaceID,
                                     smOID            aOID,
                                     void*            aBeforeSlotHeader,
                                     void*            aAfterSlotHeader,
                                     UInt             aSize);
    
    /* Update type:  SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION         */
    /* To Fix BUG-14639 */
    static IDE_RC updateNextVersionAtFixedRow(idvSQL*    aStatistics,
                                              void*      aTrans,
                                              scSpaceID  aSpaceID,
                                              smOID      aTableOID,
                                              smOID      aOID,
                                              ULong      aBeforeNext,
                                              ULong      aAfterNext);


    /* Update Type: SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG                */
    static IDE_RC setDropFlagAtFixedRow(idvSQL*         aStatistics,
                                        void*           aTrans,
                                        scSpaceID       aSpaceID,
                                        smOID           aOID,
                                        idBool          aDrop);

    /* Update Type: SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT                */
    static IDE_RC setDeleteBitAtFixRow( idvSQL*    aStatistics,
                                        void*      aTrans,
                                        scSpaceID  aSpaceID,
                                        smOID      aOID );

    /* Update type: SMR_SMC_PERS_INIT_VAR_PAGE                         */
    static IDE_RC initVarPage(idvSQL*    aStatistics,
                              void*      aTrans,
                              scSpaceID  aSpaceID,
                              scPageID   aPageID,
                              UInt       aPageListID,
                              vULong     aIdx,
                              vULong     aSlotSize,
                              vULong     aSlotCount,
                              smOID      aTableOID);

    /* Update type:  SMR_SMC_PERS_UPDATE_VAR_ROW                      */
    static IDE_RC updateVarRowHead(idvSQL*             aStatistics,
                                   void*               aTrans,
                                   scSpaceID           aSpaceID,
                                   smOID               aOID,
                                   smVCPieceHeader*    aBeforeVarColumn,
                                   smVCPieceHeader*    aAfterVarColumn);

    /* Update type:  SMR_SMC_PERS_UPDATE_VAR_ROW                 */
    static IDE_RC updateVarRow(idvSQL*         aStatistics,
                               void*           aTrans,
                               scSpaceID       aSpaceID,
                               smOID           aOID,
                               SChar*          aAfterVarRowData,
                               UShort          aAfterRowSize);

    /* Update type:  SMR_SMC_PERS_SET_VAR_ROW_DELETE_FLAG              */
    static IDE_RC setFlagAtVarRow(idvSQL*   aStatistics,
                                  void*     aTrans,
                                  scSpaceID aSpaceID,
                                  smOID     aTableOID,
                                  smOID     aOID,
                                  UShort    aBFlag,
                                  UShort    aAFlag);
    
    /* Update type:  SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT              */
    static IDE_RC updateTableSegPIDAtTableHead(idvSQL*      aStatistics,
                                               void*        aTrans,
                                               scPageID     aPageID,
                                               scOffset     aOffset,
                                               void*        aPageListEntry,
                                               scPageID     aAfterSegPID);

    /* Update Type: SMR_SMC_PERS_SET_VAR_ROW_NXT_OID */
    static IDE_RC setNxtOIDAtVarRow(idvSQL*   aStatistics,
                                    void*     aTrans,
                                    scSpaceID aSpaceID,
                                    smOID     aTableOID,
                                    smOID     aOID,
                                    smOID     aBOID,
                                    smOID     aAOID);

    static IDE_RC writeNTAForExtendDBF ( idvSQL* aStatistics,
                                         void*   aTrans, 
                                         smLSN*  aLsnNTA ); 

    // Memory Tablespace Create에 대한 로깅
    static IDE_RC writeMemoryTBSCreate( idvSQL                * aStatistics,
                                        void                  * aTrans,
                                        smiTableSpaceAttr     * aTBSAttr,
                                        smiChkptPathAttrList  * aChkptPathAttrList );
    
    // Volatile Tablespace Create 및 Drop에 대한 로깅
    static IDE_RC writeVolatileTBSCreate( idvSQL*             aStatistics,
                                          void*               aTrans,
                                          scSpaceID           aSpaceID );
    
    // Memory Tablespace DB File 생성에 대한 로깅
    static IDE_RC writeMemoryDBFileCreate( idvSQL*             aStatistics,
                                           void*               aTrans,
                                           scSpaceID           aSpaceID,
                                           UInt                aPingPongNo,
                                           UInt                aDBFileNo );

    // Memory Tablespace Drop에 대한 로깅
    static IDE_RC writeMemoryTBSDrop( idvSQL*             aStatistics,
                                      void*               aTrans,
                                      scSpaceID           aSpaceID,
                                      smiTouchMode        aTouchMode );
    
    // Volatile Tablespace Drop에 대한 로깅
    static IDE_RC writeVolatileTBSDrop( idvSQL*             aStatistics,
                                        void*               aTrans,
                                        scSpaceID           aSpaceID );

    // Memory Tablespace 의 ALTER AUTO EXTEND ... 에 대한 로깅
    static IDE_RC writeMemoryTBSAlterAutoExtend(
                      idvSQL*             aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      idBool              aBIsAutoExtend,
                      scPageID            aBNextPageCount,
                      scPageID            aBMaxPageCount,
                      idBool              aAIsAutoExtend,
                      scPageID            aANextPageCount,
                      scPageID            aAMaxPageCount );

    // Volatile Tablespace 의 ALTER AUTO EXTEND ... 에 대한 로깅
    static IDE_RC writeVolatileTBSAlterAutoExtend(
                      idvSQL*             aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      idBool              aBIsAutoExtend,
                      scPageID            aBNextPageCount,
                      scPageID            aBMaxPageCount,
                      idBool              aAIsAutoExtend,
                      scPageID            aANextPageCount,
                      scPageID            aAMaxPageCount );

    // Tablespace Attribute Flag의 변경에 대한 로깅
    // (Ex> ALTER Tablespace Log Compress ON/OFF )
    static IDE_RC writeTBSAlterAttrFlag(
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      UInt                aBeforeAttrFlag,
                      UInt                aAfterAttrFlag );
    
    // Tablespace 의 ALTER ONLINE/OFFLINE ... 에 대한 로깅
    static IDE_RC writeTBSAlterOnOff(
                      idvSQL*             aStatistics,
                      void*               aTrans,
                      scSpaceID           aSpaceID,
                      sctUpdateType       aUpdateType,
                      UInt                aBState,
                      UInt                aAState, 
                      smLSN*              aBeginLSN );
    
    // Disk Tablespace의 Create/Drop 에 대한 로깅 수행 
    static IDE_RC writeDiskTBSCreateDrop( idvSQL            * aStatistics,
                                          void              * aTrans,
                                          sctUpdateType       aUpdateType,
                                          scSpaceID           aSpaceID,
                                          smiTableSpaceAttr * aTableSpaceAttr,  /* PROJ-1923 */
                                          smLSN             * aBeginLSN );
    
    // Disk DataFile 의 Alter Online/Offline 에 대한 로깅 수행 
    static IDE_RC writeDiskDBFAlterOnOff( idvSQL*             aStatistics,
                                          void*               aTrans,
                                          scSpaceID           aSpaceID,
                                          UInt                aFileID,
                                          sctUpdateType       aUpdateType,
                                          UInt                aBState,
                                          UInt                aAState );

    // DBF 생성에 대한 로깅 수행
    static IDE_RC writeLogCreateDBF( idvSQL             * aStatistics,
                                     void               * aTrans,
                                     scSpaceID            aSpaceID,
                                     sddDataFileNode    * aFileNode,
                                     smiTouchMode         aTouchMode,
                                     smiDataFileAttr    * aFileAttr,    /* PROJ-1923 */
                                     smLSN              * aBeginLSN);
    // DBF 삭제에 대한 로깅 수행
    static IDE_RC writeLogDropDBF(idvSQL*             aStatistics,
                                  void               *aTrans,
                                  scSpaceID           aSpaceID,
                                  sddDataFileNode    *aFileNode,
                                  smiTouchMode        aTouchMode,
                                  smLSN              *aBeginLSN);

    static inline void setUpdateLogHead( smrUpdateLog*  aUpdateLog,
                                         smTID          aTransID,
                                         UInt           aFlag,
                                         smrLogType     aLogType,
                                         UInt           aTotalLogSize,
                                         smrUpdateType  aUpdateType,
                                         scGRID         aGRID,
                                         vULong         aData,
                                         UInt           aBImgSize,
                                         UInt           aAImgSize);

    // DBF 확장에 대한 로깅 수행
    static IDE_RC writeLogExtendDBF(idvSQL*             aStatistics,
                                    void               *aTrans,
                                    scSpaceID           aSpaceID,
                                    sddDataFileNode    *aFileNode,
                                    ULong               aAfterCurrSize,
                                    smLSN              *aBeginLSN);

    // DBF 축소에 대한 로깅 수행
    static IDE_RC writeLogShrinkDBF(idvSQL*             aStatistics,
                                    void               *aTrans,
                                    scSpaceID           aSpaceID,
                                    sddDataFileNode    *aFileNode,
                                    ULong               aAfterInitSize,
                                    ULong               aAfterCurrSize,
                                    smLSN              *aBeginLSN);

    // DBF Autoextend mode 변경에 대한 로깅 수행
    static IDE_RC writeLogSetAutoExtDBF(idvSQL*             aStatistics,
                                        void               *aTrans,
                                        scSpaceID           aSpaceID,
                                        sddDataFileNode    *aFileNode,
                                        idBool              aAfterAutoExtMode,
                                        ULong               aAfterNextSize,
                                        ULong               aAfterMaxSize,
                                        smLSN              *aBeginLSN);

    // Prepare 트랜잭션에 대한 트랜잭션 세그먼트 정보 복원
    static IDE_RC writeXaSegsLog( idvSQL      * aStatistics,
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
                                  scPageID      aLstUndoPID );

    static IDE_RC writeXaStartReqLog( ID_XID * aGlobalXID,
                                      smTID    aTID,
                                      smLSN  * aLSN );

    static IDE_RC writeXaPrepareReqLog( ID_XID * aGlobalXID,
                                        smTID    aTID,
                                        UInt     aGlobalTxId,
                                        UChar  * aBranchTx,
                                        UInt     aBranchTxSize,
                                        smLSN  * aLSN );

    static IDE_RC writeXaEndLog( smTID aTID,
                                 UInt aGlobalTxId );

//For Member
public:
};

extern smrRecFunction gSmrUndoFunction[SM_MAX_RECFUNCMAP_SIZE];
extern smrRecFunction gSmrRedoFunction[SM_MAX_RECFUNCMAP_SIZE];

extern smrTBSUptRecFunction gSmrTBSUptRedoFunction[SCT_UPDATE_MAXMAX_TYPE];
extern smrTBSUptRecFunction gSmrTBSUptUndoFunction[SCT_UPDATE_MAXMAX_TYPE];

void smrUpdate::setUpdateLogHead( smrUpdateLog*  aUpdateLog,
                                  smTID          aTransID,
                                  UInt           aFlag,
                                  smrLogType     aLogType,
                                  UInt           aTotalLogSize,
                                  smrUpdateType  aUpdateType,
                                  scGRID         aGRID,
                                  vULong         aData,
                                  UInt           aBImgSize,
                                  UInt           aAImgSize)
{
    smrLogHeadI::set( &( aUpdateLog->mHead ),
                      aTransID,
                      aLogType,
                      aTotalLogSize,
                      aFlag );

    aUpdateLog->mAImgSize      = aAImgSize;
    aUpdateLog->mBImgSize      = aBImgSize;
    aUpdateLog->mType          = aUpdateType;

    aUpdateLog->mGRID = aGRID;
    aUpdateLog->mData = aData;
}

#endif /* _O_SMR_UPDATE_H_ */
