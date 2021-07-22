include ../ulEnv.mk

include $(DEV_DIR)/alticore.mk
include $(PD_DIR)/lib/pd_objs.mk
include $(ID_DIR)/lib/id_objs.mk
include $(MT_DIR)/lib/mt_objs.mk
include $(CM_DIR)/lib/cm_objs.mk
include $(UL_DIR)/lib/ul_objs.mk

# a makeshift compile flag :
# 	To suppress needless warnings.
# 	Because SUNWSPro generates "statement not reached" warning
# 	for every ACI_EXCEPTION().
#
ifeq "$(OS_TARGET)" "X86_SOLARIS"
	CFLAGS+=-erroff=E_STATEMENT_NOT_REACHED
endif

# a makeshift compile flag :
# 	To suppress annoying warning, "statement is unreachable"
#		"/home/shawn/work/altidev4/src/cm/cmi/cmi.c", line 105: warning #2111-D: 
#		          statement is unreachable
#			      ACI_EXCEPTION(InvalidModule);
#			      ^
ifeq "$(OS_TARGET)" "IA64_HP_HPUX"
	CFLAGS+=+W2111
endif

INCLUDES += $(foreach i, $(QP_DIR)/include $(MM_DIR)/include, $(IDROPT)$(i))
INCLUDES += $(IDROPT)$(ST_DIR)/include

LIBDIRS   += $(LDROPT)$(ALTI_HOME)/lib

all: msg lib

msg:
	$(MAKE) -C ../msg/

lib:
	$(MAKE) odbccli
	$(MAKE) acslib
	$(MAKE) alalib
	$(MAKE) shardcli

#BUG-22936
ifeq ($(compile64),1)
	$(MAKE) unix_odbc64
endif
	$(MAKE) unix_odbc

# sort for removing duplicate rules
$(foreach i,$(sort $(ODBCCLI_SHOBJS) $(SHARDCLI_SHOBJS) $(ACS_SHOBJS) $(ALA_SHOBJS) $(ALA_OBJS) ),$(eval $(call ListRuleFunc,$i)))

odbccli:
	$(MAKE) odbccli_static
	$(MAKE) odbccli_shared

shardcli:
	$(MAKE) shardcli_static
	$(MAKE) shardcli_shared

acslib:
	$(MAKE) acslib_static
#	make acslib_shared    # BUG-15883

alalib:
	$(MAKE) alalib_static
	$(MAKE) alalib_shared

#################
# static CLI 網萄
#################
odbccli_static:
	$(Q) -$(RM) $(ALTIBASE_HOME)/lib/$(LIBPRE)odbccli.$(LIBEXT)
	$(MAKE) odbccli_static_build

odbccli_static_build: $(ODBCCLI_LIB_PATH)
ifeq "$(OS_TARGET)" "IBM_AIX"
  # BUG-31759
  # AIX needs X64 or X32 flag according to the compile bit
  ifeq ($(compile64),1)
	$(Q) $(AR) -x -X64 $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
  else
	$(Q) $(AR) -x -X32 $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
  endif
else # ifneq "$(OS_TARGET)" "IBM_AIX"
	$(Q) $(AR) -x $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
endif
	$(Q) $(AR) $(ARFLAGS) $(ALTIBASE_HOME)/lib/$(LIBPRE)odbccli.$(LIBEXT) *.$(OBJEXT) > /dev/null
	$(Q) -$(RM) *.$(OBJEXT)

$(ODBCCLI_LIB_PATH): $(ODBCCLI_OBJS)

#################
# shared CLI 網萄
#################
odbccli_shared:
ifeq "$(NEED_SHARED_LIBRARY)" "no" # defined in pd/makefiles2/*.mk : only aix 4.x no
	echo "SKIP Shared Library Build";
else
	$(MAKE) odbccli_shared_build
endif

odbccli_shared_build: $(ODBCCLI_SHOBJS)
	$(Q) -$(RM) $(ODBCCLI_SHLIB_PATH)
	$(Q) echo " SO $(ODBCCLI_SHLIB_PATH)"
