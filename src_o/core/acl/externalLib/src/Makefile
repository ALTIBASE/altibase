# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 1238 2007-11-23 08:18:31Z shsuh $
#

include ../../../../../makefiles/root.mk

include $(CORE_DIR)/core.mk

OBJ_TARGETS        = externalLib_objs
SHOBJ_TARGETS      = externalLib_shobjs

externalLib_objs_SRCS      = $(EXTERNAL_LIB_SRCS)
externalLib_shobjs_SRCS    = $(EXTERNAL_LIB_SRCS)

DEFINES           += ACP_CFG_DL_EXPORT
INCLUDES          += $(INC_OPT)$(CORE_DIR)/include $(INC_OPT)$(EXTERNAL_LIB_DIR)/include

CC_FLAGS += -w
ALINT_IGNORES += $(EXTERNAL_LIB_SRCS)

include $(MAKEFILES_DIR)/rule.mk
