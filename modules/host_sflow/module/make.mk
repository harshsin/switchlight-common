###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
host_sflow_INCLUDES := -I $(THIS_DIR)inc
host_sflow_INTERNAL_INCLUDES := -I $(THIS_DIR)src
