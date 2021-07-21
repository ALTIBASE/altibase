# $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $

include ../qpEnv.mk

SRCS= $(QTC_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

compile: $(OBJS)

clean:
	$(RM) $(OBJS) *~

-include $(OBJS:.$(OBJEXT)=.d)
