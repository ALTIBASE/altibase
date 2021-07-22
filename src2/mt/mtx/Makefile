# $Id: Makefile 84814 2019-01-28 04:13:56Z andrew.shin $

include ../mtEnv.mk

SRCS = $(MTX_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

shobjs: $(VSHOBJS)

compile: $(OBJS)

clean:
	$(RM) $(TARGET_DIR)/src/mt/mtx *~ .shobj

-include $(OBJS:.$(OBJEXT)=.d)
