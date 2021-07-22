include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk
include $(UL_DIR)/lib/ul_objs.mk

SRCS=$(ULS_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))

INCLUDES += $(foreach i, $(ST_DIR)/include $(MT_DIR)/src/include $(ID_DIR)/idk, $(IDROPT)$(i))
INCLUDES += $(IDROPT)$(UL_DIR)/include/unix-odbc

all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/ul/uls core* .dependency *~
	$(RM) $(OBJLIST)

-include $(OBJS:.$(OBJEXT)=.d)
