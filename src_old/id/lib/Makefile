# Makefile for ID library
#
# CVS Info : $Id: Makefile 71452 2015-06-22 04:21:49Z heesuk.shin $
#

include ../idEnv.mk


TARGET = $(ALTI_HOME)/lib/$(LIBPRE)id.$(LIBEXT)
SHTARGET = $(ALTI_HOME)/lib/$(LIBPRE)id_so.$(SOEXT)
PICTARGET = $(ALTI_HOME)/lib/$(LIBPRE)id_pic.$(LIBEXT)

ifeq ($(ALTI_CFG_OS),WINDOWS)
SODEPLIBS  = $(foreach i, pd, $(ALTI_HOME)/lib/$(LIBOPT)$(i)$(LIBAFT))
SODEPLIBS += $(ALTICORE_LIBDIR)/alticore_pic_static.lib
ifeq ($(compile64),1)
SODEFFILE = /DEF:$(ID_DIR)/lib/id_so64.def
else
ifeq ($(BUILD_MODE),release)
SODEFFILE = /DEF:$(ID_DIR)/lib/id_so32r.def
else
SODEFFILE = /DEF:$(ID_DIR)/lib/id_so32d.def
endif
endif
endif

all: lib piclib

lib: id_static alticore_static.touch

solib: id_dynamic

piclib: id_pic

id_static: $(TARGET)
	cp $(ID_DIR)/lib/libgpkiapi_so $(ALTI_HOME)/lib/libgpkiapi.so

id_dynamic: $(SHTARGET)

id_pic: $(PICTARGET)

$(TARGET): $(ID_OBJS)

$(SHTARGET): $(ID_SHOBJS)


$(PICTARGET): $(ID_SHOBJS)

# Make shobjs before making solib and piclib in parallel.
#
# When building the ID module in parallel, kicking off solib and
# piclib targets simultaneously may result in corrupt object files due
# to shared dependency.  Therefore build the following target from the
# parent-level makefile before making solib or piclib.
.PHONY: shobjs
shobjs: $(ID_SHOBJS)

alticore_static.touch: $(TARGET)
 ifeq "$(OS_TARGET)" "INTEL_WINDOWS"
	lib $(TARGET) netapi32.lib advapi32.lib ws2_32.lib iphlpapi.lib dbghelp.lib shell32.lib user32.lib
 endif # INTEL_WINDOWS
	touch $@

$(ID_DIR)/lib/libgpkiapi.so :
	cp $(ID_DIR)/lib/libgpkiapi_so $(ID_DIR)/lib/libgpkiapi.so

clean:
	-$(RM) $(TARGET) $(SHTARGET) $(PICTARGET) core *.a createdb destroydb SunWS_cache .shobj alticore_static.touch

ifeq ($(COMPILER_NAME),GCC)
ifneq ($(GCC_VERSION_MAJOR),3)
$(TARGET_DIR)/src/id/idu/iduProperty.$(OBJEXT): CPPFLAGS += -Wno-missing-field-initializers
$(TARGET_DIR)/src/id/idu/iduProperty%.$(OBJEXT): CPPFLAGS += -Wno-missing-field-initializers
endif
endif

-include $(ID_OBJS:.$(OBJEXT)=.d)
-include $(ID_SHOBJS:.$(OBJEXT)=.d)
