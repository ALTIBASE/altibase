# $Id: Makefile 82136 2018-01-26 04:20:33Z emlee $ #

include ../smEnv.mk

BINS = $(ALTI_HOME)/bin/dumpla$(BINEXT) \
       $(ALTI_HOME)/bin/dumplf$(BINEXT) $(ALTI_HOME)/bin/oidInfo$(BINEXT) \
       $(ALTI_HOME)/bin/makeLockDecision$(BINEXT) \
       $(ALTI_HOME)/bin/dumpci$(BINEXT) $(ALTI_HOME)/bin/dumptbl$(BINEXT) $(ALTI_HOME)/bin/dumptbbck$(BINEXT) \
       $(ALTI_HOME)/bin/dumpddf$(BINEXT) $(ALTI_HOME)/bin/aped$(BINEXT) $(ALTI_HOME)/bin/acalc$(BINEXT) \
	   $(ALTI_HOME)/bin/dumpct$(BINEXT) $(ALTI_HOME)/bin/dumpbi$(BINEXT) $(ALTI_HOME)/bin/dumptd$(BINEXT) \
	   $(ALTI_HOME)/bin/dumpidxfile$(BINEXT) $(ALTI_HOME)/bin/dumpsbf$(BINEXT) $(ALTI_HOME)/bin/awrite$(BINEXT)
BINS_AFTER = $(ALTI_HOME)/bin/dumpdb$(BINEXT) $(ALTI_HOME)/bin/shmutil$(BINEXT)

#testsm viewstat

BINSRCS = $(SM_DIR)/util/checklogfiles.cpp \
          $(SM_DIR)/util/dumploganchor.cpp \
          $(SM_DIR)/util/viewstat.cpp \
          $(SM_DIR)/util/makeLockDecision.cpp \
          $(SM_DIR)/util/SharedMutex.cpp \
          $(SM_DIR)/util/SharedSema.cpp \
	  $(SM_DIR)/util/dumptbl.cpp \
	  $(SM_DIR)/util/aped.cpp \
	  $(SM_DIR)/util/dumptd.cpp