ifeq "$(OS_TARGET)" "IBM_AIX"
    # BUG-31647
    ifeq ($(compile64),1)
	$(Q) $(SOLINK.c) $(SOOUT)$(ODBCCLI_SHLIB_PATH) $(ODBCCLI_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(LIBOPT)curses$(LIBAFT) $(CLFLAGS)
    else
	$(Q) $(SOLINK.c) $(LIBDIRS) $(SOOUT)$(ODBCCLI_SHLIB_PATH) $(ODBCCLI_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(LIBOPT)curses$(LIBAFT) $(LIBOPT)gcc_s$(LIBAFT) $(CLFLAGS)
    endif # compile64=1
else
	$(Q) $(SOLINK.c) $(SOOUT)$(ODBCCLI_SHLIB_PATH) $(ODBCCLI_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(CLFLAGS)
endif # IBM_AIX

#################
# static CLI for Shard 網萄
#################
shardcli_static:
	$(Q) -$(RM) $(ALTIBASE_HOME)/lib/$(LIBPRE)shardcli.$(LIBEXT)
	$(MAKE) shardcli_static_build


shardcli_static_build: $(SHARDCLI_LIB_PATH)
ifeq "$(OS_TARGET)" "IBM_AIX"
  ifeq ($(compile64),1)
	$(Q) $(AR) -x -X64 $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
  else
	$(Q) $(AR) -x -X32 $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
  endif
else # ifneq "$(OS_TARGET)" "IBM_AIX"
	$(Q) $(AR) -x $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
endif
	$(Q) $(AR) $(ARFLAGS) $(ALTIBASE_HOME)/lib/$(LIBPRE)shardcli.$(LIBEXT) *.$(OBJEXT) > /dev/null
	$(Q) -$(RM) *.$(OBJEXT)

$(SHARDCLI_LIB_PATH): $(SHARDCLI_OBJS)

#################
# shared CLI for Shard 網萄
#################
shardcli_shared: $(SHARDCLI_SHOBJS)
	$(Q) -$(RM) $(SHARDCLI_SHLIB_PATH)
	$(Q) echo " SO $(SHARDCLI_SHLIB_PATH)"
ifeq "$(OS_TARGET)" "IBM_AIX"
    # BUG-31647
    ifeq ($(compile64),1)
	$(Q) $(SOLINK.c) $(SOOUT)$(SHARDCLI_SHLIB_PATH) $(SHARDCLI_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(LIBOPT)curses$(LIBAFT) $(CLFLAGS)
    else
	$(Q) $(SOLINK.c) $(LIBDIRS) $(SOOUT)$(SHARDCLI_SHLIB_PATH) $(SHARDCLI_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(LIBOPT)curses$(LIBAFT) $(LIBOPT)gcc_s$(LIBAFT) $(CLFLAGS)
    endif # compile64=1
else
	$(Q) $(SOLINK.c) $(SOOUT)$(SHARDCLI_SHLIB_PATH) $(SHARDCLI_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(CLFLAGS)
endif # IBM_AIX

############################################
# ACS LIB 網萄
############################################

#################
# static acslib 網萄
#################

acslib_static: $(ACS_LIB_PATH)

$(ACS_LIB_PATH): $(ACS_OBJS)

#################
# shared acslib 網萄
#################

acslib_shared:
ifeq "$(NEED_SHARED_LIBRARY)" "no" # defined in pd/makefiles2/*.mk : only aix 4.x no
	echo "SKIP Shared Library Build";
else
	$(MAKE) acs_shared_build
endif

acs_shared_build: $(ACS_SHOBJS)
	$(Q) -$(RM) $(ACS_SHLIB_PATH)
	$(Q) echo " SO $(ACS_SHLIB_PATH)"
	$(SOLINK.c) $(SOOUT)$(ACS_SHLIB_PATH) $(ACS_SHOBJS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT) $(CLFLAGS)

############################################
# ALA LIB 網萄
############################################

#################
# static alalib 網萄
#################

ifeq "$(OS_TARGET)" "IBM_AIX"
alalib_static: $(ALA_LIB_PATH) $(ALA_OBJS)
	$(Q) $(AR) $(ARFLAGS) $(ALA_LIB_PATH) $(ALA_OBJS)
ifeq ($(compile64),1)
	$(Q) $(AR) -x -X64 $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
else
	$(Q) $(AR) -x -X32 $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
endif
	$(Q) $(AR) $(ARFLAGS) $(ALTIBASE_HOME)/lib/$(LIBPRE)ala.$(LIBEXT) *.$(OBJEXT) > /dev/null
	$(Q) $(RM) *.$(OBJEXT)
else
alalib_static: $(ALA_LIB_PATH)
	$(Q) $(AR) -x $(ALTICORE_LIBDIR)/$(LIBPRE)alticore.$(LIBEXT)
	$(Q) $(AR) $(ARFLAGS) $(ALTIBASE_HOME)/lib/$(LIBPRE)ala.$(LIBEXT) *.$(OBJEXT) > /dev/null
	$(Q) $(RM) *.$(OBJEXT)

$(ALA_LIB_PATH): $(ALA_OBJS)
endif

#################
# shared alalib 網萄
#################

alalib_shared:
ifeq "$(NEED_SHARED_LIBRARY)" "no" # defined in pd/makefiles2/*.mk : only aix 4.x no
	echo "SKIP Shared Library Build";
else
#ifeq "$(OS_TARGET)" "IBM_AIX"
#	make ala_shared_build_aix
#else
	$(MAKE) ala_shared_build
#endif
endif

ala_shared_build_aix: $(ALA_SHOBJS)
	$(Q) -$(RM) $(ALA_SHLIB_PATH)
	$(Q) echo " SO $(ALA_SHLIB_PATH)"
	$(Q) $(SOLINK.c) $(SOOUT)$(ALA_SHLIB_PATH) $(ALA_SHOBJS) $(CLFLAGS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)

ala_shared_build: $(ALA_SHOBJS)
	$(Q) -$(RM) $(ALA_SHLIB_PATH)
	$(Q) echo " SO $(ALA_SHLIB_PATH)"
	$(Q) $(SOLINK.c) $(SOOUT)$(ALA_SHLIB_PATH) $(ALA_SHOBJS) $(CLFLAGS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)

################
# Unix ODBC 網萄
################
unix_odbc: $(UNIX_ODBC_OBJS)
	$(Q) -$(RM) $(UNIX_ODBC_SHLIB_PATH)
	$(Q) echo " SO $(UNIX_ODBC_SHLIB_PATH)"
ifeq "$(OS_TARGET)" "X86_SOLARIS"
	$(Q) $(SOLINK.c) $(SOOUT)$(UNIX_ODBC_SHLIB_PATH) $(UNIX_ODBC_OBJS) $(CLFLAGS) -lxnet -lc -lnsl -lsocket $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)
else
ifeq "$(OS_TARGET)" "SPARC_SOLARIS"
	$(Q) $(SOLINK.c) $(SOOUT)$(UNIX_ODBC_SHLIB_PATH) $(UNIX_ODBC_OBJS) $(CLFLAGS) -lc -lnsl -lsocket $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)
else
	$(Q) $(SOLINK.c) $(SOOUT)$(UNIX_ODBC_SHLIB_PATH) $(UNIX_ODBC_OBJS) $(CLFLAGS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)
endif
endif

#BUG-22936
unix_odbc64: $(UNIX_ODBC64_OBJS)
	$(Q) -$(RM) $(UNIX_ODBC64_SHLIB_PATH)
	$(Q) echo " SO $(UNIX_ODBC64_SHLIB_PATH)"
ifeq "$(OS_TARGET)" "X86_SOLARIS"
	$(Q) $(SOLINK.c) $(SOOUT)$(UNIX_ODBC64_SHLIB_PATH) $(UNIX_ODBC64_OBJS) $(CLFLAGS) -lxnet -lc -lnsl -lsocket $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)
else
ifeq "$(OS_TARGET)" "SPARC_SOLARIS"
	$(Q) $(SOLINK.c) $(SOOUT)$(UNIX_ODBC64_SHLIB_PATH) $(UNIX_ODBC64_OBJS) $(CLFLAGS) -lc -lnsl -lsocket $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)
else
	$(Q) $(SOLINK.c) $(SOOUT)$(UNIX_ODBC64_SHLIB_PATH) $(UNIX_ODBC64_OBJS) $(CLFLAGS) $(LDROPT)$(ALTICORE_LIBDIR) $(ALTICORE_LIBDIR)/$(LIBPRE)alticore_pic.$(LIBEXT)
endif
endif

clean_objlist:
	$(Q) -$(RM) $(OBJLIST)
	$(Q) touch $(OBJLIST)

# ------------------------------------------------
#  static odbccli
# ----------------------------------------------*/

quiet_cmd_cc_o_c_aoc = CC $@
      cmd_cc_o_c_aoc = mkdir -p $(dir $@); $(COMPILE.cc) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(INCLUDES) $(CCOUT)$@ $<

quiet_cmd_c_o_c_aoc = CC $@
      cmd_c_o_c_aoc = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(INCLUDES) $(CCOUT)$@ $<

define COMPILE_IT_CC_AOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_cc_o_c_aoc)')
  $(Q) $(cmd_cc_o_c_aoc)
endef

define COMPILE_IT_C_AOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c_aoc)')
  $(Q) $(cmd_c_o_c_aoc)
