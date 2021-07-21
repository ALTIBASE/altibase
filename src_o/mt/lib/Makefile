# $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $

include ../mtEnv.mk
include ./mt_objs.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)mt.$(LIBEXT)
UTTARGET = $(ALTI_HOME)/lib/$(LIBPRE)mt4ut.$(LIBEXT)
CTARGET = $(ALTI_HOME)/lib/$(LIBPRE)mtc4ut.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)mt_so.$(SOEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS = $(foreach i, pd id, $(ALTI_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEFFILE = /DEF:$(MT_DIR)/lib/mt_so.def
endif

all: lib

lib: mt_static mt_c_static

solib: mt_dynamic

clib: mt_c_static

mt_static: $(TARGET) $(UTTARGET)
mt_c_static: $(CTARGET)
mt_dynamic: $(SHTARGET)

$(TARGET): $(MT_OBJS)
$(UTTARGET): $(MT_CLIENT_OBJS)
$(CTARGET): $(MT_C_CLIENT_OBJS)
$(SHTARGET): $(MT_SHOBJS)

quiet_cmd_c_o_c = CC $@
	cmd_c_o_c = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)LIB_BUILD $(INCLUDES) $(CCOUT)$@ $<

define COMPILE_IT_C
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c)')
  $(Q) $(cmd_c_o_c)
endef

$(TARGET_DIR)/%.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C)

clean:
	$(RM) $(LIBPRE)mt.$(LIBEXT) $(TARGET) $(SHTARGET) $(UTTARGET)

cclean:
	$(RM) $(LIBPRE)mtc4ut.$(LIBEXT) $(CTARGET)

ifneq ($(wildcard $(OBJS:.$(OBJEXT)=.d)),)
include $(wildcard $(OBJS:.$(OBJEXT)=.d))
endif

-include $(MT_SHOBJS:.$(OBJEXT)=.d)

-include $(MT_CLIENT_OBJS:.$(OBJEXT)=.d)

-include $(MT_C_CLIENT_OBJS:.$(OBJEXT)=.d)
