#
# $Id: Makefile 70624 2015-05-04 06:53:09Z heesuk.shin $
#

include ../qpEnv.mk
include ./qp_objs.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)qp.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)qp_so.$(SOEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS = $(foreach i, pd id rp sm cm mt lk, $(ALTI_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEFFILE = /DEF:$(QP_DIR)/lib/qp_so.def
endif

all: lib

lib: qp_static

solib: qp_dynamic

qp_static: $(TARGET)

qp_dynamic: $(SHTARGET)

$(TARGET): $(QP_OBJS)

$(SHTARGET): $(QP_SHOBJS)

check :
	@for i in $(QP_SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	$(RM) core .dependency $(OBJS) *.d SunWS_cache lib*.a $(TARGET) $(SHTARGET)

-include $(QP_OBJS:.$(OBJEXT)=.d)
-include $(QP_SHOBJS:.$(OBJEXT)=.d)
