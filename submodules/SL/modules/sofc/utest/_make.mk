###############################################################################
#
# sofc Unit Test Makefile.
#
###############################################################################
UMODULE := sofc
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
