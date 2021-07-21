ULA_API_SRCS     = $(UL_DIR)/ula/ulaAPI.c

ULA_SRCS         = $(UL_DIR)/ula/ulaComm.c                \
                   $(UL_DIR)/ula/ulaErrorMgr.c            \
                   $(UL_DIR)/ula/ulaLog.c                 \
                   $(UL_DIR)/ula/ulaMeta.c                \
                   $(UL_DIR)/ula/ulaTraceCode.c           \
                   $(UL_DIR)/ula/ulaTransTbl.c            \
                   $(UL_DIR)/ula/ulaXLogCollector.c       \
                   $(UL_DIR)/ula/ulaXLogLinkedList.c      \
                   $(UL_DIR)/ulc/ulcs/ulcsCallback.c      \
                   $(UL_DIR)/ula/ulaConvFmMT.c            \
                   $(UL_DIR)/ula/ulaConvNumeric.c

ULN_SRCS        = $(UL_DIR)/uln/ulnPrepare.c             \
                  $(UL_DIR)/uln/ulnURL.c                 \
                  $(UL_DIR)/uln/ulnParse.c               \
                  $(UL_DIR)/uln/ulnConnString.c          \
                  $(UL_DIR)/uln/ulnConnAttribute.c       \
                  $(UL_DIR)/uln/ulnDbc.c                 \
                  $(UL_DIR)/uln/ulnSetConnectAttr.c      \
                  $(UL_DIR)/uln/ulnGetConnectAttr.c      \
                  $(UL_DIR)/uln/ulnConnectCore.c         \
                  $(UL_DIR)/uln/ulnDriverConnect.c       \
                  $(UL_DIR)/uln/ulnConnect.c             \
                  $(UL_DIR)/uln/ulnDisconnect.c          \
                  $(UL_DIR)/uln/ulnConv.c                \
                  $(UL_DIR)/uln/ulnConvBit.c             \
                  $(UL_DIR)/uln/ulnConvChar.c            \
                  $(UL_DIR)/uln/ulnConvWChar.c           \
                  $(UL_DIR)/uln/ulnConvNumeric.c         \
                  $(UL_DIR)/uln/ulnConvToCHAR.c          \
                  $(UL_DIR)/uln/ulnConvToWCHAR.c         \
                  $(UL_DIR)/uln/ulnConvToULONG.c         \
                  $(UL_DIR)/uln/ulnConvToSLONG.c         \
                  $(UL_DIR)/uln/ulnConvToUSHORT.c        \
                  $(UL_DIR)/uln/ulnConvToSSHORT.c        \
                  $(UL_DIR)/uln/ulnConvToUTINYINT.c      \
                  $(UL_DIR)/uln/ulnConvToSTINYINT.c      \
                  $(UL_DIR)/uln/ulnConvToUBIGINT.c       \
                  $(UL_DIR)/uln/ulnConvToSBIGINT.c       \
                  $(UL_DIR)/uln/ulnConvToBIT.c           \
                  $(UL_DIR)/uln/ulnConvToFLOAT.c         \
                  $(UL_DIR)/uln/ulnConvToDOUBLE.c        \
                  $(UL_DIR)/uln/ulnConvToBINARY.c        \
                  $(UL_DIR)/uln/ulnConvToDATE.c          \
                  $(UL_DIR)/uln/ulnConvToTIME.c          \
                  $(UL_DIR)/uln/ulnConvToTIMESTAMP.c     \
                  $(UL_DIR)/uln/ulnConvToNUMERIC.c       \
                  $(UL_DIR)/uln/ulnConvToLOCATOR.c       \
                  $(UL_DIR)/uln/ulnConvToFILE.c          \
                  $(UL_DIR)/uln/ulnColAttribute.c        \
                  $(UL_DIR)/uln/ulnGetPlan.c             \
                  $(UL_DIR)/uln/ulnCloseCursor.c         \
                  $(UL_DIR)/uln/ulnCache.c               \
                  $(UL_DIR)/uln/ulnBindCommon.c          \
                  $(UL_DIR)/uln/ulnBindConvSize.c        \
                  $(UL_DIR)/uln/ulnBindData.c            \
                  $(UL_DIR)/uln/ulnBindParamDataInBuildAny.c     \
                  $(UL_DIR)/uln/ulnBindInfo.c            \
                  $(UL_DIR)/uln/ulnBindInfoResult.c      \
                  $(UL_DIR)/uln/ulnTypes.c               \
                  $(UL_DIR)/uln/ulnData.c                \
                  $(UL_DIR)/uln/ulnMeta.c                \
                  $(UL_DIR)/uln/ulnCommunication.c       \
                  $(UL_DIR)/uln/ulnServerMessage.c       \
                  $(UL_DIR)/uln/ulnNumResultCols.c       \
                  $(UL_DIR)/uln/ulnNumParams.c           \
                  $(UL_DIR)/uln/ulnAllocHandle.c         \
                  $(UL_DIR)/uln/ulnBindCol.c             \
                  $(UL_DIR)/uln/ulnBindParameter.c       \
                  $(UL_DIR)/uln/ulnDesc.c                \
                  $(UL_DIR)/uln/ulnDescRec.c             \
                  $(UL_DIR)/uln/ulnDiagnostic.c          \
                  $(UL_DIR)/uln/ulnEnv.c                 \
                  $(UL_DIR)/uln/ulnError.c               \
                  $(UL_DIR)/uln/ulnErrorDef.c            \
                  $(UL_DIR)/uln/ulnFreeHandle.c          \
                  $(UL_DIR)/uln/ulnFreeStmt.c            \
                  $(UL_DIR)/uln/ulnGetDiagField.c        \
                  $(UL_DIR)/uln/ulnGetDiagRec.c          \
                  $(UL_DIR)/uln/ulnInit.c                \
                  $(UL_DIR)/uln/ulnObject.c              \
                  $(UL_DIR)/uln/ulnSetDescRec.c          \
                  $(UL_DIR)/uln/ulnGetDescRec.c          \
                  $(UL_DIR)/uln/ulnSetDescField.c        \
                  $(UL_DIR)/uln/ulnGetDescField.c        \
                  $(UL_DIR)/uln/ulnGetEnvAttr.c          \
                  $(UL_DIR)/uln/ulnSetEnvAttr.c          \
                  $(UL_DIR)/uln/ulnStateMachine.c        \
                  $(UL_DIR)/uln/ulnStmt.c                \
                  $(UL_DIR)/uln/ulnEscape.c              \
                  $(UL_DIR)/uln/ulnCharSet.c             \
                  $(UL_DIR)/uln/ulnDescribeCol.c         \
                  $(UL_DIR)/uln/ulnDescribeParam.c       \
                  $(UL_DIR)/uln/ulnExecute.c             \
                  $(UL_DIR)/uln/ulnCursor.c              \
                  $(UL_DIR)/uln/ulnFetch.c               \
                  $(UL_DIR)/uln/ulnFetchOp.c             \
                  $(UL_DIR)/uln/ulnFetchScroll.c         \
                  $(UL_DIR)/uln/ulnFetchResult.c         \
                  $(UL_DIR)/uln/ulnFetchCore.c           \
                  $(UL_DIR)/uln/ulnExtendedFetch.c       \
                  $(UL_DIR)/uln/ulnExecDirect.c          \
                  $(UL_DIR)/uln/ulnRowCount.c            \
                  $(UL_DIR)/uln/ulnSetStmtAttr.c         \
                  $(UL_DIR)/uln/ulnGetStmtAttr.c         \
                  $(UL_DIR)/uln/ulnGetFunctions.c        \
                  $(UL_DIR)/uln/ulnGetTypeInfo.c         \
                  $(UL_DIR)/uln/ulnCatalogFunctions.c    \
                  $(UL_DIR)/uln/ulnTables.c              \
                  $(UL_DIR)/uln/ulnColumns.c             \
                  $(UL_DIR)/uln/ulnStatistics.c          \
                  $(UL_DIR)/uln/ulnPrimaryKeys.c         \
                  $(UL_DIR)/uln/ulnProcedureColumns.c    \
                  $(UL_DIR)/uln/ulnProcedures.c          \
                  $(UL_DIR)/uln/ulnSpecialColumns.c      \
                  $(UL_DIR)/uln/ulnForeignKeys.c         \
                  $(UL_DIR)/uln/ulnTablePrivileges.c     \
                  $(UL_DIR)/uln/ulnEndTran.c             \
                  $(UL_DIR)/uln/ulnGetInfo.c             \
                  $(UL_DIR)/uln/ulnLob.c                 \
                  $(UL_DIR)/uln/ulnGetLobLength.c        \
                  $(UL_DIR)/uln/ulnGetLob.c              \
                  $(UL_DIR)/uln/ulnPutLob.c              \
                  $(UL_DIR)/uln/ulnFreeLob.c             \
                  $(UL_DIR)/uln/ulnPutData.c             \
                  $(UL_DIR)/uln/ulnPDContext.c           \
                  $(UL_DIR)/uln/ulnParamData.c           \
                  $(UL_DIR)/uln/ulnBindFileToParam.c     \
                  $(UL_DIR)/uln/ulnBindFileToCol.c       \
                  $(UL_DIR)/uln/ulnGetData.c             \
                  $(UL_DIR)/uln/ulnSetPos.c              \
                  $(UL_DIR)/uln/ulnBulkOperations.c      \
                  $(UL_DIR)/uln/ulnTrace.c               \
                  $(UL_DIR)/uln/ulnSetScrollOptions.c    \
                  $(UL_DIR)/uln/ulnMoreResults.c         \
                  $(UL_DIR)/uln/ulnNativeSql.c           \
                  $(UL_DIR)/uln/ulnDebug.c               \
                  $(UL_DIR)/uln/ulnFailOver.c            \
                  $(UL_DIR)/uln/ulnDataSource.c          \
                  $(UL_DIR)/uln/ulnConfigFile.c          \
                  $(UL_DIR)/uln/ulnTraceLog.c            \
                  $(UL_DIR)/uln/ulnString.c              \
                  $(UL_DIR)/uln/ulnCancel.c              \
                  $(UL_DIR)/uln/ulnKeyset.c              \
                  $(UL_DIR)/uln/ulnAnalyzeStmt.c         \
                  $(UL_DIR)/uln/ulnLobCache.c            \
                  $(UL_DIR)/uln/ulnProperties.c	         \
                  $(UL_DIR)/uln/ulnTrimLob.c             \
                  $(UL_DIR)/uln/ulnSemiAsyncPrefetch.c   \
                  $(UL_DIR)/uln/ulnFetchOpAsync.c        \
                  $(UL_DIR)/uln/ulnShard.c	               \
                  $(UL_DIR)/uln/ulnSavepoint.c

