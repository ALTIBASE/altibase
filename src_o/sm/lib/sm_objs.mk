# Makefile for SM library
#
# CVS Info : $Id: sm_objs.mk 86294 2019-10-21 04:20:49Z et16 $
#

# NORMAL LIBRARY

SMT_SRCS  = $(SM_DIR)/smt/smtPJChild.cpp          \
	    $(SM_DIR)/smt/smtPJMgr.cpp

SMU_SRCS  = $(SM_DIR)/smu/smuUtility.cpp          \
	    $(SM_DIR)/smu/smuVersion.cpp          \
	    $(SM_DIR)/smu/smuQueueMgr.cpp         \
	    $(SM_DIR)/smu/smuHash.cpp             \
	    $(SM_DIR)/smu/smuDynArray.cpp         \
	    $(SM_DIR)/smu/smuTraceCode.cpp        \
	    $(SM_DIR)/smu/smuSynchroList.cpp      \
	    $(SM_DIR)/smu/smuProperty.cpp         \
	    $(SM_DIR)/smu/smuWorkerThread.cpp	  \
	    $(SM_DIR)/smu/smuJobThread.cpp        \
	    $(SM_DIR)/smu/smuMemStack.cpp

SMM_SRCS  = $(SM_DIR)/smm/smmDatabaseFile.cpp   \
	    $(SM_DIR)/smm/smmDirtyPageList.cpp  \
	    $(SM_DIR)/smm/smmDirtyPageMgr.cpp   \
	    $(SM_DIR)/smm/smmFixedMemoryMgr.cpp \
	    $(SM_DIR)/smm/smmShmKeyMgr.cpp      \
	    $(SM_DIR)/smm/smmShmKeyList.cpp     \
	    $(SM_DIR)/smm/smmExpandChunk.cpp    \
	    $(SM_DIR)/smm/smmFPLManager.cpp     \
	    $(SM_DIR)/smm/smmPLoadMgr.cpp       \
	    $(SM_DIR)/smm/smmPLoadChild.cpp     \
	    $(SM_DIR)/smm/smmDatabase.cpp       \
	    $(SM_DIR)/smm/smmTBSStartupShutdown.cpp  \
	    $(SM_DIR)/smm/smmTBSMultiPhase.cpp       \
	    $(SM_DIR)/smm/smmTBSChkptPath.cpp        \
	    $(SM_DIR)/smm/smmTBSAlterAutoExtend.cpp  \
	    $(SM_DIR)/smm/smmTBSAlterChkptPath.cpp   \
	    $(SM_DIR)/smm/smmTBSAlterDiscard.cpp     \
	    $(SM_DIR)/smm/smmTBSCreate.cpp           \
	    $(SM_DIR)/smm/smmTBSDrop.cpp             \
	    $(SM_DIR)/smm/smmTBSMediaRecovery.cpp    \
	    $(SM_DIR)/smm/smmManager.cpp        \
	    $(SM_DIR)/smm/smmSlotList.cpp       \
	    $(SM_DIR)/smm/smmUpdate.cpp         \
	    $(SM_DIR)/smm/smmFT.cpp

SDB_SRCS  = $(SM_DIR)/sdb/sdbDPathBufferMgr.cpp     \
	    $(SM_DIR)/sdb/sdbDPathBFThread.cpp      \
	    $(SM_DIR)/sdb/sdbBCBHash.cpp            \
	    $(SM_DIR)/sdb/sdbLRUList.cpp            \
	    $(SM_DIR)/sdb/sdbPrepareList.cpp        \
	    $(SM_DIR)/sdb/sdbFlushList.cpp          \
	    $(SM_DIR)/sdb/sdbCPListSet.cpp          \
	    $(SM_DIR)/sdb/sdbBufferArea.cpp         \
	    $(SM_DIR)/sdb/sdbBufferPool.cpp         \
	    $(SM_DIR)/sdb/sdbBCB.cpp                \
	    $(SM_DIR)/sdb/sdbBufferPoolStat.cpp     \
	    $(SM_DIR)/sdb/sdbFlushMgr.cpp           \
	    $(SM_DIR)/sdb/sdbFlusher.cpp            \
	    $(SM_DIR)/sdb/sdbBufferMgr.cpp          \
	    $(SM_DIR)/sdb/sdbDWRecoveryMgr.cpp      \
	    $(SM_DIR)/sdb/sdbMPRMgr.cpp             \
	    $(SM_DIR)/sdb/sdbFT.cpp

