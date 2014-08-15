
###############################################################################
#
# Inclusive Makefile for the ivd module.
#
###############################################################################
ivd_BASEDIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
include $(ivd_BASEDIR)/module/make.mk
include $(ivd_BASEDIR)/module/auto/make.mk
include $(ivd_BASEDIR)/module/src/make.mk
include $(ivd_BASEDIR)/utest/_make.mk
