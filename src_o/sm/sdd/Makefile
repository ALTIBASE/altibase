# Makefile for SM disk modules
#
# CVS Info : $Id: Makefile 70397 2015-04-20 01:43:07Z heesuk.shin $
#

include ../smEnv.mk

SRCS =  $(SDD_SRCS)


OBJS		= $(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: ofiles 

ofiles:	$(OBJS) $(OBJS_CRDB)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) core $(OBJS) *.d SunWS_cache

main : main.o
	$(LD) $(LFLAGS) -o $@ $<  $(SM_DRIVER_OBJS) -lsm  $(ID_LIB) $(LIBS)

purify:
	$(RM) main.purify
	$(MAKE) main.purify
	./main.purify

main.purify : main.o
	purify $(LD) $(LFLAGS) -o $@ $<  $(SM_DRIVER_OBJS) -lsm  $(ID_LIB) $(LIBS)

-include $(OBJS:.$(OBJEXT)=.d)
