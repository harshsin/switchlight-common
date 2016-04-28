###############################################################################
#
# icmpv6 Unit Test Makefile.
#
###############################################################################
UMODULE := icmpv6
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
