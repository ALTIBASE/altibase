# $Id: Makefile 90822 2021-05-13 04:45:36Z kclee $

include ../idEnv.mk

ALLMSB         = $(ALTI_HOME)/msg/E_ID_US7ASCII.msb $(ALTI_HOME)/msg/E_ID_KO16KSC5601.msb

LIBDIRS := $(LDROPT)$(PD_DIR)/lib $(LDROPT)$(ALTI_HOME)/lib $(LIBDIRS) $(LDROPT)$(ALTICORE_LIBDIR)
DEFINES_ERR  = $(DEFOPT)GEN_ERR_MSG
DEFINES_BUILD_FOR_UTIL = $(DEFOPT)BUILD_FOR_UTIL
DEFINES += $(DEFOPT)GEN_ERR_MSG
INCLUDES := $(INCLUDES) $(IDROPT).
LIBS += $(ALTICORE_LIB)

SRCS = ../idl/idl.cpp ../ide/ideErrorMgr_client.cpp ../idu/iduVersion.cpp ../idCore/idCore.cpp
ifeq "$(PRODUCT_TYPE)" "xdb"
SRCS+=../idu/iduShmProcType.cpp
endif
OBJS = $(foreach src, $(SRCS),$(notdir $(src:.cpp=.$(OBJEXT))))

BINS	= $(GENERRMSG)

# 소스코드에 include 될 에러코드 화일
E_HEADER = idErrorCode.ih

# 에러코드 메시지 화일

all: ErrorFiles
shobjs: 

install: $(BINS)

genErrMsg.$(OBJEXT): genErrMsg.cpp $(ID_DIR)/include/iduVersionDef.h
	$(COMPILE.cc) $(DEFINES_ERR) $(DEFINES_BUILD_FOR_UTIL) $(INCLUDES) $<

$(GENERRMSG):
ifneq "$(OS_LINUX_PACKAGE)" "ARM_WINCE"
ifneq "$(OS_TARGET)" "ARM_LINUX"
ifneq "$(OS_TARGET)" "MIPS64_LINUX"
ifneq "$(OS_TARGET)" "ELDK_LINUX"
ifeq  ($(CROSS_COMPILE),)
$(GENERRMSG): genErrMsg.$(OBJEXT)
	$(COMPILE.cc) $(DEFINES_ERR) $(DEFINES_BUILD_FOR_UTIL) $(INCLUDES) $(SRCS)
	$(LD) $(LFLAGS) $(LDOUT)$@ $(subst .cpp,.$(OBJEXT),$^) $(OBJS) $(LIBDIRS) $(LIBOPT)pd$(LIBAFT) $(LIBS)
else # CROSS_COMPILE 
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsg$(BINEXT) $(ALTI_HOME)/bin
endif # CROSS_COMPILE
else # ELDK_LINUX
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsg$(BINEXT) $(ALTI_HOME)/bin
endif # ELDK_LINUX
else # MIPS64_LINUX
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsg$(BINEXT) $(ALTI_HOME)/bin
endif # MIPS64_LINUX
else # ARM_LINUX
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsg$(BINEXT) $(ALTI_HOME)/bin
endif # ARM_LINUX
else # ARM_WINCE
	$(COPY) $(DEV_DIR)/win32-build/bin/$(PRODUCT_PREFIX)genErrMsg$(BINEXT) $(ALTI_HOME)/bin
endif # ARM_WINCE

genErrMsgManual.$(OBJEXT): genErrMsgManual.cpp $(ID_DIR)/include/iduVersionDef.h
	$(COMPILE.cc) $(DEFINES_ERR) $(DEFINES_BUILD_FOR_UTIL) $(INCLUDES) $<

$(GENERRMSGMANUAL): genErrMsgManual.$(OBJEXT)
	$(COMPILE.cc) $(DEFINES_ERR) $(DEFINES_BUILD_FOR_UTIL) $(INCLUDES) $(SRCS)
ifneq "$(OS_TARGET)" "ELDK_LINUX"
ifneq "$(OS_TARGET)" "MIPS64_LINUX"
ifeq  ($(CROSS_COMPILE),)
	$(LD) $(LFLAGS) $(LDOUT)$@ $(subst .cpp,.$(OBJEXT),$^) $(OBJS) $(LIBDIRS) $(LIBOPT)pd$(LIBAFT) $(LIBS)
#	$(CXX) $(CCFLAGS) $(DEFINES_ERR) $(DEFINES_BUILD_FOR_UTIL) $(CPPFLAGS) $(PTDIRS) $(INCLUDES) $(LFLAGS) $(LIBDIRS) -o $@ $< ../idl/idl.cpp ../ide/ideErrorMgr.cpp ../idu/iduVersion.cpp $(LIBOPT)pd$(LIBAFT) $(LIBS)
#	@cp $(BINS) ../bin
else # CROSS_COMPILE 
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsgManual$(BINEXT) $(ALTI_HOME)/bin
endif # CROSS_COMPILE
else # MIPS64_LINUX
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsgManual$(BINEXT) $(ALTI_HOME)/bin
endif # MIPS64_LINUX
else # ELDK_LINUX
	$(COPY) ~/$(PRODUCT_PREFIX)genErrMsgManual$(BINEXT) $(ALTI_HOME)/bin
endif # ELDK_LINUX


ErrorFiles : $(ALLMSB)

E_ID_US7ASCII.msb : $(BINS) E_ID_US7ASCII.msg
	$(GENERRMSG) -i E_ID_US7ASCII.msg -o $(E_HEADER)
	cp -f $(E_HEADER) ID_TRC_CODE.ih ID_TRC_CODE.ic ../include	
	sync

$(ALLMSB): E_ID_US7ASCII.msb
	$(COPY) $^ $@

manual: $(GENERRMSGMANUAL)
	$(GENERRMSGMANUAL) -i E_ID_US7ASCII.msg  > $(ALTI_HOME)/msg/E_ID_US7ASCII.txt


check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/id/msg core *.d $(PRODUCT_PREFIX)genErrMsg \
		SunWS_cache *.msb *.ih *~ .shobj $(BINS) genErrMsg.$(OBJEXT)

clean_bin:
	$(RM) $(BINS)

.PHONY: all ErrorFiles

-include $(OBJS:.$(OBJEXT)=.d)
-include genErrMsg.d
-include $(TARGET_DIR)/src/id/msg/genErrMsgManual.d
