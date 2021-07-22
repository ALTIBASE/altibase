include ../mmEnv.mk

PUROPT=-static-checking=yes -static-checking-default=aggressive -static-checking-guardzone=128 -free-queue-length=10000 -free-queue-threshold=100000
PURREBUILD=#-force-rebuild

BINS=$(ALTI_HOME)/bin/altibase$(BINEXT)

ifeq "$(OS_TARGET)" "X86_64_DARWIN"
define cmd_ld
  $(Q) $(if $(quiet),echo ' Linking $@')
  $(Q) $(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SERVER_LIBS) -flat_namespace
endef
else
define cmd_ld
  $(Q) $(if $(quiet),echo ' Linking $@')
  $(Q) $(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SERVER_LIBS)
endef
endif

all: install
	echo $(BINS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) $(TARGET_DIR)/src/mm/main core .dependency $(BINS) *.d SunWS_cache

install: $(BINS)
ifeq "$(ALTIBASE_USE_VALGRIND)" "yes"
	$(COPY) server.valgrind $(ALTI_HOME)/bin/server
else
	$(COPY) server $(ALTI_HOME)/bin
endif

	$(COPY) $(TARGET_DIR)/src/mm/main/altibase.$(OBJEXT) $(ALTI_HOME)/lib
ifeq "$(ALTIBASE_USE_VALGRIND)" "yes"
	chmod 755 $(ALTI_HOME)/bin/altibase$(BINEXT)
else
	chmod 4755 $(ALTI_HOME)/bin/altibase$(BINEXT)
endif



$(ALTI_HOME)/bin/altibase$(BINEXT) : $(TARGET_DIR)/src/mm/main/altibase.$(OBJEXT) $(SERVER_DEPS)
ifeq ($(ALTIBASE_MEMORY_CHECK),yes)
  ifeq ($(ALTIBASE_USE_VALGRIND),no)
	purify $(PUROPT) $(PURREBUILD) $(LD) $(LFLAGS) -o $@ $< $(SERVER_LIBS)
  else
	$(cmd_ld)
  endif
else
	$(cmd_ld)
endif
	$(Q) $(if $(quiet),echo ' Building Altibase DBMS server complete.')

$(ALTI_HOME)/bin/altibase.cprof : $(TARGET_DIR)/src/mm/main/altibase.$(OBJEXT) $(ALTI_HOME)/lib/libmm.a $(ALTI_HOME)/lib/libqp.a $(ALTI_HOME)/lib/libst.a $(ALTI_HOME)/lib/libsm.a
	cd $(TOOL_DIR)/cacheprof; $(MAKE) lib
	$(LD) $(LFLAGS) -o altibase.tmp  altibase.$(OBJEXT) $(SERVER_LIBS) -L$(TOOL_DIR)/cacheprof -lcs
	$(TOOL_DIR)/cacheprof/cprof altibase.tmp altibase.cprof 1024 32 2
	$(RM) altibase.tmp

DEP_DIR=$(CURDIR:$(DEV_DIR)%=$(TARGET_DIR)%)
-include $(wildcard $(DEP_DIR)/*.d)
