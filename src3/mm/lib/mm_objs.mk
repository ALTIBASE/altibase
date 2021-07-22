MMC_SRCS = $(MM_DIR)/mmc/mmcConv.cpp                   \
           $(MM_DIR)/mmc/mmcConvFmMT.cpp               \
           $(MM_DIR)/mmc/mmcConvNumeric.cpp            \
           $(MM_DIR)/mmc/mmcLob.cpp                    \
           $(MM_DIR)/mmc/mmcMutexPool.cpp              \
           $(MM_DIR)/mmc/mmcSession.cpp                \
           $(MM_DIR)/mmc/mmcStatement.cpp              \
           $(MM_DIR)/mmc/mmcStatementBegin.cpp         \
           $(MM_DIR)/mmc/mmcStatementExecute.cpp       \
           $(MM_DIR)/mmc/mmcStatementManager.cpp       \
           $(MM_DIR)/mmc/mmcTask.cpp                   \
           $(MM_DIR)/mmc/mmcTrans.cpp                  \
           $(MM_DIR)/mmc/mmcPCB.cpp                    \
           $(MM_DIR)/mmc/mmcParentPCO.cpp              \
           $(MM_DIR)/mmc/mmcChildPCO.cpp               \
           $(MM_DIR)/mmc/mmcSQLTextHash.cpp            \
           $(MM_DIR)/mmc/mmcPlanCache.cpp              \
           $(MM_DIR)/mmc/mmcPlanCacheLRUList.cpp       \
           $(MM_DIR)/mmc/mmcEventManager.cpp           \
           $(MM_DIR)/mmc/mmcSharedTrans.cpp            \
           $(MM_DIR)/mmc/mmcNonSharedTrans.cpp         \
           $(MM_DIR)/mmc/mmcTransManager.cpp

MMD_SRCS = $(MM_DIR)/mmd/mmdManager.cpp                \
           $(MM_DIR)/mmd/mmdXa.cpp                     \
           $(MM_DIR)/mmd/mmdXid.cpp                    \
           $(MM_DIR)/mmd/mmdXidManager.cpp             

