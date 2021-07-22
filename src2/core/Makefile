# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11299 2010-06-21 01:19:28Z djin $
#

include ../../makefiles/root.mk
include core.mk

SUBDIRS = $(CORE_SRCDIRS) $(CTEST_SRCDIRS) lib genmsg util

ifeq ($(DO_UNITTEST), yes)
SUBDIRS += test aci/test
endif

# Set up prerequisites for parallel build.
subdir_lib_build: $(patsubst %,subdir_%_build,$(CORE_SRCDIRS) $(CTEST_SRCDIRS))
subdir_genmsg_build subdir_util_build subdir_test_build subdir_aci/test_build: \
	subdir_lib_build

clean:
	-$(RM) -f test/msg/msgEN128.c test/msg/testMsg.c test/msg/msgKO128.c \
		test/msg/testMsg.mk \
		$(ALTI_HOME)/bin/genmsg \
		$(ALTI_HOME)/bin/utilUGuard \
		$(ALTI_HOME)/bin/utilCoreVersion \
		$(ALTI_HOME)/bin/utilUserName

include $(MAKEFILES_DIR)/rule.mk
