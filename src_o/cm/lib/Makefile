include ../cmEnv.mk


TARGET = $(ALTI_HOME)/lib/$(LIBPRE)cm.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)cm_so.$(SOEXT)

all: lib

lib: cm_static

solib: cm_dynamic

cm_static: $(TARGET)

cm_dynamic: $(SHTARGET)

$(TARGET): $(CM_OBJS)

$(SHTARGET): $(CM_SHOBJS)

clean:
	-$(RM) $(TARGET) $(SHTARGET) core *.a *.$(SOEXT) .shobj *~

-include $(CM_SHOBJS:.$(OBJEXT)=.d)