SDT_SRCS  = $(SM_DIR)/sdt/sdtWAExtentMgr.cpp        \
	    $(SM_DIR)/sdt/sdtSortSegment.cpp        \
	    $(SM_DIR)/sdt/sdtWASortMap.cpp          \
	    $(SM_DIR)/sdt/sdtTempPage.cpp           \
	    $(SM_DIR)/sdt/sdtTempRow.cpp            \
	    $(SM_DIR)/sdt/sdtHashModule.cpp         \
	    $(SM_DIR)/sdt/sdtSortModule.cpp

SCT_SRCS  = $(SM_DIR)/sct/sctTableSpaceMgr.cpp  \
	    $(SM_DIR)/sct/sctTBSAlter.cpp       \
	    $(SM_DIR)/sct/sctTBSUpdate.cpp      \
	    $(SM_DIR)/sct/sctFT.cpp

SDD_SRCS  = $(SM_DIR)/sdd/sddDataFile.cpp       \
	    $(SM_DIR)/sdd/sddDiskMgr.cpp        \
	    $(SM_DIR)/sdd/sddTableSpace.cpp     \
	    $(SM_DIR)/sdd/sddUpdate.cpp         \
	    $(SM_DIR)/sdd/sddDiskFT.cpp         \
	    $(SM_DIR)/sdd/sddDWFile.cpp

SMR_SRCS  = $(SM_DIR)/smr/smrChkptThread.cpp         \
	    $(SM_DIR)/smr/smrDirtyPageList.cpp       \
	    $(SM_DIR)/smr/smrDirtyPageListMgr.cpp    \
	    $(SM_DIR)/smr/smrLFThread.cpp            \
	    $(SM_DIR)/smr/smrLogAnchorMgr.cpp        \
	    $(SM_DIR)/smr/smrLogFile.cpp             \
	    $(SM_DIR)/smr/smrLogFileDump.cpp         \
	    $(SM_DIR)/smr/smrLogFileMgr.cpp          \
	    $(SM_DIR)/smr/smrLogMgr.cpp              \
	    $(SM_DIR)/smr/smrRecoveryMgr.cpp         \
	    $(SM_DIR)/smr/smrBackupMgr.cpp           \
	    $(SM_DIR)/smr/smrUTransQueue.cpp         \
	    $(SM_DIR)/smr/smrCompareLSN.cpp          \
	    $(SM_DIR)/smr/smrUpdate.cpp              \
	    $(SM_DIR)/smr/smrArchThread.cpp          \
	    $(SM_DIR)/smr/smrRemoteLogMgr.cpp        \
	    $(SM_DIR)/smr/smrPreReadLFileThread.cpp  \
	    $(SM_DIR)/smr/smrRedoLSNMgr.cpp          \
	    $(SM_DIR)/smr/smrLogComp.cpp             \
	    $(SM_DIR)/smr/smrCompResList.cpp         \
	    $(SM_DIR)/smr/smrCompResPool.cpp         \
	    $(SM_DIR)/smr/smrFT.cpp                  \
	    $(SM_DIR)/smr/smrArchMultiplexThread.cpp \
	    $(SM_DIR)/smr/smrLogMultiplexThread.cpp  \
	    $(SM_DIR)/smr/smrUCSNChkThread.cpp

SMRI_SRCS  = $(SM_DIR)/smr/smri/smriChangeTrackingMgr.cpp   \
	     $(SM_DIR)/smr/smri/smriBackupInfoMgr.cpp       \
	     $(SM_DIR)/smr/smri/smriFT.cpp

SDR_SRCS  = $(SM_DIR)/sdr/sdrRedoMgr.cpp        \
	    $(SM_DIR)/sdr/sdrMiniTrans.cpp      \
	    $(SM_DIR)/sdr/sdrMtxStack.cpp       \
	    $(SM_DIR)/sdr/sdrUpdate.cpp         \
	    $(SM_DIR)/sdr/sdrCorruptPageMgr.cpp

