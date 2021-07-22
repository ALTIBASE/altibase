# Makefile for ID library
#
# CVS Info : $Id: id_objs.mk 87881 2020-06-29 08:04:01Z kclee $
#

# NORMAL LIBRARY
IDL_SRCS  =    $(ID_DIR)/idl/idl.cpp

IDA_SRCS  =    $(ID_DIR)/ida/idaXa.cpp

IDKS_SRCS  =  #$(ID_DIR)/idk/idkAtomic.S

IDN_SRCS   =   $(ID_DIR)/idn/idn.cpp           \
               $(ID_DIR)/idn/idnConv.cpp       \
               $(ID_DIR)/idn/idnAscii.cpp      \
               $(ID_DIR)/idn/idnJisx0201.cpp   \
               $(ID_DIR)/idn/idnJisx0208.cpp   \
               $(ID_DIR)/idn/idnJisx0212.cpp   \
               $(ID_DIR)/idn/idnGb2312.cpp     \
               $(ID_DIR)/idn/idnBig5.cpp       \
               $(ID_DIR)/idn/idnEucjp.cpp      \
               $(ID_DIR)/idn/idnKsc5601.cpp    \
               $(ID_DIR)/idn/idnEuckr.cpp      \
               $(ID_DIR)/idn/idnSjis.cpp       \
               $(ID_DIR)/idn/idnCp932.cpp      \
               $(ID_DIR)/idn/idnCp932ext.cpp   \
               $(ID_DIR)/idn/idnUhc1.cpp       \
               $(ID_DIR)/idn/idnUhc2.cpp       \
               $(ID_DIR)/idn/idnCp949.cpp      \
               $(ID_DIR)/idn/idnCp936.cpp      \
               $(ID_DIR)/idn/idnCp936ext.cpp   \
               $(ID_DIR)/idn/idnGbk.cpp        \
               $(ID_DIR)/idn/idnGbkext1.cpp    \
               $(ID_DIR)/idn/idnGbkext2.cpp    \
               $(ID_DIR)/idn/idnGbkextinv.cpp  \
               $(ID_DIR)/idn/idnUtf8.cpp

IDT_SRCS   =   $(ID_DIR)/idt/idtBaseThread.cpp	\
			   $(ID_DIR)/idt/idtContainer.cpp	\
			   $(ID_DIR)/idt/idtCPUSet.cpp		

IDE_SRCS   =   $(ID_DIR)/ide/ideMsgLog.cpp          \
               $(ID_DIR)/ide/ideCallback.cpp        \
               $(ID_DIR)/ide/ideErrorMgr.cpp        \
               $(ID_DIR)/ide/ideFaultMgr.cpp        \
               $(ID_DIR)/ide/ideLog.cpp             \
               $(ID_DIR)/ide/ideLogEntry.cpp

IDF_SRCS   =   $(ID_DIR)/idf/idf.cpp                \
               $(ID_DIR)/idf/idfCore.cpp            \
			   $(ID_DIR)/idf/idfMemory.cpp

IDE_CLI_SRCS = $(ID_DIR)/ide/ideMsgLog.cpp       	\
               $(ID_DIR)/ide/ideErrorMgr_client.cpp 		\
               $(ID_DIR)/ide/ideFaultMgr.cpp        \
               $(ID_DIR)/ide/ideLog.cpp        \
               $(ID_DIR)/ide/ideLogEntry.cpp        \
               $(ID_DIR)/ide/ideCallback.cpp

IDM_SRCS   =   $(ID_DIR)/idm/idm.cpp       \
               $(ID_DIR)/idm/idmSNMP.cpp

