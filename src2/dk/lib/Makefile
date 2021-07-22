#
# $Id: Makefile 47243 2011-04-25 00:57:57Z vsebelev $
#

include ../dkEnv.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)dk.$(LIBEXT) 
STUBTARGET = $(ALTI_HOME)/lib/$(LIBPRE)dkstub.$(LIBEXT) 
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)dk_so.$(SOEXT) 

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS = $(foreach i, pd id qp sm mt rp cm, $(ALTI_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEFFILE = /DEF:$(DK_DIR)/lib/dk_so.def
endif

all: lib

lib: dk_static dk_static_stub

solib: dk_dynamic

dk_static: $(TARGET)

dk_static_stub: $(STUBTARGET)
	$(COPY) $(TARGET_DIR)/src/dk/dki/dkistub.$(OBJEXT) $(ALTI_HOME)/lib

dk_dynamic: $(SHTARGET)

$(TARGET): $(DK_OBJS)

$(STUBTARGET): $(DK_STUB_OBJS)

$(SHTARGET): $(DK_SHOBJS)

check :
	@for i in $(QP_SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	$(RM) core .dependency $(OBJS) *.d SunWS_cache lib*.a $(TARGET) $(STUBTARGET) $(SHTARGET)

-include $(DK_SHOBJS:.$(OBJEXT)=.d)

-include $(DK_STUB_OBJS:.$(OBJEXT)=.d)
