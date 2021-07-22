# $Id: Makefile 76891 2016-08-29 07:15:49Z djin $ #

include ../../../env.mk

DUMPMAP_BIN := $(ALTI_HOME)/bin/dumpmap$(BINEXT)
DUMPTRC_BIN := $(ALTI_HOME)/bin/dumptrc$(BINEXT)
ALTICRC_BIN := $(ALTI_HOME)/bin/alticrc$(BINEXT)

BINS= $(ALTICRC_BIN) $(DUMPMAP_BIN) $(DUMPTRC_BIN) 

MODULE_LIST = mm dk qp sd rp mt st sm cm id pd
SERVER_LIBS = $(foreach i,$(MODULE_LIST),$(LIBOPT)$(i)$(LIBAFT)) $(ALTICORE_STATIC_LIB) $(LIBS)

all: install

clean:
	$(RM) core .dependency $(BINS) map.ic alticrc.txt $(ALTI_HOME)/bin/altibase.map *.d SunWS_cache 

install: $(BINS)

$(DUMPMAP_BIN): $(TARGET_DIR)/src/id/util/dumpmap.$(OBJEXT) $(ALTI_HOME)/bin/altibase.map
	$(RM) map.ic 
	$(LD) $(INCLUDES) $(CFLAGS) $(LFLAGS) $(LDOUT)$@ $< $(LIBDIRS) $(LIBS) $(SERVER_LIBS)

$(ALTICRC_BIN): $(TARGET_DIR)/src/id/util/alticrc.$(OBJEXT)
	$(RM) alticrc.txt
	$(LD) $(INCLUDES) $(CFLAGS) $(LFLAGS) $(LDOUT)$@ $< $(LIBDIRS) $(LIBS) $(SERVER_LIBS)

alticrc.txt: $(ALTICRC_BIN)
	$(ALTICRC_BIN) > alticrc.txt

map.ic: $(DUMPMAP_BIN)
	$(DUMPMAP_BIN) > map.ic


$(TARGET_DIR)/src/id/util/dumptrc.$(OBJEXT): map.ic   alticrc.txt
  
$(ALTI_HOME)/bin/altibase.map : $(ALTI_HOME)/bin/altibase$(BINEXT)
ifeq "$(OS_TARGET)" "POWERPC64_LINUX"
	objdump -d $(ALTI_HOME)/bin/altibase | grep ">:" > $(ALTI_HOME)/bin/altibase.map
else
	$(NM) $(NMFLAGS) $(ALTI_HOME)/bin/altibase$(BINEXT) > $(ALTI_HOME)/bin/altibase.map
endif

$(DUMPTRC_BIN): $(TARGET_DIR)/src/id/util/dumptrc.$(OBJEXT)
	$(DUMPMAP_BIN) > map.ic
	$(ALTICRC_BIN) > alticrc.txt
	$(LD) $(INCLUDES) $(CFLAGS) $(LFLAGS) $(LDOUT)$@ $< $(LIBDIRS) $(LIBS) $(SERVER_LIBS)

DEP_DIR=$(CURDIR:$(DEV_DIR)%=$(TARGET_DIR)%)
-include $(wildcard $(DEP_DIR)/*.d)