IDU_SRCS   =   $(ID_DIR)/idu/iduVersion.cpp    \
               $(ID_DIR)/idu/iduCheckLicense.cpp \
               $(ID_DIR)/idu/iduMemory.cpp \
               $(ID_DIR)/idu/iduVarMemList.cpp \
               $(ID_DIR)/idu/iduStack.cpp \
               $(ID_DIR)/idu/iduMutex.cpp \
               $(ID_DIR)/idu/iduMutexEntry.cpp \
               $(ID_DIR)/idu/iduMutexEntryPOSIX.cpp \
               $(ID_DIR)/idu/iduMutexEntryPOSIX_client.cpp \
               $(ID_DIR)/idu/iduMutexMgr.cpp \
               $(ID_DIR)/idu/iduMemClientInfo.cpp \
               $(ID_DIR)/idu/iduMemList.cpp \
               $(ID_DIR)/idu/iduMemListOld.cpp \
               $(ID_DIR)/idu/iduMemMgr.cpp \
               $(ID_DIR)/idu/iduMemMgr_single.cpp \
               $(ID_DIR)/idu/iduMemMgr_libc.cpp \
               $(ID_DIR)/idu/iduMemMgr_tlsf.cpp \
               $(ID_DIR)/idu/iduMemMgr_innocense.cpp \
               $(ID_DIR)/idu/iduMemPoolMgr.cpp \
               $(ID_DIR)/idu/iduMemPool.cpp \
               $(ID_DIR)/idu/iduFile.cpp     \
               $(ID_DIR)/idu/iduFileAIO.cpp     \
               $(ID_DIR)/idu/iduFileIOVec.cpp     \
               $(ID_DIR)/idu/iduAIOQueue.cpp     \
               $(ID_DIR)/idu/iduRunTimeInfo.cpp \
               $(ID_DIR)/idu/iduStackMgr.cpp \
               $(ID_DIR)/idu/iduPtrList.cpp \
               $(ID_DIR)/idu/iduSessionEvent.cpp \
               $(ID_DIR)/idu/iduOIDMemory.cpp \
               $(ID_DIR)/idu/iduArgument.cpp \
               $(ID_DIR)/idu/iduMemPool2.cpp \
               $(ID_DIR)/idu/iduHash.cpp \
               $(ID_DIR)/idu/iduHashUtil.cpp \
               $(ID_DIR)/idu/iduStringHash.cpp \
               $(ID_DIR)/idu/iduFixedTable.cpp \
               $(ID_DIR)/idu/iduLimitManager.cpp \
               $(ID_DIR)/idu/iduCompression.cpp \
               $(ID_DIR)/idu/iduLZ4.cpp \
               $(ID_DIR)/idu/iduLatch.cpp \
               $(ID_DIR)/idu/iduLatchFT.cpp \
               $(ID_DIR)/idu/iduLatchTypePosix.cpp \
               $(ID_DIR)/idu/iduLatchTypePosix2.cpp \
               $(ID_DIR)/idu/iduLatchTypeNative.cpp  \
               $(ID_DIR)/idu/iduLatchTypeNative2.cpp  \
               $(ID_DIR)/idu/iduTable.cpp \
               $(ID_DIR)/idu/iduProperty.cpp \
               $(ID_DIR)/idu/iduFileStream.cpp \
               $(ID_DIR)/idu/iduBridgeForC.cpp \
               $(ID_DIR)/idu/iduTraceCode.cpp       \
               $(ID_DIR)/idu/iduVarString.cpp \
               $(ID_DIR)/idu/iduVarMemString.cpp \
               $(ID_DIR)/idu/iduQueue.cpp \
               $(ID_DIR)/idu/iduQueueDualLock.cpp \
               $(ID_DIR)/idu/iduHeap.cpp \
               $(ID_DIR)/idu/iduPriorityQueue.cpp \
               $(ID_DIR)/idu/iduHeapSort.cpp \
               $(ID_DIR)/idu/iduReusedMemoryHandle.cpp  \
               $(ID_DIR)/idu/iduGrowingMemoryHandle.cpp \
               $(ID_DIR)/idu/iduFXStack.cpp \
               $(ID_DIR)/idu/iduCond.cpp \
               $(ID_DIR)/idu/iduFatalCallback.cpp \
               $(ID_DIR)/idu/iduFitManager.cpp    \
               $(ID_DIR)/idu/iduShmProcType.cpp
#              $(ID_DIR)/idu/iduQueueLockFree.cpp
#              $(ID_DIR)/idu/iduSema.cpp

ifeq "$(OS_TARGET)" "X86_SOLARIS"
ifeq "$(compile64)" "1"
IDUS_SRCS  =   $(ID_DIR)/idu/x86_cas64.s
else
IDUS_SRCS  =   $(ID_DIR)/idu/x86_cas32.s
endif
endif

