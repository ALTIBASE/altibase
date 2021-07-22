# $Id: Makefile 19393 2006-12-12 04:25:44Z sjkim $
#

include ./toolEnv.mk

DIRS += altipkg license

ifeq "$(ALTIBASE_PRODUCT)" "xdb"
DIRS += dumpshm
endif

all: compile

.PHONY: $(DIRS)

$(DIRS):
	$(MAKE) -k -C $@ $(SUBDIR_TARGET)

compile:
	$(MAKE) $(S) $(DIRS)  SUBDIR_TARGET=all

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

