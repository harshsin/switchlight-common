###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
slshared_INCLUDES := -I $(THIS_DIR)inc
slshared_INTERNAL_INCLUDES := -I $(THIS_DIR)src
