include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk
include $(MT_DIR)/lib/mt_objs.mk
include $(DEV_DIR)/alticore.mk

ifneq "$(OS_LINUX_PACKAGE)" "ARM_WINCE"
DEFINES  += $(DEFOPT)USE_NEW_IOSTREAM
endif # ARM_WINCE

INCLUDES     += $(foreach i,$(MT_DIR)/include $(SM_DIR)/include $(ST_DIR)/include, $(IDROPT)$(i))
INCLUDES_CLI += $(foreach i,$(MT_DIR)/include $(SM_DIR)/include $(ST_DIR)/include, $(IDROPT)$(i))
LFLAGS       += $(foreach i,$(ID_DIR)/lib $(ST_DIR)/lib $(SM_DIR)/lib $(MT_DIR)/lib,$(LDROPT)$(i))

SERVER_LIBS = $(foreach i,mt sm id st,$(LIBOPT)$(i)$(LIBAFT)) $(LIBS)

COMPILE = compile
INSTALL = install
