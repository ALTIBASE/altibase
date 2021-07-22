# Makefile for ID interface
#
# CVS Info : $Id: Makefile 26804 2008-07-01 02:06:37Z kimmkeun $
#

include ../idEnv.mk

SRCS = $(IDF_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: ofiles

ofiles:	$(OBJS)

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

clean:
	-$(RM) $(TARGET_DIR)/src/id/idf core *.d SunWS_cache .shobj

-include $(OBJS:.$(OBJEXT)=.d)
