
###############################################################################
#
# Inclusive Makefile for the pdua module.
#
# Autogenerated 2015-11-11 14:15:20.107899
#
###############################################################################
pdua_BASEDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(pdua_BASEDIR)/module/make.mk
include $(pdua_BASEDIR)/module/auto/make.mk
include $(pdua_BASEDIR)/module/src/make.mk
include $(pdua_BASEDIR)/utest/_make.mk

