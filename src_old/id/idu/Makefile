# $Id: Makefile 71103 2015-06-03 03:08:44Z heesuk.shin $

include ../idEnv.mk


SRCS = $(IDU_SRCS) $(IDU_CLI_SRCS)
OBJS = $(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT)) \
 $(IDUC_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT)) \
 $(IDUC_CLI_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))
VSHOBJS = $(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%_shobj.$(OBJEXT)) \
          $(IDUC_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%_shobj.$(OBJEXT)) \
          $(IDUC_CLI_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%_shobj.$(OBJEXT))

all: ofiles

ofiles:	$(OBJS)
shobjs: $(VSHOBJS)

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

clean:
	$(RM) $(TARGET_DIR)/src/id/idu core $(SRCS_SA_LINK) *.d SunWS_cache .shobj

ifeq ($(COMPILER_NAME),GCC)
ifneq ($(GCC_VERSION_MAJOR),3)
$(TARGET_DIR)/src/id/idu/iduProperty.$(OBJEXT): CPPFLAGS += -Wno-missing-field-initializers
endif
endif

-include $(OBJS:.$(OBJEXT)=.d)
