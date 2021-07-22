##############################################################################
# $Id: Makefile 68602 2015-01-23 00:13:11Z sbjang $
##############################################################################

CXX = cxx
LD  = ld
CXXFLAGS = -I. $(DEBUG_FLAG)
LFLAGS  += $(DEBUG_FLAG)
ATOM_FLAG = -all -pthread -L. -toolargs="1024 16 4"   $(DEBUG_FLAG) $(NONDEBUG_FLAG)

DEBUG_FLAG= -g
#NONDEBUG_FLAG = -Wca,O0 

LIB_INST  = lib_cprof_inst.o
LIB_ANAL  = lib_cprof_anal.o

INST_SRC  = csInstrument.cpp 
ANAL_SRC  = csInterface.cpp csSimul.cpp csStat.cpp

INST_OBJ  = $(INST_SRC:.cpp=.$(OBJEXT))
ANAL_OBJ  = $(ANAL_SRC:.cpp=.$(OBJEXT))

ALL_OBJ   = $(INST_OBJ) $(ANAL_OBJ)

all : lib sample.prof  testStat #testSimulator 

sample.prof : sample lib
	cprof sample sample.prof 1024 32 2

sample : sample.$(OBJEXT)  libcs.a
	$(CXX) $(CXXFLAGS) -L. -o sample sample.$(OBJEXT) -lcs

testSimulator : testSimulator.$(OBJEXT) csSimul.o
	$(CXX) $(LFLAGS) -o testSimulator testSimulator.$(OBJEXT) csSimul.o

testStat : testStat.$(OBJEXT) csStat.o
	$(CXX) $(LFLAGS) -o testStat testStat.$(OBJEXT) csStat.o


##################################################################
#                         Library                                #
##################################################################
lib : libcs.a $(LIB_INST) $(LIB_ANAL)
	cp $(LIB_INST) $(ALTI_HOME)/lib
	cp $(LIB_ANAL) $(ALTI_HOME)/lib

libcs.a : csProto.o
	$(LD) -r -o libcs.a csProto.$(OBJEXT) -lcxx -lexc

$(LIB_INST) : $(INST_OBJ)
	$(LD) -r -o $(LIB_INST) $(INST_OBJ) -lcxx -lexc

$(LIB_ANAL) : $(ANAL_OBJ)
	$(LD) -r -o $(LIB_ANAL) $(ANAL_OBJ) -lcxx -lexc

.cpp.$(OBJEXT) :
	$(CXX) $(CXXFLAGS) -c $< -o $@ 

clean :
	$(RM) *.atom *.$(OBJEXT) $(ALL_OBJ) $(LIB_INST) $(LIB_ANAL) sample.prof sample


#sample.prof : sample sample.inst.$(OBJEXT) sample.anal.o
#	atom $(ATOM_FLAG) sample sample.inst.$(OBJEXT) sample.anal.$(OBJEXT) -o sample.prof 

#sample.anal.$(OBJEXT) : anal1.$(OBJEXT) anal2.o
#	$(LD) -r -o sample.anal.$(OBJEXT) anal1.$(OBJEXT) anal2.$(OBJEXT) -lcxx -lexc

#sample.inst.$(OBJEXT) : inst1.$(OBJEXT) inst2.o
#	$(LD) -r -o sample.inst.$(OBJEXT) inst1.$(OBJEXT) inst2.$(OBJEXT) -lcxx -lexc
ABC=make
test :
	sh sh2.sh
