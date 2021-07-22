#*****************************************************************************
# * $Id: Makefile 82185 2018-02-02 10:49:37Z hess $
# ***************************************************************************/

include ../toolEnv.mk

INCLUDES += -I. 
LIBDIRS  += -L$(ID_DIR)/src/lib/

SRCS= accs.cpp accsCPP_yacc.cpp accsCPP_lex.cpp accsMgr.cpp  \
      accsProperty_yacc.cpp accsProperty_lex.cpp accsPropertyMgr.cpp \
      accsSymbol_yacc.cpp accsSymbol_lex.cpp accsSymbolMgr.cpp accsSymbolItem.cpp
OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

all: accs

accs : $(OBJS)
	$(LD) $(LFLAGS)  -o $@ $(OBJS) $(CLI_SA_LIBS) 

.cpp.$(OBJEXT) :
	$(COMPILE.cc) $(INCLUDES) -DMT_UNIX_CLIENT $(CC_OUTPUT_FLAG)$@ $<

accsCPP_yacc.cpp : accsCPP.y
	$(BISON) -p CPP_ -d -t -v -o accsCPP_yacc.cpp $<
accsCPP_lex.cpp : accsCPP.l
	$(FLEX)  -PCPP_ -Cfar  -oaccsCPP_lex.cpp $<

accsProperty_yacc.cpp : accsProperty.y
	$(BISON) -p property_ -d -t -v -o $@ $<
accsProperty_lex.cpp : accsProperty.l
	$(FLEX)  -Pproperty_ -Cfar  -o$@ $<

accsSymbol_yacc.cpp : accsSymbol.y
	$(BISON) -p symbol_ -d -t -v -o $@ $<
accsSymbol_lex.cpp : accsSymbol.l
	$(FLEX)  -Psymbol_ -Cfar  -o$@ $<

clean :
	$(RM)  *.$(OBJEXT) *~ accsCPP_yacc.cpp accsCPP_yacc.cpp.h accsCPP_lex.cpp *.output accs accsProperty_yacc.cpp accsProperty_lex.cpp accsProperty_yacc.cpp.h accsSymbol_yacc.cpp accsSymbol_lex.cpp accsSymbol_yacc.cpp.h 

test :
	./accs <./sample/sample_comment.cpp

os :
	./accs < ./sample/OS.h

comment :
	./accs < ./sample/sample_comment.cpp

prep :
	./accs < ./sample/sample_prep.cpp

string :
	./accs < ./sample/sample_string.cpp

typedef :
	./accs < ./sample/sample_typedef.cpp

define :
	./accs < ./sample/sample_define.cpp

suffix :
	./accs < ./sample/sample_typesuffix.cpp

-include $(OBJS:.$(OBJEXT)=.d)
