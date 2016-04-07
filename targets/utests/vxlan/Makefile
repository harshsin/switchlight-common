###############################################################################
#
#
#
###############################################################################

include ../../../init.mk
MODULE := vxlan_utest
TEST_MODULE := vxlan
DEPENDMODULES := AIM indigo loci BigHash murmur slshared uCli BigList IOF
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
include $(BUILDER)/build-unit-test.mk
