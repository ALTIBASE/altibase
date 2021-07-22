#
# $Id: Makefile,v 1.2 2005/09/10 11:34:30 jdlee Exp $
#

include ../stEnv.mk

SRCS= $(STV_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))


all: $(COMPILE)

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
	$(RM) $(TARGET_DIR)/src/st/stv core .dependency *.d SunWS_cache

archive:
	$(AR) $(ARFLAGS) $(STLIB) $(OBJS)

-include $(OBJS:.$(OBJEXT)=.d)
