# $Id: #

include ../toolEnv.mk

LIBS := $(LIBOPT)id$(LIBAFT) \
	$(LIBOPT)pd$(LIBAFT) \
	$(ALTICORE_STATIC_LIB) \
	$(LIBS)

DUMPSHM_BIN := $(ALTI_HOME)/bin/$(PRODUCT_PREFIX)dumpshm$(BINEXT)

all: $(DUMPSHM_BIN)

$(DUMPSHM_BIN): $(TARGET_DIR)/tool/dumpshm/dumpshm.$(OBJEXT)
	$(LD) $(INCLUDES) $(CFLAGS) $(LFLAGS) $(LDOUT)$@ $< $(LIBDIRS) $(LIBS)

clean:
	$(RM) *.log *.$(OBJEXT) $(DUMPSHM_BIN) $(SAMPLE_OBJS) core

install: 

alint:

sure: clean all

.PHONY: all clean alint sure dep depend

DEP_DIR=$(CURDIR:$(DEV_DIR)%=$(TARGET_DIR)%)
-include $(wildcard $(DEP_DIR)/*.d)
