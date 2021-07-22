include ../cmEnv.mk

SRCS=$(CMX_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))


all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) core .dependency $(OBJS) *~

-include $(OBJS:.$(OBJEXT)=.d)
