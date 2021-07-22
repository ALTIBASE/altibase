#
# $Id: Makefile,v 1.5 2005/09/21 06:23:00 qa Exp $
#

include ../stEnv.mk
include ./st_objs.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)st.$(LIBEXT)
STUBTARGET = $(ALTI_HOME)/lib/$(LIBPRE)ststub.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)st_so.$(SOEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS = $(foreach i, pd id sm qp mt cm rp lk, $(ALTI_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEFFILE = /DEF:$(ST_DIR)/lib/st_so.def
endif

all: lib

lib: st_static st_static_stub

solib: st_dynamic

st_static: $(TARGET)

st_static_stub: $(STUBTARGET)
	$(COPY) $(TARGET_DIR)/src/st/sti/stistub.$(OBJEXT) $(ALTI_HOME)/lib

st_dynamic: $(SHTARGET)

$(TARGET): $(ST_OBJS)

$(STUBTARGET): $(ST_STUB_OBJS)

$(SHTARGET): $(ST_SHOBJS)

check :
	@for i in $(ST_SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	$(RM) $(TARGET_DIR)/src/st core .dependency *.d SunWS_cache $(LIBPRE)*.$(LIBEXT) $(TARGET) $(STUBTARGET) $(SHTARGET)

archive:
	$(AR) $(ARFLAGS) $(STLIB) $(ST_OBJS)

-include $(ST_SHOBJS:.$(OBJEXT)=.d)

-include $(ST_STUB_OBJS:.$(OBJEXT)=.d)
