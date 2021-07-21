# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 1238 2007-11-23 08:18:31Z shsuh $
#

include ../../../../../../makefiles/root.mk

include $(CORE_DIR)/core.mk

ifeq ($(ALTI_CFG_OS),LINUX)

OBJ_TARGETS        = zookeeper_objs
SHOBJ_TARGETS      = zookeeper_shobjs

zookeeper_objs_SRCS      = $(ZOOKEEPER_SRCS)
zookeeper_shobjs_SRCS    = $(ZOOKEEPER_SRCS)

DEFINES           += ACP_CFG_DL_EXPORT
INCLUDES          += $(INC_OPT)$(CORE_DIR)/include $(INC_OPT)$(EXTERNAL_LIB_DIR)/include

CC_FLAGS += -DTHREADED
ALINT_IGNORES += $(EXTERNAL_LIB_SRCS)

endif

include $(MAKEFILES_DIR)/rule.mk
