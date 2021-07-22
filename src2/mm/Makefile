include mmEnv.mk

DIRS=msg mmc mmd mmm mmq mmt mmt5 mmu lib # mms

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

all: bins #install
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

.PHONY: link_solib
link_solib: compile
	$(MAKE) -C lib solib

.PHONY: bins
bins: compile
	$(MAKE) -C main

mmi_link:
	cd mmi; ./mmiUnlink.sh; ./mmiLink.sh

compile:
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=

archive:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=archive;

install:
	$(MAKE) -C main $(INSTALL)

check :
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=check;

pmake:
	cd msg; $(MAKE)
	for i in $(DIRS); do (xterm -e ../../pmake.sh $$i & 2>/dev/null ) done

pmake2:
	cd msg; $(MAKE)
	$(Q) for i in $(DIRS); do (cd $$i; $(MAKE) $(S) & ) done

alint:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=alint;

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	cd lib && $(MAKE) clean 
	cd main && $(MAKE) clean 
	$(MAKE) clean_ut

clearall:
	for i in $(DIRS); do (cd $$i; $(RM) core $(OBJS) *.p *.log SunWS_cache;) done

sure:

tags tag:
	\$(RM) tags
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -a {} \;

ctags:
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -f ../../tags -a {} \;
	-$(RM) tags
	-ln -s ../../tags .

gen_rec_list :
	@for i in $(MM_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat

build_ut:

clean_ut:

