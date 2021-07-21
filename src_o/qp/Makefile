#
# $Id: Makefile 87559 2020-05-21 05:17:13Z jake.jang $
#

include qpEnv.mk

DIRS=msg qtc qcc qcd qcm qcu qcp qcg qdb qdc qdd qdm qdn qdr qds qdx qdv qdp qdt qmc qmv qmn qmo qmg qms qmx qmr qci qrc qsf qsv qso qsx qss qsc qdq qcs qdsd lib

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

all: compile
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

compile:
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=

link_solib: compile
	$(MAKE) -C lib solib

archive:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=archive;

alint:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=alint;

pmake:
	cd msg; $(MAKE);
	for i in $(DIRS); do (xterm -e ../../pmake.sh $$i & 2>/dev/null ) done

pmake2:
	cd msg; $(MAKE);
	$(Q) for i in $(DIRS); do (cd $$i; $(MAKE) $(S) & ) done

install:
	cd main; $(MAKE) $(INSTALL)

check :
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=check;

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	$(MAKE) -C lib clean
	$(MAKE) clean_ut

clearall:
	for i in $(DIRS); do (cd $$i; $(RM) core $(OBJS) *.p *.log .pure SunWS_cache;) done

sure:

pure:
	$(MAKE) INSTALL=install_pure

tags tag:
	-$(RM) tags
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -a {} \;

ctags:
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -f ../../tags -a {} \;
	-$(RM) tags
	-ln -s ../../tags .

gen_rec_list :
	@for i in $(QP_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat

build_ut:
	$(MAKE) -s -C unittest

clean_ut:
	$(MAKE) -s -C unittest clean
