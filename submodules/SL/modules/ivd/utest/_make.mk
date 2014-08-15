###############################################################################
#
# ivd Unit Test Makefile.
#
###############################################################################
UMODULE := ivd
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
