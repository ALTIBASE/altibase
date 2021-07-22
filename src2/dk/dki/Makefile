#
# $Id$
#

include ../dkEnv.mk

SRCS= $(DKI_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

DIRS = dkiv dkif dkiq dkis

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

all: odirs compile 

compile: $(OBJS)
compile_pp: $(PPOBJS)
compile_pg: $(PGOBJS)
compile_pt: $(PTOBJS)

.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

odirs:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	$(RM) core .dependency $(OBJS) *.d SunWS_cache *~

-include $(OBJS:.$(OBJEXT)=.d)
