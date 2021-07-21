# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id:
#

include ../../../../makefiles/root.mk
include $(CORE_DIR)/core.mk

ifeq ($(ALTI_CFG_OS),WINDOWS)
unittest:$(BUILD_DIR)/dbghelp.dll

$(BUILD_DIR)/dbghelp.dll:
	@$(ECHO) "Copying debug DLLs"
	@$(CP) $(CORE_DIR)/utils/windows/dbghelp$(COMPILE_BIT).dll $(BUILD_DIR)/dbghelp.dll
endif

SUBDIRS                = 

UNITTEST_TARGETS       = aciProperty \
                         aciHashUtil \
                         aciErrorMgr \
                         aciAciConv  \
                         aciVarString \
                         aciVa \
                         aciVersion \
                         aciTypes

PERFTEST_TARGETS       = 


EXEC_TARGETS           =

aciProperty_NAME           = testAciProperty
aciProperty_SRCS           = testAciProperty.c
aciProperty_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)

aciHashUtil_NAME           = testAciHashUtil
aciHashUtil_SRCS           = testAciHashUtil.c
aciHashUtil_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)

aciErrorMgr_NAME           = testAciErrorMgr
aciErrorMgr_SRCS           = testAciErrorMgr.c
aciErrorMgr_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)

aciAciConv_NAME           = testAciConv
aciAciConv_SRCS           = testAciConv.c
aciAciConv_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)

aciVarString_NAME           = testAciVarString
aciVarString_SRCS           = testAciVarString.c
aciVarString_LIBS           = $(CTEST_LIBS) $(CORE_LIBS)

aciVa_NAME                  = testAciVa
aciVa_SRCS                  = testAciVa.c
aciVa_LIBS                  = $(CTEST_LIBS) $(CORE_LIBS)

aciVersion_NAME                  = testAciVersion
aciVersion_SRCS                  = testAciVersion.c
aciVersion_LIBS                  = $(CTEST_LIBS) $(CORE_LIBS)

aciTypes_NAME                  = testAciTypes
aciTypes_SRCS                  = testAciTypes.c
aciTypes_LIBS                  = $(CTEST_LIBS) $(CORE_LIBS)


##############################################################################
# Performance Test Module 
##############################################################################


ALINT_IGNORES         += testAcpPrintf.c
ALINT_IGNORES         += testAclConf.c

define shtest_template
$(1)_SHLIBS = $($(1)_LIBS)
$(1)_LIBS = 
endef

ifneq ($(TEST_SHLIB),)
$(foreach target, $(UNITTEST_TARGETS), $(eval $(call shtest_template,$(target))))
$(foreach target, $(PERFTEST_TARGETS), $(eval $(call shtest_template,$(target))))
DEFINES               += ACP_CFG_DL_IMPORT
else
DEFINES               += ACP_CFG_DL_STATIC
endif


include $(MAKEFILES_DIR)/rule.mk
