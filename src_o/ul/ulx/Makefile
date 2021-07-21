include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk
include $(UL_DIR)/lib/ul_objs.mk

SRCS=$(ULX_SRCS)
TEMP_OBJS=$(SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))
OBJS=$(TEMP_OBJS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

INCLUDES += $(foreach i, $(RP_DIR)/include $(QP_DIR)/include $(MM_DIR)/include $(ID_DIR)/idk, $(IDROPT)$(i))
INCLUDES += $(IDROPT)$(UL_DIR)/include/unix-odbc

all: $(OBJS)

alint: $(SRCS:.c=.lint) $(SRCS:.cpp=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/ul/ulx core* .dependency *~
	$(RM) $(OBJLIST)

-include $(OBJS:.$(OBJEXT)=.d)
