include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk
include $(ST_DIR)/lib/st_objs.mk

INCLUDES += $(foreach i,$(ST_DIR)/include $(QP_DIR)/include $(SD_DIR)/include $(RP_DIR)/include $(SM_DIR)/include $(MT_DIR)/include ,$(IDROPT)$(i))
INCLUDES_CLI += $(foreach i,$(ST_DIR)/include $(QP_DIR)/include $(SD_DIR)/include $(SM_DIR)/include $(MT_DIR)/include ,$(IDROPT)$(i))
LFLAGS += $(foreach i,$(ID_DIR)/lib $(SM_DIR)/lib $(MT_DIR)/lib $(QP_DIR)/lib $(SD_DIR)/lib $(ST_DIR)/lib,$(LDROPT)$(i))

SERVER_LIBS = $(foreach i,st qp sd sm mt id ,$(LIBOPT)$(i)$(LIBAFT)) $(LIBS)  
SA_LLIBS    = $(LIBOPT)sm_sa$(LIBAFT) $(LIBS)
STLIB       = $(ST_DIR)/lib/$(LIBPRE)st.$(LIBEXT)
CLILIB      = $(UT_DIR)/cli/lib/$(LIBPRE)cli.$(LIBEXT)

# 
COMPILE=compile
INSTALL=install
