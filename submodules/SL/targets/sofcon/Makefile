###############################################################################
#
#  Makefile
#
###############################################################################
include ../../init.mk

MODULE := sofcm
include $(BUILDER)/standardinit.mk

LIBRARY := sofcl
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk

DEPENDMODULES := AIM BigList uCli IOF OS loci NSS sofc

include $(BUILDER)/dependmodules.mk

BINARY := sofcon

$(BINARY)_LIBRARIES := $(LIBRARY_TARGETS) 
include $(BUILDER)/bin.mk

include $(BUILDER)/targets.mk

LDFLAGS += -lpthread

GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DOS_CONFIG_INCLUDE_POSIX=1
GLOBAL_LINK_LIBS += -lrt
