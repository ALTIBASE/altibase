# $Id: Makefile 74195 2016-01-20 10:23:44Z junyoung $
#

include ./utEnv.mk
include $(PD_DIR)/lib/pd_objs.mk
include $(ID_DIR)/lib/id_objs.mk

CLI_DIRS = #empty
DIRS = ${CLI_DIRS}
ifeq "$(NEED_BUILD_LIBEDIT)" "yes"
    CLI_DIRS += libedit
endif
CLI_DIRS += msg util

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk
#DIRS += checkServer altiProfile aheartbeat adapter
DIRS += checkServer altiProfile aheartbeat 

CLI_DIRS += iloader3 isql utm altiWrap package altiMon

include moddep.mk

DIRS += altiAudit

ifneq "$(ALTIBASE_ENABLE_GCOV)" "yes"
    CLI_DIRS += audit
endif
ifeq "$(OS_TARGET)" "INTEL_WINDOWS"
    DIRS += service
endif

ALTIBASE_ENV_PREFIX := ALTIBASE_

all: compile

.PHONY: $(DIRS)

$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

.PHONY: compile
compile: $(PD_SHOBJS) $(ID_SHOBJS)
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=compile

build_cli:
	@echo "DIRS is ${CLI_DIRS}";
	$(MAKE) $(S) $(CLI_DIRS) SUBDIR_TARGET=compile

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean

alint:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=alint

sure:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=sure

archive:

tags tag:
	-$(RM) tags
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -a {} \;

ctags:
	for i in $(DIRS); do (cd $$i; $(MAKE) $(S) ctags ) done

gen_rec_list:
	@for i in $(UT_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat

build_ut:
	$(MAKE) -s -C unittest

clean_ut:
	$(MAKE) -s -C unittest clean
