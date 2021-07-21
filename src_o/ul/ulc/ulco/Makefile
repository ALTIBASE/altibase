include ../../ulEnv.mk

DIRS = ulcox

all: # nothing do to
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

clean:
	$(RM) $(TARGET_DIR)/src/ul/ulc/ulco core .dependency *~
	$(MAKE) $(DIRS) SUBDIR_TARGET=clean
