include ../ulEnv.mk
include $(UL_DIR)/lib/ul_objs.mk

ULA_SRCS_ALL = $(ULA_SRCS) $(ULA_API_SRCS)

ALA_STATIC_OBJS   = $(patsubst $(DEV_DIR)/src/%,$(TARGET_DIR)/%_aoc.$(OBJEXT),$(basename $(ULA_SRCS_ALL)))
ALA_SHARED_OBJS   = $(patsubst $(DEV_DIR)/src/%,$(TARGET_DIR)/%_soc.$(OBJEXT),$(basename $(ULA_SRCS_ALL)))

ALA_OBJS_ALL = $(ALA_STATIC_OBJS) $(ALA_SHARED_OBJS)

OBJLIST += $(ALA_OBJS_ALL)

all:

clean:
	$(RM) $(TARGET_DIR)/src/ul/ula core .dependency *~
	$(RM) $(OBJLIST)
