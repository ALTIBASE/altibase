# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 1238 2007-11-23 08:18:31Z shsuh $
#

include ../../../../../makefiles/root.mk

include $(CORE_DIR)/core.mk

OBJ_TARGETS        = libedit_objs
SHOBJ_TARGETS      = libedit_shobjs

libedit_objs_SRCS      = $(LIBEDIT_SRCS)
libedit_shobjs_SRCS    = $(LIBEDIT_SRCS)

DEFINES           += ACP_CFG_DL_EXPORT
INCLUDES          += $(INC_OPT)$(CORE_DIR)/include $(INC_OPT)$(LIBEDIT_DIR)/include

CC_FLAGS += -w
ALINT_IGNORES += $(LIBEDIT_SRCS)

include $(MAKEFILES_DIR)/rule.mk
