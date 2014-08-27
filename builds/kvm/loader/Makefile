###########################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################
ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

SWITCHLIGHT_REQUIRED_PACKAGES := kernel-x86-64:amd64

include $(SWITCHLIGHT)/make/component.mk

ifndef KVM_HDA
KVM_HDA := loader-amd64-hda.img
endif

ifndef KVM_HDB
KVM_HDB := loader-amd64-hdb.img
endif

ifndef KVM_ISO
KVM_ISO := loader-amd64.iso
endif

include $(SWITCHLIGHT)/make/kvm.mk
