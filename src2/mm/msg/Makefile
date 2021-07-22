include ../mmEnv.mk

# NLS 중 영어 에러 메시지 화일 정의 
E_MM_US_MSG    = E_MM_US7ASCII.msg
ALLMSG         = $(E_MM_US_MSG)

E_MM_US_MSB    = $(E_MM_US_MSG:.msg=.msb)

E_MM_US_TXT    = $(E_MM_US_MSG:.msg=.txt)

ALLMSB         = $(ALTI_HOME)/msg/E_MM_US7ASCII.msb $(ALTI_HOME)/msg/E_MM_KO16KSC5601.msb

E_MM_HEADER    = mmErrorCode.ih

ALL_HEADER      =  $(E_MM_HEADER)


all: $(COMPILE)

compile:  $(ALLMSB)

$(E_MM_US_MSB) : $(E_MM_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_MM_US_MSG) -o $(E_MM_HEADER)
	cp -f MM_TRC_CODE.ih MM_TRC_CODE.ic $(E_MM_HEADER) ../include
	sync

$(ALLMSB): $(E_MM_US_MSB)
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i $(E_MM_US_MSG) > $(ALTI_HOME)/msg/$(E_MM_US_TXT)


check :
	@echo "no check"

clean:
	$(RM) core .dependency $(BINS) $(OBJS) *.d SunWS_cache *~ *.msb *.ih

install: $(BINS)
	mv $(BINS) ../bin
install_pure: $(BINS_purify) $(BINS_quantify) $(BINS_purecov)
	mv $(BINS_purify) $(BINS_quantify) $(BINS_purecov) ../bin
