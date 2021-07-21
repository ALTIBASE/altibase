# $Id: Makefile 90822 2021-05-13 04:45:36Z kclee $

include ../smEnv.mk

#SRCS    = genErrMsg.cpp
#BINS	= genErrMsg

# 소스코드에 include 될 에러코드 화일

E_HEADER = smErrorCode.ih
ALLMSB         = $(ALTI_HOME)/msg/E_SM_US7ASCII.msb $(ALTI_HOME)/msg/E_SM_KO16KSC5601.msb

# 에러코드 메시지 화일

all: ErrorFiles
#genErrMsg 

install: $(BINS)
	@cp $(BINS) ../bin

#genErrMsg: genErrMsg.o
#	$(LD) $(LFLAGS) -o $@ $< $(ID_DIR)/idl/idl.cpp $(LIBS)
#	@cp $(BINS) ../bin


ErrorFiles : $(ALLMSB)

E_SM_US7ASCII.msb : E_SM_US7ASCII.msg $(GENERRMSG)
	$(GENERRMSG) -i E_SM_US7ASCII.msg -o $(E_HEADER)
	$(COPY) SM_TRC_CODE.ih SM_TRC_CODE.ic  $(E_HEADER) ../include	
	sync

$(ALLMSB): E_SM_US7ASCII.msb
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i E_SM_US7ASCII.msg  > $(ALTI_HOME)/msg/E_SM_US7ASCII.txt

header :
	@echo "no header"

check :
	@echo "no check"


clean:
	$(RM) core $(OBJS) *.d genErrMsg  SunWS_cache  *.msb *.ih *~

clean_bin:
	$(RM) $(BINS)
