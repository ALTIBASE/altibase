include ../toolEnv.mk

BIN=altipkg$(BINEXT) altipkg_v$(BINEXT)

INCLUDES += $(foreach i,$(MM_DIR)/include $(QP_DIR)/include $(SD_DIR)/include $(RP_DIR)/include $(MT_DIR)/include $(ST_DIR)/include $(SM_DIR)/include $(CM_DIR)/include .,$(IDROPT)$(i))

LIBDIR=$(LDROPT)$(SM_DIR)/lib $(LDROPT)$(ID_DIR)/lib $(LDROPT)$(PD_DIR)/lib $(LDROPT)$(ALTI_HOME)/lib

ALTIPKG_OBJS=$(TARGET_DIR)/tool/altipkg/altipkg.$(OBJEXT) $(TARGET_DIR)/tool/altipkg/dist.$(OBJEXT) $(TARGET_DIR)/tool/altipkg/verify.$(OBJEXT) $(TARGET_DIR)/tool/altipkg/pkglib.$(OBJEXT) $(TARGET_DIR)/src/sm/smu/smuVersion.$(OBJEXT)

all : $(BIN)

ifneq "$(OS_TARGET)" "ELDK_LINUX"
ifneq "$(OS_TARGET)" "MIPS64_LINUX"
ifeq ($(CROSS_COMPILE),)
altipkg$(BINEXT): $(ALTI_HOME)/lib/$(LIBPRE)cm.$(LIBEXT) \
		  $(ALTI_HOME)/lib/$(LIBPRE)id.$(LIBEXT) \
		  $(ALTI_HOME)/lib/$(LIBPRE)pd.$(LIBEXT) \
		  $(ALTICORE_STATIC_LIB)

altipkg$(BINEXT): $(ALTIPKG_OBJS)
	$(LD) $(LFLAGS) $(LDOUT)$@ $^ $(LIBDIR) $(LIBS)
else # CROSS_COMPILE
altipkg$(BINEXT):
	$(COPY) ~/altipkg$(BINEXT) .
endif # CROSS_COMPILE
else # MIPS64_LINUX
altipkg$(BINEXT):
	$(COPY) ~/altipkg$(BINEXT) .
endif # MIPS64_LINUX
else # ELDK_LINUX
altipkg$(BINEXT):
	$(COPY) ~/altipkg$(BINEXT) .
endif # ELDK_LINUX

altipkg_v$(BINEXT): altipkg$(BINEXT)
	$(COPY) altipkg$(BINEXT) altipkg_v$(BINEXT)
# -----------------------------------------------------------
# CPU OS MajorVer MinorVer Edition Devtype BitType Compiler
# -----------------------------------------------------------

#PKGARG = -a "$(ALTI_CFG_CPU)" -a "$(ALTI_CFG_OS)" -a "$(ALTI_CFG_OS_MAJOR)" -a ""$(ALTI_CFG_OS_MINOR)"" -a "$(ALTI_CFG_EDITION)" -a "*" -a "$(ALTI_CFG_BITTYPE)" -a "$(COMPILER_NAME)" -a "$(BINEXT)" -a ".$(OBJEXT)" -a "$(LIBPRE)" -a ".$(LIBEXT)" -a ".$(SOEXT)"

PKGARG = -a "$(ALTI_CFG_CPU)" -a "QNX" -a "$(ALTI_CFG_OS_MAJOR)" -a ""$(ALTI_CFG_OS_MINOR)"" -a "$(ALTI_CFG_EDITION)" -a "*" -a "$(ALTI_CFG_BITTYPE)" -a "$(COMPILER_NAME)" -a "$(BINEXT)" -a ".$(OBJEXT)" -a "$(LIBPRE)" -a ".$(LIBEXT)" -a ".$(SOEXT)"

disttest:
	$(RM) -rf $(ALTIBASE_DEV)/altipkgdir
	$(DEBUG) altipkg$(BINEXT) -m sampledesc.map  -i -d $(PKGARG)

copytest:
	$(DEBUG) altipkg$(BINEXT) -m sampledesc.map  -i -e -r "copydir" -d $(PKGARG)

veritest:
	(cd $(ALTIBASE_DEV)/altipkgdir &&  $(ALTIBASE_DEV)/tool/altipkg/altipkg$(BINEXT) -v -i )

clean:
	$(RM) *.log $(TARGET_DIR)/tool/altipkg/*.$(OBJEXT) $(BIN) $(SAMPLE_OBJS) core

-include $(ALTIPKG_OBJS:.$(OBJEXT)=.d)
