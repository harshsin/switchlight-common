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

# 
# These are the local packages that must be build
# for this SWI configuration. 
#
SWITCHLIGHT_REQUIRED_PACKAGES := kernel-85xx:powerpc \
				 initrd-powerpc:powerpc \
				 pcli:all \
				 faultd:powerpc \
				 sofcon:powerpc \
				 ofad-6.2.7-internal:powerpc \
				 brcmd-6.2.7-internal:powerpc 

include $(SWITCHLIGHT)/make/component.mk


