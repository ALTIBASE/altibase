include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))/../../env.mk

# UTIL侩 何啊可记 贸府

UL_DIR_JDBC      = $(UL_DIR)/ulj

ACS_DIR          = $(UL_DIR)/acs # TASK-1963 AST

ULSD_DIR         = ulsd

INCLUDES += $(ALTICORE_INCLUDES) $(foreach i,$(SM_DIR)/include $(MT_DIR)/include $(ID_DIR)/include $(CM_DIR)/include $(SD_DIR)/include $(UL_DIR)/include,$(IDROPT)$(i))

INCLUDES += $(IDROPT)$(UL_DIR)/include/unix-odbc

CLFLAGS += $(foreach i, $(MT_DIR)/lib $(SM_DIR)/lib $(ID_DIR)/lib $(UL_DIR)/lib, $(LDROPT)$(i))
LFLAGS  += $(foreach i, $(MT_DIR)/lib $(SM_DIR)/lib $(ID_DIR)/lib $(UL_DIR)/lib, $(LDROPT)$(i))

JAVAC    = $(JAVA_HOME)/bin/javac
JAVA     = $(JAVA_HOME)/bin/java
JAR      = $(JAVA_HOME)/bin/jar
JAVALIB  = $(JAVA_HOME)/lib

quiet_cmd_c_o_c = CC $@
	cmd_c_o_c = mkdir -p $(dir $@); $(COMPILE.c) $(DEFOPT)LIB_BUILD $(INCLUDES) $(CCOUT)$@ $<

define COMPILE_IT_C
  $(Q) $(if $(quiet),echo ' $($(quiet)cmd_c_o_c)')
  $(Q) $(cmd_c_o_c)
endef

$(TARGET_DIR)/%.$(OBJEXT): $(DEV_DIR)/%.c
	$(COMPILE_IT_C)
