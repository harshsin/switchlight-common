###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
collecta_INCLUDES := -I $(THIS_DIR)inc
collecta_INTERNAL_INCLUDES := -I $(THIS_DIR)src
collecta_DEPENDMODULE_ENTRIES := init:collecta ucli:collecta