MMM_SRCS = $(MM_DIR)/mmm/mmm.cpp                       \
           $(MM_DIR)/mmm/mmmDescInit.cpp               \
           $(MM_DIR)/mmm/mmmDescPreProcess.cpp         \
           $(MM_DIR)/mmm/mmmDescProcess.cpp            \
           $(MM_DIR)/mmm/mmmDescControl.cpp            \
           $(MM_DIR)/mmm/mmmDescMeta.cpp               \
           $(MM_DIR)/mmm/mmmDescDowngrade.cpp          \
           $(MM_DIR)/mmm/mmmDescService.cpp            \
           $(MM_DIR)/mmm/mmmDescShutdown.cpp           \
           $(MM_DIR)/mmm/mmmActBeginControl.cpp        \
           $(MM_DIR)/mmm/mmmActBeginMeta.cpp           \
           $(MM_DIR)/mmm/mmmActBeginDowngrade.cpp      \
           $(MM_DIR)/mmm/mmmActBeginPreProcess.cpp     \
           $(MM_DIR)/mmm/mmmActBeginProcess.cpp        \
           $(MM_DIR)/mmm/mmmActBeginService.cpp        \
           $(MM_DIR)/mmm/mmmActBeginShutdown.cpp       \
           $(MM_DIR)/mmm/mmmActCallback.cpp            \
           $(MM_DIR)/mmm/mmmActCheckEnv.cpp            \
           $(MM_DIR)/mmm/mmmActCheckIdeTSD.cpp         \
           $(MM_DIR)/mmm/mmmActCheckLicense.cpp        \
           $(MM_DIR)/mmm/mmmActDaemon.cpp              \
           $(MM_DIR)/mmm/mmmActEndControl.cpp          \
           $(MM_DIR)/mmm/mmmActEndMeta.cpp             \
           $(MM_DIR)/mmm/mmmActEndDowngrade.cpp        \
           $(MM_DIR)/mmm/mmmActEndPreProcess.cpp       \
           $(MM_DIR)/mmm/mmmActEndProcess.cpp          \
           $(MM_DIR)/mmm/mmmActEndService.cpp          \
           $(MM_DIR)/mmm/mmmActEndShutdown.cpp         \
           $(MM_DIR)/mmm/mmmActFixedTable.cpp          \
           $(MM_DIR)/mmm/mmmActInitBootMsgLog.cpp      \
           $(MM_DIR)/mmm/mmmActLogRLimit.cpp           \
           $(MM_DIR)/mmm/mmmActInitCM.cpp              \
           $(MM_DIR)/mmm/mmmActInitGPKI.cpp            \
           $(MM_DIR)/mmm/mmmActInitLockFile.cpp        \
           $(MM_DIR)/mmm/mmmActInitMT.cpp              \
           $(MM_DIR)/mmm/mmmActInitMemoryMgr.cpp       \
           $(MM_DIR)/mmm/mmmActInitMemPoolMgr.cpp      \
           $(MM_DIR)/mmm/mmmActInitModuleMsgLog.cpp    \
           $(MM_DIR)/mmm/mmmActInitNLS.cpp             \
           $(MM_DIR)/mmm/mmmActInitOS.cpp              \
           $(MM_DIR)/mmm/mmmActInitQP.cpp              \
           $(MM_DIR)/mmm/mmmActInitSecurity.cpp        \
           $(MM_DIR)/mmm/mmmActInitREPL.cpp            \
           $(MM_DIR)/mmm/mmmActInitSM.cpp              \
           $(MM_DIR)/mmm/mmmActInitSNMP.cpp            \
           $(MM_DIR)/mmm/mmmActInitService.cpp         \
           $(MM_DIR)/mmm/mmmActInitThreadManager.cpp   \
           $(MM_DIR)/mmm/mmmActInitXA.cpp              \
           $(MM_DIR)/mmm/mmmActInitQueryProfile.cpp    \
           $(MM_DIR)/mmm/mmmActInitDK.cpp              \
           $(MM_DIR)/mmm/mmmActInitAudit.cpp           \
           $(MM_DIR)/mmm/mmmActInitJobManager.cpp      \
           $(MM_DIR)/mmm/mmmActInitSnapshotExport.cpp  \
           $(MM_DIR)/mmm/mmmActInitRBHash.cpp		   \
           $(MM_DIR)/mmm/mmmActLoadMsb.cpp             \
           $(MM_DIR)/mmm/mmmActPreallocMemory.cpp      \
           $(MM_DIR)/mmm/mmmActProperty.cpp            \
           $(MM_DIR)/mmm/mmmActShutdownCM.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownEND.cpp         \
           $(MM_DIR)/mmm/mmmActShutdownFixedTable.cpp  \
           $(MM_DIR)/mmm/mmmActShutdownGPKI.cpp        \
           $(MM_DIR)/mmm/mmmActShutdownMT.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownPrepare.cpp     \
           $(MM_DIR)/mmm/mmmActShutdownQP.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownSecurity.cpp    \
           $(MM_DIR)/mmm/mmmActShutdownREPL.cpp        \
           $(MM_DIR)/mmm/mmmActShutdownSM.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownSNMP.cpp        \
           $(MM_DIR)/mmm/mmmActShutdownService.cpp     \
           $(MM_DIR)/mmm/mmmActShutdownTimer.cpp       \
           $(MM_DIR)/mmm/mmmActShutdownXA.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownQueryProfile.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownDK.cpp          \
           $(MM_DIR)/mmm/mmmActShutdownProperty.cpp    \
           $(MM_DIR)/mmm/mmmActSignal.cpp              \
           $(MM_DIR)/mmm/mmmActSystemUpTime.cpp        \
           $(MM_DIR)/mmm/mmmActPIDInfo.cpp             \
           $(MM_DIR)/mmm/mmmActThread.cpp              \
           $(MM_DIR)/mmm/mmmActTimer.cpp               \
           $(MM_DIR)/mmm/mmmActVersionInfo.cpp         \
           $(MM_DIR)/mmm/mmmActWaitAdmin.cpp           \
           $(MM_DIR)/mmm/mmmActInitSD.cpp    	       \
           $(MM_DIR)/mmm/mmmActShutdownSD.cpp