BINOBJS=$(BINSRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

OBJS=$(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

MODULE_LIST = mm dk qp sd rp mt st sm cm id pd
SERVER_LIBS = $(foreach i,$(MODULE_LIST),$(LIBOPT)$(i)$(LIBAFT)) $(ALTICORE_STATIC_LIB) $(LIBS)
SERVER_DEPS = $(foreach i,$(MODULE_LIST),$(ALTI_HOME)/lib/$(LIBPRE)$(i).$(LIBEXT))
INCLUDES += $(foreach i,$(MM_DIR)/include $(QP_DIR)/include $(DR_DIR)/include $(RP_DIR)/include $(MT_DIR)/include $(ST_DIR)/include $(SM_DIR)/include $(CM_DIR)/include $(DK_DIR)/include,$(IDROPT)$(i))

#cmd_ld = $(LD) $(LFLAGS) $(LDOUT)$@ /PDB:$(subst $(BINEXT),.pdb,$@) $^ $(SMLIB) $(ID_LIB) $(LIBS)
cmd_ld = $(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SMLIB) $(ID_LIB) $(LIBS)

all: $(COMPILE) install

all_after: $(BINS_AFTER)

compile:  $(OBJS) $(BINS)

header :
	@for i in $(SRCS); do ( cp $$i $$i.org ; cat $$i.org | gawk  -f $(TOOL_DIR)/add_header/header.awk > $$i ) done

check :
	@for i in $(SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

alint: $(SRCS:.cpp=.lint)

clean:
	$(RM) core .dependency $(BINS) $(BINS_AFTER) $(OBJS) $(TARGET_DIR)/src/sm/util/*.$(OBJEXT) *.d SunWS_cache

install: $(BINS)
        # TASK-2693 dumplf, dumpla를 제품화 시켜야 함 
	$(COPY) DUMPLF.ban $(ALTI_HOME)/msg
	$(COPY) DUMPLA.ban $(ALTI_HOME)/msg
        # BUG-28510 dump유틸들의 Banner양식을 통일해야 합니다.
	$(COPY) DUMPCI.ban $(ALTI_HOME)/msg
	$(COPY) DUMPDDF.ban $(ALTI_HOME)/msg
	$(COPY) DUMPCT.ban $(ALTI_HOME)/msg
	$(COPY) DUMPBI.ban $(ALTI_HOME)/msg

$(ALTI_HOME)/bin/testmem$(BINEXT): $(TARGET_DIR)/src/sm/util/testmem.$(OBJEXT) $(ID_LIB)
	$(MYPURIFY) $(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB) $(LIBS)

$(ALTI_HOME)/bin/dumptbl$(BINEXT): $(TARGET_DIR)/src/sm/util/dumptbl.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpla$(BINEXT): $(TARGET_DIR)/src/sm/util/dumploganchor.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/copyfile$(BINEXT): $(TARGET_DIR)/src/sm/util/copyfile.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumplf$(BINEXT): $(TARGET_DIR)/src/sm/util/dumplf.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumptbbck$(BINEXT): $(TARGET_DIR)/src/sm/util/dumptbbck.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpdb$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpdb.$(OBJEXT)
	$(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SERVER_LIBS)

$(ALTI_HOME)/bin/shmutil$(BINEXT): $(TARGET_DIR)/src/sm/util/shmutil.$(OBJEXT)
	$(LD) $(LFLAGS) $(LDOUT)$@ $^ $(SERVER_LIBS)

$(ALTI_HOME)/bin/dumpci$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpci.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpddf$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpddf.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/viewstat$(BINEXT): $(TARGET_DIR)/src/sm/util/viewstat.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/oidInfo$(BINEXT) : $(TARGET_DIR)/src/sm/util/oidInfo.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/makeLockDecision$(BINEXT) : $(TARGET_DIR)/src/sm/util/makeLockDecision.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/SharedMutex$(BINEXT) : $(TARGET_DIR)/src/sm/util/SharedMutex.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/SharedSema$(BINEXT) : $(TARGET_DIR)/src/sm/util/SharedSema.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/FinalIPC$(BINEXT) : $(TARGET_DIR)/src/sm/util/FinalIPC.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/IPC_MQ$(BINEXT) : $(TARGET_DIR)/src/sm/util/IPC_MQ.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/mq1$(BINEXT) : $(TARGET_DIR)/src/sm/util/mq1.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/mq2$(BINEXT) : $(TARGET_DIR)/src/sm/util/mq2.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/myping$(BINEXT) : $(TARGET_DIR)/src/sm/util/myping.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/SleepTime$(BINEXT) : $(TARGET_DIR)/src/sm/util/SleepTime.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/checkSleep$(BINEXT) : $(TARGET_DIR)/src/sm/util/checkSleep.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS) 
	$(cmd_ld)

$(ALTI_HOME)/bin/checkMultiCrypt$(BINEXT) : $(TARGET_DIR)/src/sm/util/checkMultiCrypt.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  $(ID_LIB) $(LIBS) 
	$(cmd_ld)

$(ALTI_HOME)/bin/DoorServer$(BINEXT) : $(TARGET_DIR)/src/sm/util/DoorServer.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm  -ldoor $(ID_LIB) $(LIBS) 
	$(cmd_ld)

$(ALTI_HOME)/bin/checkStack$(BINEXT) : $(TARGET_DIR)/src/sm/util/checkStack.$(OBJEXT) $(TARGET_DIR)/src/sm/util/checkStack2.$(OBJEXT) $(TARGET_DIR)/src/sm/util/checkStack3.$(OBJEXT) $(TARGET_DIR)/src/sm/util/checkStack4.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ checkStack.$(OBJEXT) checkStack2.$(OBJEXT) checkStack3.$(OBJEXT) checkStack4.$(OBJEXT) -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/checkNBS$(BINEXT) : $(TARGET_DIR)/src/sm/util/checkNBS.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ checkNBS.$(OBJEXT) -lsm  $(ID_LIB) $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/aped$(BINEXT): $(TARGET_DIR)/src/sm/util/aped.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpidxfile$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpidxfile.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/acalc$(BINEXT): $(TARGET_DIR)/src/sm/util/acalc.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpct$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpct.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpbi$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpbi.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumpsbf$(BINEXT): $(TARGET_DIR)/src/sm/util/dumpsbf.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/dumptd$(BINEXT): $(TARGET_DIR)/src/sm/util/dumptd.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

$(ALTI_HOME)/bin/awrite$(BINEXT): $(TARGET_DIR)/src/sm/util/awrite.$(OBJEXT) $(ID_LIB)
#	$(LD) $(LFLAGS) -o $@ $< -lsm $(ID_LIB)  $(LIBS)
	$(cmd_ld)

-include $(BINOBJS:.$(OBJEXT)=.d)
