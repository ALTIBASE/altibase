include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk
include $(SM_DIR)/lib/sm_objs.mk

# SM侩 何啊可记 贸府 
INCLUDES += $(foreach i,$(MT_DIR)/include $(CORE_EXT_DIR)/include $(ST_DIR)/include $(SM_DIR)/include $(SM_DIR)/smu,$(IDROPT)$(i))
LFLAGS   += $(foreach i,$(ALTI_HOME)/lib $(PD_DIR)/lib $(ID_DIR)/lib $(SM_DIR)/lib $(MT_DIR)/lib,$(LDROPT)$(i))

#SMLIB = $(LIBOPT)sm$(LIBAFT)
SMLIB = $(ALTI_HOME)/lib/$(LIBPRE)sm.$(LIBEXT)

ID_LIB = $(foreach i,id pd,$(ALTI_HOME)/lib/$(LIBPRE)$(i).$(LIBEXT)) \
	 $(ALTICORE_STATIC_LIB)