ULNW_SRCS		= $(UL_DIR)/ulnw/ulnwCPoolAllocHandle.c	 \
				  $(UL_DIR)/ulnw/ulnwCPoolFreeHandle.c   \
				  $(UL_DIR)/ulnw/ulnwCPoolGetAttr.c      \
				  $(UL_DIR)/ulnw/ulnwCPoolSetAttr.c      \
				  $(UL_DIR)/ulnw/ulnwCPoolInitialize.c   \
				  $(UL_DIR)/ulnw/ulnwCPoolFinalize.c     \
				  $(UL_DIR)/ulnw/ulnwCPoolGetConnection.c \
				  $(UL_DIR)/ulnw/ulnwCPoolReturnConnection.c \
				  $(UL_DIR)/ulnw/ulnwCPoolManageConnections.c \
				  $(UL_DIR)/ulnw/ulnwTrace.c

ULS_SRCS        = $(UL_DIR)/uls/ulsCreateObject.c        \
                  $(UL_DIR)/uls/ulsDateFunc.c            \
                  $(UL_DIR)/uls/ulsSearchObject.c            \
                  $(UL_DIR)/uls/ulsUtil.c            \
                  $(UL_DIR)/uls/ulsEnvHandle.c           \
                  $(UL_DIR)/uls/ulsError.c               \
                  $(UL_DIR)/uls/ulsByteOrder.c

