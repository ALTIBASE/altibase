include ../qpEnv.mk

SRCS= $(QMS_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

compile: $(OBJS)

clean:
	$(RM) $(OBJS) *~

-include $(OBJS:.$(OBJEXT)=.d)
