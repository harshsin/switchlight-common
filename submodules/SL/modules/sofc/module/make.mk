###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
sofc_INCLUDES := -I $(THIS_DIR)inc
sofc_INTERNAL_INCLUDES := -I $(THIS_DIR)src
sofc_DEPENDMODULE_ENTRIES := init:sofc ucli:sofc

