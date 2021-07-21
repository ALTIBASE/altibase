# $Id: Makefile 70906 2015-05-21 08:10:22Z heesuk.shin $
#


include ../idEnv.mk


SRCS = $(IDX_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT)) 

all: ofiles

ofiles:	$(OBJS)

shobjs: $(VSHOBJS) # old command. should be deprecated

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

clean:
	$(RM) $(TARGET_DIR)/src/id/idx core $(SRCS_SA_LINK) *.d SunWS_cache .shobj

-include $(OBJS:.$(OBJEXT)=.d)