ifeq "$(OS_TARGET)" "SPARC_SOLARIS"
ifeq "$(compile64)" "1"
IDUS_SRCS  =   $(ID_DIR)/idu/sparc_cas64.s
else
IDUS_SRCS  =   $(ID_DIR)/idu/sparc_cas32.s
endif
endif


IDU_CLI_SRCS = $(ID_DIR)/idu/iduVersion.cpp    \
               $(ID_DIR)/idu/iduMemory.cpp \
               $(ID_DIR)/idu/iduVarMemList.cpp \
               $(ID_DIR)/idu/iduMemMgr.cpp \
               $(ID_DIR)/idu/iduMemMgr_single.cpp \
               $(ID_DIR)/idu/iduMemMgr_libc.cpp \
               $(ID_DIR)/idu/iduMemMgr_tlsf.cpp \
               $(ID_DIR)/idu/iduMemPoolMgr.cpp \
               $(ID_DIR)/idu/iduMemPool.cpp \
               $(ID_DIR)/idu/iduMemList.cpp \
               $(ID_DIR)/idu/iduMemListOld.cpp \
               $(ID_DIR)/idu/iduStack.cpp \
               $(ID_DIR)/idu/iduHash.cpp \
               $(ID_DIR)/idu/iduHashUtil.cpp \
               $(ID_DIR)/idu/iduFileStream.cpp \
               $(ID_DIR)/idu/iduStringHash.cpp \
               $(ID_DIR)/idu/iduStackMgr.cpp \
               $(ID_DIR)/idu/iduMutex.cpp \
               $(ID_DIR)/idu/iduMutexMgr.cpp \
               $(ID_DIR)/idu/iduMutexEntry.cpp \
               $(ID_DIR)/idu/iduMutexEntryPOSIX.cpp \
               $(ID_DIR)/idu/iduMutexEntryPOSIX_client.cpp \
               $(ID_DIR)/idu/iduProperty.cpp \
               $(ID_DIR)/idu/iduTraceCode.cpp \
               $(ID_DIR)/idu/iduVarString.cpp \
               $(ID_DIR)/idu/iduBridgeForC.cpp \
               $(ID_DIR)/idu/iduFixedTable.cpp  \
               $(ID_DIR)/idu/iduLimitManager_client.cpp \
               $(ID_DIR)/idu/iduCond.cpp \
               $(ID_DIR)/idu/iduCompression.cpp

IDUC_SRCS =    $(ID_DIR)/idu/iduBarrier.c          \
               $(ID_DIR)/idu/iduMutexEntryNative.c \
               $(ID_DIR)/idu/iduMutexEntryNative_client.c

IDUC_CLI_SRCS = $(ID_DIR)/idu/iduBarrier.c          \
                $(ID_DIR)/idu/iduMutexEntryNative.c \
                $(ID_DIR)/idu/iduMutexEntryNative_client.c

IDS_SRCS   =   $(ID_DIR)/ids/idsCrypt.cpp    \
               $(ID_DIR)/ids/idsDES.cpp      \
               $(ID_DIR)/ids/idsAES.cpp      \
               $(ID_DIR)/ids/idsSHA1.cpp     \
               $(ID_DIR)/ids/idsSHA256.cpp   \
               $(ID_DIR)/ids/idsSHA512.cpp   \
               $(ID_DIR)/ids/idsPassword.cpp \
               $(ID_DIR)/ids/idsGPKI.cpp     \
               $(ID_DIR)/ids/idsRC4.cpp      \
               $(ID_DIR)/ids/idsBase64.cpp   \
               $(ID_DIR)/ids/idsAltiWrap.cpp

IDP_SRCS   =   $(ID_DIR)/idp/idp.cpp     \
               $(ID_DIR)/idp/idpBase.cpp \
               $(ID_DIR)/idp/idpUInt.cpp \
               $(ID_DIR)/idp/idpULong.cpp \
               $(ID_DIR)/idp/idpSInt.cpp \
               $(ID_DIR)/idp/idpSLong.cpp \
               $(ID_DIR)/idp/idpString.cpp \
               $(ID_DIR)/idp/idpDescResource.cpp

