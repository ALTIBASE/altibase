include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk
include $(RP_DIR)/lib/rp_objs.mk

INCLUDES += $(foreach i,$(RP_DIR)/include $(CM_DIR)/include $(DK_DIR)/include $(SM_DIR)/include $(QP_DIR)/include $(SD_DIR)/include $(ST_DIR)/include $(MT_DIR)/include,$(IDROPT)$(i))