SMP_SRCS  = $(SM_DIR)/smp/smpFixedPageList.cpp  \
	    $(SM_DIR)/smp/smpVarPageList.cpp    \
	    $(SM_DIR)/smp/smpAllocPageList.cpp  \
	    $(SM_DIR)/smp/smpFreePageList.cpp   \
	    $(SM_DIR)/smp/smpManager.cpp        \
	    $(SM_DIR)/smp/smpUpdate.cpp         \
	    $(SM_DIR)/smp/smpTBSAlterOnOff.cpp  \
	    $(SM_DIR)/smp/smpFT.cpp

SDP_SRCS  = $(SM_DIR)/sdp/sdpPhyPage.cpp         \
	    $(SM_DIR)/sdp/sdpSglPIDList.cpp      \
	    $(SM_DIR)/sdp/sdpSglRIDList.cpp      \
	    $(SM_DIR)/sdp/sdpDblPIDList.cpp      \
	    $(SM_DIR)/sdp/sdpDblRIDList.cpp      \
	    $(SM_DIR)/sdp/sdpTableSpace.cpp      \
	    $(SM_DIR)/sdp/sdpUpdate.cpp          \
	    $(SM_DIR)/sdp/sdpPageList.cpp        \
	    $(SM_DIR)/sdp/sdpSegDescMgr.cpp      \
	    $(SM_DIR)/sdp/sdpSegment.cpp         \
	    $(SM_DIR)/sdp/sdpTBSDump.cpp         \
	    $(SM_DIR)/sdp/sdpModule.cpp          \
	    $(SM_DIR)/sdp/sdpSlotDirectory.cpp   \
	    $(SM_DIR)/sdp/sdpDPathInfoMgr.cpp    \
	    $(SM_DIR)/sdp/sdpFT.cpp

SDPTB_SRCS  = $(SM_DIR)/sdp/sdptb/sdptbBit.cpp            \
	      $(SM_DIR)/sdp/sdptb/sdptbExtent.cpp         \
	      $(SM_DIR)/sdp/sdptb/sdptbGroup.cpp          \
	      $(SM_DIR)/sdp/sdptb/sdptbSpaceDDL.cpp       \
	      $(SM_DIR)/sdp/sdptb/sdptbUpdate.cpp         \
	      $(SM_DIR)/sdp/sdptb/sdptbVerifyAndDump.cpp  \
	      $(SM_DIR)/sdp/sdptb/sdptbFT.cpp

SDPSF_SRCS  = $(SM_DIR)/sdp/sdpsf/sdpsfSegDDL.cpp         \
	      $(SM_DIR)/sdp/sdpsf/sdpsfSH.cpp             \
	      $(SM_DIR)/sdp/sdpsf/sdpsfUpdate.cpp         \
	      $(SM_DIR)/sdp/sdpsf/sdpsfExtMgr.cpp         \
	      $(SM_DIR)/sdp/sdpsf/sdpsfExtDirPage.cpp     \
	      $(SM_DIR)/sdp/sdpsf/sdpsfExtDirPageList.cpp \
	      $(SM_DIR)/sdp/sdpsf/sdpsfUFmtPIDList.cpp    \
	      $(SM_DIR)/sdp/sdpsf/sdpsfFreePIDList.cpp    \
	      $(SM_DIR)/sdp/sdpsf/sdpsfPvtFreePIDList.cpp \
	      $(SM_DIR)/sdp/sdpsf/sdpsfAllocPage.cpp      \
	      $(SM_DIR)/sdp/sdpsf/sdpsfFindPage.cpp       \
	      $(SM_DIR)/sdp/sdpsf/sdpsfVerifyAndDump.cpp  \
	      $(SM_DIR)/sdp/sdpsf/sdpsfFT.cpp

SDPST_SRCS  = $(SM_DIR)/sdp/sdpst/sdpstAllocExtInfo.cpp  \
	      $(SM_DIR)/sdp/sdpst/sdpstCache.cpp         \
	      $(SM_DIR)/sdp/sdpst/sdpstSH.cpp            \
	      $(SM_DIR)/sdp/sdpst/sdpstExtDir.cpp        \
	      $(SM_DIR)/sdp/sdpst/sdpstBMP.cpp           \
	      $(SM_DIR)/sdp/sdpst/sdpstRtBMP.cpp         \
	      $(SM_DIR)/sdp/sdpst/sdpstItBMP.cpp         \
	      $(SM_DIR)/sdp/sdpst/sdpstLfBMP.cpp         \
	      $(SM_DIR)/sdp/sdpst/sdpstAllocPage.cpp     \
	      $(SM_DIR)/sdp/sdpst/sdpstFreePage.cpp      \
	      $(SM_DIR)/sdp/sdpst/sdpstFindPage.cpp      \
	      $(SM_DIR)/sdp/sdpst/sdpstStackMgr.cpp      \
	      $(SM_DIR)/sdp/sdpst/sdpstUpdate.cpp        \
	      $(SM_DIR)/sdp/sdpst/sdpstFT.cpp            \
	      $(SM_DIR)/sdp/sdpst/sdpstSegDDL.cpp        \
	      $(SM_DIR)/sdp/sdpst/sdpstDPath.cpp         \
	      $(SM_DIR)/sdp/sdpst/sdpstVerifyAndDump.cpp

