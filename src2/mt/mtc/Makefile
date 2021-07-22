# $Id: Makefile 70913 2015-05-21 09:03:59Z sungminee $

include ../mtEnv.mk

SRCS = $(MTC_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(VSHOBJS)

compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtc *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
