# $Id: Makefile 84991 2019-03-11 09:21:00Z andrew.shin $

include mtEnv.mk

DIRS = msg mtc mtd mtl mtv mtf mtk mtu mtx lib
include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

CDIRS = mtcc mtcd mtcl mtca mtcn

.PHONY: all
all: compile
.PHONY: $(DIRS) $(CDIRS)

$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

$(CDIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

.PHONY: link_solib
link_solib: compile
	$(MAKE) -C lib solib

clib: ccompile
	$(MAKE) -C lib clib

compile:
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=

ccompile:
	@echo "CDIRS is ${CDIRS}";
	$(MAKE) $(S) $(CDIRS) SUBDIR_TARGET=

shobjs:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=shobjs

pmake2:
	cd msg; $(MAKE)
	$(Q) for i in $(DIRS); do (cd $$i; $(MAKE) $(S) & ) done

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	cd lib && $(MAKE) clean
	$(MAKE) clean_ut

cclean:
	$(MAKE) $(S) $(CDIRS) SUBDIR_TARGET=clean
	cd lib && $(MAKE) cclean

ctags:
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -f ../../tags -a {} \;
	 -$(RM) tags
	-ln -s ../../tags .

gen_rec_list :
	@for i in $(MT_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat

build_ut:
	$(MAKE) -s -C unittest

clean_ut:
	$(MAKE) -s -C unittest clean
