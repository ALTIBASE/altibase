#
# $Id: Makefile 70397 2017-11-17 17:16:00Z kitaek.lee $
#

include ../ulEnv.mk
include $(UL_DIR)/lib/ul_objs.mk

INCLUDES += $(foreach i, $(QP_DIR)/include $(MM_DIR)/include $(ID_DIR)/idk, $(IDROPT)$(i))

all: $(OBJS)

alint: $(SRCS:.c=.lint)

clean:
	$(RM) core* .dependency *.$(OBJEXT) *~
	$(RM) $(OBJLIST)
