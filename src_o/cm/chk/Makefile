include ../cmEnv.mk

LIBDIRS := $(LDROPT)$(ALTI_HOME)/lib
INCLUDES := $(INCLUDES) $(IDROPT). $(IDROPT)../../src/include

BINS = checkIPC$(BINEXT) checkSock$(BINEXT)


all: $(BINS) check


checkIPC$(BINEXT): checkIPC.$(OBJEXT)
	$(LD) $(LFLAGS) $(PTDIRS) $(LDOUT)$@ $^ $(LIBDIRS) $(LIBOPT)id$(LIBAFT) $(LIBOPT)pd$(LIBAFT) $(LIBS)

checkSock$(BINEXT): checkSock.$(OBJEXT)
	$(LD) $(LFLAGS) $(PTDIRS) $(LDOUT)$@ $^ $(LIBDIRS) $(LIBOPT)id$(LIBAFT) $(LIBOPT)pd$(LIBAFT) $(LIBS)

check:
	./checkIPC$(BINEXT)
	./checkSock$(BINEXT)

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) $(BINS) core $(OBJS) *.d *~
