# Build msg before others in $(DIRS)
ifeq ($(filter msg,$(DIRS)),msg)
.PHONY: msg
$(filter-out msg,$(DIRS)): msg
endif
