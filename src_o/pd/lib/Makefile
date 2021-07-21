# Makefile for PD library
#
# CVS Info : $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $
#

include ../pdEnv.mk

TARGET = $(ALTI_HOME)/lib/$(LIBPRE)pd.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)pd_so.$(SOEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
ifeq ($(compile64),1)
ifeq ($(BUILD_MODE),release)
SODEFFILE = /DEF:$(PD_DIR)/lib/pd_so64r.def
else
SODEFFILE = /DEF:$(PD_DIR)/lib/pd_so64d.def
endif
else
ifeq ($(BUILD_MODE),release)
SODEFFILE = /DEF:$(PD_DIR)/lib/pd_so32r.def
else
SODEFFILE = /DEF:$(PD_DIR)/lib/pd_so32d.def
endif
endif
endif

all: lib

lib: pd_static

solib: pd_dynamic

pd_static:  $(TARGET)
# BUG-31647
ifeq "$(OS_TARGET)" "IBM_AIX"
ifeq ($(compile64),1)
else
	cp external/libgcc_s.a $(ALTI_HOME)/lib/
endif
endif

pd_dynamic: $(SHTARGET)

$(TARGET) : $(PD_OBJS)

$(SHTARGET) : $(PD_SHOBJS)

clean:
	-$(RM) core $(TARGET) $(SHTARGET) createdb destroydb SunWS_cache .shobj

-include $(PD_SHOBJS:.$(OBJEXT)=.d)
