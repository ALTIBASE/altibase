# Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
# All rights reserved.

# $Id: Makefile 11315 2010-06-22 09:36:35Z djin $
#

include ../../../makefiles/root.mk
include $(CORE_DIR)/core.mk

LIB_TARGETS         = core_lib ctest_lib alticore_$(ALTICORE_VERSION)_lib altictest_$(ALTICORE_VERSION)_lib alticore_$(ALTICORE_VERSION)_piclib altictest_$(ALTICORE_VERSION)_piclib alticore_lib altictest_lib alticore_piclib altictest_piclib
SHLIB_TARGETS       = core_shlib ctest_shlib alticore_$(ALTICORE_VERSION)_shlib altictest_$(ALTICORE_VERSION)_shlib alticore_shlib altictest_shlib

core_lib_NAME       = $(CORE_LIBS)
core_lib_OBJS       = $(CORE_OBJS)

ctest_lib_NAME      = $(CTEST_LIBS)
ctest_lib_OBJS      = $(CTEST_OBJS)

core_shlib_NAME     = $(CORE_LIBS)
core_shlib_OBJS     = $(CORE_SHOBJS)

ctest_shlib_NAME    = $(CTEST_LIBS)
ctest_shlib_OBJS    = $(CTEST_SHOBJS)
ctest_shlib_SHLIBS  = $(CORE_LIBS)

alticore_$(ALTICORE_VERSION)_lib_NAME       = $(ALTICORE_LIBS)
alticore_$(ALTICORE_VERSION)_lib_OBJS       = $(CORE_OBJS)
altictest_$(ALTICORE_VERSION)_lib_NAME      = $(ALTICTEST_LIBS)
altictest_$(ALTICORE_VERSION)_lib_OBJS      = $(CTEST_OBJS)

alticore_$(ALTICORE_VERSION)_piclib_NAME       = $(ALTICORE_PICLIBS)
alticore_$(ALTICORE_VERSION)_piclib_OBJS       = $(CORE_SHOBJS)
altictest_$(ALTICORE_VERSION)_piclib_NAME      = $(ALTICTEST_PICLIBS)
altictest_$(ALTICORE_VERSION)_piclib_OBJS      = $(CTEST_SHOBJS)

alticore_$(ALTICORE_VERSION)_shlib_NAME     = $(ALTICORE_LIBS)
alticore_$(ALTICORE_VERSION)_shlib_OBJS     = $(CORE_SHOBJS)
altictest_$(ALTICORE_VERSION)_shlib_NAME    = $(ALTICTEST_LIBS)
altictest_$(ALTICORE_VERSION)_shlib_OBJS    = $(CTEST_SHOBJS)
altictest_$(ALTICORE_VERSION)_shlib_SHLIBS  = $(ALTICORE_LIBS)

alticore_lib_NAME       = alticore
alticore_lib_OBJS       = $(CORE_OBJS)
altictest_lib_NAME      = altictest
altictest_lib_OBJS      = $(CTEST_OBJS)

alticore_piclib_NAME       = alticore_pic
alticore_piclib_OBJS       = $(CORE_SHOBJS)
altictest_piclib_NAME      = altictest_pic
altictest_piclib_OBJS      = $(CTEST_SHOBJS)

alticore_shlib_NAME     = alticore
alticore_shlib_OBJS     = $(CORE_SHOBJS)
altictest_shlib_NAME    = altictest
altictest_shlib_OBJS    = $(CTEST_SHOBJS)
altictest_shlib_SHLIBS  = alticore

OBJDIRS            += $(CORE_OBJDIRS) $(CTEST_OBJDIRS)

include $(MAKEFILES_DIR)/rule.mk
