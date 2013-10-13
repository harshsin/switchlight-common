###############################################################################
#
#  /utest/_make.mk
#
#  IVS Unit Testing Definitions
#
###############################################################################
UMODULE := IVS
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk

