###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
vxlan_INCLUDES := -I $(THIS_DIR)inc
vxlan_INTERNAL_INCLUDES := -I $(THIS_DIR)src
vxlan_DEPENDMODULE_ENTRIES := init:vxlan ucli:vxlan

