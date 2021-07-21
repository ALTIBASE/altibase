include ../cmEnv.mk

SRCS=$(CMN_SRCS)
OBJS=$(patsubst $(DEV_DIR)/%, $(TARGET_DIR)/%.$(OBJEXT), $(basename $(SRCS)))


all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) core .dependency $(OBJS) *~

-include $(OBJS:.$(OBJEXT)=.d)
