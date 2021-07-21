include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk
include $(ID_DIR)/lib/id_objs.mk

INCLUDES += $(foreach i,$(CORE_EXT_DIR)/include,$(IDROPT)$(i))
