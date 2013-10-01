ifndef SWITCHLIGHT
$(error $$SWITCHLIGHT is undefined.)
endif

SWITCHLIGHT_REQUIRED_PACKAGES := kernel-x86-64:amd64

include $(SWITCHLIGHT)/make/component.mk

ifndef KVM_HDA
KVM_HDA := loader-i386-hda.img
endif

ifndef KVM_ISO
KVM_ISO := loader-i386.iso
endif

include $(SWITCHLIGHT)/make/kvm.mk