endef

$(TARGET_DIR)/%_aoc.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C_AOC)

$(TARGET_DIR)/%_aoc.$(OBJEXT): $(DEV_DIR)/%.cpp
	$(COMPILE_IT_CC_AOC)

# ------------------------------------------------
#  static shardcli
# ----------------------------------------------*/

quiet_cmd_cc_o_c_sd_aoc = CC $@
      cmd_cc_o_c_sd_aoc = mkdir -p $(dir $@); $(COMPILE.cc) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)COMPILE_SHARDCLI $(INCLUDES) $(CCOUT)$@ $<

quiet_cmd_c_o_c_sd_aoc = CC $@
      cmd_c_o_c_sd_aoc = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)COMPILE_SHARDCLI $(INCLUDES) $(CCOUT)$@ $<

define COMPILE_IT_CC_SD_AOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_cc_o_c_sd_aoc)')
  $(Q) $(cmd_cc_o_c_sd_aoc)
endef

define COMPILE_IT_C_SD_AOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c_sd_aoc)')
  $(Q) $(cmd_c_o_c_sd_aoc)
endef

$(TARGET_DIR)/%_sd_aoc.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C_SD_AOC)

$(TARGET_DIR)/%_sd_aoc.$(OBJEXT): $(DEV_DIR)/%.cpp
	$(COMPILE_IT_CC_SD_AOC)

