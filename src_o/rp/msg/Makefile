#
# $Id: Makefile 90822 2021-05-13 04:45:36Z kclee $
#

include ../rpEnv.mk

# NLS 중 영어 에러 메시지 화일 정의 
E_RP_US_MSG    = E_RP_US7ASCII.msg
ALLMSG         = $(E_RP_US_MSG)

E_RP_US_MSB    = $(E_RP_US_MSG:.msg=.msb)

E_RP_US_TXT    = $(E_RP_US_MSG:.msg=.txt)

ALLMSB         = $(ALTI_HOME)/msg/E_RP_US7ASCII.msb $(ALTI_HOME)/msg/E_RP_KO16KSC5601.msb

E_RP_HEADER    = rpErrorCode.ih

ALL_HEADER      = $(E_RP_HEADER)


all: compile

compile:  $(ALLMSB)

$(E_RP_US_MSB) : $(E_RP_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_RP_US_MSG) -o $(E_RP_HEADER)
	cp -f RP_TRC_CODE.ih RP_TRC_CODE.ic $(E_RP_HEADER) ../include
	sync

$(ALLMSB): $(E_RP_US_MSB)
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i $(E_RP_US_MSG) > $(ALTI_HOME)/msg/$(E_RP_US_TXT)


check :
	@echo "no check"

clean:
	$(RM) core .dependency $(BINS) $(OBJS) *.d SunWS_cache *~ *.msb *.ih

install: $(BINS)
	mv $(BINS) ../bin
install_pure: $(BINS_purify) $(BINS_quantify) $(BINS_purecov)
	mv $(BINS_purify) $(BINS_quantify) $(BINS_purecov) ../bin
