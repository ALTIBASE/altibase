# Generated automatically from Makefile.in by configure.

# $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $

include pdEnv.mk

DIRS	= pdl $(ALTIDEV_PORTPKG_PATH) 

all:	lib bin
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

ofiles:
	@echo "DIRS is ${DIRS}";
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=ofiles

lib: ofiles
	$(MAKE) -C lib

link_solib: lib
	$(MAKE) -C lib solib

bin: lib link_solib

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
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	$(MAKE) clean -C lib
	$(RM) -f *~ *.*~

tags tag:
	\$(RM) tags
	find `pwd` \( -name "*.h" -o -name "*.cpp" \) -exec ctags -a {} \;

ctags:
	find `pwd` \( -name "*.h" -o -name "*.cpp" -o -name "*.i" \) -exec ctags -f ../../tags --language-force=C -a {} \;
	-$(RM) tags
	-ln -s ../../tags .

gen_rec_list :
	@for i in $(PD_SRCS); do (gawk -v SRC_INPUT=$$i -f $(TOOL_DIR)/recovery_util/smrMakePoint.awk ) done >> $(ALTI_HOME)/conf/recovery.dat
