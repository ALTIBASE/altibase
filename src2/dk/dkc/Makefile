#
# $Id$
#

include ../dkEnv.mk

GENSRCS= dkcParser.cpp dkcLexer.cpp
GENSTUFFS= dkcParser.output dkcParser.hpp

LEX=$(FLEX)
LEXFLAGS=-Cfar

YACC= $(BISON)
YACCFLAGS=-d -t -v

SRCS= $(DKC_SRCS)
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: compile

compile: $(GENOBJS) $(OBJS)
compile_pp: $(PPOBJS)
compile_pg: $(PGOBJS)
compile_pt: $(PTOBJS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) core .dependency $(OBJS) *.d SunWS_cache *~ $(GENSRCS) $(GENSTUFFS)

.SECONDARY:

%.cpp: %.l
	$(LEX)  $(LEXFLAGS)  -o$*.cpp $*.l
	$(SED) s,$(FLEX_REPLACE_BEFORE),$(FLEX_REPLACE_AFTER), < $*.cpp > $*.cpp.old
	$(RM) -f $*.cpp
	mv $*.cpp.old $*.cpp

%.cpp: %.y
ifeq "$(BISON_ENV_NEEDED)" "yes"
	$(MAKE) $@ BISON_SIMPLE=$(BISON_SIMPLE_PATH) BISON_HAIRY=$(BISON_HAIRY_PATH) BISON_ENV_NEEDED=no
else
	$(YACC) $(YACCFLAGS) -p $(subst .cpp,_,$(notdir $@)) -o $*.cpp $*.y
endif

-include $(OBJS:.$(OBJEXT)=.d)