SDPSC_SRCS  = $(SM_DIR)/sdp/sdpsc/sdpscSegDDL.cpp        \
	      $(SM_DIR)/sdp/sdpsc/sdpscSH.cpp            \
	      $(SM_DIR)/sdp/sdpsc/sdpscED.cpp            \
	      $(SM_DIR)/sdp/sdpsc/sdpscCache.cpp         \
	      $(SM_DIR)/sdp/sdpsc/sdpscAllocPage.cpp     \
	      $(SM_DIR)/sdp/sdpsc/sdpscUpdate.cpp        \
	      $(SM_DIR)/sdp/sdpsc/sdpscFT.cpp            \
	      $(SM_DIR)/sdp/sdpsc/sdpscVerifyAndDump.cpp

SMC_SRCS  = $(SM_DIR)/smc/smcCatalogTable.cpp    \
	    $(SM_DIR)/smc/smcObject.cpp          \
	    $(SM_DIR)/smc/smcSequence.cpp        \
	    $(SM_DIR)/smc/smcSequenceUpdate.cpp  \
	    $(SM_DIR)/smc/smcTableBackupFile.cpp \
	    $(SM_DIR)/smc/smcRecord.cpp          \
	    $(SM_DIR)/smc/smcRecordUpdate.cpp    \
	    $(SM_DIR)/smc/smcTableUpdate.cpp     \
	    $(SM_DIR)/smc/smcTableSpace.cpp      \
	    $(SM_DIR)/smc/smcTable.cpp           \
	    $(SM_DIR)/smc/smcLob.cpp             \
	    $(SM_DIR)/smc/smcLobUpdate.cpp       \
	    $(SM_DIR)/smc/smcFT.cpp

SDC_SRCS  = $(SM_DIR)/sdc/sdcTSSegment.cpp         \
	    $(SM_DIR)/sdc/sdcTableCTL.cpp          \
	    $(SM_DIR)/sdc/sdcTSSlot.cpp            \
	    $(SM_DIR)/sdc/sdcUndoRecord.cpp        \
	    $(SM_DIR)/sdc/sdcUndoSegment.cpp       \
	    $(SM_DIR)/sdc/sdcUpdate.cpp            \
	    $(SM_DIR)/sdc/sdcLob.cpp               \
	    $(SM_DIR)/sdc/sdcLobUpdate.cpp         \
	    $(SM_DIR)/sdc/sdcRow.cpp               \
	    $(SM_DIR)/sdc/sdcRowUpdate.cpp         \
	    $(SM_DIR)/sdc/sdcTXSegFreeList.cpp     \
	    $(SM_DIR)/sdc/sdcTXSegMgr.cpp          \
	    $(SM_DIR)/sdc/sdcFT.cpp                \
	    $(SM_DIR)/sdc/sdcDPathInsertMgr.cpp



SMN_SRCS  = $(SM_DIR)/smn/smnManager.cpp            \
	    $(SM_DIR)/smn/smnModules.cpp            \
	    $(SM_DIR)/smn/smnpIWManager.cpp         \
	    $(SM_DIR)/smn/smnIndexFile.cpp          \
	    $(SM_DIR)/smn/smnFT.cpp

SMNN_SRCS = $(SM_DIR)/smn/smnn/smnnModule.cpp       \
			$(SM_DIR)/smn/smnn/smnnMetaModule.cpp

SMNP_SRCS = $(SM_DIR)/smn/smnp/smnpModule.cpp

SMNF_SRCS = $(SM_DIR)/smn/smnf/smnfModule.cpp

