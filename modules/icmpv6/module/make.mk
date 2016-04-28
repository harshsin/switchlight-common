###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
icmpv6_INCLUDES := -I $(THIS_DIR)inc
icmpv6_INTERNAL_INCLUDES := -I $(THIS_DIR)src
icmpv6_DEPENDMODULE_ENTRIES := init:icmpv6 ucli:icmpv6

