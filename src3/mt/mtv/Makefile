# $Id: Makefile 70913 2015-05-21 09:03:59Z sungminee $

include ../mtEnv.mk

SRCS = $(MTV_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(TARGET_DIR)/src/mt/mtv/mtv_shobj.$(OBJEXT) $(TARGET_DIR)/src/mt/mtv/mtvModulesForClient_shobj.o

compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtv *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
