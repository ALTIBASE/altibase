# $Id: Makefile 15403 2006-03-28 00:15:35Z sjkim $

include ../stEnv.mk

# NLS 중 영어 에러 메시지 화일 정의
E_ST_US_MSG    = E_ST_US7ASCII.msg
ALLMSG         = $(E_ST_US_MSG)

E_ST_US_MSB    = $(E_ST_US_MSG:.msg=.msb)

E_ST_US_TXT    = $(E_ST_US_MSG:.msg=.txt)

ALLMSB         = $(ALTI_HOME)/msg/E_ST_US7ASCII.msb $(ALTI_HOME)/msg/E_ST_KO16KSC5601.msb

E_ST_HEADER    = stErrorCode.ih

ALL_HEADER      = $(E_ST_HEADER)


all: $(COMPILE)

shobjs: $(VSHOBJS)

compile:  $(ALLMSB)

$(E_ST_US_MSB) : $(E_ST_US_MSG) $(GENERRMSG)
	$(GENERRMSG) -i $(E_ST_US_MSG) -o $(E_ST_HEADER)
	cp -f ST_TRC_CODE.ih ST_TRC_CODE.ic $(E_ST_HEADER) ../include
	sync

$(ALLMSB): $(E_ST_US_MSB)
	$(COPY) $^ $@

manual:
	$(GENERRMSGMANUAL) -i $(E_ST_US_MSG) > $(ALTI_HOME)/msg/$(E_ST_US_TXT)


check :
	@echo "no check"

clean:
	$(RM) *~ *.msb *.ih

