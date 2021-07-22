# $Id: Makefile 70913 2015-05-21 09:03:59Z sungminee $

include ../mtEnv.mk

SRCS = $(MTF_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(TARGET_DIR)/src/mt/mtf/mtf_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtf/mtfModulesForClient_shobj.o

compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtf *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
