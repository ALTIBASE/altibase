# platform_g++_common.GNU,v 4.3 2002/04/09 05:57:47 kitty Exp
#
# Common file help turn on/off explicit template instantiation

CXX_VERSION := $(shell $(CXX) -dumpversion)

# If no option has been specified, try to enable templates based on the
# version of the compiler.
#
ifeq ($(templates),)
  ifeq (4.,$(findstring 4.,$(CSS_VERSION)))
    templates = automatic
    ifeq (2.95,$(findstring 2.95,$(CXX_VERSION)))
      templates = automatic
    else
      ifeq (3.,$(findstring 3.,$(CXX_VERSION)))
        templates = automatic
      else
        ifeq (2.96,$(findstring 2.96,$(CXX_VERSION)))
          templates = automatic
        else
          ifeq (egcs, $(findstring egcs, $(CXX_VERSION)))
            templates = explicit
          else
            templates = explicit
          endif
        endif
      endif
    endif
  endif
endif

# Turn on the proper flags for explicit template instantiation.
#
ifeq ($(templates),explicit)
  ifeq ($(TEMPLATES_FLAG),) # Turn on flags if none is speficied.
    TEMPLATES_FLAG=-fno-implicit-templates
  endif
  CPPFLAGS += -DPDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
endif