ULU_SRCS        = $(UL_DIR)/ulu/uluMemory.c              \
                  $(UL_DIR)/ulu/uluArray.c               \
                  $(UL_DIR)/ulu/uluLock.c

ULX_SRCS        = $(UL_DIR)/ulx/ulxXaConnection.c        \
                  $(UL_DIR)/ulx/ulxXaFunction.c          \
                  $(UL_DIR)/ulx/ulxXaInterface.c         \
                  $(UL_DIR)/ulx/ulxXaProtocol.c          \
                  $(UL_DIR)/ulx/ulxMsgLog.c

ULSDN_SRCS      = $(UL_DIR)/ulsd/ulsdnExecute.c         \
                  $(UL_DIR)/ulsd/ulsdnDbc.c             \
                  $(UL_DIR)/ulsd/ulsdnDescribeCol.c     \
                  $(UL_DIR)/ulsd/ulsdnBindData.c        \
                  $(UL_DIR)/ulsd/ulsdnTrans.c           \
                  $(UL_DIR)/ulsd/ulsdnStmt.c            \
                  $(UL_DIR)/ulsd/ulsdnFailover.c        \
                  $(UL_DIR)/ulsd/ulsdnLob.c             \
                  $(UL_DIR)/ulsd/ulsdnDistTxInfo.c

