include $(ALTICORE_HOME)/makefiles/config.mk
include $(ALTICORE_HOME)/makefiles/platform.mk
include $(ALTI_HOME)/install/altibase_env.mk

PERFACP_SRC	= perfacp.cpp
PERFID_SRC 	= perfid.cpp

PERFACP_EXE	= perfacp
PERFID_EXE 	= perfid

EXE = $(PERFACP_EXE) $(PERFID_EXE)

all: perfacp perfid

$(PERFACP_EXE): $(PERFACP_SRC)
	$(CXX) $(CC_OUT_OPT) $(PERFACP_EXE) $(PERFACP_SRC) $(INC_OPT)$(ALTICORE_HOME)/include $(LIBDIR_OPT)$(ALTICORE_HOME)/lib32 $(LIB_OPT)alticore

$(PERFID_EXE): $(PERFID_SRC)
	$(CXX) $(CC_OUT_OPT) $(PERFID_EXE) $(PERFID_SRC) $(INCLUDES) $(LFLAGS) $(LIB_OPT)id $(LIB_OPT)pd $(LIBS)

clean:
	rm -f $(EXE)
