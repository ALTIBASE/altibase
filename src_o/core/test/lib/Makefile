# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11322 2010-06-23 05:38:37Z djin $
#

include ../../../../makefiles/root.mk
include $(CORE_DIR)/core.mk


SHOBJ_TARGETS              = libtestAcpDl_shobj
SHLIB_TARGETS              = libtestAcpDl_shlib

libtestAcpDl_shobj_SRCS    = libtestAcpDl.c
libtestAcpDl_shobj_DEFINES = ACP_CFG_DL_EXPORT

libtestAcpDl_shlib_NAME    = testAcpDl
libtestAcpDl_shlib_OBJS    = $(libtestAcpDl_shobj_SRCS:.c=$(SHOBJ_SUF))


ifeq ($(ALTI_CFG_OS),WINDOWS)
SO_LIBS                   += alticore_pic_static.lib altictest_pic_static.lib
endif

ifeq ($(ALTI_CFG_OS),DARWIN)
SO_LIBS                   += $(ALTI_HOME)/lib/libalticore_pic.a
SO_LIBS                   += $(ALTI_HOME)/lib/libaltictest_pic.a
endif

ifeq ($(ALTI_CFG_OS),AIX)
SO_LIBS                   += $(ALTI_HOME)/lib/libalticore_pic.a
SO_LIBS                   += $(ALTI_HOME)/lib/libaltictest_pic.a
endif

ifeq ($(ALTI_CFG_OS),HPUX)
SO_LIBS                   += $(ALTI_HOME)/lib/libalticore_pic.a
SO_LIBS                   += $(ALTI_HOME)/lib/libaltictest_pic.a
endif

include $(MAKEFILES_DIR)/rule.mk
