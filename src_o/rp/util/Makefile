include ../rpEnv.mk

INCLUDES += $(foreach i,$(MM_DIR)/include $(DK_DIR)/include,$(IDROPT)$(i))
LFLAGS += $(foreach i,$(ALTI_HOME)/lib $(PD_DIR)/lib $(RP_DIR)/lib $(MM_DIR)/lib $(DK_DIR)/lib,$(LDROPT)$(i))
MODULE_LIST = dk qp sd rp mt st sm cm id pd
SERVER_LIBS = $(foreach i,$(MODULE_LIST),$(LIBOPT)$(i)$(LIBAFT)) \
        $(ALTICORE_STATIC_LIB) $(LIBS)
SERVER_DEPS = $(foreach i,$(MODULE_LIST),$(ALTI_HOME)/lib/$(LIBPRE)$(i).$(LIBEXT))
BINS=$(ALTI_HOME)/bin/dumpxlf$(BINEXT)

define cmd_ld
  $(Q) $(if $(quiet),echo ' Linking $@')
  $(Q) $(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SERVER_LIBS) $(SERVER_DEPS)
endef

all: install
	echo $(BINS)

clean:
	$(RM) $(TARGET_DIR)/src/rp/util core .dependency $(BINS) *.d SunWS_cache

install: $(BINS)

$(ALTI_HOME)/bin/dumpxlf$(BINEXT) : $(TARGET_DIR)/src/rp/util/dumpxlf.$(OBJEXT) $(SERVER_DEPS)
	$(cmd_ld)

test:
	$(Q) echo $(cmd_cc_o_c)

DEP_DIR=$(CURDIR:$(DEV_DIR)%=$(TARGET_DIR)%)
-include $(wildcard $(DEP_DIR)/*.d)

