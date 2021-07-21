# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 1296 2007-11-29 04:36:41Z shsuh $
#

include ../../makefiles/root.mk
include $(CORE_DIR)/core.mk


EXEC_TARGETS       = calc

calc_NAME          = calc
calc_SRCS          = calc.c calcParse.c calcLex.c
calc_LIBS          = $(CORE_LIBS)


GENERATED_SRCS    += calcParse.c calcLex.c
GENERATED_HDR_DIR += $(CORE_DIR)/sample/include
INCLUDES          += $(CORE_DIR)/sample/include


include $(MAKEFILES_DIR)/rule.mk
