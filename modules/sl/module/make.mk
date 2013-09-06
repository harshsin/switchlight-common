###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
sl_INCLUDES := -I $(THIS_DIR)inc
sl_INTERNAL_INCLUDES := -I $(THIS_DIR)src
sl_DEPENDMODULE_ENTRIES := init:sl ucli:sl

