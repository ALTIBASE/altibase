#
# $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $
#

include ../rpEnv.mk

SRCS= $(RPC_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))


all: compile

compile: $(OBJS)
compile_pp: $(PPOBJS)
compile_pg: $(PGOBJS)
compile_pt: $(PTOBJS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) core .dependency $(OBJS) *.d SunWS_cache *~

-include $(OBJS:.$(OBJEXT)=.d)
