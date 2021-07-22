# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11299 2010-06-21 01:19:28Z djin $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk


OBJ_TARGETS      = acp_objs
SHOBJ_TARGETS    = acp_shobjs

acp_objs_SRCS    = $(ACP_SRCS) $(ACP_ASM_SRCS) $(ACP_GASM_SRCS)
acp_shobjs_SRCS  = $(ACP_SRCS) $(ACP_ASM_SRCS) $(ACP_GASM_SRCS)

INCLUDES        += $(INC_OPT).
DEFINES         += ACP_CFG_DL_EXPORT
ALINT_IGNORES    = acpCStrDouble.c


include $(MAKEFILES_DIR)/rule.mk
