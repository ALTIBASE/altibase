include ../ulEnv.mk

# 소스코드에 include 될 에러코드 화일

UL_E_HEADER = ulErrorCode.ih
UT_E_HEADER = utErrorCode.ih
LA_E_HEADER = ulaErrorCode.ih LA_TRC_CODE.ih
LA_E_CODE = LA_TRC_CODE.ic
LP_E_HEADER = ulpErrorCode.ih

# 에러코드 메시지 화일

all:ofiles

ofiles: ErrorFiles

compile: ErrorFiles

install: $(BINS)
	@cp $(BINS) ../bin


ErrorFiles : E_UL_US7ASCII.c E_LA_US7ASCII.c E_LP_US7ASCII.c # E_UT_US7ASCII.c

E_UL_US7ASCII.c : E_UL_US7ASCII.msg
	$(GENERRMSG) -c -i E_UL_US7ASCII.msg -o $(UL_E_HEADER)
	$(COPY) -f $(UL_E_HEADER) ../include
	$(COPY) -f $@ $(UL_DIR)/uln
	sync

E_LA_US7ASCII.c : E_LA_US7ASCII.msg
	$(GENERRMSG) -c -i E_LA_US7ASCII.msg -o $(LA_E_HEADER)
	$(COPY) -f $(LA_E_HEADER) ../include
	$(COPY) -f $(LA_E_CODE) ../ula
	$(COPY) -f $@ $(UL_DIR)/ula
	sync

E_LP_US7ASCII.c : E_LP_US7ASCII.msg
	$(GENERRMSG) -c -i E_LP_US7ASCII.msg -o $(LP_E_HEADER)
	$(COPY) -f $(LP_E_HEADER) $(UL_DIR)/ulp/include	
	$(COPY) -f $@ $(UL_DIR)/ulp
	$(COPY) -f $@ $(UL_DIR)/ulp/lib
	sync

manual:
	$(GENERRMSGMANUAL) -c -i E_UL_US7ASCII.msg  > $(ALTI_HOME)/msg/E_UL_US7ASCII.txt
	$(GENERRMSGMANUAL) -c -i E_LA_US7ASCII.msg  > $(ALTI_HOME)/msg/E_LA_US7ASCII.txt
	$(GENERRMSGMANUAL) -c -i E_LP_US7ASCII.msg  > $(ALTI_HOME)/msg/E_LP_US7ASCII.txt

header :
	@echo "no header"

check :
	@echo "no check"

clean:
	$(RM) core $(OBJS) *.d genErrMsg  SunWS_cache  *.msb *.ih *~ E_UL_US7ASCII.c E_LA_US7ASCII.c E_LP_US7ASCII.c # E_UT_US7ASCII.c

clean_bin:
	$(RM) $(BINS)
