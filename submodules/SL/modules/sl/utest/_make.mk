###############################################################################
#
# sl Unit Test Makefile.
#
###############################################################################
UMODULE := sl
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
