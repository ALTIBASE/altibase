# $Id: Makefile 26804 2008-07-01 02:06:37Z kimmkeun $

include ../stEnv.mk

SRCS = $(STU_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(VSHOBJS)

compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/st/stu *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
