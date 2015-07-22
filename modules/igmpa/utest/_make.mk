###############################################################################
#
# igmpa Unit Test Makefile.
#
###############################################################################
UMODULE := igmpa
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