SMNB_SRCS = $(SM_DIR)/smn/smnb/smnbModule.cpp               \
	    $(SM_DIR)/smn/smnb/smnbMetaModule.cpp           \
	    $(SM_DIR)/smn/smnb/smnbValuebaseBuild.cpp       \
	    $(SM_DIR)/smn/smnb/smnbPointerbaseBuild.cpp     \
	    $(SM_DIR)/smn/smnb/smnbFT.cpp

SDN_SRCS  = $(SM_DIR)/sdn/sdnIndexCTL.cpp          \
	    $(SM_DIR)/sdn/sdnManager.cpp           \
	    $(SM_DIR)/sdn/sdnUpdate.cpp

SDNN_SRCS = $(SM_DIR)/sdn/sdnn/sdnnModule.cpp

SDNP_SRCS = $(SM_DIR)/sdn/sdnp/sdnpModule.cpp       \


SDNB_SRCS = $(SM_DIR)/sdn/sdnb/sdnbModule.cpp       \
	    $(SM_DIR)/sdn/sdnb/sdnbFT.cpp           \
	    $(SM_DIR)/sdn/sdnb/sdnbBUBuild.cpp      \
	    $(SM_DIR)/sdn/sdnb/sdnbTDBuild.cpp

SMX_SRCS  = $(SM_DIR)/smx/smxOIDList.cpp          \
	    $(SM_DIR)/smx/smxTouchPageList.cpp    \
	    $(SM_DIR)/smx/smxSavepointMgr.cpp     \
	    $(SM_DIR)/smx/smxTrans.cpp            \
	    $(SM_DIR)/smx/smxTransFreeList.cpp    \
	    $(SM_DIR)/smx/smxTransMgr.cpp         \
	    $(SM_DIR)/smx/smxLCL.cpp              \
	    $(SM_DIR)/smx/smxTableInfoMgr.cpp     \
	    $(SM_DIR)/smx/smxMinSCNBuild.cpp      \
	    $(SM_DIR)/smx/smxFT.cpp               \
	    $(SM_DIR)/smx/smxLegacyTransMgr.cpp


SML_SRCS  = $(SM_DIR)/sml/smlLockMgr.cpp          \
	    $(SM_DIR)/sml/smlFT.cpp

SMA_SRCS  = $(SM_DIR)/sma/smaDeleteThread.cpp   \
	    $(SM_DIR)/sma/smaLogicalAger.cpp    \
	    $(SM_DIR)/sma/smapManager.cpp       \
	    $(SM_DIR)/sma/smaRefineDB.cpp       \
	    $(SM_DIR)/sma/smaFT.cpp

SVR_SRCS  = $(SM_DIR)/svr/svrLogMgr.cpp        \
	    $(SM_DIR)/svr/svrRecoveryMgr.cpp

SVC_SRCS  = $(SM_DIR)/svc/svcRecord.cpp        \
	    $(SM_DIR)/svc/svcRecordUndo.cpp    \
	    $(SM_DIR)/svc/svcLob.cpp           \
	    $(SM_DIR)/svc/svcFT.cpp

SVM_SRCS  = $(SM_DIR)/svm/svmDatabase.cpp      \
	    $(SM_DIR)/svm/svmExpandChunk.cpp   \
	    $(SM_DIR)/svm/svmFPLManager.cpp    \
	    $(SM_DIR)/svm/svmManager.cpp       \
	    $(SM_DIR)/svm/svmTBSCreate.cpp     \
	    $(SM_DIR)/svm/svmTBSDrop.cpp       \
	    $(SM_DIR)/svm/svmTBSAlterAutoExtend.cpp   \
	    $(SM_DIR)/svm/svmTBSStartupShutdown.cpp   \
	    $(SM_DIR)/svm/svmUpdate.cpp        \
	    $(SM_DIR)/svm/svmFT.cpp

SVP_SRCS  = $(SM_DIR)/svp/svpAllocPageList.cpp \
	    $(SM_DIR)/svp/svpFixedPageList.cpp \
	    $(SM_DIR)/svp/svpFreePageList.cpp  \
	    $(SM_DIR)/svp/svpManager.cpp       \
	    $(SM_DIR)/svp/svpVarPageList.cpp   \
	    $(SM_DIR)/svp/svpFT.cpp

SVNN_SRCS = $(SM_DIR)/svn/svnn/svnnModule.cpp

