#
# $Id: Makefile 36624 2009-11-10 06:49:06Z bethy $
#

include ../dkEnv.mk

# NLS 중 영어 에러 메시지 화일 정의 
E_DK_US_MSG    = E_DK_US7ASCII.msg
ALLMSG         = $(E_DK_US_MSG)

E_DK_US_MSB    = $(E_DK_US_MSG:.msg=.msb)

E_DK_US_TXT    = $(E_DK_US_MSG:.msg=.txt)

ALLMSB         = $(ALTI_HOME)/msg/E_DK_US7ASCII.msb $(ALTI_HOME)/msg/E_DK_KO16KSC5601.msb

E_DK_HEADER    = dkErrorCode.ih

ALL_HEADER      = $(E_DK_HEADER)
 

all: compile

compile:  $(ALLMSB)

$(E_DK_US_MSB) : $(E_DK_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_DK_US_MSG) -o $(E_DK_HEADER)
	cp -f DK_TRC_CODE.ih DK_TRC_CODE.ic $(E_DK_HEADER) ../include
	sync

$(ALLMSB): $(E_DK_US_MSB)
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i $(E_DK_US_MSG) > $(ALTI_HOME)/msg/$(E_DK_US_TXT)


check :
	@echo "no check"

clean:
	$(RM) core .dependency $(BINS) $(OBJS) *.d SunWS_cache *~ *.msb *.ih *.ic

install: $(BINS)
	mv $(BINS) ../bin
install_pure: $(BINS_purify) $(BINS_quantify) $(BINS_purecov)
	mv $(BINS_purify) $(BINS_quantify) $(BINS_purecov) ../bin
