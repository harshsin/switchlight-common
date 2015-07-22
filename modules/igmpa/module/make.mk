###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
igmpa_INCLUDES := -I $(THIS_DIR)inc
igmpa_INTERNAL_INCLUDES := -I $(THIS_DIR)src
igmpa_DEPENDMODULE_ENTRIES := init:igmpa ucli:igmpa

