#
# $Id: Makefile 70624 2015-05-04 06:53:09Z heesuk.shin $
#

include ../sdEnv.mk
include ./sd_objs.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)sd.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)sd_so.$(SOEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS = $(foreach i, pd id rp sm cm mt lk, $(ALTI_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEFFILE = /DEF:$(SD_DIR)/lib/sd_so.def
endif

all: lib

lib: sd_static

solib: sd_dynamic

sd_static: $(TARGET)

sd_dynamic: $(SHTARGET)

$(TARGET): $(SD_OBJS)

$(SHTARGET): $(SD_SHOBJS)

check :
	@for i in $(SD_SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	$(RM) core .dependency $(OBJS) *.d SunWS_cache lib*.a $(TARGET) $(SHTARGET)

-include $(SD_OBJS:.$(OBJEXT)=.d)
-include $(SD_SHOBJS:.$(OBJEXT)=.d)
