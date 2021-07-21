include ../mmEnv.mk
include ./mm_objs.mk


TARGET = $(ALTI_HOME)/lib/$(LIBPRE)mm.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)mm_so.$(SOEXT)

all: lib

lib: mm_static

solib: mm_dynamic

mm_static: $(TARGET)

mm_dynamic: $(SHTARGET)

$(TARGET): $(MM_OBJS)

$(SHTARGET): $(MM_SHOBJS)

check :
	@for i in $(MM_SRCS); do (gawk -v API_INPUT=$(TOOL_DIR)/coding_chk/API.txt -v SRC_INPUT=$$i -f $(TOOL_DIR)/coding_chk/apiCheck.awk ) done

clean:
	-$(RM) $(TARGET) $(SHTARGET) $(SHOBJS) core .dependency $(OBJS)

-include $(MM_OBJS:.$(OBJEXT)=.d)
-include $(MM_SHOBJS:.$(OBJEXT)=.d)
