# Makefile for ID interface
#
# CVS Info : $Id: Makefile 43424 2011-03-16 05:48:37Z gurugio $
#

include ../idEnv.mk


INCLUDES += $(IDROPT)./

SRCS = $(IDCORE_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

LDFLAGS  += -lalticore$(ALTICORE_VERSION)
UNITTEST_LIBS = $(LIBOPT)pd$(LIBAFT) $(ALTICTEST_LIB) $(ALTICORE_LIB)

all: ofiles

ofiles:	$(OBJS)
shobjs: $(VSHOBJS)

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	-$(RM) $(TARGET_DIR)/src/id/idCore core *.d SunWS_cache .shobj

-include $(OBJS:.$(OBJEXT)=.d)
