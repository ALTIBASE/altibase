#
# $Id$
#

DKI_SRCS = $(DK_DIR)/dki/dki.cpp

DKIV_SRCS = $(DK_DIR)/dki/dkiv/dkiv.cpp		\
            $(DK_DIR)/dki/dkiv/dkivDblinkAltiLinkerStatus.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkDatabaseLinkInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkLinkerSessionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkLinkerControlSessionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkLinkerDataSessionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkGlobalTransactionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkRemoteTransactionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkNotifierTransactionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkShardNotifierTransactionInfo.cpp \
            $(DK_DIR)/dki/dkiv/dkivDblinkRemoteStatementInfo.cpp

DKIF_SRCS = $(DK_DIR)/dki/dkif/dkif.cpp \
            $(DK_DIR)/dki/dkif/dkifUtil.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteAllocStatement.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteExecuteStatement.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteExecuteImmediateInternal.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteBindVariable.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteFreeStatement.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteNextRow.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueChar.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueVarchar.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueInteger.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueSmallint.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueBigint.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueDouble.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueReal.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueFloat.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetColumnValueDate.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetErrorInfo.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteAllocStatementBatch.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteFreeStatementBatch.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteBindVariableBatch.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteAddBatch.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteExecuteBatch.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetResultCountBatch.cpp \
            $(DK_DIR)/dki/dkif/dkifRemoteGetResultBatch.cpp

DKIQ_SRCS = $(DK_DIR)/dki/dkiq/dkiq.cpp \
            $(DK_DIR)/dki/dkiq/dkiqDatabaseLinkCallback.cpp \
            $(DK_DIR)/dki/dkiq/dkiqRemoteTableCallback.cpp

DKIS_SRCS = $(DK_DIR)/dki/dkis/dkis.cpp \
            $(DK_DIR)/dki/dkis/dkis2PCCallback.cpp \
            $(DK_DIR)/dki/dkis/dkisIndexModule.cpp \
            $(DK_DIR)/dki/dkis/dkisRemoteQuery.cpp

DKU_SRCS = $(DK_DIR)/dku/dkuTraceCode.cpp	\
           $(DK_DIR)/dku/dkuProperty.cpp	

DKM_SRCS = $(DK_DIR)/dkm/dkm.cpp

DKC_SRCS = $(DK_DIR)/dkc/dkc.cpp	\
           $(DK_DIR)/dkc/dkcUtil.cpp	\
           $(DK_DIR)/dkc/dkcParser.cpp  \
           $(DK_DIR)/dkc/dkcLexer.cpp

DKA_SRCS = $(DK_DIR)/dka/dkaLinkerProcessMgr.cpp

DKD_SRCS = $(DK_DIR)/dkd/dkdDataBufferMgr.cpp \
           $(DK_DIR)/dkd/dkdDataMgr.cpp \
           $(DK_DIR)/dkd/dkdRemoteTableMgr.cpp \
           $(DK_DIR)/dkd/dkdDiskTempTableMgr.cpp \
           $(DK_DIR)/dkd/dkdRecordBufferMgr.cpp \
           $(DK_DIR)/dkd/dkdDiskTempTable.cpp \
           $(DK_DIR)/dkd/dkdTypeConverter.cpp \
           $(DK_DIR)/dkd/dkdResultSetMetaCache.cpp \
           $(DK_DIR)/dkd/dkdMisc.cpp

DKO_SRCS = $(DK_DIR)/dko/dkoLinkObjMgr.cpp \
           $(DK_DIR)/dko/dkoLinkInfo.cpp

DKP_SRCS = $(DK_DIR)/dkp/dkpProtocolMgr.cpp \
           $(DK_DIR)/dkp/dkpBatchStatementMgr.cpp

DKS_SRCS = $(DK_DIR)/dks/dksSessionMgr.cpp

DKT_SRCS = $(DK_DIR)/dkt/dktDtxInfo.cpp \
           $(DK_DIR)/dkt/dktNotifier.cpp \
           $(DK_DIR)/dkt/dktGlobalTxMgr.cpp \
           $(DK_DIR)/dkt/dktGlobalCoordinator.cpp \
           $(DK_DIR)/dkt/dktRemoteTx.cpp \
           $(DK_DIR)/dkt/dktRemoteStmt.cpp

DKN_SRCS = $(DK_DIR)/dkn/dknLink.cpp

DKI_STUB_SRCS = $(DK_DIR)/dki/dkistub.cpp

DK_SRCS  = $(DKI_SRCS) $(DKIV_SRCS) $(DKIF_SRCS) $(DKU_SRCS) $(DKM_SRCS) \
           $(DKIQ_SRCS) $(DKD_SRCS) $(DKC_SRCS) $(DKA_SRCS) $(DKO_SRCS) \
           $(DKP_SRCS) $(DKS_SRCS) $(DKT_SRCS) $(DKIS_SRCS) $(DKN_SRCS)

DK_OBJS        = $(DK_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
DK_SHOBJS      = $(DK_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))

DK_STUB_OBJS   = $(DKI_STUB_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))
DK_STUB_SHOBJS = $(DKI_STUB_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT))
