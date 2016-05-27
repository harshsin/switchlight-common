###############################################################################
#
# 
#
###############################################################################

include ../../../init.mk
MODULE := collecta_utest
TEST_MODULE := collecta
DEPENDMODULES := AIM BigList cjson Configuration debug_counter indigo IOF loci \
                 OS PPE timer_wheel slshared SocketManager

GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DINDIGO_LINUX_TIME
include $(BUILDER)/build-unit-test.mk

GLOBAL_LINK_LIBS += -lm -rdynamic
