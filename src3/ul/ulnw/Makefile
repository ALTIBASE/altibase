include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk
include $(UL_DIR)/lib/ul_objs.mk

SRCS=$(ULNW_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) core* .dependency *.$(OBJEXT) *~
	$(RM) $(OBJLIST)

-include $(OBJS:.$(OBJEXT)=.d)
