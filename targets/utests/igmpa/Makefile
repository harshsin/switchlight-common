###############################################################################
#
# 
#
###############################################################################

include ../../../init.mk
MODULE := igmpa_utest
TEST_MODULE := igmpa
DEPENDMODULES := AIM indigo loci PPE IOF OS BigHash murmur SocketManager cjson Configuration BigList timer_wheel slshared debug_counter
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DSLSHARED_CONFIG_PKTIN_LISTENER_REGISTER=1
GLOBAL_CFLAGS += -DINDIGO_LINUX_TIME
GLOBAL_CFLAGS += -DINDIGO_MEM_STDLIB
GLOBAL_LINK_LIBS += -lm
include $(BUILDER)/build-unit-test.mk
