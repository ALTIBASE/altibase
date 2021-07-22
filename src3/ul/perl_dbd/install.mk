include $(ALTIBASE_HOME)/install/altibase_env.mk

all: install

clean:
	$(RM) core .dependency $(BINS) *.$(OBJEXT) *.d SunWS_cache

install:
ifeq "$(OS_TARGET)" "IBM_AIX"
	-cp ${ALTIBASE_HOME}/lib/libodbccli_sl.so ${ALTIBASE_HOME}/lib/libodbccli_sl.a
else
ifeq "$(OS_TARGET)" "IA64_HP_HPUX"
	-cp ${ALTIBASE_HOME}/lib/libodbccli_sl.sl ${ALTIBASE_HOME}/lib/libodbccli_sl.so
else
ifeq "$(OS_TARGET)" "HP_HPUX"
	-cp ${ALTIBASE_HOME}/lib/libodbccli_sl.sl ${ALTIBASE_HOME}/lib/libodbccli_sl.so
endif
endif
endif
	perl Makefile.PL -o "$(ALTIBASE_HOME)" -c "$(CXX)" -f "$(CFLAGS)" -p "$(PIC)" -l "$(SOFLAGS) $(CFLAGS) $(LDFLAGS)"

