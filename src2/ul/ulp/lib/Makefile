include ../../ulEnv.mk
include $(DEV_DIR)/alticore.mk
include ../ulp_objs.mk

INCLUDES += $(foreach i, ../include ., $(IDROPT)$(i))

OBJS = $(ULP_LIB_OBJS)
SRCS = $(ULP_LIB_SRCS)

ULPLIB = $(LIBPRE)apre.$(LIBEXT)
ULPSHLIB  = $(LIBPRE)apre_sl.$(SOEXT)

ULP_LIB_OBJS = $(ULP_LIB_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))
ULP_LIB_SHOBJS = $(foreach name, $(ULP_LIB_SRCS), $(addsuffix _soc.$(OBJEXT), $(basename $(name))))

.PHONY: all
all: install

.PHONY: compile
compile: install

.PHONY: archive
archive: $(ULPLIB) apre_shared_lib

$(ULPLIB): $(ULP_LIB_OBJS)

$(TARGET_DIR)/%.$(OBJEXT): $(DEV_DIR)/%.c
	$(Q) echo " CC $@"
	$(Q) mkdir -p $(dir $@)
	$(Q) $(COMPILE.c) $(DEFOPT)ACP_CFG_DL_STATIC $(INCLUDES) $(CC_OUTPUT_FLAG)$@ $<

.PHONY: install
install: archive
	$(COPY) $(ULPLIB) $(ALTI_HOME)/lib
	$(COPY) $(ULPLIB) $(ALTI_HOME)/lib/$(LIBPRE)sesc.$(LIBEXT)
	$(COPY) $(ULPSHLIB) $(ALTI_HOME)/lib
	$(COPY) $(ULPSHLIB) $(ALTI_HOME)/lib/$(LIBPRE)sesc_sl.$(SOEXT)
	$(COPY) ../include/ulpLibInterface.h $(ALTI_HOME)/include
	$(COPY) ../include/ses.h $(ALTI_HOME)/include/ses.h
	$(COPY) ../include/aprePredefinedTypes.h $(ALTI_HOME)/include

clean:
	$(RM) $(TARGET_DIR)/src/ul/ulp/lib core $(ULPLIB) $(ULP_LIB_SHOBJS)
	$(RM) $(ULPSHLIB)

###################################
# Make Shared Library (BUG-24627)
###################################

apre_shared_lib:
ifeq "$(NEED_SHARED_LIBRARY)" "no" # defined in pd/makefiles2/*.mk : only aix 4.x no
	echo "SKIP Shared Library Build";
else
	$(MAKE) build_shared_lib
endif

ifeq "$(OS_TARGET)" "X86_64_DARWIN"
build_shared_lib: $(ULP_LIB_SHOBJS)
	$(Q) $(SOLINK.c) $(SOOUT)$(ULPSHLIB) $(ULP_LIB_SHOBJS) $(LIBDIRS) $(OLIBS) $(CLFLAGS) -lodbccli
else
build_shared_lib: $(ULP_LIB_SHOBJS)
	$(Q) $(SOLINK.c) $(SOOUT)$(ULPSHLIB) $(ULP_LIB_SHOBJS) $(LIBDIRS) $(OLIBS) $(CLFLAGS)
endif

%_soc.$(OBJEXT): %.c
	$(Q) echo " CC $@"
	$(Q) mkdir -p $(dir $@)
	$(Q) $(COMPILE.c) $(DEFOPT)LIB_BUILD $(INCLUDES) $(PIC) $(CCOUT)$@ $<

-include $(OBJS:.$(OBJEXT)=.d)