ULSD_SRC        = $(UL_DIR)/ulsd/ulsdError.c            \
                  $(UL_DIR)/ulsd/ulsdCommunication.c    \
                  $(UL_DIR)/ulsd/ulsdnEx.c              \
                  $(UL_DIR)/ulsd/ulsdNode.c             \
                  $(UL_DIR)/ulsd/ulsdTrans.c            \
                  $(UL_DIR)/ulsd/ulsdDisconnect.c       \
                  $(UL_DIR)/ulsd/ulsdData.c             \
                  $(UL_DIR)/ulsd/ulsdCursor.c           \
                  $(UL_DIR)/ulsd/ulsdConnect.c          \
                  $(UL_DIR)/ulsd/ulsdShard.c            \
                  $(UL_DIR)/ulsd/ulsdStatement.c        \
                  $(UL_DIR)/ulsd/ulsdPrepare.c          \
                  $(UL_DIR)/ulsd/ulsdStmtAttr.c         \
                  $(UL_DIR)/ulsd/ulsdConnectAttr.c      \
                  $(UL_DIR)/ulsd/ulsdLob.c              \
                  $(UL_DIR)/ulsd/ulsdModule.c           \
                  $(UL_DIR)/ulsd/ulsdModule_COORD.c     \
                  $(UL_DIR)/ulsd/ulsdModule_META.c      \
                  $(UL_DIR)/ulsd/ulsdModule_NODE.c      \
                  $(UL_DIR)/ulsd/ulsdFailover.c         \
                  $(UL_DIR)/ulsd/ulsdExtension.c        \
                  $(UL_DIR)/ulsd/ulsdRebuild.c          \
                  $(UL_DIR)/ulsd/ulsdUtils.c

# ULSD를 기반으로 ULN에서도 사용하는 함수들
ULSD_EXT_SRCS = $(UL_DIR)/ulsd/ulsdDistTxInfo.c

ULO_SRC_SQL_WRAPPER       = $(UL_DIR)/ulc/ulcSqlWrapper.c

ULO_SRCS_CLI    = $(ULO_SRC_SQL_WRAPPER)                \
                  $(UL_DIR)/ulc/ulcs/ulcsCallback.c     \
                  $(UL_DIR)/ulc/ulcs/ulcsSqlExtension.c \
                  $(UL_DIR)/ulc/ulcSqlWrapperForSd.c

ULSD_SRC_SQL_WRAPPER      = $(UL_DIR)/ulsd/ulsdSqlWrapper.c          \
                            $(UL_DIR)/ulsd/ulsdSqlExtensionWrapper.c

ULO_SRCS_CLI_FOR_SD	= $(ULSD_SRC_SQL_WRAPPER)               \
                      $(UL_DIR)/ulc/ulcs/ulcsCallback.c     \
                      $(UL_DIR)/ulc/ulcSqlWrapperForSd.c

ULO_SRCS_ACS    = $(UL_DIR)/ulc/ulcAcsWrapper.c

ULO_SRCS_UNIX_ODBC  = $(ULO_SRC_SQL_WRAPPER) \
                     $(UL_DIR)/ulc/ulco/ulcox/ulcoxCallback.c

UL_SRCS_CLI         = $(ULN_SRCS) $(ULU_SRCS) $(ULO_SRCS_CLI) $(ULX_SRCS) $(MT_C_CLIENT_SRCS) $(ULNW_SRCS) $(ULSDN_SRCS) $(ULSD_EXT_SRCS)
UL_SRCS_SD_CLI      = $(ULN_SRCS) $(ULU_SRCS) $(ULO_SRCS_CLI_FOR_SD) $(ULX_SRCS) $(MT_C_CLIENT_SRCS) $(ULNW_SRCS) $(ULSDN_SRCS) $(ULSD_SRC) $(ULSD_EXT_SRCS)
UL_SRCS_UNIX_ODBC   = $(ULN_SRCS) $(ULU_SRCS) $(ULO_SRCS_UNIX_ODBC) $(MT_C_CLIENT_SRCS) $(ULX_SRCS) $(ULNW_SRCS) $(ULSDN_SRCS) $(ULSD_EXT_SRCS)

UL_SRCS_ACS         = $(ULS_SRCS) $(ULO_SRCS_ACS)
UL_SRCS_ULA         = $(MT_C_CLIENT_SRCS) $(ULN_SRCS) $(ULU_SRCS) $(ULS_SRCS) $(ULSDN_SRCS) $(ULA_API_SRCS) $(ULA_SRCS) $(ULX_SRCS) $(ULSD_EXT_SRCS)