IDV_SRCS   =   $(ID_DIR)/idv/idv.cpp     \
               $(ID_DIR)/idv/idvHandlerDefault.cpp \
               $(ID_DIR)/idv/idvHandlerTimer.cpp  \
               $(ID_DIR)/idv/idvHandlerClock.cpp  \
               $(ID_DIR)/idv/idvTimeFuncNone.cpp \
               $(ID_DIR)/idv/idvTimeFuncThread.cpp \
               $(ID_DIR)/idv/idvTimeFuncLibrary.cpp \
               $(ID_DIR)/idv/idvTimeFuncClock.cpp \
               $(ID_DIR)/idv/idvProfile.cpp \
               $(ID_DIR)/idv/idvAudit.cpp 

IDVC_SRCS =    $(ID_DIR)/idv/idvTimeGetClock.c

IDP_FIXED_SRCS = $(ID_DIR)/idp/idpFT.cpp \
                 $(ID_DIR)/idu/iduMemMgrFT.cpp \
                 $(ID_DIR)/idu/iduMutexMgrFT.cpp \
                 $(ID_DIR)/ide/ideMsgLogFT.cpp

IDW_SRCS    =  $(ID_DIR)/idw/idwService.cpp

IDD_SRCS 	=  $(ID_DIR)/idd/iddRBTree.cpp			\
               $(ID_DIR)/idd/iddTRBTree.cpp			\
               $(ID_DIR)/idd/iddRBHash.cpp

IDCORE_SRCS = $(ID_DIR)/idCore/idCore.cpp

# PROJ-1685
IDX_SRCS = $(ID_DIR)/idx/idxProc.cpp \
           $(ID_DIR)/idx/idxProcFT.cpp \
           $(ID_DIR)/idx/idxLocalSock.cpp

# for fixed table only server
ID_FIXEDTABLE_SRCS = $(IDP_FIXED_SRCS)

ifeq "$(OS_TARGET)" "INTEL_WINDOWS"
ifneq "$(OS_TARGET2)" "WINCE"
ID_STACKTRACE=$(ID_DIR)/idu/iduWinCallstack.cpp
endif # WINCE
else
ifeq "$(OS_TARGET)" "DEC_TRU64"
ID_STACKTRACE=$(ID_DIR)/idu/iduDecCallstack.cpp
endif # DEC_TRU64
endif # INTEL_WINDOW


ID_SRCS    =   $(IDL_SRCS) $(IDT_SRCS) $(IDP_SRCS) \
               $(IDA_SRCS) $(IDN_SRCS) $(IDM_SRCS) \
               $(IDE_SRCS) $(IDU_SRCS) $(IDS_SRCS) \
               $(IDV_SRCS) $(ID_FIXEDTABLE_SRCS) $(ID_STACKTRACE) \
               $(IDW_SRCS) $(IDF_SRCS) $(IDCORE_SRCS) $(IDX_SRCS) \
			   $(IDD_SRCS)

##################################################################
# server libs
##################################################################

ID_OBJS  = $(ID_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT)) \
 $(IDUS_SRCS:$(DEV_DIR)/%.s=$(TARGET_DIR)/%.$(OBJEXT)) \
 $(IDUC_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT)) \
 $(IDVC_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT)) \
 $(IDKS_SRCS:$(DEV_DIR)/%.S=$(TARGET_DIR)/%.$(OBJEXT))
ID_SHOBJS  = $(ID_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT)) \
 $(IDUS_SRCS:$(DEV_DIR)/%.s=$(TARGET_DIR)/%_shobj.$(OBJEXT)) \
 $(IDUC_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%_shobj.$(OBJEXT)) \
 $(IDVC_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%_shobj.$(OBJEXT)) \
 $(IDKS_SRCS:$(DEV_DIR)/%.S=$(TARGET_DIR)/%_shobj.$(OBJEXT))

##################################################################
# client libs
##################################################################

ID_CLIENT_SRCS =  $(IDL_SRCS) $(IDT_SRCS) $(IDKS_SRCS) $(IDLW_SRCS)\
                  $(IDA_SRCS) $(IDN_SRCS) $(IDM_SRCS) $(IDP_SRCS) \
                  $(IDE_CLI_SRCS) $(IDU_CLI_SRCS) $(IDS_SRCS) $(IDV_SRCS)\
		  		  $(IDVC_SRCS) $(IDUC_CLI_SRCS) $(ID_STACKTRACE) $(IDF_SRCS) \
		   	 	  $(IDUS_SRCS)

