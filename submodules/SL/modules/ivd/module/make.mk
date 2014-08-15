###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
ivd_INCLUDES := -I $(THIS_DIR)inc
ivd_INTERNAL_INCLUDES := -I $(THIS_DIR)src
ivd_DEPENDMODULE_ENTRIES := init:ivd ucli:ivd
