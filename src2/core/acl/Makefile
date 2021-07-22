# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11299 2010-06-21 01:19:28Z djin $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk

OBJ_TARGETS        = acl_objs
SHOBJ_TARGETS      = acl_shobjs

acl_objs_SRCS      = $(ACL_SRCS)
acl_shobjs_SRCS    = $(ACL_SRCS)


GENERATED_SRCS     = $(ACL_GSRCS)
GENERATED_HDR_DIR  = $(CORE_DIR)/include

DEFINES           += ACP_CFG_DL_EXPORT

ALINT_SILENCES    += readline.h aclCompression.h lz4.h aclLZ4.h 

ALINT_IGNORES     += aclCompression.c lz4.c aclLZ4.c

# 
# HERE : External libs not developed by Altibase Data Stream Dev.
#

ifeq ($(READLINE_SUPPORT),yes)
INCLUDES += $(INC_OPT)$(CORE_DIR)/acl/libedit/include
DEFINES  += ACL_READLINE_SUPPORT
endif

INCLUDES += $(INC_OPT)$(CORE_EXT_DIR)/include

include $(MAKEFILES_DIR)/rule.mk
