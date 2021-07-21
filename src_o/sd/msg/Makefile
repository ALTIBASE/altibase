#
# $Id: Makefile 70601 2015-04-30 01:29:54Z heesuk.shin $
#

include ../sdEnv.mk

# NLS 중 영어 에러 메시지 화일 정의 
E_SD_US_MSG    = E_SD_US7ASCII.msg
ALLMSG         = $(E_SD_US_MSG)

E_SD_US_MSB    = $(E_SD_US_MSG:.msg=.msb)

E_SD_US_TXT    = $(E_SD_US_MSG:.msg=.txt)

ALLMSB         = $(ALTI_HOME)/msg/E_SD_US7ASCII.msb $(ALTI_HOME)/msg/E_SD_KO16KSC5601.msb

E_SD_HEADER    = sdErrorCode.ih

ALL_HEADER      = $(E_SD_HEADER)


all: $(COMPILE)

compile:  $(ALLMSB)

$(E_SD_US_MSB) : $(E_SD_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_SD_US_MSG) -o $(E_SD_HEADER)
	cp -f SD_TRC_CODE.ih SD_TRC_CODE.ic $(E_SD_HEADER) ../include
	sync

$(ALLMSB): $(E_SD_US_MSB)
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i $(E_SD_US_MSG) > $(ALTI_HOME)/msg/$(E_SD_US_TXT)


check :
	@echo "no check"

clean:
	$(RM) core .dependency $(BINS) $(OBJS) *.d SunWS_cache *~ *.msb *.ih

install: $(BINS)
	mv $(BINS) ../bin
install_pure: $(BINS_purify) $(BINS_quantify) $(BINS_purecov)
	mv $(BINS_purify) $(BINS_quantify) $(BINS_purecov) ../bin
