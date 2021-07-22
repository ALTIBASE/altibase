include ../pdEnv.mk # added by jdlee

LIB   = libPDL.a
SHLIB = libPDL.$(SOEXT)
SRCS  = $(PDL_SRCS)
#OBJS  = $(SRCS:.cpp=.$(OBJEXT))
OBJS  = $(SRCS:$(DEV_DIR)/%.cpp=$(TARGET_DIR)/%.$(OBJEXT))

###############################################################

all: ofiles

env:
	echo $(OBJS)

ofiles: $(OBJS)
vshobjs: $(VSHOBJS)

$(OBJDIRS):
	test -d $@ || mkdir $@

clean:
	-$(RM) $(OBJS) *~ *.bak *.rpo *.sym lib*.*_pure_* Makefile.old core
	-$(RM) $(OBJDIRS)

realclean: clean
	-$(RM) $(BIN) $(BIN:%=%_debug) $(BIN:%=%_profile) $(BIN:%=%_optimize) $(LIB:%=%) $(LIB:%=%_debug) $(LIB:%=%_profile) $(LIB:%=%_optimize) $(SHLIB:%=%) $(SHLIB:%=%_debug) $(SHLIB:%=%_profile) $(SHLIB:%=%_optimize) $(SHLIBA:%=%) $(BIN2) $(BIN2:%=%_debug) $(BIN2:%=%_profile) $(BIN2:%=%_optimize) $(LIB2:%=%) $(LIB2:%=%_debug) $(LIB2:%=%_profile) $(LIB2:%=%_optimize) $(SHLIB2:%=%) $(SHLIB2:%=%_debug) $(SHLIB2:%=%_profile) $(SHLIB2:%=%_optimize) $(SHLIBA2:%=%)


$(VLIB): $(VLOBJS)

$(VSHLIB): $(VSHOBJS)
ifdef SHLIBBUILD
	$(SHLIBBUILD)
else
	$(SOLINK.cc) $(SO_OUTPUT_FLAG)$@ $(VSHOBJS) $(LFLAGS) $(LIBS)
endif

$(SHLIBA): $(VSHLIB)

-include $(OBJS:.$(OBJEXT)=.d)
