include ../ulEnv.mk
include $(UL_DIR)/lib/ul_objs.mk

INCLUDES += $(foreach i, $(QP_DIR)/include $(MM_DIR)/include $(ID_DIR)/idk, $(IDROPT)$(i))
INCLUDES += $(IDROPT)$(UL_DIR)/include/unix-odbc

all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) core* .dependency *.$(OBJEXT) *~
	$(RM) $(OBJLIST)
