# Generated automatically from Makefile.in by configure.

# $Id: Makefile 87881 2020-06-29 08:04:01Z kclee $

include idEnv.mk

IDSRC	= $(ID_DIR)

DIRS	= msg ida idc idd ide idk idl idm idn ids idt idu idp idv idw idf idx idCore
ifeq "$(OS_TARGET)" "PC_CYGWIN"
DIRS	= ida idc ide idd idl idm idn ids idt idu idp idv idf idx
endif # PC_CYGWIN
ifeq "$(OS_LINUX_PACKAGE)" "ARM_WINCE"
DIRS	= msg ida idc idd ide idl idm idn ids idt idu idp idf idx
endif # ARM_WINCE

GREPED_FILE = grep.txt
RESULT_FILE = result.txt

REVISION_HEADER = $(DEV_DIR)/src/id/include/iduRevision.h

all: $(REVISION_HEADER) lib bin
.PHONY: $(DIRS) lib
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

all: ofiles

include moddep.mk

$(REVISION_HEADER):
ifeq "$(OS_TARGET)" "INTEL_WINDOWS"
	@bash idMakeRev.sh > $(REVISION_HEADER)
else
	@sh ./idMakeRev.sh > $(REVISION_HEADER)
endif


ofiles:
ifneq "$(OS_TARGET)" "INTEL_WINDOWS"  # temporary close
ifeq ($(DO_UNITTEST), yes)
ifeq ($(compile64), 1)
	@echo
	@echo
	@echo "Checking This Platform Runtime-Checking. Check your Runtime Env if stop"
	@echo
	@echo
	-@if [ $(ENV_TEST) = 1 ]; then cd chk; $(MAKE); fi
	-@if [ -f ./chk/$(RESULT_FILE) ]; then grep "Hint Sys Error" ./chk/$(RESULT_FILE); fi
	@if [ ! -f ./chk/$(GREPED_FILE) ]; then echo "Check Env Failure"; exit 1; fi
	@echo
	@echo
	@echo "OK..Runtime Checking SUCCESS!!"
	@echo
	@echo
endif # compile64
endif # DO_UNITTEST
endif # INTEL_WINDOWS
	@echo "DIRS is ${DIRS}";
	$(Q) $(MAKE) $(S) $(DIRS) SUBDIR_TARGET=all

build_ut:
	$(MAKE) -s -C unittest

clean_ut:
	$(MAKE) -s -C unittest clean

lib: ofiles
	$(MAKE) -C lib lib

link_solib: lib shobjs
	$(MAKE) -C lib solib

link_piclib: lib shobjs
	$(MAKE) -C lib piclib

shobjs:
	$(MAKE) -C lib shobjs

ifeq "$(OS_TARGET)" "X86_64_DARWIN"
bin: lib link_piclib 
else
bin: lib link_piclib link_solib
endif

check:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=check

pmake:
	cd chk; $(MAKE) clean; $(MAKE)
	cd msg; $(MAKE)
	for i in $(DIRS); do (xterm -e ../../pmake.sh $$i & 2>/dev/null ) done

pmake2:
	cd chk; $(MAKE) clean; $(MAKE)
	cd msg; $(MAKE)
	$(Q) for i in $(DIRS); do (cd $$i; $(MAKE) $(S) & ) done

alint:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=alint

loop:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=$(TARGET)

clean:
	$(RM) $(REVISION_HEADER)
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	$(MAKE) -C chk clean
	$(MAKE) -C lib clean
	$(MAKE) clean_ut

tags tag:
	\$(RM) tags
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name "*.i" \) -exec ctags -a -h ".h.i" {} \;

ctags:
	find `pwd` \( -name "*.[ih]" -o -name "*.cpp" -o -name "*.i" \) -exec ctags -f ../../tags --language-force=C -a -h ".h.i" {} \;
	-$(RM) tags
	-ln -s ../../tags .

gen_rec_list :
	@for i in $(ID_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat
