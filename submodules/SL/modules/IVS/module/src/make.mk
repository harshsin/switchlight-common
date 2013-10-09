###############################################################################
#
#  /module/src/make.mk
#
#  IVS Builder Information
#
###############################################################################
LIBRARY := IVS
IVS_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk

