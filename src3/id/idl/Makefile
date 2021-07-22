# Makefile for ID interface
#
# CVS Info : $Id: Makefile 70906 2015-05-21 08:10:22Z heesuk.shin $
#

include ../idEnv.mk

INCLUDES += $(IDROPT)./

SRCS = $(IDL_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: ofiles

ofiles:	$(OBJS)
shobjs: $(VSHOBJS)

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	-$(RM) $(TARGET_DIR)/src/id/idl core *.d SunWS_cache .shobj

-include $(OBJS:.$(OBJEXT)=.d)
