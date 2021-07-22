# Makefile for C/DBC library

include ../../ulEnv.mk

include $(DEV_DIR)/alticore.mk

CDBC_DIR=$(UL_DIR)/cdbc

INCLUDES += $(IDROPT). $(foreach i, $(UL_DIR)/include $(QP_DIR)/include $(MM_DIR)/include $(ID_DIR)/idk ../include, $(IDROPT)$(i))
INCLUDES += $(IDROPT)$(UL_DIR)/include/unix-odbc
LIBDIRS  += $(LDROPT)$(ALTI_HOME)/lib

## LIBRARY NAME
CDBC_LIB_NAME   = alticapi
CDBC_INC_PATH   = $(ALTI_HOME)/include/$(CDBC_LIB_NAME).h
CDBC_LIB_PATH   = $(ALTI_HOME)/lib/$(LIBPRE)$(CDBC_LIB_NAME).$(LIBEXT)
CDBC_SHLIB_PATH = $(ALTI_HOME)/lib/$(LIBPRE)$(CDBC_LIB_NAME)_sl.$(SOEXT)

## SOURCES
CDBCEXT = c
CDBC_SRCS = \
	$(CDBC_DIR)/src/cdbcBufferMng.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcError.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcLog.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcVersion.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcInfo.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcCharset.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcHandle.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcOption.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcConnect.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcTrans.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcMeta.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcArray.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcFailover.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcBind.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcExecute.$(CDBCEXT) \
	$(CDBC_DIR)/src/cdbcFetch.$(CDBCEXT)
CDBC_STOBJS = \
	$(CDBC_SRCS:$(DEV_DIR)/%.$(CDBCEXT)=$(TARGET_DIR)/%_aoc.$(OBJEXT))
CDBC_SHOBJS = \
	$(CDBC_SRCS:$(DEV_DIR)/%.$(CDBCEXT)=$(TARGET_DIR)/%_soc.$(OBJEXT))
CDBC_OBJS = \
	$(CDBC_STOBJS) $(CDBC_SHOBJS)

CDBC_SHDEFLIBS = $(foreach i, odbccli_sl, $(LIBOPT)$(i)$(LIBAFT))



all: build

build: compile

compile: cdbc

cdbc: cdbc_static cdbc_shared

dist:
	cp $(CDBC_DIR)/include/alticdbc.h $(CDBC_INC_PATH)

#############################
# common

$(TARGET_DIR)/%_aoc.$(OBJEXT): $(DEV_DIR)/%.c
	$(Q) mkdir -p $(dir $@)
	$(Q) echo CC $@
	$(Q) $(COMPILE.c) $(CFLAGS) $(DEFOPT)LIB_BUILD $(INCLUDES) $(CCOUT)$@ $<

$(TARGET_DIR)/%_soc.$(OBJEXT): $(DEV_DIR)/%.c
	$(Q) mkdir -p $(dir $@)
	$(Q) echo CC $@
	$(Q) $(COMPILE.c) $(CFLAGS) $(DEFOPT)LIB_BUILD $(INCLUDES) $(PIC) $(CCOUT)$@ $<

#############################
# C/DBC static library build

cdbc_static: $(CDBC_LIB_PATH)

$(CDBC_LIB_PATH): $(CDBC_STOBJS)

#############################
# C/DBC shared library build

cdbc_shared:
ifeq "$(NEED_SHARED_LIBRARY)" "no" # defined in pd/makefiles2/*.mk : only aix 4.x no
	echo "SKIP Shared Library Build";
else
	$(MAKE) cdbc_shared_build
endif

cdbc_shared_build: $(CDBC_SHOBJS)
	$(Q) echo " SO $(CDBC_SHLIB_PATH)"
	$(Q) $(SOLINK.c) $(SOOUT)$(CDBC_SHLIB_PATH) $(CDBC_SHOBJS) $(LIBDIRS) $(CLFLAGS) $(CDBC_SHDEFLIBS)

#############################
# clean

clean:
	$(RM) $(TARGET_DIR)/src/ul/cdbc/src $(CDBC_LIB_PATH) $(CDBC_SHLIB_PATH) $(CDBC_INC_PATH) *.ilk *.pdb *.manifest core
