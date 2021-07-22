# $Id: Makefile 80505 2017-07-17 02:24:47Z yoonhee.kim $ #

include ../../env.mk
include ../toolEnv.mk

LFLAGS += $(LIBDIRS)

PUROPT=-static-checking=yes -static-checking-default=aggressive -static-checking-guardzone=128 -free-queue-length=10000 -free-queue-threshold=100000
PURREBUILD=#-force-rebuild

DESTROYDB_OBJ=

ifneq "$(OS_TARGET)" "INTEL_WINDOWS"
BINS=$(ALTI_HOME)/bin/makekey$(BINEXT) $(ALTI_HOME)/bin/printkey$(BINEXT)
else
ifeq "$(OS_TARGET2)" "WINCE"
BINS=
else
BINS=
endif
endif # INTEL_WINDOWS

compile: all

all: install

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) core .dependency $(BINS) $(TARGET_DIR)/tool/license/*.$(OBJEXT) *.d SunWS_cache

install: $(BINS)

	$(COPY) addcopy $(ALTI_HOME)/bin

$(ALTI_HOME)/bin/printkey$(BINEXT) : $(TARGET_DIR)/tool/license/printkey.$(OBJEXT)
	$(Q) $(LD) $(LFLAGS) -o $@ $< -lid -lpd $(ALTICORE_STATIC_LIB) $(LIBS)

$(TARGET_DIR)/tool/license/printkey.$(OBJEXT): printkey.cpp
	$(COMPILE_IT)

$(ALTI_HOME)/bin/makekey$(BINEXT) : $(TARGET_DIR)/tool/license/makekey.$(OBJEXT)
	$(Q) $(LD) $(LFLAGS) -o $@ $< -lid -lpd $(ALTICORE_STATIC_LIB) $(LIBS)

$(TARGET_DIR)/tool/license/makekey.$(OBJEXT): makekey.cpp
	$(COMPILE_IT)


-include $(wildcard $(TARGET_DIR)/tool/license/*.d)
