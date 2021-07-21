include ../cmEnv.mk

# NLS 중 영어 에러 메시지 화일 정의 
E_CM_US_MSG    = E_CM_US7ASCII.msg
ALLMSG         = $(E_CM_US_MSG)

E_CM_US_MSB    = $(E_CM_US_MSG:.msg=.msb)

E_CM_US_TXT    = $(E_CM_US_MSG:.msg=.txt)

E_CM_US_C      = $(E_CM_US_MSG:.msg=.c)

ALLMSB         = $(ALTI_HOME)/msg/E_CM_US7ASCII.msb $(ALTI_HOME)/msg/E_CM_KO16KSC5601.msb

E_CM_HEADER    = cmErrorCode.ih

ALL_HEADER      =  $(E_CM_HEADER)


all: $(ALLMSB) $(E_CM_US_C)

$(E_CM_US_MSB) : $(E_CM_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_CM_US_MSG) -o $(E_CM_HEADER)
	cp -f CM_TRC_CODE.ih CM_TRC_CODE.ic $(E_CM_HEADER) ../include
	sync

$(E_CM_US_C) : $(E_CM_US_MSG)
	$(GENERRMSG) -m -c -i $(E_CM_US_MSG) -o $(E_CM_HEADER)
	mv -f $@ $(CM_DIR)/cmi

$(ALLMSB): $(E_CM_US_MSB)
	$(COPY) $^ $@

archive:
	$(AR) $(ARFLAGS) $(CMLIB) $(OBJS) $(GENOBJS) $(ANTLROBJS)

manual:
	$(GENERRMSGMANUAL) -i $(E_CM_US_MSG) > $(ALTI_HOME)/msg/$(E_CM_US_TXT)

clean:
	$(RM) core .dependency $(BINS) $(OBJS) *.d SunWS_cache *~ *.msb *.ih *.ic

install: $(BINS)
	mv $(BINS) ../bin
install_pure: $(BINS_purify) $(BINS_quantify) $(BINS_purecov)
	mv $(BINS_purify) $(BINS_quantify) $(BINS_purecov) ../bin
