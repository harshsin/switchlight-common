###############################################################################
#
#  /module/make.mk
#
#  IVS public includes are defined here
#
###############################################################################
THISDIR := $(dir $(lastword $(MAKEFILE_LIST)))
IVS_INCLUDES := -I $(THISDIR)inc
IVS_INTERNAL_INCLUDES := -I $(THISDIR)src
IVS_DEPENDMODULE_ENTRIES := init:ivs

