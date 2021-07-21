# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 4378 2009-01-30 07:51:52Z sjkim $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk


EXEC_TARGETS       = utilUGuard \
					 utilUserName \
					 utilCoreVersion

UNITGUARD_SRCS     = utilUGuard.c
USERNAME_SRCS	   = utilUserName.c
COREVERSION_SRCS   = utilCoreVersion.c

utilUGuard_NAME        = utilUGuard
utilUGuard_SRCS        = $(UNITGUARD_SRCS)
utilUGuard_LIBS        = $(CORE_LIBS)

utilUserName_NAME      = utilUserName
utilUserName_SRCS      = $(USERNAME_SRCS)
utilUserName_LIBS      = $(CORE_LIBS)

utilCoreVersion_NAME   = utilCoreVersion
utilCoreVersion_SRCS   = $(COREVERSION_SRCS)
utilCoreVersion_LIBS   = $(CORE_LIBS)


DEFINES           += ACP_CFG_DL_STATIC

UNITGUARD = $(BINS_DIR)/utilUGuard$(EXEC_SUF)

UTILUSERNAME = $(BINS_DIR)/utilUserName$(EXEC_SUF)

COREVERSION = $(BINS_DIR)/utilCoreVersion$(EXEC_SUF)

post_all: $(UNITGUARD) $(UTILUSERNAME) $(COREVERSION)

include $(MAKEFILES_DIR)/rule.mk
