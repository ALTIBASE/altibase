#
# $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $
#

include ../rpEnv.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)rp.$(LIBEXT)
STUBTARGET =  $(ALTI_HOME)/lib/$(LIBPRE)rpstub.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)rp_so.$(SOEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS = $(foreach i, pd id cm sm mt qp lk, $(ALTIBASE_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEFFILE = /DEF:$(RP_DIR)/lib/rp_so.def
endif

all: lib

lib: rp_static rp_static_stub

solib: rp_dynamic

rp_static: $(TARGET)

rp_static_stub: $(STUBTARGET)
	$(COPY) $(TARGET_DIR)/src/rp/rpi/rpistub.$(OBJEXT) $(ALTI_HOME)/lib

rp_dynamic: $(SHTARGET)

$(TARGET): $(RP_OBJS)

$(STUBTARGET): $(RP_STUB_OBJS)

$(SHTARGET): $(RP_SHOBJS)

check :
	@for i in $(QP_SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	$(RM) core .dependency $(OBJS) *.d SunWS_cache lib*.a $(TARGET) $(STUBTARGET) $(SHTARGET)

-include $(RP_SHOBJS:.$(OBJEXT)=.d)

-include $(RP_STUB_OBJS:.$(OBJEXT)=.d)