MMQ_SRCS = $(MM_DIR)/mmq/mmqManager.cpp                \
           $(MM_DIR)/mmq/mmqQueueInfo.cpp

MMT_SRCS = $(MM_DIR)/mmt/mmtAdminManager.cpp           \
           $(MM_DIR)/mmt/mmtSessionManager.cpp         \
           $(MM_DIR)/mmt/mmtThreadManager.cpp          \
           $(MM_DIR)/mmt/mmtDispatcher.cpp             \
           $(MM_DIR)/mmt/mmtServiceThread.cpp          \
           $(MM_DIR)/mmt/mmtCmsAdmin.cpp               \
           $(MM_DIR)/mmt/mmtCmsBind.cpp                \
           $(MM_DIR)/mmt/mmtCmsConnection.cpp          \
           $(MM_DIR)/mmt/mmtCmsControl.cpp             \
           $(MM_DIR)/mmt/mmtCmsCreateDB.cpp            \
           $(MM_DIR)/mmt/mmtCmsDropDB.cpp              \
           $(MM_DIR)/mmt/mmtCmsError.cpp               \
           $(MM_DIR)/mmt/mmtCmsExecute.cpp             \
           $(MM_DIR)/mmt/mmtCmsFetch.cpp               \
           $(MM_DIR)/mmt/mmtCmsLob.cpp                 \
           $(MM_DIR)/mmt/mmtCmsPrepare.cpp             \
           $(MM_DIR)/mmt/mmtCmsXa.cpp                  \
           $(MM_DIR)/mmt/mmtCmsShard.cpp               \
           $(MM_DIR)/mmt/mmtInternalSql.cpp            \
           $(MM_DIR)/mmt/mmtLoadBalancer.cpp           \
           $(MM_DIR)/mmt/mmtAuditManager.cpp           \
           $(MM_DIR)/mmt/mmtJobManager.cpp			   \
           $(MM_DIR)/mmt/mmtSnapshotExportManager.cpp

MMT5_SRCS = $(MM_DIR)/mmt5/mmt5CmsBind.cpp               \
           $(MM_DIR)/mmt5/mmt5CmsConnection.cpp          \
           $(MM_DIR)/mmt5/mmt5CmsControl.cpp             \
           $(MM_DIR)/mmt5/mmt5CmsError.cpp               \
           $(MM_DIR)/mmt5/mmt5CmsExecute.cpp             \
           $(MM_DIR)/mmt5/mmt5CmsFetch.cpp               \
           $(MM_DIR)/mmt5/mmt5CmsLob.cpp                 \
           $(MM_DIR)/mmt5/mmt5CmsPrepare.cpp             \
           $(MM_DIR)/mmt5/mmt5CmsXa.cpp

MMI_SRCS = $(MM_DIR)/mmi/mmi.cpp

MMU_SRCS = $(MM_DIR)/mmu/mmuDump.cpp                   \
           $(MM_DIR)/mmu/mmuFixedTable.cpp             \
           $(MM_DIR)/mmu/mmuServerStat.cpp             \
           $(MM_DIR)/mmu/mmuOS.cpp                     \
           $(MM_DIR)/mmu/mmuTraceCode.cpp              \
           $(MM_DIR)/mmu/mmuProperty.cpp               \
           $(MM_DIR)/mmu/mmuAccessList.cpp

MMS_SRCS = $(MM_DIR)/mms/mmsSNMP.cpp                        \
		   $(MM_DIR)/mms/mmsSNMPModule.cpp                  \
		   $(MM_DIR)/mms/mmsSNMPModuleAltiStatusTable.cpp   \
		   $(MM_DIR)/mms/mmsSNMPModuleAltiPropertyTable.cpp

MM_SRCS  = $(MMC_SRCS) $(MMD_SRCS) $(MMM_SRCS) $(MMQ_SRCS) $(MMT_SRCS) $(MMT5_SRCS) $(MMI_SRCS) $(MMU_SRCS) $(MMS_SRCS)
MM_OBJS  = $(MM_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
MM_SHOBJS  = $(MM_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

MM_ODBC_OBJS   = $(MMI_ODBC_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
MM_ODBC_SHOBJS = $(MMI_ODBC_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))
