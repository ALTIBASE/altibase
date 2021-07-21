# Generated automatically from Makefile.in by configure.

# $Id: Makefile 80299 2017-06-18 04:00:46Z et16 $

include smEnv.mk

SMSRC	= $(SM_DIR)

DIRS	= msg smu smm sct sdn sdd sdb sdt sds sdr sdp sdc smn smr smp smt smc smx sml sma smi svm svp svc svr svn sgm lib
include $(ALTIDEV_HOME)/makefiles/moddep-common.mk
include moddep.mk

ifneq "$(OS_LINUX_PACKAGE)" "ARM_WINCE"
DIRS	+= util
else # ARM_WINCE
DIRS	+= win32-build/CE_smCreateDB
endif # ARM_WINCE
# sma smp smc smi smn sml smr smt smx  smm sms  util


all:	ofiles
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

ofiles:
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=

link_solib: ofiles
	$(MAKE) -C lib solib

check :
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=check

gen_rec_list :
	@for i in $(SM_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done > $(ALTI_HOME)/conf/recovery.dat

gen_rec_doc :
	@for i in $(SM_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakeDoc.awk ) done | grep -v "total count 0" >> $(ALTI_HOME)/conf/recovery_doc.dat

alint:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=alint

pmake:
	cd msg; $(MAKE) 
	for i in $(DIRS); do (xterm -e ../../pmake.sh $$i & 2>/dev/null ) done

pmake2:
	cd msg; $(MAKE) 
	$(Q) for i in $(DIRS); do (cd $$i; $(MAKE) $(S) & ) done

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	$(MAKE) -C lib clean
	$(RM) *~ *.*~ 
	$(MAKE) clean_ut

tags tag:
	\$(RM) TAGS
	etags ./*/*.cpp ./*/*/*.cpp ./*/*.h

ctags:
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name dummy -prune \)  -exec ctags -f ../../tags -a {} \;
	-$(RM) tags
	-ln -s ../../tags .

build_ut:
	$(MAKE) -s -C unittest

clean_ut:
	$(MAKE) -s -C unittest clean
