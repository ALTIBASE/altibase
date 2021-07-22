DIRS=src

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

all: compile
.PHONY: compile $(DIRS)

$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

compile:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=$(COMPILE)

cdbc_static:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=cdbc_static

cdbc_shared:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=cdbc_shared

dist:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=dist

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
