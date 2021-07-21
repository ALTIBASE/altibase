#
# If you want to modify flex, bison files, you need flex version upper than 2.5.35.
# Then, do 'make parser'
#

include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk
include ./ulp_objs.mk

DIRS = lib

include $(ALTIDEV_HOME)/makefiles/moddep-common.mk

BINS = apre$(BINEXT)

GENSTUFFS = ulpCompl.cpp ulpCompy.cpp ulpCompy.cpp.h ulpCompy.hpp ulpCompy.cpp.output ulpCompy.output ulpPreprocl.cpp ulpPreprocy.cpp ulpPreprocy.cpp.h ulpPreprocy.hpp ulpPreprocy.cpp.output ulpPreprocy.output ulpPreprocifl.cpp ulpPreprocify.cpp ulpPreprocify.cpp.h ulpPreprocify.hpp ulpPreprocify.cpp.output ulpPreprocify.output

FLEXFILES = ulpCompl.cpp ulpPreprocl.cpp

INCLUDES += $(foreach i, ./include ., $(IDROPT)$(i))

# parse error가 났을 때, 보다 자세한 정보를 출력하는게 좋다.
CCFLAGS += -DYYERROR_VERBOSE
DCCFLAGS += -DYYERROR_VERBOSE

LFLAGS  += $(foreach i, $(ALTI_HOME)/lib $(ALTICORE_LIBDIR),  $(LDROPT)$(i))

OBJS = $(ULP_GEN_OBJS) $(ULP_BIN_OBJS)

ULP_BIN_OBJS = $(ULP_BIN_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

ULP_GEN_OBJS = $(ULP_GEN_SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

LEX = $(FLEX)

LEXFLAGS = -8 -Car

ULP_DEP_LIBS = $(foreach i, id pd, $(LIBOPT)$(i)$(LIBAFT)) \
	$(ALTICORE_STATIC_LIB) $(LIBS)

ULPARC = $(LIBPRE)apreobj.$(LIBEXT)

SRCS = $(ULP_GEN_SRCS) $(ULP_BIN_SRCS)

YACC = $(BISON)

YACCFLAGS = -d -v

.PHONY: all $(DIRS) compile parser genparser archive install header check alint clean dep

all: compile

compile: $(DIRS) archive $(BINS) install

parser: $(DIRS) genparser archive $(BINS) install

$(ULP_GEN_OBJS): $(ULP_GEN_SRCS)

$(DIRS):
	$(MAKE) -C $@ $(SUBDIR_TARGET)

genparser: $(foreach file, $(ULP_GEN_SRCS), $(notdir $(file)))

#=========== Lexer ==========#
ulpCompl.cpp: ulpCompl.l
	$(LEX) $(LEXFLAGS) -o$*.cpp $*.l
ulpPreprocl.cpp: ulpPreprocl.l
	$(LEX) $(LEXFLAGS) -o$*.cpp $*.l
ulpPreprocifl.cpp: ulpPreprocifl.l
	$(LEX) $(LEXFLAGS) -o$*.cpp $*.l

#=========== Parser ==========#
ulpPreprocy.cpp: ulpPreprocy.y
ifeq "$(BISON_ENV_NEEDED)" "yes"
	$(MAKE) $@ BISON_SIMPLE=$(BISON_SIMPLE_PATH) BISON_HAIRY=$(BISON_HAIRY_PATH) BISON_ENV_NEEDED=no
else
	$(YACC) $(YACCFLAGS) -p PP -o $*.cpp $*.y
endif

ulpPreprocify.cpp: ulpPreprocify.y
ifeq "$(BISON_ENV_NEEDED)" "yes"
	$(MAKE) $@ BISON_SIMPLE=$(BISON_SIMPLE_PATH) BISON_HAIRY=$(BISON_HAIRY_PATH) BISON_ENV_NEEDED=no
else
	$(YACC) $(YACCFLAGS) -p PPIF -o $*.cpp $*.y
endif

ulpCompy.cpp: ulpCompy.y
ifeq "$(BISON_ENV_NEEDED)" "yes"
	$(MAKE) $@ BISON_SIMPLE=$(BISON_SIMPLE_PATH) BISON_HAIRY=$(BISON_HAIRY_PATH) BISON_ENV_NEEDED=no
else
	$(YACC) $(YACCFLAGS) -p COMP -o $*.cpp $*.y
endif
#=============================#

archive: $(ULPARC)

$(ULPARC): $(ULP_GEN_OBJS) $(ULP_BIN_OBJS)

$(BINS): $(ULP_GEN_OBJS) $(ULP_BIN_OBJS)
	@echo " LD $@"
	@$(LD) $(LFLAGS) $(LDOUT)$@ $^ $(ULP_DEP_LIBS)

$(TARGET_DIR)/%.$(OBJEXT): $(DEV_DIR)/%.cpp
	$(COMPILE_IT)

install: $(BINS) $(ULPARC)
	$(COPY) $(BINS) $(ALTI_HOME)/bin/$(BINS) 
	$(COPY) $(BINS) $(ALTI_HOME)/bin/sesc$(BINEXT) 
	$(RM)  $(BINS)
header:
	@for i in $(SRCS); do ( $(COPY) $$i $$i.org ; cat $$i.org | gawk -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check:
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/ul/ulp core .dependency *.d SunWS_cache $(ULPARC) $(BINS)
	$(MAKE) $(S) $(DIRS) SUBDIR_TARGET=clean

-include $(OBJS:.$(OBJEXT)=.d)
