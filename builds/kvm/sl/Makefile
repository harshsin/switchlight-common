############################################################
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

SWITCHLIGHT_REQUIRED_PACKAGES := loader-i386:i386

include $(SWITCHLIGHT)/make/component.mk

KVM_HDA := switchlight-i386.img
KVM_ISO := loader-i386.iso

include $(SWITCHLIGHT)/make/kvm.mk

