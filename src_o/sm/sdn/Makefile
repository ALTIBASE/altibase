# Makefile for SM disk modules
#
# $Id: Makefile 80299 2017-06-18 04:00:46Z et16 $
#
# !!! Warning !!! Don't touch directory structure.
#

include ../smEnv.mk

SRCS = $(SDN_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

DIRS = sdnn sdnp sdnb

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

all: odirs ofiles
.PHONY: $(DIRS)
$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)
ofiles:	$(OBJS)

odirs: 
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=
header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean
	$(RM) core $(OBJS) *.d SunWS_cache

-include $(OBJS:.$(OBJEXT)=.d)
