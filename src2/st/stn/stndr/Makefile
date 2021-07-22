# Makefile for SM disk modules
#
# CVS Info : $Id: Makefile 70913 2015-05-21 09:03:59Z sungminee $
#

include ../../stEnv.mk

SRCS =  $(STNDR_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: ofiles 

ofiles:	$(OBJS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/st/stn/stndr core *.d SunWS_cache

-include $(OBJS:.$(OBJEXT)=.d)
