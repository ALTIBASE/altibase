# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11299 2010-06-21 01:19:28Z djin $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk


EXEC_TARGETS       = genmsg


genmsg_NAME        = genmsg
genmsg_SRCS        = $(GENMSG_SRCS)
genmsg_LIBS        = $(CORE_LIBS)


GENERATED_SRCS     = $(GENMSG_GSRCS)
GENERATED_HDR_DIR  = $(CORE_DIR)/include

DEFINES           += ACP_CFG_DL_STATIC

ALINT_IGNORES = genmsgCode.c

post_all: $(GENMSG)

include $(MAKEFILES_DIR)/rule.mk
