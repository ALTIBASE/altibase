# $Id: Makefile 82326 2018-02-22 10:00:47Z lswhh $

include ../idEnv.mk
include $(DEV_DIR)/alticore.mk

SRCS    = checkEnv.cpp
BINS	= $(ALTI_HOME)/bin/checkEnv$(BINEXT)
RESULT_FILE = result.txt
GREPED_FILE = grep.txt
CCFLAGS := ${CCFLAGS} -I.
LDFLAGS  += -lalticore$(ALTICORE_VERSION)
LFLAGS   += $(LIBDIRS) $(LDFLAGS)

INCLUDES += $(IDROPT)$(UL_DIR)/include $(IDROPT)$(UL_DIR)/include/unix-odbc
DEFINES = $(DEFOPT)BUILD_FOR_UTIL
UNITTEST_LIBS = $(LIBOPT)pd$(LIBAFT) $(ALTICTEST_LIB) $(ALTICORE_LIB)

ifeq ($(DO_UNITTEST), yes)
ifeq ($(compile64), 1)
all: $(ALTI_HOME)/bin/checkPlatform 
else
# BUGBUG 32bit 컴파일시 링크에러 발생하기 때문에 수정하기 전까지 막아둠.
# $(DEV_DIR)/rule.mk와 동시에 수정해야 한다.
all: fake
endif
else
all: fake
.PHONY: fake
endif

fake :
	touch $(GREPED_FILE)

## BUGBUG : test 프로그램을 다시 정교하게 작성 요망.
$(ALTI_HOME)/bin/checkPlatform : $(ALTI_HOME)/bin/checkEnv$(BINEXT)
	@echo
	@echo "Testing Runtime Env..."
	@echo
	$(RM) $(RESULT_FILE) $(GREPED_FILE)
	$(ALTI_HOME)/bin/checkEnv$(BINEXT) > $(RESULT_FILE)
	-grep "Hint Sys Error" $(RESULT_FILE) > $(GREPED_FILE)
ifeq "$(OS_TARGET)" "INTEL_WINDOWS"
	$(ALTI_HOME)/bin/checkEnv$(BINEXT)
endif

$(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT): $(ID_DIR)/idl/idl.cpp
	mkdir -p $(dir $@)
	$(CXX) -c $(CCFLAGS) $(CPPFLAGS) $(DEFINES) $(PTDIRS) $(INCLUDES) $(LIBDIRS) -o $@ $<

$(ALTI_HOME)/bin/checkEnv$(BINEXT): $(TARGET_DIR)/src/id/chk/checkEnv.$(OBJEXT) $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT) $(TARGET_DIR)/src/id/idu/iduVersion.$(OBJEXT)
	$(LD) $(LFLAGS) $(LDOUT)$@ $^ $(LIBDIRS) $(UNITTEST_LIBS) $(LIBS)


$(ALTI_HOME)/bin/checkConnect: $(TARGET_DIR)/src/id/chk/checkConnect.$(OBJEXT) $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT)
	$(CXX) $(CCFLAGS) $(CPPFLAGS) $(DEFINES) $(PTDIRS) $(INCLUDES)  $(LIBDIRS) -o $@ $< $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT) $(ID_DIR)/idu/iduVersion.cpp -lpd $(LIBS)

$(ALTI_HOME)/bin/checkIPC: $(TARGET_DIR)/src/id/chk/checkIPC.$(OBJEXT) $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT)
	$(CXX) $(CCFLAGS) $(CPPFLAGS) $(DEFINES) $(PTDIRS) $(INCLUDES)  $(LIBDIRS) -o $@ $< $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT) $(ID_DIR)/idu/iduVersion.cpp -lpd $(LIBS)

$(ALTI_HOME)/bin/checkIPC2: $(TARGET_DIR)/src/id/chk/checkIPC2.$(OBJEXT) $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT)
	$(CXX) $(CCFLAGS) $(CPPFLAGS) $(DEFINES) $(PTDIRS) $(INCLUDES)  $(LIBDIRS) -o $@ $< $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT) $(ID_DIR)/idu/iduVersion.cpp -lpd $(LIBS)

$(ALTI_HOME)/bin/checkCond: $(TARGET_DIR)/src/id/chk/checkCond.$(OBJEXT) $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT)
	$(CXX) $(CCFLAGS) $(CPPFLAGS) $(DEFINES) $(PTDIRS) $(INCLUDES)  $(LIBDIRS) -o $@ $< $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT) $(ID_DIR)/idu/iduVersion.cpp -lpd $(LIBS)

$(ALTI_HOME)/bin/checkType: $(TARGET_DIR)/src/id/chk/checkType.$(OBJEXT) $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT)
	$(CXX) $(CCFLAGS) $(CPPFLAGS) $(DEFINES) $(PTDIRS) $(INCLUDES) $(LFLAGS) $(LIBDIRS) -o $@ $< $(TARGET_DIR)/src/id/idl/idl_utl.$(OBJEXT) $(ID_DIR)/idu/iduVersion.cpp -lpd $(LIBS)

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	$(RM) core $(TARGET_DIR)/src/id/chk/*.$(OBJEXT) *.d $(ALTI_HOME)/bin/checkEnv$(BINEXT) SunWS_cache  *.msb *.ih *~ .shobj *.txt

alint: $(SRCS:.cpp=.lint)

clean_bin:
	$(RM) $(BINS)
