###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
AET_INCLUDES := -I $(THIS_DIR)inc
AET_INTERNAL_INCLUDES := -I $(THIS_DIR)src
AET_DEPENDMODULE_ENTRIES := init:aet ucli:aet

