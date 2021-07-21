# $Id: Makefile 90822 2021-05-13 04:45:36Z kclee $

include ../mtEnv.mk

# NLS 중 영어 에러 메시지 화일 정의
E_MT_US_MSG    = E_MT_US7ASCII.msg
ALLMSG         = $(E_MT_US_MSG)

E_MT_US_MSB    = $(E_MT_US_MSG:.msg=.msb)

E_MT_US_TXT    = $(E_MT_US_MSG:.msg=.txt)

ALLMSB         = $(ALTI_HOME)/msg/E_MT_US7ASCII.msb $(ALTI_HOME)/msg/E_MT_KO16KSC5601.msb

E_MT_HEADER    = mtErrorCode.ih

ALL_HEADER      = $(E_MT_HEADER)


all: $(COMPILE)

shobjs: $(VSHOBJS)

compile:  $(ALLMSB)

$(E_MT_US_MSB) : $(E_MT_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_MT_US_MSG) -o $(E_MT_HEADER)
	cp -f MT_TRC_CODE.ih MT_TRC_CODE.ic $(E_MT_HEADER) ../include
	sync

$(ALLMSB): $(E_MT_US_MSB)
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i $(E_MT_US_MSG) > $(ALTI_HOME)/msg/$(E_MT_US_TXT)


check :
	@echo "no check"

clean:
	$(RM) *~ *.msb *.ih

