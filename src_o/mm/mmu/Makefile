include ../mmEnv.mk

SRCS=$(MMU_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))


all: $(COMPILE)

compile: $(OBJS)

mmuDump : mmuDump.$(OBJEXT) 
	$(LD) $(LFLAGS) -o $@ mmuDump.$(OBJEXT) $(SERVER_LIBS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/mm/mmu core .dependency SunWS_cache

-include $(OBJS:.$(OBJEXT)=.d)