SVNP_SRCS = $(SM_DIR)/svn/svnp/svnpModule.cpp

SVNB_SRCS = $(SM_DIR)/svn/svnb/svnbModule.cpp

SGM_SRCS  = $(SM_DIR)/sgm/sgmManager.cpp

SMI_SRCS  = $(SM_DIR)/smi/smiTableSpace.cpp       \
	    $(SM_DIR)/smi/smiObject.cpp           \
	    $(SM_DIR)/smi/smiTrans.cpp            \
	    $(SM_DIR)/smi/smiStatement.cpp        \
	    $(SM_DIR)/smi/smiTable.cpp            \
	    $(SM_DIR)/smi/smiTableCursor.cpp      \
	    $(SM_DIR)/smi/smiMain.cpp             \
	    $(SM_DIR)/smi/smiMisc.cpp             \
	    $(SM_DIR)/smi/smiFixedTable.cpp       \
	    $(SM_DIR)/smi/smiLogRec.cpp           \
	    $(SM_DIR)/smi/smiMediaRecovery.cpp    \
	    $(SM_DIR)/smi/smiBackup.cpp           \
	    $(SM_DIR)/smi/smiReadLogByOrder.cpp   \
	    $(SM_DIR)/smi/smiLob.cpp              \
	    $(SM_DIR)/smi/smiTableBackup.cpp      \
	    $(SM_DIR)/smi/smiVolTableBackup.cpp   \
	    $(SM_DIR)/smi/smiDataPort.cpp         \
	    $(SM_DIR)/smi/smiStatistics.cpp       \
	    $(SM_DIR)/smi/smiLegacyTrans.cpp      \
	    $(SM_DIR)/smi/smiTempTable.cpp        \
	    $(SM_DIR)/smi/smiSortTempTable.cpp    \
	    $(SM_DIR)/smi/smiHashTempTable.cpp

SCP_SRCS  = $(SM_DIR)/scp/scpModule.cpp           \
	    $(SM_DIR)/scp/scpManager.cpp          \
	    $(SM_DIR)/scp/scpFT.cpp

SCPF_SRCS = $(SM_DIR)/scp/scpf/scpfModule.cpp     \
	    $(SM_DIR)/scp/scpf/scpfFT.cpp

SDS_SRCS  = $(SM_DIR)/sds/sdsBufferMgr.cpp        \
	    $(SM_DIR)/sds/sdsBufferArea.cpp       \
	    $(SM_DIR)/sds/sdsBCB.cpp              \
	    $(SM_DIR)/sds/sdsMeta.cpp             \
	    $(SM_DIR)/sds/sdsFlusher.cpp          \
	    $(SM_DIR)/sds/sdsFlushMgr.cpp         \
	    $(SM_DIR)/sds/sdsFile.cpp             \
	    $(SM_DIR)/sds/sdsFT.cpp

SM_SRCS   = $(SMU_SRCS) $(SCT_SRCS)  $(SDT_SRCS)                        \
	    $(SDD_SRCS) $(SDB_SRCS) $(SDS_SRCS) $(SDR_SRCS)             \
	    $(SDP_SRCS) $(SDPSF_SRCS) $(SDPST_SRCS)                     \
	    $(SDPSC_SRCS) $(SDPTB_SRCS) $(SMR_SRCS) $(SMM_SRCS)         \
	    $(SMP_SRCS) $(SMX_SRCS) $(SML_SRCS) $(SMRI_SRCS)            \
	    $(SDC_SRCS) $(SMC_SRCS) $(SDN_SRCS)                         \
	    $(SDNB_SRCS) $(SDNN_SRCS) $(SDNP_SRCS) $(SMN_SRCS)          \
	    $(SMNN_SRCS) $(SMNP_SRCS) $(SMNB_SRCS)                      \
	    $(SMNF_SRCS) $(SMA_SRCS)                                    \
	    $(SGM_SRCS)                                                 \
	    $(SVR_SRCS) $(SVC_SRCS) $(SVM_SRCS) $(SVP_SRCS)             \
	    $(SVNN_SRCS) $(SVNP_SRCS) $(SVNB_SRCS)                      \
	    $(SMT_SRCS) $(SMI_SRCS)                                     \
	    $(SCP_SRCS) $(SCPF_SRCS)

SM_OBJS = $(SM_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
SM_SHOBJS = $(SM_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))
