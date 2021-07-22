include ../qpEnv.mk

INCLUDES += $(foreach i,$(MM_DIR)/include $(DK_DIR)/include,$(IDROPT)$(i))
LFLAGS += $(foreach i,$(ALTI_HOME)/lib $(PD_DIR)/lib $(RP_DIR)/lib $(MM_DIR)/lib $(DK_DIR)/lib,$(LDROPT)$(i))
MODULE_LIST = mm dk qp sd rp mt st sm cm id pd
SERVER_LIBS = $(foreach i,$(MODULE_LIST),$(LIBOPT)$(i)$(LIBAFT)) \
              $(ALTICORE_STATIC_LIB) $(LIBS)

BINS=$(ALTI_HOME)/bin/extprocAgent$(BINEXT)

define cmd_ld
  $(Q) $(if $(quiet),echo ' Linking $@')
  $(Q) $(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SERVER_LIBS)
endef

all: install
	echo $(BINS)

clean:
	$(RM) $(TARGET_DIR)/src/qp/util core .dependency $(BINS) *.d SunWS_cache

install: $(BINS)

# PROJ-1685
$(ALTI_HOME)/bin/extprocAgent$(BINEXT) : $(TARGET_DIR)/src/qp/util/extprocAgent.$(OBJEXT) $(SERVER_DEPS)
	$(cmd_ld)

test:
	$(Q) echo $(cmd_cc_o_c)

DEP_DIR=$(CURDIR:$(DEV_DIR)%=$(TARGET_DIR)%)
-include $(wildcard $(DEP_DIR)/*.d)

