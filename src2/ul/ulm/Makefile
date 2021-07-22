include ../ulEnv.mk
include $(DEV_DIR)/alticore.mk

ULM_DIR        = $(UL_DIR)/ulm
ULM_LIB_SRCS   = $(ULM_DIR)/ulmApi.c $(ULM_DIR)/ulmApiCore.c

ULM_LIB_OBJS   = $(ULM_LIB_SRCS:$(DEV_DIR)/%.c=$(TARGET_DIR)/%.$(OBJEXT))
ULM_LIB_SHOBJS = $(foreach name, $(ULM_LIB_SRCS), $(addsuffix _soc.$(OBJEXT), $(basename $(name))))

ULM_LIB        = $(ALTI_HOME)/lib/$(LIBPRE)altibaseMonitor.$(LIBEXT)
ULM_SHLIB      = $(ALTI_HOME)/lib/$(LIBPRE)altibaseMonitor_sl.$(SOEXT)

ULM_SHARED_INTERFACE_HEADER = altibaseMonitor.h

all : archive

compile : archive

archive : $(ULM_LIB) ulm_shared_lib install

$(ULM_LIB) : $(ULM_LIB_OBJS)

install : $(ULM_LIB)
	$(COPY) $(UL_DIR)/include/ulmApi.h $(ALTI_HOME)/include/$(ULM_SHARED_INTERFACE_HEADER)

clean :
	$(RM) $(TARGET_DIR)/src/ul/ulm
	$(RM) $(ULM_LIB) $(ULM_SHLIB) $(ALTI_HOME)/include/$(ULM_SHARED_INTERFACE_HEADER)


##########################
# Make shared library
##########################

ulm_shared_lib :
ifeq "$(NEED_SHARED_LIBRARY)" "no" # defined in pd/makefiles2/*.mk : only aix 4.x no
	echo "SKIP Shared Library Build";
else
	$(MAKE) build_shared_lib
endif

build_shared_lib : $(ULM_LIB_SHOBJS)
	$(Q) $(SOLINK.c) $(SOOUT)$(ULM_SHLIB) $(ULM_LIB_SHOBJS) $(LIBDIRS) $(OLIBS) $(CLFLAGS)

%_soc.$(OBJEXT) : %.c 
	$(Q) echo " CC $@"
	$(Q) mkdir -p $(dir $@)
	$(Q) $(COMPILE.c) $(DEFOPT)LIB_BUILD $(INCLUDES) $(PIC) $(CCOUT)$@ $<
