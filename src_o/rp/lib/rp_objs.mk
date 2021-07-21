#
# $Id: rp_objs.mk 90266 2021-03-19 05:23:09Z returns $
#


RPC_SRCS = $(RP_DIR)/rpc/rpcExecute.cpp \
           $(RP_DIR)/rpc/rpcManager.cpp \
		   $(RP_DIR)/rpc/rpcManagerMisc.cpp \
		   $(RP_DIR)/rpc/rpcHBT.cpp \
		   $(RP_DIR)/rpc/rpcDDLSyncManager.cpp \
		   $(RP_DIR)/rpc/rpcDDLASyncManager.cpp \
		   $(RP_DIR)/rpc/rpcResourceManager.cpp \
		   $(RP_DIR)/rpc/rpcValidate.cpp 	\
		   $(RP_DIR)/rpc/rpcReceiverList.cpp

RPD_SRCS = $(RP_DIR)/rpd/rpdLogAnalyzer.cpp \
           $(RP_DIR)/rpd/rpdMeta.cpp \
           $(RP_DIR)/rpd/rpdQueue.cpp \
           $(RP_DIR)/rpd/rpdTransTbl.cpp \
           $(RP_DIR)/rpd/rpdTransEntry.cpp \
           $(RP_DIR)/rpd/rpdSenderInfo.cpp \
           $(RP_DIR)/rpd/rpdLogBufferMgr.cpp \
           $(RP_DIR)/rpd/rpdCatalog.cpp \
           $(RP_DIR)/rpd/rpdAnalyzingTransTable.cpp \
           $(RP_DIR)/rpd/rpdTransSlotNode.cpp \
           $(RP_DIR)/rpd/rpdReplicatedTransGroupNode.cpp \
           $(RP_DIR)/rpd/rpdAnalyzedTransTable.cpp \
           $(RP_DIR)/rpd/rpdReplicatedTransGroup.cpp \
           $(RP_DIR)/rpd/rpdDelayedLogQueue.cpp	\
           $(RP_DIR)/rpd/rpdConvertSQL.cpp \
           $(RP_DIR)/rpd/rpdXLogfileMgr.cpp \
           $(RP_DIR)/rpd/rpdXLogfileFlusher.cpp \
           $(RP_DIR)/rpd/rpdXLogfileCreater.cpp 	\
		   $(RP_DIR)/rpd/rpdLockTableManager.cpp	\
		   ${RP_DIR}/rpd/rpdStatistics.cpp

RPS_SRCS = $(RP_DIR)/rps/rpsSmExecutor.cpp		\
		   $(RP_DIR)/rps/rpsSQLExecutor.cpp

RPX_SRCS = $(RP_DIR)/rpx/rpxReceiver.cpp \
		   $(RP_DIR)/rpx/rpxReceiverApplier.cpp \
		   $(RP_DIR)/rpx/rpxReceiverError.cpp \
		   $(RP_DIR)/rpx/rpxReceiverApply.cpp \
		   $(RP_DIR)/rpx/rpxReceiverHandshake.cpp \
		   $(RP_DIR)/rpx/rpxReceiverMisc.cpp \
		   $(RP_DIR)/rpx/rpxSender.cpp \
		   $(RP_DIR)/rpx/rpxSenderApply.cpp \
		   $(RP_DIR)/rpx/rpxSenderSync.cpp \
		   $(RP_DIR)/rpx/rpxSenderXLog.cpp \
		   $(RP_DIR)/rpx/rpxSenderXLSN.cpp \
		   $(RP_DIR)/rpx/rpxSenderHandshake.cpp \
		   $(RP_DIR)/rpx/rpxSenderMisc.cpp \
		   $(RP_DIR)/rpx/rpxSenderSyncParallel.cpp \
		   $(RP_DIR)/rpx/rpxDDLExecutor.cpp \
		   $(RP_DIR)/rpx/rpxPJMgr.cpp \
		   $(RP_DIR)/rpx/rpxPJChild.cpp \
		   $(RP_DIR)/rpx/rpxReplicator.cpp \
		   $(RP_DIR)/rpx/rpxSync.cpp \
		   $(RP_DIR)/rpx/rpxAheadAnalyzer.cpp \
		   $(RP_DIR)/rpx/rpxTempSender.cpp \
		   $(RP_DIR)/rpx/rpxXLogTransfer.cpp 
RPI_SRCS = $(RP_DIR)/rpi/rpi.cpp

RPI_STUB_SRCS = $(RP_DIR)/rpi/rpistub.cpp 

RPU_SRCS = $(RP_DIR)/rpu/rpuProperty.cpp \
           $(RP_DIR)/rpu/rpuTraceCode.cpp

RPN_SRCS = $(RP_DIR)/rpn/rpnComm.cpp \
           $(RP_DIR)/rpn/rpnCommA5.cpp \
           $(RP_DIR)/rpn/rpnCommA7.cpp \
           $(RP_DIR)/rpn/rpnMessenger.cpp \
           $(RP_DIR)/rpn/rpnSocket.cpp \
           $(RP_DIR)/rpn/rpnPoll.cpp

RPR_SRCS = $(RP_DIR)/rpr/rprSNMapMgr.cpp

RP_SRCS  = $(RPC_SRCS) $(RPX_SRCS) $(RPD_SRCS) $(RPS_SRCS) $(RPU_SRCS) $(RPN_SRCS) $(RPR_SRCS) $(RPI_SRCS)

RP_OBJS        = $(RP_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
RP_SHOBJS      = $(RP_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

RP_STUB_OBJS   = $(RPI_STUB_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
RP_STUB_SHOBJS = $(RPI_STUB_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))
