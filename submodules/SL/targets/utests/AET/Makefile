###############################################################################
#
# 
#
###############################################################################
include ../../../init.mk

MODULE := AET_utest
TEST_MODULE := AET
DEPENDMODULES := OS AIM indigo loci BigList uCli murmur IOF

GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DINDIGO_LINUX_LOGGING
GLOBAL_CFLAGS += -DINDIGO_LINUX_TIME
GLOBAL_CFLAGS += -DINDIGO_FAULT_ON_ASSERT
GLOBAL_CFLAGS += -DINDIGO_MEM_STDLIB
GLOBAL_CFLAGS += -DOF_OBJECT_TRACKING
GLOBAL_CFLAGS += -DOS_CONFIG_INCLUDE_NONE=1
GLOBAL_CFLAGS += -Wall

GLOBAL_CFLAGS += -I$(SUBMODULE_INDIGO)/modules/SocketManager/module/inc
GLOBAL_CFLAGS += -I$(SUBMODULE_INDIGO)/modules/OFStateManager/module/inc

include $(BUILDER)/build-unit-test.mk
