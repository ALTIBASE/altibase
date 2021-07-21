include ../qpEnv.mk

SRCS= $(QMR_SRCS)

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: $(COMPILE)

compile: $(OBJS)

clean:
	$(RM) $(OBJS) *~

-include $(OBJS:.$(OBJEXT)=.d)
