# $Id: Makefile 26804 2008-07-01 02:06:37Z kimmkeun $

include ../mtEnv.mk

SRCS = $(MTCC_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(VSHOBJS)

compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtcc *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