# ------------------------------------------------
#  dynamic odbccli
# ----------------------------------------------*/

quiet_cmd_cc_o_c_soc = CC $@
      cmd_cc_o_c_soc = mkdir -p $(dir $@); $(COMPILE.cc) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(INCLUDES)  $(PIC) $(CCOUT)$@ $<

quiet_cmd_c_o_c_soc = CC $@
      cmd_c_o_c_soc = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(INCLUDES) $(PIC) $(CCOUT)$@ $<

define COMPILE_IT_CC_SOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_cc_o_c_soc)')
  $(Q) $(cmd_cc_o_c_soc)
endef

define COMPILE_IT_C_SOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c_soc)')
  $(Q) $(cmd_c_o_c_soc)
endef

$(TARGET_DIR)/%_soc.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C_SOC)

$(TARGET_DIR)/%_soc.$(OBJEXT): $(DEV_DIR)/%.cpp
	$(COMPILE_IT_CC_SOC)

# ------------------------------------------------
#  dynamic shardcli
# ----------------------------------------------*/

quiet_cmd_cc_o_c_sd_soc = CC $@
      cmd_cc_o_c_sd_soc = mkdir -p $(dir $@); $(COMPILE.cc) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)COMPILE_SHARDCLI $(INCLUDES)  $(PIC) $(CCOUT)$@ $<

quiet_cmd_c_o_c_sd_soc = CC $@
      cmd_c_o_c_sd_soc = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)COMPILE_SHARDCLI $(INCLUDES) $(PIC) $(CCOUT)$@ $<

define COMPILE_IT_CC_SD_SOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_cc_o_c_sd_soc)')
  $(Q) $(cmd_cc_o_c_sd_soc)
endef

define COMPILE_IT_C_SD_SOC
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c_sd_soc)')
  $(Q) $(cmd_c_o_c_sd_soc)
endef

$(TARGET_DIR)/%_sd_soc.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C_SD_SOC)

$(TARGET_DIR)/%_sd_soc.$(OBJEXT): $(DEV_DIR)/%.cpp
	$(COMPILE_IT_CC_SD_SOC)

