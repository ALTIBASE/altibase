include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk
include $(UL_DIR)/lib/ul_objs.mk

SRCS=$(ULU_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/ul/ulu core .dependency *~
	$(RM) $(OBJLIST)

-include $(OBJS:.$(OBJEXT)=.d)