#
# 최종적으로 생산되어야 하는 라이브러리들의 파일명들
#
SHARDCLI_LIB_PATH      = $(ALTI_HOME)/lib/$(LIBPRE)shardcli.$(LIBEXT)
SHARDCLI_SHLIB_PATH    = $(ALTI_HOME)/lib/$(LIBPRE)shardcli_sl.$(SOEXT)
ODBCCLI_LIB_PATH       = $(ALTI_HOME)/lib/$(LIBPRE)odbccli.$(LIBEXT)
ODBCCLI_SHLIB_PATH     = $(ALTI_HOME)/lib/$(LIBPRE)odbccli_sl.$(SOEXT)
ALA_LIB_PATH           = $(ALTI_HOME)/lib/$(LIBPRE)ala.$(LIBEXT)
ALA_SHLIB_PATH         = $(ALTI_HOME)/lib/$(LIBPRE)ala_sl.$(SOEXT)

# Fix BUG-22936 
UNIX_ODBC64_SHLIB_PATH   = $(ALTI_HOME)/lib/$(LIBPRE)altibase_odbc-64bit-ul64.$(SOEXT)
ifeq ($(compile64),1)
UNIX_ODBC_SHLIB_PATH   = $(ALTI_HOME)/lib/$(LIBPRE)altibase_odbc-64bit-ul32.$(SOEXT)
else
UNIX_ODBC_SHLIB_PATH   = $(ALTI_HOME)/lib/$(LIBPRE)altibase_odbc.$(SOEXT)
endif

ACS_LIB_PATH       = $(ALTI_HOME)/lib/$(LIBPRE)acs.$(LIBEXT)
ACS_SHLIB_PATH     = $(ALTI_HOME)/lib/$(LIBPRE)acs_sl.$(SOEXT)

#####################
# Source file Listing
#####################

# LIBRARY Source File Specify
BASE_SRCS       = $(CM_CLIEINT_SRCS)

ODBCCLI_SRCS    = $(BASE_SRCS) $(UL_SRCS_CLI)
SHARDCLI_SRCS   = $(BASE_SRCS) $(UL_SRCS_SD_CLI)
ALA_SRCS        = $(BASE_SRCS) $(UL_SRCS_ULA)
UNIX_ODBC_SRCS  = $(BASE_SRCS) $(UL_SRCS_UNIX_ODBC)

ACS_SRCS        = $(UL_SRCS_ACS)

#####################
# Object File Listing
#####################

ODBCCLI_OBJS    = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_aoc.$(OBJEXT)     ,$(basename $(ODBCCLI_SRCS)))
ODBCCLI_SHOBJS  = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_soc.$(OBJEXT)     ,$(basename $(ODBCCLI_SRCS)))
SHARDCLI_OBJS   = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_sd_aoc.$(OBJEXT)  ,$(basename $(SHARDCLI_SRCS)))
SHARDCLI_SHOBJS = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_sd_soc.$(OBJEXT)  ,$(basename $(SHARDCLI_SRCS)))
#BUG-22936
UNIX_ODBC64_OBJS = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_64.$(OBJEXT)     ,$(basename $(UNIX_ODBC_SRCS)))
UNIX_ODBC_OBJS  = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_soc.$(OBJEXT)     ,$(basename $(patsubst $(UL_DIR)/ulc/ulcSqlWrapper.c,,$(patsubst $(UL_DIR)/uln/ulnSetConnectAttr.c,,$(UNIX_ODBC_SRCS))))) \
                  $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_unixodbc.$(OBJEXT),$(basename $(UL_DIR)/uln/ulnSetConnectAttr.c $(UL_DIR)/ulc/ulcSqlWrapper.c))

ACS_OBJS   = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_aoc.$(OBJEXT),$(basename $(ACS_SRCS)))
ACS_SHOBJS = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_soc.$(OBJEXT),$(basename $(ACS_SRCS)))
ALA_OBJS   = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_aoc.$(OBJEXT),$(basename $(ALA_SRCS)))
ALA_SHOBJS = $(patsubst $(DEV_DIR)/%,$(TARGET_DIR)/%_soc.$(OBJEXT),$(basename $(ALA_SRCS)))

###########################################
# Indirect build 시 생성할 object list 파일
###########################################

OBJLIST=objlist.txt