# ------------------------------------------------
#  unix odbc build
# ----------------------------------------------*/

quiet_cmd_cc_o_c_unix = CC $@
      cmd_cc_o_c_unix = mkdir -p $(dir $@); $(COMPILE.cc) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)ALTIBASE_ODBC $(INCLUDES) $(PIC) $(CCOUT)$@ $<

quiet_cmd_c_o_c_unix = CC $@
      cmd_c_o_c_unix = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)ALTIBASE_ODBC $(INCLUDES) $(PIC) $(CCOUT)$@ $<

define COMPILE_IT_CC_UNIX
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_cc_o_c_unix)')
  $(Q) $(cmd_cc_o_c_unix)
endef

define COMPILE_IT_C_UNIX
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c_unix)')
  $(Q) $(cmd_c_o_c_unix)
endef

$(TARGET_DIR)/%_unixodbc.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C_UNIX)

# ------------------------------------------------
#  unix odbc64 build  # bug-22936
# ----------------------------------------------*/

quiet_cmd_cc_o_c_unix64 = CC $@
      cmd_cc_o_c_unix64 = mkdir -p $(dir $@); $(COMPILE.cc) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)ALTIBASE_ODBC $(DEFOPT)BUILD_REAL_64_BIT_MODE $(INCLUDES) $(PIC) $(CCOUT)$@ $<

quiet_cmd_c_o_c_unix64 = CC $@
      cmd_c_o_c_unix64 = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(DEFOPT)LIB_BUILD $(DEFOPT)ALTIBASE_ODBC $(DEFOPT)BUILD_REAL_64_BIT_MODE $(INCLUDES) $(PIC) $(CCOUT)$@ $<

define COMPILE_IT_CC_UNIX64
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_cc_o_c_unix64)')
  $(Q) $(cmd_cc_o_c_unix64)
endef

define COMPILE_IT_C_UNIX64
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c_unix64)')
  $(Q) $(cmd_c_o_c_unix64)
endef

$(TARGET_DIR)/%_64.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C_UNIX64)

clean:
	-$(RM) core $(OBJS) *.d SunWS_cache .shobj
#	-$(RM) $(ODBCCLI_OBJS)
#	-$(RM) $(ODBCCLI_SHOBJS)
#	-$(RM) $(UNIX_ODBC_OBJS)
#	-$(RM) $(ACSLIB_OBJS)
#	-$(RM) $(ACSLIB_SHOBJS)
	-$(RM) $(TARGET_DIR)/src/ul/ulx/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/uln/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/ulu/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/uls/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/ulc/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/ulc/ulcs/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/ulc/ulco/ulcox/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/ula/*.$(OBJEXT)
	-$(RM) $(TARGET_DIR)/src/ul/ulsd/*.$(OBJEXT)
	-$(RM) $(ODBCCLI_LIB_PATH)
	-$(RM) $(ODBCCLI_SHLIB_PATH)
	-$(RM) $(UNIX_ODBC_SHLIB_PATH)
	-$(RM) $(UNIX_ODBC64_SHLIB_PATH)
	-$(RM) $(ALA_SHLIB_PATH)
	-$(RM) $(ALA_LIB_PATH)
	-$(RM) $(ACS_LIB_PATH)
	-$(RM) $(ACS_SHLIB_PATH)
	-$(RM) $(SHARDCLI_LIB_PATH)
	-$(RM) $(SHARDCLI_SHLIB_PATH)
	-$(RM) *.lib *.pdb *.exp

-include $(ODBCCLI_OBJS:.$(OBJEXT)=.d)
-include $(ODBCCLI_SHOBJS:.$(OBJEXT)=.d)
-include $(ACS_SHOBJS:.$(OBJEXT)=.d)
-include $(ALA_OBJS:.$(OBJEXT)=.d)
-include $(ALA_SHOBJS:.$(OBJEXT)=.d)

-include $(SHARDCLI_OBJS:.$(OBJEXT)=.d)
-include $(SHARDCLI_SHOBJS:.$(OBJEXT)=.d)

ifeq ($(compile64),1)
-include $(UNIX_ODBC64_OBJS:.$(OBJEXT)=.d)
else
-include $(UNIX_ODBC_OBJS:.$(OBJEXT)=.d)
endif
