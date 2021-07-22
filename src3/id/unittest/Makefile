####################################################
#   Unit Test Makefile
#
#   Usage :
#       Just add the name of Unit Test Souce Code
####################################################

BLD_DIR = id/unittest

include $(ALTIDEV_HOME)/unittest.mk

ifeq   ($(OS_TARGET), SPARC_SOLARIS)
# ignore multiple function definitions
LD_FLAGS   += -z muldefs
endif #($(OS_TARGET), SPARC_SOLARIS)

# UNITTEST_TARGETS =  iduMemMgr iduMemory iduVarMemList idSample idlFile idlStr iduMemClientInfo idp iduCond
UNITTEST_TARGETS = iduMemMgr ideMsgLog ideLogEntry iduMemListOld
PERFTEST_TARGETS = perfIduMemMgr

idSample_NAME = unittestIdSample
idSample_SRCS = unittestIdSample.cpp
idSample_SHLIBS = alticore altictest  pd_so

idlFile_NAME = unittestIdFile
idlFile_SRCS = unittestIdFile.cpp
idlFile_SHLIBS = alticore altictest  pd_so

idlStr_NAME = unittestIdStr
idlStr_SRCS = unittestIdStr.cpp
idlStr_SHLIBS = alticore altictest  pd_so

iduMemory_NAME = unittestIduMemory
iduMemory_SRCS = unittestIduMemory.cpp
iduMemory_SHLIBS = alticore altictest  pd_so
iduMemory_LIBS = id

iduVarMemList_NAME = unittestIduVarMemList
iduVarMemList_SRCS = unittestIduVarMemList.cpp
iduVarMemList_SHLIBS = alticore altictest  pd_so
iduVarMemList_LIBS = id

iduMemListOld_NAME = unittestIduMemListOld
iduMemListOld_SRCS = unittestIduMemListOld.cpp
iduMemListOld_SHLIBS = alticore altictest  pd_so
iduMemListOld_LIBS = id

iduMemMgr_NAME = unittestIduMemMgr
iduMemMgr_SRCS = unittestIduMemMgr.cpp
iduMemMgr_SHLIBS = alticore altictest  pd_so
iduMemMgr_LIBS = id

iduCond_NAME = unittestIduCond
iduCond_SRCS = unittestIduCond.cpp
iduCond_SHLIBS = alticore altictest  pd_so
iduCond_LIBS = id

iduLatch_NAME = unittestIduLatch
iduLatch_SRCS = unittestIduLatch.cpp
iduLatch_SHLIBS = alticore altictest  pd_so
iduLatch_LIBS = id

iduMemClientInfo_NAME = unittestIduMemClientInfo
iduMemClientInfo_SRCS = unittestIduMemClientInfo.cpp
iduMemClientInfo_SHLIBS = alticore altictest id_so pd_so
iduMemClientInfo_LIBS = id

idp_NAME = unittestIdp
idp_SRCS = unittestIdp.cpp
idp_SHLIBS = alticore altictest pd_so
idp_LIBS = id

ideMsgLog_NAME   := unittestIdeMsgLog
ideMsgLog_SHLIBS := alticore altictest pd_so
ideMsgLog_LIBS   := id
ifneq ($(IDE_MSGLOG_SIMULATE_STARVATION),)
# The reason for including ../ide/ideMsgLog.cpp here is to simulate
# threads suffering from random starvation during test.
VPATH            := ../ide/
COMP_FLAGS       += -DIDE_MSGLOG_SIMULATE_STARVATION
ifneq ($(VERBOSE_MODE),)
COMP_FLAGS       += -DIDE_MSGLOG_UNITTEST_VERBOSE
endif # VERBOSE_MODE
ideMsgLog_SRCS   := unittestIdeMsgLog.cpp ideMsgLog.cpp
else # IDE_MSGLOG_SIMULATE_STARVATION
ideMsgLog_SRCS   := unittestIdeMsgLog.cpp
endif

# ideLogEntry
ideLogEntry_NAME   := unittestIdeLogEntry
ideLogEntry_SRCS   := unittestIdeLogEntry.cpp
ideLogEntry_SHLIBS := alticore altictest pd_so
ideLogEntry_LIBS   := id

# activate only for performance test
perfIduMemMgr_NAME = unittestPerfIduMemMgr
perfIduMemMgr_SRCS = unittestPerfIduMemMgr.cpp
perfIduMemMgr_SHLIBS = alticore altictest  pd_so
perfIduMemMgr_LIBS = id

# Run unittests one by one.
.NOTPARALLEL: unittest_content

include $(MAKEFILES_DIR)/rule.mk
