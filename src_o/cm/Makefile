include cmEnv.mk

DIRS = msg cmb cmi cmm cmn cmp cmt cmu cmx lib

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

.PHONY: all
all: compile

.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

.PHONY: link_solib
link_solib: compile
	$(MAKE) -C lib solib

.PHONY: compile
compile:
	@if [ $(ENV_TEST) = 1 ]; then $(MAKE) $(S) chk SUBDIR_TARGET=; fi
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=

alint:
	$(MAKE) $(DIRS) SUBDIR_TARGET=alint

clean:
	$(MAKE) $(DIRS) SUBDIR_TARGET=clean
	$(MAKE) -C lib clean
	$(MAKE) clean_ut

gen_rec_list :
	@for i in $(CM_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat

build_ut:

clean_ut:

