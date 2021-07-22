include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk
include $(SD_DIR)/lib/sd_objs.mk

INCLUDES += $(foreach i,$(QP_DIR)/include $(SD_DIR)/include $(RP_DIR)/include $(MT_DIR)/include $(ST_DIR)/include $(SM_DIR)/include $(CM_DIR)/include $(UL_DIR)/include $(CORE_EXT_DIR)/include ,$(IDROPT)$(i))
LFLAGS   += $(foreach i,$(ID_DIR)/lib $(MT_DIR)/lib $(ST_DIR)/lib $(SM_DIR)/lib $(QP_DIR)/lib $(SD_DIR)/lib,$(LDROPT)$(i))
INCLUDES_CLI += $(foreach i,$(QP_DIR)/include $(SD_DIR)/include $(RP_DIR)/include $(MT_DIR)/include $(ST_DIR)/include $(SM_DIR)/include $(CM_DIR)/include,$(IDROPT)$(i))

SERVER_LIBS = $(foreach i,sd qp rp mt st rc sm id,$(LIBOPT)$(i)$(LIBAFT)) $(LIBS)
SA_LLIBS    = $(LIBOPT)sm_sa$(LIBAFT) $(LIBS)
SDLIB       = $(SD_DIR)/lib/$(LIBPRE)sd.$(LIBEXT)
CLILIB      = $(UT_DIR)/cli/lib/$(LIBPRE)cli.$(LIBEXT)

# 나중에 삭제할 수 있도록 : 현재 호환성을 위해 정의해 둠
COMPILE=compile
INSTALL=install
