include ../ulEnv.mk

DIRS = ulcs ulco

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

all: compile
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

lib: compile

compile:
	$(MAKE) $(DIRS) SUBDIR_TARGET=

alint:
	$(MAKE) $(DIRS) SUBDIR_TARGET=alint;

clean:
	$(RM) core .dependency *.$(OBJEXT) *~
	$(MAKE) $(DIRS) SUBDIR_TARGET=clean

